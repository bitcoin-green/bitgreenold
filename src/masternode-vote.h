// Copyright (c) 2017-2018 The Bitcoin Green developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MASTERNODE_VOTE_H
#define MASTERNODE_VOTE_H

#include "base58.h"
#include "init.h"
#include "key.h"
#include "main.h"
#include "masternode.h"
#include "net.h"
#include "sync.h"
#include "util.h"
#include <boost/lexical_cast.hpp>

using namespace std;

extern CCriticalSection cs_communityvote;

class CCommunityVoteManager;
class CCommunityProposal;
class CCommunityProposalBroadcast;
class CVoteProposal;
class CVoteProposalBroadcast;

#define VOTE_ABSTAIN 0
#define VOTE_YES 1
#define VOTE_NO 2

static const CAmount COMMUNITY_VOTE_FEE_TX = (25 * COIN);
static const int64_t COMMUNITY_VOTE_UPDATE_MIN = 60 * 60;

extern std::vector<CCommunityProposalBroadcast> vecCommunityProposals;

extern CCommunityVoteManager communityVote;
void DumpCommunityVotes();

//! Check the collateral transaction for the community vote proposal
bool IsCommunityCollateralValid(uint256 nTxCollateralHash, uint256 nExpectedHash, std::string& strError, int64_t& nTime, int& nConf);

//
// CCommunityVote - Allow a masternode node to vote and broadcast throughout the network
//

class CCommunityVote
{
public:
    bool fValid;  // if the vote is currently valid / counted
    bool fSynced; // if we've sent this to our peers
    CTxIn vin;
    uint256 nProposalHash;
    int nVote;
    int64_t nTime;
    std::vector<unsigned char> vchSig;

    CCommunityVote();
    CCommunityVote(CTxIn vin, uint256 nProposalHash, int nVoteIn);

    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    bool SignatureValid(bool fSignatureCheck);
    void Relay();

    std::string GetVoteString()
    {
        std::string ret = "ABSTAIN";
        if (nVote == VOTE_YES) ret = "YES";
        if (nVote == VOTE_NO) ret = "NO";
        return ret;
    }

    uint256 GetHash()
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vin;
        ss << nProposalHash;
        ss << nVote;
        ss << nTime;
        return ss.GetHash();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(vin);
        READWRITE(nProposalHash);
        READWRITE(nVote);
        READWRITE(nTime);
        READWRITE(vchSig);
    }
};

/** Save Vote Manager (communityvote.dat)
 */
class CCommunityDB
{
private:
    boost::filesystem::path pathDB;
    std::string strMagicMessage;

public:
    enum ReadResult {
        Ok,
        FileError,
        HashReadError,
        IncorrectHash,
        IncorrectMagicMessage,
        IncorrectMagicNumber,
        IncorrectFormat
    };

    CCommunityDB();
    bool Write(const CCommunityVoteManager& objToSave);
    ReadResult Read(CCommunityVoteManager& objToLoad, bool fDryRun = false);
};

//
// Vote Manager : Contains all proposals for the community vote
//
class CCommunityVoteManager
{
private:
    // hold txes until they mature enough to use
    map<uint256, uint256> mapCollateralTxids;

public:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    // keep track of the scanning errors I've seen
    map<uint256, CCommunityProposal> mapProposals;

    std::map<uint256, CCommunityProposalBroadcast> mapSeenMasternodeCommunityProposals;
    std::map<uint256, CCommunityVote> mapSeenMasternodeCommunityVotes;
    std::map<uint256, CCommunityVote> mapOrphanMasternodeCommunityVotes;

    CCommunityVoteManager()
    {
        mapProposals.clear();
    }

    void ClearSeen()
    {
        mapSeenMasternodeCommunityProposals.clear();
        mapSeenMasternodeCommunityVotes.clear();
    }

    int sizeProposals() { return (int)mapProposals.size(); }

    void ResetSync();
    void MarkSynced();
    void Sync(CNode* node, uint256 nProp, bool fPartial = false);

    void Calculate();
    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    void NewBlock();
    CCommunityProposal* FindProposal(const std::string& strProposalName);
    CCommunityProposal* FindProposal(uint256 nHash);
    std::pair<std::string, std::string> GetVotes(std::string strProposalName);

    std::vector<CCommunityProposal*> GetAllProposals();
    bool AddProposal(CCommunityProposal& voteProposal);

    bool UpdateProposal(CCommunityVote& vote, CNode* pfrom, std::string& strError);
    bool PropExists(uint256 nHash);

    void CheckOrphanVotes();
    void Clear()
    {
        LOCK(cs);

        LogPrintf("Community Vote object cleared\n");
        mapProposals.clear();
        mapSeenMasternodeCommunityProposals.clear();
        mapSeenMasternodeCommunityVotes.clear();
        mapOrphanMasternodeCommunityVotes.clear();
    }
    void CheckAndRemove();
    std::string ToString() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(mapSeenMasternodeCommunityProposals);
        READWRITE(mapSeenMasternodeCommunityVotes);
        READWRITE(mapOrphanMasternodeCommunityVotes);

        READWRITE(mapProposals);
    }
};

//
// Community Vote Proposal : Contains the masternode votes for each community proposal
//

class CCommunityProposal
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
    CAmount nAlloted;

public:
    bool fValid;
    std::string strProposalName;
    std::string strProposalDescription;
    int nBlockEnd;
    int64_t nTime;
    uint256 nFeeTXHash;

    map<uint256, CCommunityVote> mapVotes;
    //cache object

    CCommunityProposal();
    CCommunityProposal(const CCommunityProposal& other);
    CCommunityProposal(std::string strProposalNameIn, std::string strProposalDescriptionIn, int nBlockEndIn, uint256 nFeeTXHashIn);

    void Calculate();
    bool AddOrUpdateVote(CCommunityVote& vote, std::string& strError);
    bool HasMinimumRequiredSupport();
    std::pair<std::string, std::string> GetVotes();

    bool IsValid(std::string& strError, bool fCheckCollateral = true);

    bool IsEstablished()
    {
        // 5 minutes
        return (nTime < GetTime() - (60 * 5));
    }

    std::string GetName() { return strProposalName; }
    std::string GetDescription() { return strProposalDescription; }
    int GetBlockEnd() { return nBlockEnd; }
    double GetRatio();
    int GetYeas();
    int GetNays();
    int GetAbstains();

    void CleanAndRemove(bool fSignatureCheck);

    uint256 GetHash()
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << strProposalName;
        ss << strProposalDescription;
        ss << nBlockEnd;
        uint256 h1 = ss.GetHash();

        return h1;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        //for syncing with other clients
        READWRITE(LIMITED_STRING(strProposalName, 20));
        READWRITE(LIMITED_STRING(strProposalDescription, 160));
        READWRITE(nBlockEnd);
        READWRITE(nTime);
        READWRITE(nFeeTXHash);

        //for saving to the serialized db
        READWRITE(mapVotes);
    }
};

// Proposals are cast then sent to peers with this object, which leaves the votes out
class CCommunityProposalBroadcast : public CCommunityProposal
{
public:
    CCommunityProposalBroadcast() : CCommunityProposal() {}
    CCommunityProposalBroadcast(const CCommunityProposal& other) : CCommunityProposal(other) {}
    CCommunityProposalBroadcast(const CCommunityProposalBroadcast& other) : CCommunityProposal(other) {}
    CCommunityProposalBroadcast(std::string strProposalNameIn, std::string strProposalDescriptionIn, int nBlockEndIn, uint256 nFeeTXHashIn);

    void swap(CCommunityProposalBroadcast& first, CCommunityProposalBroadcast& second) // nothrow
    {
        // enable ADL (not necessary in our case, but good practice)
        using std::swap;

        // by swapping the members of two classes,
        // the two classes are effectively swapped
        swap(first.strProposalName, second.strProposalName);
        swap(first.strProposalDescription, second.strProposalDescription);
        swap(first.nBlockEnd, second.nBlockEnd);
        swap(first.nTime, second.nTime);
        swap(first.nFeeTXHash, second.nFeeTXHash);
        first.mapVotes.swap(second.mapVotes);
    }

    CCommunityProposalBroadcast& operator=(CCommunityProposalBroadcast from)
    {
        swap(*this, from);
        return *this;
    }

    void Relay();

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        //for syncing with other clients

        READWRITE(LIMITED_STRING(strProposalName, 20));
        READWRITE(LIMITED_STRING(strProposalDescription, 160));
        READWRITE(nTime);
        READWRITE(nBlockEnd);
        READWRITE(nFeeTXHash);
    }
};

#endif // MASTERNODE_VOTE_H
