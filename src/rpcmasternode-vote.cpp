// Copyright (c) 2017-2018 The Bitcoin Green Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "db.h"
#include "init.h"
#include "main.h"
#include "masternode-vote.h"
#include "masternode-payments.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "masternode-helpers.h"
#include "rpcserver.h"
#include "utilmoneystr.h"

#include <univalue.h>

#include <fstream>
using namespace std;

void communityToJSON(CCommunityProposal* pcommunityProposal, UniValue& bObj)
{
    bObj.push_back(Pair("Name", pcommunityProposal->GetName()));
    bObj.push_back(Pair("Description", pcommunityProposal->GetDescription()));
    bObj.push_back(Pair("Hash", pcommunityProposal->GetHash().ToString()));
    bObj.push_back(Pair("FeeHash", pcommunityProposal->nFeeTXHash.ToString()));
    bObj.push_back(Pair("BlockEnd", (int64_t)pcommunityProposal->GetBlockEnd()));
    bObj.push_back(Pair("Ratio", pcommunityProposal->GetRatio()));
    bObj.push_back(Pair("Yeas", (int64_t)pcommunityProposal->GetYeas()));
    bObj.push_back(Pair("Nays", (int64_t)pcommunityProposal->GetNays()));
    bObj.push_back(Pair("Abstains", (int64_t)pcommunityProposal->GetAbstains()));
    bObj.push_back(Pair("IsEstablished", pcommunityProposal->IsEstablished()));

    std::string strError = "";
    bObj.push_back(Pair("IsValid", pcommunityProposal->IsValid(strError)));
    bObj.push_back(Pair("IsValidReason", strError.c_str()));
    bObj.push_back(Pair("fValid", pcommunityProposal->fValid));
}

UniValue preparecommunityproposal(const UniValue& params, bool fHelp)
{
    CBlockIndex* pindexPrev = chainActive.Tip();

    if (fHelp || params.size() != 3)
        throw runtime_error(
            "preparecommunityproposal \"proposal-name\" \"proposal-description\" block-end\n"
            "\nPrepare a community vote proposal for network by signing and creating tx\n"

            "\nArguments:\n"
            "1. \"proposal-name\":        (string, required) Desired proposal name (20 character limit)\n"
            "2. \"proposal-description\": (string, required) Description of proposal (160 character limit)\n"
            "3. block-end:              (numeric, required) Last block available for votes\n"

            "\nResult:\n"
            "\"xxxx\"       (string) community vote proposal fee hash (if successful) or error message (if failed)\n"
            "\nExamples:\n" +
            HelpExampleCli("preparecommunityproposal", "\"test-proposal\" \"proposal-description\" 820800") +
            HelpExampleRpc("preparecommunityproposal", "\"test-proposal\" \"proposal-description\" 820800"));

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    std::string strProposalName = SanitizeString(params[0].get_str());
    if (strProposalName.size() > 20)
        throw runtime_error("Invalid proposal name, limit of 20 characters.");

    std::string strProposalDescription = SanitizeString(params[1].get_str());
    if (strProposalDescription.size() > 160)
        throw runtime_error("Invalid proposal description, limit of 160 characters.");

    int nBlockEnd = params[2].get_int();
    if (nBlockEnd < pindexPrev->nHeight)
        throw runtime_error("Invalid block end - must be a higher than current block height.");

    //*************************************************************************

    // create transaction 15 minutes into the future, to allow for confirmation time
    CCommunityProposalBroadcast communityProposalBroadcast(strProposalName, strProposalDescription, nBlockEnd, 0);

    std::string strError = "";
    if (!communityProposalBroadcast.IsValid(strError, false))
        throw runtime_error("Community Proposal is not valid - " + communityProposalBroadcast.GetHash().ToString() + " - " + strError);

    bool useIX = false; //true;

    CWalletTx wtx;
    if (!pwalletMain->GetCommunityVoteSystemCollateralTX(wtx, communityProposalBroadcast.GetHash(), useIX))
        throw runtime_error("Error making collateral transaction for community proposal. Please check your wallet balance.");

    // make our change address
    CReserveKey reservekey(pwalletMain);
    //send the tx to the network
    pwalletMain->CommitTransaction(wtx, reservekey, useIX ? "ix" : "tx");

    return wtx.GetHash().ToString();
}

UniValue submitcommunityproposal(const UniValue& params, bool fHelp)
{
    int nBlockMin = 0;
    CBlockIndex* pindexPrev = chainActive.Tip();

    if (fHelp || params.size() != 4)
        throw runtime_error(
            "submitcommunityproposal \"proposal-name\" \"proposal-description\" block-end \"fee-tx\"\n"
            "\nSubmit community proposal to the network\n"

            "\nArguments:\n"
            "1. \"proposal-name\":        (string, required) Desired proposal name (20 character limit)\n"
            "2. \"proposal-description\": (string, required) Description of proposal (160 character limit)\n"
            "3. block-end:              (numeric, required) Last block available for votes\n"
            "4. \"fee-tx\":             (string, required) Transaction hash from preparecommunityproposal command\n"

            "\nResult:\n"
            "\"xxxx\"       (string) proposal hash (if successful) or error message (if failed)\n"
            "\nExamples:\n" +
            HelpExampleCli("submitcommunityproposal", "\"test-proposal\" \"proposal-description\" 820800") +
            HelpExampleRpc("submitcommunityproposal", "\"test-proposal\" \"proposal-description\" 820800"));

    // Check these inputs the same way we check the vote commands:
    // **********************************************************

    std::string strProposalName = SanitizeString(params[0].get_str());
    if (strProposalName.size() > 20)
        throw runtime_error("Invalid proposal name, limit of 20 characters.");

    std::string strProposalDescription = SanitizeString(params[1].get_str());
    if (strProposalDescription.size() > 160)
        throw runtime_error("Invalid proposal description, limit of 160 characters.");

    int nBlockEnd = params[2].get_int();
    if (nBlockEnd < pindexPrev->nHeight)
        throw runtime_error("Invalid block end - must be a higher than current block height.");

    uint256 hash = ParseHashV(params[3], "parameter 1");

    //create the proposal incase we're the first to make it
    CCommunityProposalBroadcast communityProposalBroadcast(strProposalName, strProposalDescription, nBlockEnd, hash);

    std::string strError = "";
    int nConf = 0;
    if (!IsCommunityCollateralValid(hash, communityProposalBroadcast.GetHash(), strError, communityProposalBroadcast.nTime, nConf))
        throw runtime_error("Proposal FeeTX is not valid - " + hash.ToString() + " - " + strError);

    if (!masternodeSync.IsBlockchainSynced())
        throw runtime_error("Must wait for client to sync with masternode network. Try again in a minute or so.");

    communityVote.mapSeenMasternodeCommunityProposals.insert(make_pair(communityProposalBroadcast.GetHash(), communityProposalBroadcast));
    communityProposalBroadcast.Relay();
    if(communityVote.AddProposal(communityProposalBroadcast)) {
        return communityProposalBroadcast.GetHash().ToString();
    }
    throw runtime_error("Invalid proposal, see debug.log for details.");
}

UniValue getcommunityinfo(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getcommunityinfo ( \"proposal\" )\n"
            "\nShow current masternode community proposals\n"

            "\nArguments:\n"
            "1. \"proposal\"    (string, optional) Proposal name\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"Name\": \"xxxx\",               (string) Proposal Name\n"
            "    \"Description\": \"xxxx\",        (string) Proposal Description\n"
            "    \"Hash\": \"xxxx\",               (string) Proposal vote hash\n"
            "    \"FeeHash\": \"xxxx\",            (string) Proposal fee hash\n"
            "    \"BlockEnd\": n,                  (numeric) Proposal ending block\n"
            "    \"Ratio\": x.xxx,               (numeric) Ratio of yeas vs nays\n"
            "    \"Yeas\": n,                    (numeric) Number of yea votes\n"
            "    \"Nays\": n,                    (numeric) Number of nay votes\n"
            "    \"Abstains\": n,                (numeric) Number of abstains\n"
            "    \"IsEstablished\": true|false,  (boolean) Established (true) or (false)\n"
            "    \"IsValid\": true|false,        (boolean) Valid (true) or Invalid (false)\n"
            "    \"IsValidReason\": \"xxxx\",      (string) Error message, if any\n"
            "    \"fValid\": true|false,         (boolean) Valid (true) or Invalid (false)\n"
            "  }\n"
            "  ,...\n"
            "]\n"
            "\nExamples:\n" +
            HelpExampleCli("getcommunityinfo", "") + HelpExampleRpc("getcommunityinfo", ""));

    UniValue ret(UniValue::VARR);

    std::string strShow = "valid";
    if (params.size() == 1) {
        std::string strProposalName = SanitizeString(params[0].get_str());
        CCommunityProposal* pcommunityProposal = communityVote.FindProposal(strProposalName);
        if (pcommunityProposal == NULL) throw runtime_error("Unknown proposal name");
        UniValue bObj(UniValue::VOBJ);
        communityToJSON(pcommunityProposal, bObj);
        ret.push_back(bObj);
        return ret;
    }

    std::vector<CCommunityProposal*> winningProps = communityVote.GetAllProposals();
    for (CCommunityProposal* pcommunityProposal : winningProps) {
        if (strShow == "valid" && !pcommunityProposal->fValid) continue;

        UniValue bObj(UniValue::VOBJ);
        communityToJSON(pcommunityProposal, bObj);

        ret.push_back(bObj);
    }

    return ret;
}

UniValue checkcommunityproposals(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "checkcommunityproposals\n"
            "\nInitiates a community proposal check cycle manually\n"
            "\nExamples:\n" +
            HelpExampleCli("checkcommunityproposals", "") + HelpExampleRpc("checkcommunityproposals", ""));

    communityVote.CheckAndRemove();

    return NullUniValue;
}

UniValue getcommunityproposalvotes(const UniValue& params, bool fHelp)
{
    if (params.size() != 1)
        throw runtime_error(
            "getcommunityproposalvotes \"proposal-name\"\n"
            "\nPrint vote information for a community proposal\n"

            "\nArguments:\n"
            "1. \"proposal-name\":      (string, required) Name of the proposal\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"mnId\": \"xxxx\",        (string) Hash of the masternode's collateral transaction\n"
            "    \"nHash\": \"xxxx\",       (string) Hash of the vote\n"
            "    \"Vote\": \"YES|NO\",      (string) Vote cast ('YES' or 'NO')\n"
            "    \"nTime\": xxxx,         (numeric) Time in seconds since epoch the vote was cast\n"
            "    \"fValid\": true|false,  (boolean) 'true' if the vote is valid, 'false' otherwise\n"
            "  }\n"
            "  ,...\n"
            "]\n"
            "\nExamples:\n" +
            HelpExampleCli("getcommunityproposalvotes", "\"test-proposal\"") + HelpExampleRpc("getcommunityproposalvotes", "\"test-proposal\""));

    std::string strProposalName = SanitizeString(params[0].get_str());

    UniValue ret(UniValue::VARR);

    CCommunityProposal* pcommunityProposal = communityVote.FindProposal(strProposalName);

    if (pcommunityProposal == NULL) throw runtime_error("Unknown proposal name");

    std::map<uint256, CCommunityVote>::iterator it = pcommunityProposal->mapVotes.begin();
    while (it != pcommunityProposal->mapVotes.end()) {
        UniValue bObj(UniValue::VOBJ);
        bObj.push_back(Pair("mnId", (*it).second.vin.prevout.hash.ToString()));
        bObj.push_back(Pair("nHash", (*it).first.ToString().c_str()));
        bObj.push_back(Pair("Vote", (*it).second.GetVoteString()));
        bObj.push_back(Pair("nTime", (int64_t)(*it).second.nTime));
        bObj.push_back(Pair("fValid", (*it).second.fValid));

        ret.push_back(bObj);

        it++;
    }

    return ret;
}

UniValue mncommunityvote(const UniValue& params, bool fHelp)
{
    std::string strCommand;
    if (params.size() >= 1) {
        strCommand = params[0].get_str();
    }

    if (fHelp || (params.size() == 3 && (strCommand != "local" && strCommand != "many")) || (params.size() == 4 && strCommand != "alias") ||
        params.size() > 4 || params.size() < 3)
        throw runtime_error(
            "mncommunityvote \"local|many|alias\" \"votehash\" \"yes|no\" ( \"alias\" )\n"
            "\nVote on a community proposal\n"

            "\nArguments:\n"
            "1. \"mode\"      (string, required) The voting mode. 'local' for voting directly from a masternode, 'many' for voting with a MN controller and casting the same vote for each MN, 'alias' for voting with a MN controller and casting a vote for a single MN\n"
            "2. \"votehash\"  (string, required) The vote hash for the proposal\n"
            "3. \"votecast\"  (string, required) Your vote. 'yes' to vote for the proposal, 'no' to vote against\n"
            "4. \"alias\"     (string, required for 'alias' mode) The MN alias to cast a vote for.\n"

            "\nResult:\n"
            "{\n"
            "  \"overall\": \"xxxx\",      (string) The overall status message for the vote cast\n"
            "  \"detail\": [\n"
            "    {\n"
            "      \"node\": \"xxxx\",      (string) 'local' or the MN alias\n"
            "      \"result\": \"xxxx\",    (string) Either 'Success' or 'Failed'\n"
            "      \"error\": \"xxxx\",     (string) Error message, if vote failed\n"
            "    }\n"
            "    ,...\n"
            "  ]\n"
            "}\n"

            "\nExamples:\n" +
            HelpExampleCli("mncommunityvote", "\"local\" \"ed2f83cedee59a91406f5f47ec4d60bf5a7f9ee6293913c82976bd2d3a658041\" \"yes\"") +
            HelpExampleRpc("mncommunityvote", "\"local\" \"ed2f83cedee59a91406f5f47ec4d60bf5a7f9ee6293913c82976bd2d3a658041\" \"yes\""));

    uint256 hash = ParseHashV(params[1], "parameter 1");
    std::string strVote = params[2].get_str();

    if (strVote != "yes" && strVote != "no") return "You can only vote 'yes' or 'no'";
    int nVote = VOTE_ABSTAIN;
    if (strVote == "yes") nVote = VOTE_YES;
    if (strVote == "no") nVote = VOTE_NO;

    int success = 0;
    int failed = 0;

    UniValue resultsObj(UniValue::VARR);

    if (strCommand == "local") {
        CPubKey pubKeyMasternode;
        CKey keyMasternode;
        std::string errorMessage;

        UniValue statusObj(UniValue::VOBJ);

        while (true) {
            if (!masternodeSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode)) {
                failed++;
                statusObj.push_back(Pair("node", "local"));
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("error", "Masternode signing error, could not set key correctly: " + errorMessage));
                resultsObj.push_back(statusObj);
                break;
            }

            CMasternode* pmn = mnodeman.Find(activeMasternode.vin);
            if (pmn == NULL) {
                failed++;
                statusObj.push_back(Pair("node", "local"));
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("error", "Failure to find masternode in list : " + activeMasternode.vin.ToString()));
                resultsObj.push_back(statusObj);
                break;
            }

            CCommunityVote vote(activeMasternode.vin, hash, nVote);
            if (!vote.Sign(keyMasternode, pubKeyMasternode)) {
                failed++;
                statusObj.push_back(Pair("node", "local"));
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("error", "Failure to sign."));
                resultsObj.push_back(statusObj);
                break;
            }

            std::string strError = "";
            if (communityVote.UpdateProposal(vote, NULL, strError)) {
                success++;
                communityVote.mapSeenMasternodeCommunityVotes.insert(make_pair(vote.GetHash(), vote));
                vote.Relay();
                statusObj.push_back(Pair("node", "local"));
                statusObj.push_back(Pair("result", "success"));
                statusObj.push_back(Pair("error", ""));
            } else {
                failed++;
                statusObj.push_back(Pair("node", "local"));
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("error", "Error voting : " + strError));
            }
            resultsObj.push_back(statusObj);
            break;
        }

        UniValue returnObj(UniValue::VOBJ);
        returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed)));
        returnObj.push_back(Pair("detail", resultsObj));

        return returnObj;
    }

    if (strCommand == "many") {
        BOOST_FOREACH (CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            std::string errorMessage;
            std::vector<unsigned char> vchMasterNodeSignature;
            std::string strMasterNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            UniValue statusObj(UniValue::VOBJ);

            if (!masternodeSigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)) {
                failed++;
                statusObj.push_back(Pair("node", mne.getAlias()));
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("error", "Masternode signing error, could not set key correctly: " + errorMessage));
                resultsObj.push_back(statusObj);
                continue;
            }

            CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
            if (pmn == NULL) {
                failed++;
                statusObj.push_back(Pair("node", mne.getAlias()));
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("error", "Can't find masternode by pubkey"));
                resultsObj.push_back(statusObj);
                continue;
            }

            CCommunityVote vote(pmn->vin, hash, nVote);
            if (!vote.Sign(keyMasternode, pubKeyMasternode)) {
                failed++;
                statusObj.push_back(Pair("node", mne.getAlias()));
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("error", "Failure to sign."));
                resultsObj.push_back(statusObj);
                continue;
            }

            std::string strError = "";
            if (communityVote.UpdateProposal(vote, NULL, strError)) {
                communityVote.mapSeenMasternodeCommunityVotes.insert(make_pair(vote.GetHash(), vote));
                vote.Relay();
                success++;
                statusObj.push_back(Pair("node", mne.getAlias()));
                statusObj.push_back(Pair("result", "success"));
                statusObj.push_back(Pair("error", ""));
            } else {
                failed++;
                statusObj.push_back(Pair("node", mne.getAlias()));
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("error", strError.c_str()));
            }

            resultsObj.push_back(statusObj);
        }

        UniValue returnObj(UniValue::VOBJ);
        returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed)));
        returnObj.push_back(Pair("detail", resultsObj));

        return returnObj;
    }

    if (strCommand == "alias") {
        std::string strAlias = params[3].get_str();
        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        for (CMasternodeConfig::CMasternodeEntry mne : masternodeConfig.getEntries()) {

            if (strAlias != mne.getAlias()) continue;

            std::string errorMessage;
            std::vector<unsigned char> vchMasterNodeSignature;
            std::string strMasterNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            UniValue statusObj(UniValue::VOBJ);

            if(!masternodeSigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)){
                failed++;
                statusObj.push_back(Pair("node", mne.getAlias()));
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("error", "Masternode signing error, could not set key correctly: " + errorMessage));
                resultsObj.push_back(statusObj);
                continue;
            }

            CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
            if(pmn == NULL)
            {
                failed++;
                statusObj.push_back(Pair("node", mne.getAlias()));
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("error", "Can't find masternode by pubkey"));
                resultsObj.push_back(statusObj);
                continue;
            }

            CCommunityVote vote(pmn->vin, hash, nVote);
            if(!vote.Sign(keyMasternode, pubKeyMasternode)){
                failed++;
                statusObj.push_back(Pair("node", mne.getAlias()));
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("error", "Failure to sign."));
                resultsObj.push_back(statusObj);
                continue;
            }

            std::string strError = "";
            if(communityVote.UpdateProposal(vote, NULL, strError)) {
                communityVote.mapSeenMasternodeCommunityVotes.insert(make_pair(vote.GetHash(), vote));
                vote.Relay();
                success++;
                statusObj.push_back(Pair("node", mne.getAlias()));
                statusObj.push_back(Pair("result", "success"));
                statusObj.push_back(Pair("error", ""));
            } else {
                failed++;
                statusObj.push_back(Pair("node", mne.getAlias()));
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("error", strError.c_str()));
            }

            resultsObj.push_back(statusObj);
        }

        UniValue returnObj(UniValue::VOBJ);
        returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed)));
        returnObj.push_back(Pair("detail", resultsObj));

        return returnObj;
    }

    return NullUniValue;
}
