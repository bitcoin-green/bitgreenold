// Copyright (c) 2014-2016 The Dash developers
// Copyright (c) 2016-2017 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "swifttx.h"
#include "activemasternode.h"
#include "base58.h"
#include "key.h"
#include "main.h"
#include "masternodeman.h"
#include "masternode-helpers.h"
#include "masternodeconfig.h"
#include "net.h"
#include "protocol.h"
#include "spork.h"
#include "sync.h"
#include "util.h"
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

std::map<uint256, CTransaction> mapTxLockReq;
std::map<uint256, CTransaction> mapTxLockReqRejected;
std::map<uint256, CConsensusVote> mapTxLockVote;
std::map<uint256, CTransactionLock> mapTxLocks;
std::map<COutPoint, uint256> mapLockedInputs;
std::map<uint256, int64_t> mapUnknownVotes; //track votes with no tx for DOS
int nCompleteTXLocks;

//txlock - Locks transaction
//
//step 1.) Broadcast intention to lock transaction inputs, "txlreg", CTransaction
//step 2.) Top SWIFTTX_SIGNATURES_TOTAL masternodes, open connect to top 1 masternode.
//         Send "txvote", CTransaction, Signature, Approve
//step 3.) Top 1 masternode, waits for SWIFTTX_SIGNATURES_REQUIRED messages. Upon success, sends "txlock'

void ProcessMessageSwiftTX(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if (fLiteMode) return; //disable all masternode related functionality
    if (!IsSporkActive(SPORK_2_SWIFTTX)) return;
    if (!masternodeSync.IsBlockchainSynced()) return;

    if (strCommand == "ix") {
        //LogPrintf("ProcessMessageSwiftTX::ix\n");
        CDataStream vMsg(vRecv);
        CTransaction tx;
        vRecv >> tx;

        CInv inv(MSG_TXLOCK_REQUEST, tx.GetHash());
        pfrom->AddInventoryKnown(inv);

        if (mapTxLockReq.count(tx.GetHash()) || mapTxLockReqRejected.count(tx.GetHash())) {
            return;
        }

        if (!IsIXTXValid(tx)) {
            return;
        }

        BOOST_FOREACH (const CTxOut o, tx.vout) {
            // IX supports normal scripts and unspendable scripts (used in DS collateral and Budget collateral).
            // TODO: Look into other script types that are normal and can be included
            if (!o.scriptPubKey.IsNormalPaymentScript() && !o.scriptPubKey.IsUnspendable()) {
                LogPrintf("ProcessMessageSwiftTX::ix - Invalid Script %s\n", tx.ToString().c_str());
                return;
            }
        }

        int nBlockHeight = CreateNewLock(tx);

        bool fMissingInputs = false;
        CValidationState state;

        bool fAccepted = false;
        {
            LOCK(cs_main);
            fAccepted = AcceptToMemoryPool(mempool, state, tx, true, &fMissingInputs);
        }
        if (fAccepted) {
            RelayInv(inv);

            DoConsensusVote(tx, nBlockHeight);

            mapTxLockReq.insert(make_pair(tx.GetHash(), tx));

            LogPrintf("ProcessMessageSwiftTX::ix - Transaction Lock Request: %s %s : accepted %s\n",
                pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str(),
                tx.GetHash().ToString().c_str());

            return;

        } else {
            mapTxLockReqRejected.insert(make_pair(tx.GetHash(), tx));

            // can we get the conflicting transaction as proof?

            LogPrintf("ProcessMessageSwiftTX::ix - Transaction Lock Request: %s %s : rejected %s\n",
                pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str(),
                tx.GetHash().ToString().c_str());

            BOOST_FOREACH (const CTxIn& in, tx.vin) {
                if (!mapLockedInputs.count(in.prevout)) {
                    mapLockedInputs.insert(make_pair(in.prevout, tx.GetHash()));
                }
            }

            // resolve conflicts
            std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(tx.GetHash());
            if (i != mapTxLocks.end()) {
                //we only care if we have a complete tx lock
                if ((*i).second.CountSignatures() >= SWIFTTX_SIGNATURES_REQUIRED) {
                    if (!CheckForConflictingLocks(tx)) {
                        LogPrintf("ProcessMessageSwiftTX::ix - Found Existing Complete IX Lock\n");

                        //reprocess the last 15 blocks
                        ReprocessBlocks(15);
                        mapTxLockReq.insert(make_pair(tx.GetHash(), tx));
                    }
                }
            }

            return;
        }
    } else if (strCommand == "txlvote") // SwiftTX Lock Consensus Votes
    {
        CConsensusVote ctx;
        vRecv >> ctx;

        CInv inv(MSG_TXLOCK_VOTE, ctx.GetHash());
        pfrom->AddInventoryKnown(inv);

        if (mapTxLockVote.count(ctx.GetHash())) {
            return;
        }

        mapTxLockVote.insert(make_pair(ctx.GetHash(), ctx));

        if (ProcessConsensusVote(pfrom, ctx)) {
            //Spam/Dos protection
            /*
                Masternodes will sometimes propagate votes before the transaction is known to the client.
                This tracks those messages and allows it at the same rate of the rest of the network, if
                a peer violates it, it will simply be ignored
            */
            if (!mapTxLockReq.count(ctx.txHash) && !mapTxLockReqRejected.count(ctx.txHash)) {
                if (!mapUnknownVotes.count(ctx.vinMasternode.prevout.hash)) {
                    mapUnknownVotes[ctx.vinMasternode.prevout.hash] = GetTime() + (60 * 10);
                }

                if (mapUnknownVotes[ctx.vinMasternode.prevout.hash] > GetTime() &&
                    mapUnknownVotes[ctx.vinMasternode.prevout.hash] - GetAverageVoteTime() > 60 * 10) {
                    LogPrintf("ProcessMessageSwiftTX::ix - masternode is spamming transaction votes: %s %s\n",
                        ctx.vinMasternode.ToString().c_str(),
                        ctx.txHash.ToString().c_str());
                    return;
                } else {
                    mapUnknownVotes[ctx.vinMasternode.prevout.hash] = GetTime() + (60 * 10);
                }
            }
            RelayInv(inv);
        }

        return;
    }
}

bool IsIXTXValid(const CTransaction& txCollateral)
{
    if (txCollateral.vout.size() < 1) return false;
    if (txCollateral.nLockTime != 0) return false;

    CAmount nValueIn = 0;
    CAmount nValueOut = 0;
    bool missingTx = false;

    BOOST_FOREACH (const CTxOut o, txCollateral.vout)
        nValueOut += o.nValue;

    BOOST_FOREACH (const CTxIn i, txCollateral.vin) {
        CTransaction tx2;
        uint256 hash;
        if (GetTransaction(i.prevout.hash, tx2, hash, true)) {
            if (tx2.vout.size() > i.prevout.n) {
                nValueIn += tx2.vout[i.prevout.n].nValue;
            }
        } else {
            missingTx = true;
        }
    }

    if (nValueOut > GetSporkValue(SPORK_5_MAX_VALUE) * COIN) {
        LogPrint("swifttx", "IsIXTXValid - Transaction value too high - %s\n", txCollateral.ToString().c_str());
        return false;
    }

    if (missingTx) {
        LogPrint("swifttx", "IsIXTXValid - Unknown inputs in IX transaction - %s\n", txCollateral.ToString().c_str());
        /*
            This happens sometimes for an unknown reason, so we'll return that it's a valid transaction.
            If someone submits an invalid transaction it will be rejected by the network anyway and this isn't
            very common, but we don't want to block IX just because the client can't figure out the fee.
        */
        return true;
    }

    if (nValueIn - nValueOut < COIN * 0.01) {
        LogPrint("swifttx", "IsIXTXValid - did not include enough fees in transaction %d\n%s\n", nValueOut - nValueIn, txCollateral.ToString().c_str());
        return false;
    }

    return true;
}

int64_t CreateNewLock(CTransaction tx)
{
    int64_t nTxAge = 0;
    BOOST_REVERSE_FOREACH (CTxIn i, tx.vin) {
        nTxAge = GetInputAge(i);
        if (nTxAge < 5) //1 less than the "send IX" gui requires, incase of a block propagating the network at the time
        {
            LogPrintf("CreateNewLock - Transaction not found / too new: %d / %s\n", nTxAge, tx.GetHash().ToString().c_str());
            return 0;
        }
    }

    /*
        Use a blockheight newer than the input.
        This prevents attackers from using transaction mallibility to predict which masternodes
        they'll use.
    */
    int nBlockHeight = (chainActive.Tip()->nHeight - nTxAge) + 4;

    if (!mapTxLocks.count(tx.GetHash())) {
        LogPrintf("CreateNewLock - New Transaction Lock %s !\n", tx.GetHash().ToString().c_str());

        CTransactionLock newLock;
        newLock.nBlockHeight = nBlockHeight;
        newLock.nExpiration = GetTime() + (60 * 60); //locks expire after 60 minutes (24 confirmations)
        newLock.nTimeout = GetTime() + (60 * 5);
        newLock.txHash = tx.GetHash();
        mapTxLocks.insert(make_pair(tx.GetHash(), newLock));
    } else {
        mapTxLocks[tx.GetHash()].nBlockHeight = nBlockHeight;
        LogPrint("swifttx", "CreateNewLock - Transaction Lock Exists %s !\n", tx.GetHash().ToString().c_str());
    }


    return nBlockHeight;
}

// check if we need to vote on this transaction
void DoConsensusVote(CTransaction& tx, int64_t nBlockHeight)
{
    if (!fMasterNode) return;

    int n = mnodeman.GetMasternodeRank(activeMasternode.vin, nBlockHeight, MIN_SWIFTTX_PROTO_VERSION);

    if (n == -1) {
        LogPrint("swifttx", "SwiftTX::DoConsensusVote - Unknown Masternode\n");
        return;
    }

    if (n > SWIFTTX_SIGNATURES_TOTAL) {
        LogPrint("swifttx", "SwiftTX::DoConsensusVote - Masternode not in the top %d (%d)\n", SWIFTTX_SIGNATURES_TOTAL, n);
        return;
    }
    /*
        nBlockHeight calculated from the transaction is the authoritive source
    */

    LogPrint("swifttx", "SwiftTX::DoConsensusVote - In the top %d (%d)\n", SWIFTTX_SIGNATURES_TOTAL, n);

    CConsensusVote ctx;
    ctx.vinMasternode = activeMasternode.vin;
    ctx.txHash = tx.GetHash();
    ctx.nBlockHeight = nBlockHeight;
    if (!ctx.Sign()) {
        LogPrintf("SwiftTX::DoConsensusVote - Failed to sign consensus vote\n");
        return;
    }
    if (!ctx.SignatureValid()) {
        LogPrintf("SwiftTX::DoConsensusVote - Signature invalid\n");
        return;
    }

    mapTxLockVote[ctx.GetHash()] = ctx;

    CInv inv(MSG_TXLOCK_VOTE, ctx.GetHash());
    RelayInv(inv);
}

//received a consensus vote
bool ProcessConsensusVote(CNode* pnode, CConsensusVote& ctx)
{
    int n = mnodeman.GetMasternodeRank(ctx.vinMasternode, ctx.nBlockHeight, MIN_SWIFTTX_PROTO_VERSION);

    CMasternode* pmn = mnodeman.Find(ctx.vinMasternode);
    if (pmn != nullptr)
        LogPrint("swifttx", "SwiftTX::ProcessConsensusVote - Masternode ADDR %s %d\n", pmn->addr.ToString().c_str(), n);

    if (n == -1) {
        //can be caused by past versions trying to vote with an invalid protocol
        LogPrint("swifttx", "SwiftTX::ProcessConsensusVote - Unknown Masternode\n");
        mnodeman.AskForMN(pnode, ctx.vinMasternode);
        return false;
    }

    if (n > SWIFTTX_SIGNATURES_TOTAL) {
        LogPrint("swifttx", "SwiftTX::ProcessConsensusVote - Masternode not in the top %d (%d) - %s\n", SWIFTTX_SIGNATURES_TOTAL, n, ctx.GetHash().ToString().c_str());
        return false;
    }

    if (!ctx.SignatureValid()) {
        LogPrintf("SwiftTX::ProcessConsensusVote - Signature invalid\n");
        // don't ban, it could just be a non-synced masternode
        mnodeman.AskForMN(pnode, ctx.vinMasternode);
        return false;
    }

    if (!mapTxLocks.count(ctx.txHash)) {
        LogPrintf("SwiftTX::ProcessConsensusVote - New Transaction Lock %s !\n", ctx.txHash.ToString().c_str());

        CTransactionLock newLock;
        newLock.nBlockHeight = 0;
        newLock.nExpiration = GetTime() + (60 * 60);
        newLock.nTimeout = GetTime() + (60 * 5);
        newLock.txHash = ctx.txHash;
        mapTxLocks.insert(make_pair(ctx.txHash, newLock));
    } else
        LogPrint("swifttx", "SwiftTX::ProcessConsensusVote - Transaction Lock Exists %s !\n", ctx.txHash.ToString().c_str());

    //compile consessus vote
    std::map<uint256, CTransactionLock>::iterator i = mapTxLocks.find(ctx.txHash);
    if (i != mapTxLocks.end()) {
        (*i).second.AddSignature(ctx);

#ifdef ENABLE_WALLET
        if (pwalletMain) {
            //when we get back signatures, we'll count them as requests. Otherwise the client will think it didn't propagate.
            if (pwalletMain->mapRequestCount.count(ctx.txHash))
                pwalletMain->mapRequestCount[ctx.txHash]++;
        }
#endif

        LogPrint("swifttx", "SwiftTX::ProcessConsensusVote - Transaction Lock Votes %d - %s !\n", (*i).second.CountSignatures(), ctx.GetHash().ToString().c_str());

        if ((*i).second.CountSignatures() >= SWIFTTX_SIGNATURES_REQUIRED) {
            LogPrint("swifttx", "SwiftTX::ProcessConsensusVote - Transaction Lock Is Complete %s !\n", (*i).second.GetHash().ToString().c_str());

            CTransaction& tx = mapTxLockReq[ctx.txHash];
            if (!CheckForConflictingLocks(tx)) {
#ifdef ENABLE_WALLET
                if (pwalletMain) {
                    if (pwalletMain->UpdatedTransaction((*i).second.txHash)) {
                        nCompleteTXLocks++;
                    }
                }
#endif

                if (mapTxLockReq.count(ctx.txHash)) {
                    BOOST_FOREACH (const CTxIn& in, tx.vin) {
                        if (!mapLockedInputs.count(in.prevout)) {
                            mapLockedInputs.insert(make_pair(in.prevout, ctx.txHash));
                        }
                    }
                }

                // resolve conflicts

                //if this tx lock was rejected, we need to remove the conflicting blocks
                if (mapTxLockReqRejected.count((*i).second.txHash)) {
                    //reprocess the last 15 blocks
                    ReprocessBlocks(15);
                }
            }
        }
        return true;
    }


    return false;
}

bool CheckForConflictingLocks(CTransaction& tx)
{
    /*
        It's possible (very unlikely though) to get 2 conflicting transaction locks approved by the network.
        In that case, they will cancel each other out.

        Blocks could have been rejected during this time, which is OK. After they cancel out, the client will
        rescan the blocks and find they're acceptable and then take the chain with the most work.
    */
    BOOST_FOREACH (const CTxIn& in, tx.vin) {
        if (mapLockedInputs.count(in.prevout)) {
            if (mapLockedInputs[in.prevout] != tx.GetHash()) {
                LogPrintf("SwiftTX::CheckForConflictingLocks - found two complete conflicting locks - removing both. %s %s", tx.GetHash().ToString().c_str(), mapLockedInputs[in.prevout].ToString().c_str());
                if (mapTxLocks.count(tx.GetHash())) mapTxLocks[tx.GetHash()].nExpiration = GetTime();
                if (mapTxLocks.count(mapLockedInputs[in.prevout])) mapTxLocks[mapLockedInputs[in.prevout]].nExpiration = GetTime();
                return true;
            }
        }
    }

    return false;
}

int64_t GetAverageVoteTime()
{
    std::map<uint256, int64_t>::iterator it = mapUnknownVotes.begin();
    int64_t total = 0;
    int64_t count = 0;

    while (it != mapUnknownVotes.end()) {
        total += it->second;
        count++;
        it++;
    }

    return total / count;
}

void CleanTransactionLocksList()
{
    if (chainActive.Tip() == nullptr) return;

    std::map<uint256, CTransactionLock>::iterator it = mapTxLocks.begin();

    while (it != mapTxLocks.end()) {
        if (GetTime() > it->second.nExpiration) { //keep them for an hour
            LogPrintf("Removing old transaction lock %s\n", it->second.txHash.ToString().c_str());

            if (mapTxLockReq.count(it->second.txHash)) {
                CTransaction& tx = mapTxLockReq[it->second.txHash];

                BOOST_FOREACH (const CTxIn& in, tx.vin)
                    mapLockedInputs.erase(in.prevout);

                mapTxLockReq.erase(it->second.txHash);
                mapTxLockReqRejected.erase(it->second.txHash);

                BOOST_FOREACH (CConsensusVote& v, it->second.vecConsensusVotes)
                    mapTxLockVote.erase(v.GetHash());
            }

            mapTxLocks.erase(it++);
        } else {
            it++;
        }
    }
}

uint256 CConsensusVote::GetHash() const
{
    return vinMasternode.prevout.hash + vinMasternode.prevout.n + txHash;
}


bool CConsensusVote::SignatureValid()
{
    std::string errorMessage;
    std::string strMessage = txHash.ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight);
    //LogPrintf("verify strMessage %s \n", strMessage.c_str());

    CMasternode* pmn = mnodeman.Find(vinMasternode);

    if (pmn == nullptr) {
        LogPrintf("SwiftTX::CConsensusVote::SignatureValid() - Unknown Masternode\n");
        return false;
    }

    if (!masternodeSigner.VerifyMessage(pmn->pubKeyMasternode, vchMasterNodeSignature, strMessage, errorMessage)) {
        LogPrintf("SwiftTX::CConsensusVote::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

bool CConsensusVote::Sign()
{
    std::string errorMessage;

    CKey key2;
    CPubKey pubkey2;
    std::string strMessage = txHash.ToString().c_str() + boost::lexical_cast<std::string>(nBlockHeight);
    //LogPrintf("signing strMessage %s \n", strMessage.c_str());
    //LogPrintf("signing privkey %s \n", strMasterNodePrivKey.c_str());

    if (!masternodeSigner.SetKey(strMasterNodePrivKey, errorMessage, key2, pubkey2)) {
        LogPrintf("CConsensusVote::Sign() - ERROR: Invalid masternodeprivkey: '%s'\n", errorMessage.c_str());
        return false;
    }

    if (!masternodeSigner.SignMessage(strMessage, errorMessage, vchMasterNodeSignature, key2)) {
        LogPrintf("CConsensusVote::Sign() - Sign message failed");
        return false;
    }

    if (!masternodeSigner.VerifyMessage(pubkey2, vchMasterNodeSignature, strMessage, errorMessage)) {
        LogPrintf("CConsensusVote::Sign() - Verify message failed");
        return false;
    }

    return true;
}


bool CTransactionLock::SignaturesValid()
{
    BOOST_FOREACH (CConsensusVote vote, vecConsensusVotes) {
        int n = mnodeman.GetMasternodeRank(vote.vinMasternode, vote.nBlockHeight, MIN_SWIFTTX_PROTO_VERSION);

        if (n == -1) {
            LogPrintf("CTransactionLock::SignaturesValid() - Unknown Masternode\n");
            return false;
        }

        if (n > SWIFTTX_SIGNATURES_TOTAL) {
            LogPrintf("CTransactionLock::SignaturesValid() - Masternode not in the top %s\n", SWIFTTX_SIGNATURES_TOTAL);
            return false;
        }

        if (!vote.SignatureValid()) {
            LogPrintf("CTransactionLock::SignaturesValid() - Signature not valid\n");
            return false;
        }
    }

    return true;
}

void CTransactionLock::AddSignature(CConsensusVote& cv)
{
    vecConsensusVotes.push_back(cv);
}

int CTransactionLock::CountSignatures()
{
    /*
        Only count signatures where the BlockHeight matches the transaction's blockheight.
        The votes have no proof it's the correct blockheight
    */

    if (nBlockHeight == 0) return -1;

    int n = 0;
    BOOST_FOREACH (CConsensusVote v, vecConsensusVotes) {
        if (v.nBlockHeight == nBlockHeight) {
            n++;
        }
    }
    return n;
}
