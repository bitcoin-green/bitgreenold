// Copyright (c) 2017-2018 The Bitcoin Green developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "init.h"
#include "main.h"

#include "activemasternode.h"
#include "addrman.h"
#include "masternode-vote.h"
#include "masternode-sync.h"
#include "masternode-helpers.h"
#include "masternodeconfig.h"
#include "masternode.h"
#include "masternodeman.h"
#include "util.h"
#include "wallet.h"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

CCommunityVoteManager communityVote;
CCriticalSection cs_communityvote;

std::map<uint256, int64_t> askedForSourceProposalOrVote;
std::vector<CCommunityProposalBroadcast> vecCommunityProposals;

bool IsCommunityCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError, int64_t& nTime, int& nConf)
{
    CTransaction txCollateral;
    uint256 nBlockHash;
    if (!GetTransaction(nTxCollateralHash, txCollateral, nBlockHash, true)) {
        strError = strprintf("Can't find collateral tx %s", txCollateral.ToString());
        LogPrint("masternode", "::IsCommunityCollateralValid - %s\n", strError);
        return false;
    }

    if (txCollateral.vout.size() < 1) return false;
    if (txCollateral.nLockTime != 0) return false;

    CScript findScript;
    findScript << OP_RETURN << ToByteVector(nExpectedHash);

    bool foundOpReturn = false;
    for (const CTxOut o : txCollateral.vout) {
        if (!o.scriptPubKey.IsNormalPaymentScript() && !o.scriptPubKey.IsUnspendable()) {
            strError = strprintf("Invalid Script %s", txCollateral.ToString());
            LogPrint("masternode", "::IsCommunityCollateralValid - %s\n", strError);
            return false;
        }
        if (o.scriptPubKey == findScript && o.nValue >= COMMUNITY_VOTE_FEE_TX) foundOpReturn = true;
    }

    if (!foundOpReturn) {
        strError = strprintf("Couldn't find opReturn %s in %s", nExpectedHash.ToString(), txCollateral.ToString());
        LogPrint("masternode", "::IsCommunityCollateralValid - %s\n", strError);
        return false;
    }

    // RETRIEVE CONFIRMATIONS AND NTIME
    /*
        - nTime starts as zero and is passed-by-reference out of this function and stored in the external proposal
        - nTime is never validated via the hashing mechanism and comes from a full-validated source (the blockchain)
    */

    int conf = GetIXConfirmations(nTxCollateralHash);
    if (nBlockHash != uint256(0)) {
        BlockMap::iterator mi = mapBlockIndex.find(nBlockHash);
        if (mi != mapBlockIndex.end() && (*mi).second) {
            CBlockIndex* pindex = (*mi).second;
            if (chainActive.Contains(pindex)) {
                conf += chainActive.Height() - pindex->nHeight + 1;
                nTime = pindex->nTime;
            }
        }
    }

    nConf = conf;

    // if we're syncing we won't have swiftTX information, so accept 1 confirmation
    if (conf >= Params().Budget_Fee_Confirmations()) {
        return true;
    } else {
        strError = strprintf("Collateral requires at least %d confirmations - %d confirmations", Params().Budget_Fee_Confirmations(), conf);
        LogPrint("masternode", "::IsCommunityCollateralValid - %s\n", strError, conf);
        return false;
    }
}

void CCommunityVoteManager::CheckOrphanVotes()
{
    LOCK(cs);

    std::string strError = "";
    std::map<uint256, CCommunityVote>::iterator it1 = mapOrphanMasternodeCommunityVotes.begin();
    while (it1 != mapOrphanMasternodeCommunityVotes.end()) {
        if (communityVote.UpdateProposal(((*it1).second), NULL, strError)) {
            LogPrint("masternode", "CCommunityVoteManager::CheckOrphanVotes - Proposal/CommunityVote is known, activating and removing orphan vote\n");
            mapOrphanMasternodeCommunityVotes.erase(it1++);
        } else {
            ++it1;
        }
    }

    LogPrint("masternode", "CCommunityVoteManager::CheckOrphanVotes - Done\n");
}

//
// CCommunityDB
//

CCommunityDB::CCommunityDB()
{
    pathDB = GetDataDir() / "communityvote.dat";
    strMagicMessage = "MasternodeCommunityVote";
}

bool CCommunityDB::Write(const CCommunityVoteManager& objToSave)
{
    LOCK(objToSave.cs);

    int64_t nStart = GetTimeMillis();

    // serialize, checksum data up to that point, then append checksum
    CDataStream ssObj(SER_DISK, CLIENT_VERSION);
    ssObj << strMagicMessage;                   // masternode cache file specific magic message
    ssObj << FLATDATA(Params().MessageStart()); // network specific magic number
    ssObj << objToSave;
    uint256 hash = Hash(ssObj.begin(), ssObj.end());
    ssObj << hash;

    // open output file, and associate with CAutoFile
    FILE* file = fopen(pathDB.string().c_str(), "wb");
    CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull())
        return error("%s : Failed to open file %s", __func__, pathDB.string());

    // Write and commit header, data
    try {
        fileout << ssObj;
    } catch (std::exception& e) {
        return error("%s : Serialize or I/O error - %s", __func__, e.what());
    }
    fileout.fclose();

    LogPrint("masternode", "Written info to communityvote.dat  %dms\n", GetTimeMillis() - nStart);

    return true;
}

CCommunityDB::ReadResult CCommunityDB::Read(CCommunityVoteManager& objToLoad, bool fDryRun)
{
    LOCK(objToLoad.cs);

    int64_t nStart = GetTimeMillis();
    // open input file, and associate with CAutoFile
    FILE* file = fopen(pathDB.string().c_str(), "rb");
    CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
    if (filein.IsNull()) {
        error("%s : Failed to open file %s", __func__, pathDB.string());
        return FileError;
    }

    // use file size to size memory buffer
    int fileSize = boost::filesystem::file_size(pathDB);
    int dataSize = fileSize - sizeof(uint256);
    // Don't try to resize to a negative number if file is small
    if (dataSize < 0)
        dataSize = 0;
    vector<unsigned char> vchData;
    vchData.resize(dataSize);
    uint256 hashIn;

    // read data and checksum from file
    try {
        filein.read((char*)&vchData[0], dataSize);
        filein >> hashIn;
    } catch (std::exception& e) {
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return HashReadError;
    }
    filein.fclose();

    CDataStream ssObj(vchData, SER_DISK, CLIENT_VERSION);

    // verify stored checksum matches input data
    uint256 hashTmp = Hash(ssObj.begin(), ssObj.end());
    if (hashIn != hashTmp) {
        error("%s : Checksum mismatch, data corrupted", __func__);
        return IncorrectHash;
    }

    unsigned char pchMsgTmp[4];
    std::string strMagicMessageTmp;
    try {
        // de-serialize file header (masternode cache file specific magic message) and ..
        ssObj >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (strMagicMessage != strMagicMessageTmp) {
            error("%s : Invalid masternode cache magic message", __func__);
            return IncorrectMagicMessage;
        }

        // de-serialize file header (network specific magic number) and ..
        ssObj >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp))) {
            error("%s : Invalid network magic number", __func__);
            return IncorrectMagicNumber;
        }

        // de-serialize data into CCommunityVoteManager object
        ssObj >> objToLoad;
    } catch (std::exception& e) {
        objToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }

    LogPrint("masternode", "Loaded info from communityvote.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrint("masternode", "  %s\n", objToLoad.ToString());
    if (!fDryRun) {
        LogPrint("masternode", "Community vote manager - cleaning....\n");
        objToLoad.CheckAndRemove();
        LogPrint("masternode", "Community vote manager - result:\n");
        LogPrint("masternode", "  %s\n", objToLoad.ToString());
    }

    return Ok;
}

void DumpCommunityVotes()
{
    int64_t nStart = GetTimeMillis();

    CCommunityDB votedb;
    CCommunityVoteManager tempCommunityVote;

    LogPrint("masternode", "Verifying communityvote.dat format...\n");
    CCommunityDB::ReadResult readResult = votedb.Read(tempCommunityVote, true);
    // there was an error and it was not an error on file opening => do not proceed
    if (readResult == CCommunityDB::FileError)
        LogPrint("masternode", "Missing communityvote file - communityvote.dat, will try to recreate\n");
    else if (readResult != CCommunityDB::Ok) {
        LogPrint("masternode", "Error reading communityvote.dat: ");
        if (readResult == CCommunityDB::IncorrectFormat)
            LogPrint("masternode", "magic is ok but data has invalid format, will try to recreate\n");
        else {
            LogPrint("masternode", "file format is unknown or invalid, please fix it manually\n");
            return;
        }
    }
    LogPrint("masternode", "Writting info to communityvote.dat...\n");
    votedb.Write(communityVote);

    LogPrint("masternode", "Community vote dump finished  %dms\n", GetTimeMillis() - nStart);
}

bool CCommunityVoteManager::AddProposal(CCommunityProposal& voteProposal)
{
    LOCK(cs);
    std::string strError = "";
    if (!voteProposal.IsValid(strError)) {
        LogPrint("masternode", "CCommunityVoteManager::AddProposal - invalid vote proposal - %s\n", strError);
        return false;
    }

    if (mapProposals.count(voteProposal.GetHash())) {
        return false;
    }

    mapProposals.insert(make_pair(voteProposal.GetHash(), voteProposal));
    LogPrint("masternode", "CCommunityVoteManager::AddProposal - proposal %s added\n", voteProposal.GetName().c_str());
    return true;
}

void CCommunityVoteManager::CheckAndRemove()
{
    LogPrint("mncommunityvote", "CCommunityVoteManager::CheckAndRemove\n");

    std::string strError = "";

    LogPrint("mncommunityvote", "CCommunityVoteManager::CheckAndRemove - mapProposals cleanup - size before: %d\n", mapProposals.size());
    std::map<uint256, CCommunityProposal>::iterator it2 = mapProposals.begin();
    while (it2 != mapProposals.end()) {
        CCommunityProposal* pcommunityProposal = &((*it2).second);
        pcommunityProposal->fValid = pcommunityProposal->IsValid(strError);
        if (!strError.empty()) {
            LogPrint("masternode", "CCommunityVoteManager::CheckAndRemove - Invalid vote proposal - %s\n", strError);
            strError = "";
        } else {
             LogPrint("masternode", "CCommunityVoteManager::CheckAndRemove - Found valid vote proposal: %s %s\n",
                      pcommunityProposal->strProposalName.c_str(), pcommunityProposal->nFeeTXHash.ToString().c_str());
        }

        ++it2;
    }

    LogPrint("masternode", "CCommunityVoteManager::CheckAndRemove - PASSED\n");
}

CCommunityProposal* CCommunityVoteManager::FindProposal(const std::string& strProposalName)
{
    //find the prop with the highest yes count

    int nYesCount = -99999;
    CCommunityProposal* pcommunityProposal = NULL;

    std::map<uint256, CCommunityProposal>::iterator it = mapProposals.begin();
    while (it != mapProposals.end()) {
        if ((*it).second.strProposalName == strProposalName && (*it).second.GetYeas() > nYesCount) {
            pcommunityProposal = &((*it).second);
            nYesCount = pcommunityProposal->GetYeas();
        }
        ++it;
    }

    if (nYesCount == -99999) return NULL;

    return pcommunityProposal;
}

CCommunityProposal* CCommunityVoteManager::FindProposal(uint256 nHash)
{
    LOCK(cs);

    if (mapProposals.count(nHash))
        return &mapProposals[nHash];

    return NULL;
}

std::vector<CCommunityProposal*> CCommunityVoteManager::GetAllProposals()
{
    LOCK(cs);

    std::vector<CCommunityProposal*> vVoteProposalRet;

    std::map<uint256, CCommunityProposal>::iterator it = mapProposals.begin();
    while (it != mapProposals.end()) {
        (*it).second.CleanAndRemove(false);

        CCommunityProposal* pcommunityProposal = &((*it).second);
        vVoteProposalRet.push_back(pcommunityProposal);

        ++it;
    }

    return vVoteProposalRet;
}

//
// Sort by votes, if there's a tie sort by their feeHash TX
//
struct sortCommunityProposalsByVotes {
    bool operator()(const std::pair<CCommunityProposal*, int>& left, const std::pair<CCommunityProposal*, int>& right)
    {
        if (left.second != right.second)
            return (left.second > right.second);
        return (left.first->nFeeTXHash > right.first->nFeeTXHash);
    }
};

void CCommunityVoteManager::NewBlock()
{
    TRY_LOCK(cs, fVoteNewBlock);
    if (!fVoteNewBlock) return;

    if (masternodeSync.RequestedMasternodeAssets <= MASTERNODE_SYNC_COMMUNITYVOTE) return;

    // incremental sync with our peers
    if (masternodeSync.IsSynced()) {
        LogPrint("masternode", "CCommunityVoteManager::NewBlock - incremental sync started\n");
        if (chainActive.Height() % 1440 == rand() % 1440) {
            ClearSeen();
            ResetSync();
        }

        LOCK(cs_vNodes);
        BOOST_FOREACH (CNode* pnode, vNodes)
            if (pnode->nVersion >= ActiveProtocol())
                Sync(pnode, 0, true);

        MarkSynced();
    }

    CheckAndRemove();

    //remove invalid votes once in a while (we have to check the signatures and validity of every vote, somewhat CPU intensive)
    LogPrint("masternode", "CCommunityVoteManager::NewBlock - askedForSourceProposalOrVote cleanup - size: %d\n", askedForSourceProposalOrVote.size());
    std::map<uint256, int64_t>::iterator it = askedForSourceProposalOrVote.begin();
    while (it != askedForSourceProposalOrVote.end()) {
        if ((*it).second > GetTime() - (60 * 60 * 24)) {
            ++it;
        } else {
            askedForSourceProposalOrVote.erase(it++);
        }
    }

    LogPrint("masternode", "CCommunityVoteManager::NewBlock - mapProposals cleanup - size: %d\n", mapProposals.size());
    std::map<uint256, CCommunityProposal>::iterator it2 = mapProposals.begin();
    while (it2 != mapProposals.end()) {
        (*it2).second.CleanAndRemove(false);
        ++it2;
    }

    LogPrint("masternode", "CCommunityVoteManager::NewBlock - vecCommunityProposals cleanup - size: %d\n", vecCommunityProposals.size());
    std::vector<CCommunityProposalBroadcast>::iterator it4 = vecCommunityProposals.begin();
    while (it4 != vecCommunityProposals.end()) {
        std::string strError = "";
        int nConf = 0;
        if (!IsCommunityCollateralValid((*it4).nFeeTXHash, (*it4).GetHash(), strError, (*it4).nTime, nConf)) {
            ++it4;
            continue;
        }

        if (!(*it4).IsValid(strError)) {
            LogPrint("masternode", "mcprop - invalid community vote proposal - %s\n", strError);
            it4 = vecCommunityProposals.erase(it4);
            continue;
        }

        CCommunityProposal communityProposal((*it4));
        if (AddProposal(communityProposal)) {
            (*it4).Relay();
        }

        LogPrint("masternode", "mcprop - new community vote - %s\n", (*it4).GetHash().ToString());
        it4 = vecCommunityProposals.erase(it4);
    }

    LogPrint("masternode", "CCommunityVoteManager::NewBlock - PASSED\n");
}

void CCommunityVoteManager::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    // lite mode is not supported
    if (fLiteMode) return;
    if (!masternodeSync.IsBlockchainSynced()) return;

    LOCK(cs_communityvote);

    if (strCommand == "mncvs") { // Masternode community vote sync
        uint256 nProp;
        vRecv >> nProp;

        if (Params().NetworkID() == CBaseChainParams::MAIN) {
            if (nProp == 0) {
                if (pfrom->HasFulfilledRequest("mncvs")) {
                    LogPrint("masternode", "mncvs - peer already asked me for the list\n");
                    Misbehaving(pfrom->GetId(), 20);
                    return;
                }
                pfrom->FulfilledRequest("mncvs");
            }
        }

        Sync(pfrom, nProp);
        LogPrint("mncommunityvote", "mncvs - Sent Masternode community votes to peer %i\n", pfrom->GetId());
    }

    if (strCommand == "mcprop") { // Masternode Community Proposal
        CCommunityProposalBroadcast communityProposalBroadcast;
        vRecv >> communityProposalBroadcast;

        if (mapSeenMasternodeCommunityProposals.count(communityProposalBroadcast.GetHash())) {
            masternodeSync.AddedCommunityItem(communityProposalBroadcast.GetHash());
            return;
        }

        std::string strError = "";
        int nConf = 0;
        if (!IsCommunityCollateralValid(communityProposalBroadcast.nFeeTXHash, communityProposalBroadcast.GetHash(), strError, communityProposalBroadcast.nTime, nConf)) {
            LogPrint("masternode", "Community Proposal FeeTX is not valid - %s - %s\n", communityProposalBroadcast.nFeeTXHash.ToString(), strError);
            if (nConf >= 1) vecCommunityProposals.push_back(communityProposalBroadcast);
            return;
        }

        mapSeenMasternodeCommunityProposals.insert(make_pair(communityProposalBroadcast.GetHash(), communityProposalBroadcast));

        if (!communityProposalBroadcast.IsValid(strError)) {
            LogPrint("masternode", "mcprop - invalid community proposal - %s\n", strError);
            return;
        }

        CCommunityProposal communityProposal(communityProposalBroadcast);
        if (AddProposal(communityProposal)) {
            communityProposalBroadcast.Relay();
        }
        masternodeSync.AddedCommunityItem(communityProposalBroadcast.GetHash());

        LogPrint("masternode", "mcprop - new community - %s\n", communityProposalBroadcast.GetHash().ToString());

        // We might have active votes for this proposal that are valid now
        CheckOrphanVotes();
    }

    if (strCommand == "mcvote") { // Masternode Community Vote
        CCommunityVote vote;
        vRecv >> vote;
        vote.fValid = true;

        if (mapSeenMasternodeCommunityVotes.count(vote.GetHash())) {
            masternodeSync.AddedCommunityItem(vote.GetHash());
            return;
        }

        CMasternode* pmn = mnodeman.Find(vote.vin);
        if (pmn == NULL) {
            LogPrint("masternode", "mcvote - unknown masternode - vin: %s\n", vote.vin.prevout.hash.ToString());
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }

        mapSeenMasternodeCommunityVotes.insert(make_pair(vote.GetHash(), vote));
        if (!vote.SignatureValid(true)) {
            LogPrint("masternode", "mcvote - signature invalid\n");
            if (masternodeSync.IsSynced())
                Misbehaving(pfrom->GetId(), 20);
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, vote.vin);
            return;
        }

        std::string strError = "";
        if (UpdateProposal(vote, pfrom, strError)) {
            vote.Relay();
            masternodeSync.AddedCommunityItem(vote.GetHash());
        }

        LogPrint("masternode", "mcvote - new vote for community vote %s - %s\n", vote.nProposalHash.ToString(),  vote.GetHash().ToString());
    }
}

bool CCommunityVoteManager::PropExists(uint256 nHash)
{
    if (mapProposals.count(nHash)) return true;
    return false;
}

// mark that a full sync is needed
void CCommunityVoteManager::ResetSync()
{
    LOCK(cs);

    std::map<uint256, CCommunityProposalBroadcast>::iterator it1 = mapSeenMasternodeCommunityProposals.begin();
    while (it1 != mapSeenMasternodeCommunityProposals.end()) {
        CCommunityProposal* pcommunityProposal = FindProposal((*it1).first);
        if (pcommunityProposal && pcommunityProposal->fValid) {
            // mark votes
            std::map<uint256, CCommunityVote>::iterator it2 = pcommunityProposal->mapVotes.begin();
            while (it2 != pcommunityProposal->mapVotes.end()) {
                (*it2).second.fSynced = false;
                ++it2;
            }
        }
        ++it1;
    }
}

void CCommunityVoteManager::MarkSynced()
{
    LOCK(cs);

    /*
        Mark that we've sent all valid items
    */

    std::map<uint256, CCommunityProposalBroadcast>::iterator it1 = mapSeenMasternodeCommunityProposals.begin();
    while (it1 != mapSeenMasternodeCommunityProposals.end()) {
        CCommunityProposal* pcommunityProposal = FindProposal((*it1).first);
        if (pcommunityProposal && pcommunityProposal->fValid) {
            //mark votes
            std::map<uint256, CCommunityVote>::iterator it2 = pcommunityProposal->mapVotes.begin();
            while (it2 != pcommunityProposal->mapVotes.end()) {
                if ((*it2).second.fValid)
                    (*it2).second.fSynced = true;
                ++it2;
            }
        }
        ++it1;
    }
}


void CCommunityVoteManager::Sync(CNode* pfrom, uint256 nProp, bool fPartial)
{
    LOCK(cs);

    /*
        Sync with a client on the network

        --

        This code checks each of the hash maps for all known community proposals, then checks them against the
        community vote object to see if they're OK. If all checks pass, we'll send it to the peer.

    */

    int nInvCount = 0;

    std::map<uint256, CCommunityProposalBroadcast>::iterator it1 = mapSeenMasternodeCommunityProposals.begin();
    while (it1 != mapSeenMasternodeCommunityProposals.end()) {
        CCommunityProposal* pcommunityProposal = FindProposal((*it1).first);
        if (pcommunityProposal && pcommunityProposal->fValid && (nProp == 0 || (*it1).first == nProp)) {
            pfrom->PushInventory(CInv(MSG_COMMUNITY_PROPOSAL, (*it1).second.GetHash()));
            nInvCount++;

            // send votes
            std::map<uint256, CCommunityVote>::iterator it2 = pcommunityProposal->mapVotes.begin();
            while (it2 != pcommunityProposal->mapVotes.end()) {
                if ((*it2).second.fValid) {
                    if ((fPartial && !(*it2).second.fSynced) || !fPartial) {
                        pfrom->PushInventory(CInv(MSG_COMMUNITY_VOTE, (*it2).second.GetHash()));
                        nInvCount++;
                    }
                }
                ++it2;
            }
        }
        ++it1;
    }

    pfrom->PushMessage("ssc", MASTERNODE_SYNC_COMMUNITYVOTE_PROP, nInvCount);
    LogPrint("mncommunityvote", "CCommunityVoteManager::Sync - sent %d items\n", nInvCount);
}

bool CCommunityVoteManager::UpdateProposal(CCommunityVote& vote, CNode* pfrom, std::string& strError)
{
    LOCK(cs);

    if (!mapProposals.count(vote.nProposalHash)) {
        if (pfrom) {
            // only ask for missing items after our syncing process is complete --
            //   otherwise we'll think a full sync succeeded when they return a result
            if (!masternodeSync.IsSynced()) return false;

            LogPrint("masternode", "CCommunityVoteManager::UpdateProposal - Unknown proposal %d, asking for source proposal\n", vote.nProposalHash.ToString());
            mapOrphanMasternodeCommunityVotes[vote.nProposalHash] = vote;

            if (!askedForSourceProposalOrVote.count(vote.nProposalHash)) {
                pfrom->PushMessage("mncvs", vote.nProposalHash);
                askedForSourceProposalOrVote[vote.nProposalHash] = GetTime();
            }
        }

        strError = "Proposal not found!";
        return false;
    }

    return mapProposals[vote.nProposalHash].AddOrUpdateVote(vote, strError);
}

CCommunityProposal::CCommunityProposal()
{
    strProposalName = "unknown";
    strProposalDescription = "unknown";
    nBlockEnd = 0;
    nTime = 0;
    fValid = true;
}

CCommunityProposal::CCommunityProposal(std::string strProposalNameIn, std::string strProposalDescriptionIn, int nBlockEndIn, uint256 nFeeTXHashIn)
{
    strProposalName = strProposalNameIn;
    strProposalDescription = strProposalDescriptionIn;
    nBlockEnd = nBlockEndIn;
    nFeeTXHash = nFeeTXHashIn;
    fValid = true;
}

CCommunityProposal::CCommunityProposal(const CCommunityProposal& other)
{
    strProposalName = other.strProposalName;
    strProposalDescription = other.strProposalDescription;
    nBlockEnd = other.nBlockEnd;
    nTime = other.nTime;
    nFeeTXHash = other.nFeeTXHash;
    mapVotes = other.mapVotes;
    fValid = true;
}

bool CCommunityProposal::IsValid(std::string& strError, bool fCheckCollateral)
{
    if (GetNays() - GetYeas() > mnodeman.CountEnabled(ActiveProtocol()) / 10) {
        strError = "Proposal " + strProposalName + ": Active removal";
        return false;
    }

    if (fCheckCollateral) {
        int nConf = 0;
        if (!IsCommunityCollateralValid(nFeeTXHash, GetHash(), strError, nTime, nConf)) {
            strError = "Proposal " + strProposalName + ": Invalid collateral";
            return false;
        }
    }

    return true;
}

bool CCommunityProposal::AddOrUpdateVote(CCommunityVote& vote, std::string& strError)
{
    std::string strAction = "New vote inserted:";
    LOCK(cs);

    uint256 hash = vote.vin.prevout.GetHash();

    if (mapVotes.count(hash)) {
        if (mapVotes[hash].nTime > vote.nTime) {
            strError = strprintf("new vote older than existing vote - %s\n", vote.GetHash().ToString());
            LogPrint("mncommunityvote", "CCommunityProposal::AddOrUpdateVote - %s\n", strError);
            return false;
        }
        if (vote.nTime - mapVotes[hash].nTime < COMMUNITY_VOTE_UPDATE_MIN) {
            strError = strprintf("time between votes is too soon - %s - %lli sec < %lli sec\n", vote.GetHash().ToString(), vote.nTime - mapVotes[hash].nTime, COMMUNITY_VOTE_UPDATE_MIN);
            LogPrint("mncommunityvote", "CCommunityProposal::AddOrUpdateVote - %s\n", strError);
            return false;
        }
        strAction = "Existing vote updated:";
    }

    if (vote.nTime > GetTime() + (60 * 60)) {
        strError = strprintf("new vote is too far ahead of current time - %s - nTime %lli - Max Time %lli\n", vote.GetHash().ToString(), vote.nTime, GetTime() + (60 * 60));
        LogPrint("mncommunityvote", "CCommunityProposal::AddOrUpdateVote - %s\n", strError);
        return false;
    }

    mapVotes[hash] = vote;
    LogPrint("mncommunityvote", "CCommunityProposal::AddOrUpdateVote - %s %s\n", strAction.c_str(), vote.GetHash().ToString().c_str());

    return true;
}

// If masternode voted for a proposal, but is now invalid -- remove the vote
void CCommunityProposal::CleanAndRemove(bool fSignatureCheck)
{
    std::map<uint256, CCommunityVote>::iterator it = mapVotes.begin();

    while (it != mapVotes.end()) {
        (*it).second.fValid = (*it).second.SignatureValid(fSignatureCheck);
        ++it;
    }
}

double CCommunityProposal::GetRatio()
{
    int yeas = 0;
    int nays = 0;

    std::map<uint256, CCommunityVote>::iterator it = mapVotes.begin();

    while (it != mapVotes.end()) {
        if ((*it).second.nVote == VOTE_YES) yeas++;
        if ((*it).second.nVote == VOTE_NO) nays++;
        ++it;
    }

    if (yeas + nays == 0) return 0.0f;

    return ((double)(yeas) / (double)(yeas + nays));
}

int CCommunityProposal::GetYeas()
{
    int ret = 0;

    std::map<uint256, CCommunityVote>::iterator it = mapVotes.begin();
    while (it != mapVotes.end()) {
        if ((*it).second.nVote == VOTE_YES && (*it).second.fValid) ret++;
        ++it;
    }

    return ret;
}

int CCommunityProposal::GetNays()
{
    int ret = 0;

    std::map<uint256, CCommunityVote>::iterator it = mapVotes.begin();
    while (it != mapVotes.end()) {
        if ((*it).second.nVote == VOTE_NO && (*it).second.fValid) ret++;
        ++it;
    }

    return ret;
}

int CCommunityProposal::GetAbstains()
{
    int ret = 0;

    std::map<uint256, CCommunityVote>::iterator it = mapVotes.begin();
    while (it != mapVotes.end()) {
        if ((*it).second.nVote == VOTE_ABSTAIN && (*it).second.fValid) ret++;
        ++it;
    }

    return ret;
}

CCommunityProposalBroadcast::CCommunityProposalBroadcast(std::string strProposalNameIn, std::string strProposalDescriptionIn, int nBlockEndIn, uint256 nFeeTXHashIn)
{
    strProposalName = strProposalNameIn;
    strProposalDescription = strProposalDescriptionIn;
    nBlockEnd = nBlockEndIn;
    nFeeTXHash = nFeeTXHashIn;
}

void CCommunityProposalBroadcast::Relay()
{
    CInv inv(MSG_COMMUNITY_PROPOSAL, GetHash());
    RelayInv(inv);
}

CCommunityVote::CCommunityVote()
{
    vin = CTxIn();
    nProposalHash = 0;
    nVote = VOTE_ABSTAIN;
    nTime = 0;
    fValid = true;
    fSynced = false;
}

CCommunityVote::CCommunityVote(CTxIn vinIn, uint256 nProposalHashIn, int nVoteIn)
{
    vin = vinIn;
    nProposalHash = nProposalHashIn;
    nVote = nVoteIn;
    nTime = GetAdjustedTime();
    fValid = true;
    fSynced = false;
}

void CCommunityVote::Relay()
{
    CInv inv(MSG_COMMUNITY_VOTE, GetHash());
    RelayInv(inv);
}

bool CCommunityVote::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    // Choose coins to use
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;

    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + nProposalHash.ToString() + boost::lexical_cast<std::string>(nVote) + boost::lexical_cast<std::string>(nTime);

    if (!masternodeSigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode)) {
        LogPrint("masternode", "CCommunityVote::Sign - Error upon calling SignMessage");
        return false;
    }

    if (!masternodeSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrint("masternode", "CCommunityVote::Sign - Error upon calling VerifyMessage");
        return false;
    }

    return true;
}

bool CCommunityVote::SignatureValid(bool fSignatureCheck)
{
    std::string errorMessage;
    std::string strMessage = vin.prevout.ToStringShort() + nProposalHash.ToString() + boost::lexical_cast<std::string>(nVote) + boost::lexical_cast<std::string>(nTime);

    CMasternode* pmn = mnodeman.Find(vin);

    if (pmn == NULL) {
        LogPrint("masternode", "CCommunityVote::SignatureValid() - Unknown Masternode - %s\n", vin.prevout.hash.ToString());
        return false;
    }

    if (!fSignatureCheck) return true;

    if (!masternodeSigner.VerifyMessage(pmn->pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrint("masternode", "CCommunityVote::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

std::string CCommunityVoteManager::ToString() const
{
    std::ostringstream info;

    info << "Proposals: " << (int)mapProposals.size() << ", Seen Community Proposals: " << (int)mapSeenMasternodeCommunityProposals.size() << ", Seen Community Votes: " << (int)mapSeenMasternodeCommunityVotes.size();

    return info.str();
}
