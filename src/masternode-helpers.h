// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Copyright (c) 2017-2018 The Bitcoin Green developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MASTERNODEHELPERS_H
#define MASTERNODEHELPERS_H

#include "main.h"
#include "sync.h"
#include "base58.h"

/** Helper object for signing and checking signatures
 */
class CMasternodeSigner
{
public:
    CScript collateralPubKey;

    /// Is the inputs associated with this public key? (and there is BITG collateral - checking if valid masternode)
    bool IsVinAssociatedWithPubkey(CTxIn& vin, CPubKey& pubkey);
    /// Set the private/public key values, returns true if successful
    bool GetKeysFromSecret(std::string strSecret, CKey& keyRet, CPubKey& pubkeyRet);
    /// Set the private/public key values, returns true if successful
    bool SetKey(std::string strSecret, std::string& errorMessage, CKey& key, CPubKey& pubkey);
    /// Sign the message, returns true if successful
    bool SignMessage(std::string strMessage, std::string& errorMessage, std::vector<unsigned char>& vchSig, CKey key);
    /// Verify the message, returns true if succcessful
    bool VerifyMessage(CPubKey pubkey, std::vector<unsigned char>& vchSig, std::string strMessage, std::string& errorMessage);

    bool SetCollateralAddress(std::string strAddress);

    void InitCollateralAddress()
    {
        SetCollateralAddress(Params().MasternodePoolDummyAddress());
    }

};

void ThreadMasternodePool();

extern CMasternodeSigner masternodeSigner;


#endif
