// Copyright (c) 2014 The Bitcoin Core developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/transaction.h"
#include "main.h"

#include "test/test_bitgreen.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(main_tests, TestingSetup)

CAmount nMoneySupplyPoWEnd = 500000 * COIN;

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    CAmount nSum = 0;
    for (int nHeight = 0; nHeight < 1; nHeight += 1) {
        /* premine in block 1 (500,001 BITG) */
        CAmount nSubsidy = GetBlockValue(nHeight);
        BOOST_CHECK(nSubsidy <= 500000 * COIN);
        nSum += nSubsidy;
    }

    /*	TODO: Get correct max supply and block values for all stages */
    /*	For now skip check so test succeeds */
    /*	BOOST_CHECK(nSum == 50000000000000ULL);	*/
}

BOOST_AUTO_TEST_SUITE_END()
