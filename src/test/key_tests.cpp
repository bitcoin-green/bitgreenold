// Copyright (c) 2012-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "key.h"

#include "base58.h"
#include "script/script.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace std;

static const string strSecret1     ("2a6Rd6XtMKvFrjkDi6FcfSrJkpaKc382bCDadKjxPsSAHKXoZok");
static const string strSecret2     ("2YtPcre7RfCg6TdakgACcAiFBakczN41n1D68nvseWST5eVqqxU");
static const string strSecret1C    ("7w4wgKRKeiW4aXfm6NBWQh6dNQz57eN21vWXRKibVwc3xfUaExcT");
static const string strSecret2C    ("7qjpqG6oMqcXZ6YTymqcEDs3bhyXrBvJRdvMfPinYYwL7HbEPDnQ");
static const CBitcoinAddress addr1 ("GPbEQ1hJxpJkWBDKMpc4y2KkugwuUaRUTY");
static const CBitcoinAddress addr2 ("GMLyt6XF2uyf5D154j7VW8oYGbcmvREGBm");
static const CBitcoinAddress addr1C("GRswJtXD3dWyWpz8oXdbxWeTEvYAuXmQUx");
static const CBitcoinAddress addr2C("GfpzbEUtk8je9RvaSMuCkScEU6fvichQLm");


static const string strAddressBad("Xta1praZQjyELweyMByXyiREw1ZRsjXzVP");

#ifdef KEY_TESTS_DUMPINFO
void dumpKeyInfo()
{
    CKey k;
    k.MakeNewKey(false);
    CPrivKey s = k.GetPrivKey();

    printf("Generating new key\n");

    for (int nCompressed=0; nCompressed<2; nCompressed++)
    {
        bool bCompressed = nCompressed == 1;

        CKey key;
        key.SetPrivKey(s, bCompressed);
        CPrivKey secret = key.GetPrivKey();
        CPubKey pubKey = key.GetPubKey();

        CBitcoinSecret bsecret;
        bsecret.SetKey(key);

        printf("  * %s:\n", bCompressed ? "compressed" : "uncompressed");
        printf("    * secret (base58): %s\n", bsecret.ToString().c_str());
        printf("    * pubkey (hex): %s\n", HexStr(pubKey).c_str());
        printf("    * address (base58): %s\n", CBitcoinAddress(CTxDestination(pubKey.GetID())).ToString().c_str());
}
#endif


BOOST_AUTO_TEST_SUITE(key_tests)

BOOST_AUTO_TEST_CASE(key_test1)
{
    CBitcoinSecret bsecret1, bsecret2, bsecret1C, bsecret2C, baddress1;
    BOOST_CHECK( bsecret1.SetString (strSecret1));
    BOOST_CHECK( bsecret2.SetString (strSecret2));
    BOOST_CHECK( bsecret1C.SetString(strSecret1C));
    BOOST_CHECK( bsecret2C.SetString(strSecret2C));
    BOOST_CHECK(!baddress1.SetString(strAddressBad));

    CKey key1  = bsecret1.GetKey();
    BOOST_CHECK(key1.IsCompressed() == false);
    CKey key2  = bsecret2.GetKey();
    BOOST_CHECK(key2.IsCompressed() == false);
    CKey key1C = bsecret1C.GetKey();
    BOOST_CHECK(key1C.IsCompressed() == true);
    CKey key2C = bsecret2C.GetKey();
    BOOST_CHECK(key2C.IsCompressed() == true);

    CPubKey pubkey1  = key1. GetPubKey();
    CPubKey pubkey2  = key2. GetPubKey();
    CPubKey pubkey1C = key1C.GetPubKey();
    CPubKey pubkey2C = key2C.GetPubKey();

    BOOST_CHECK(key1.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key1C.VerifyPubKey(pubkey1));
    BOOST_CHECK(key1C.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key1C.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key1C.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key2.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key2.VerifyPubKey(pubkey1C));
    BOOST_CHECK(key2.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key2.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key2C.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key2C.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key2C.VerifyPubKey(pubkey2));
    BOOST_CHECK(key2C.VerifyPubKey(pubkey2C));

    BOOST_CHECK(addr1.Get()  == CTxDestination(pubkey1.GetID()));
    BOOST_CHECK(addr2.Get()  == CTxDestination(pubkey2.GetID()));
    BOOST_CHECK(addr1C.Get() == CTxDestination(pubkey1C.GetID()));
    BOOST_CHECK(addr2C.Get() == CTxDestination(pubkey2C.GetID()));

    for (int n=0; n<16; n++)
    {
        string strMsg = strprintf("Very secret message %i: 11", n);
        uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());

        // normal signatures

        vector<unsigned char> sign1, sign2, sign1C, sign2C;

        BOOST_CHECK(key1.Sign (hashMsg, sign1));
        BOOST_CHECK(key2.Sign (hashMsg, sign2));
        BOOST_CHECK(key1C.Sign(hashMsg, sign1C));
        BOOST_CHECK(key2C.Sign(hashMsg, sign2C));

        BOOST_CHECK( pubkey1.Verify(hashMsg, sign1));
        BOOST_CHECK(!pubkey1.Verify(hashMsg, sign2));
        BOOST_CHECK( pubkey1.Verify(hashMsg, sign1C));
        BOOST_CHECK(!pubkey1.Verify(hashMsg, sign2C));

        BOOST_CHECK(!pubkey2.Verify(hashMsg, sign1));
        BOOST_CHECK( pubkey2.Verify(hashMsg, sign2));
        BOOST_CHECK(!pubkey2.Verify(hashMsg, sign1C));
        BOOST_CHECK( pubkey2.Verify(hashMsg, sign2C));

        BOOST_CHECK( pubkey1C.Verify(hashMsg, sign1));
        BOOST_CHECK(!pubkey1C.Verify(hashMsg, sign2));
        BOOST_CHECK( pubkey1C.Verify(hashMsg, sign1C));
        BOOST_CHECK(!pubkey1C.Verify(hashMsg, sign2C));

        BOOST_CHECK(!pubkey2C.Verify(hashMsg, sign1));
        BOOST_CHECK( pubkey2C.Verify(hashMsg, sign2));
        BOOST_CHECK(!pubkey2C.Verify(hashMsg, sign1C));
        BOOST_CHECK( pubkey2C.Verify(hashMsg, sign2C));

        // compact signatures (with key recovery)

        vector<unsigned char> csign1, csign2, csign1C, csign2C;

        BOOST_CHECK(key1.SignCompact (hashMsg, csign1));
        BOOST_CHECK(key2.SignCompact (hashMsg, csign2));
        BOOST_CHECK(key1C.SignCompact(hashMsg, csign1C));
        BOOST_CHECK(key2C.SignCompact(hashMsg, csign2C));

        CPubKey rkey1, rkey2, rkey1C, rkey2C;

        BOOST_CHECK(rkey1.RecoverCompact (hashMsg, csign1));
        BOOST_CHECK(rkey2.RecoverCompact (hashMsg, csign2));
        BOOST_CHECK(rkey1C.RecoverCompact(hashMsg, csign1C));
        BOOST_CHECK(rkey2C.RecoverCompact(hashMsg, csign2C));

        BOOST_CHECK(rkey1  == pubkey1);
        BOOST_CHECK(rkey2  == pubkey2);
        BOOST_CHECK(rkey1C == pubkey1C);
        BOOST_CHECK(rkey2C == pubkey2C);
    }

    // test deterministic signing

    std::vector<unsigned char> detsig, detsigc;
    string strMsg = "Very deterministic message";
    uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());
    BOOST_CHECK(key1.Sign(hashMsg, detsig));
    BOOST_CHECK(key1C.Sign(hashMsg, detsigc));
    BOOST_CHECK(detsig == detsigc);
    BOOST_CHECK(detsig == ParseHex("30450221008e7fe2176fc31d4c8ade9f7f07361409eff432d62e98dce653a95850a7793f910220487bcbf390003988a0cc46f3ed2bf013b78ca3a40406bde7991004487ba58695"));
    BOOST_CHECK(key2.Sign(hashMsg, detsig));
    BOOST_CHECK(key2C.Sign(hashMsg, detsigc));
    BOOST_CHECK(detsig == detsigc);
    BOOST_CHECK(detsig == ParseHex("30440220469e065172b99b782ac742d54a568867eb13274864665e605272a8f11c696cdf02205892001019e2813b39887c3f5e67048751b16ebb5fd2f9f3a38639538234e4f1"));
    BOOST_CHECK(key1.SignCompact(hashMsg, detsig));
    BOOST_CHECK(key1C.SignCompact(hashMsg, detsigc));
    BOOST_CHECK(detsig == ParseHex("1b8e7fe2176fc31d4c8ade9f7f07361409eff432d62e98dce653a95850a7793f91487bcbf390003988a0cc46f3ed2bf013b78ca3a40406bde7991004487ba58695"));
    BOOST_CHECK(detsigc == ParseHex("1f8e7fe2176fc31d4c8ade9f7f07361409eff432d62e98dce653a95850a7793f91487bcbf390003988a0cc46f3ed2bf013b78ca3a40406bde7991004487ba58695"));
    BOOST_CHECK(key2.SignCompact(hashMsg, detsig));
    BOOST_CHECK(key2C.SignCompact(hashMsg, detsigc));
    BOOST_CHECK(detsig == ParseHex("1c469e065172b99b782ac742d54a568867eb13274864665e605272a8f11c696cdf5892001019e2813b39887c3f5e67048751b16ebb5fd2f9f3a38639538234e4f1"));
    BOOST_CHECK(detsigc == ParseHex("20469e065172b99b782ac742d54a568867eb13274864665e605272a8f11c696cdf5892001019e2813b39887c3f5e67048751b16ebb5fd2f9f3a38639538234e4f1"));
}

BOOST_AUTO_TEST_SUITE_END()
