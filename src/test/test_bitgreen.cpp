// Copyright (c) 2011-2013 The Bitcoin Core developers
// Copyright (c) 2017 The PIVX developers
// Copyright (c) 2017-2019 The BitGreen Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_TEST_MODULE BitGreen Test Suite

#include "test_bitgreen.h"

#include "main.h"
#include "random.h"
#include "txdb.h"
#include "ui_interface.h"
#include "util.h"
#ifdef ENABLE_WALLET
#include "db.h"
#include "wallet.h"
#endif

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>

CClientUIInterface uiInterface;
CWallet* pwalletMain;

extern bool fPrintToConsole;
extern void noui_connect();

TestingSetup::TestingSetup()
{
    ECC_Start();
    SetupEnvironment();
    fPrintToDebugLog = false; // don't want to write to debug.log file
    fCheckBlockIndex = true;
    SelectParams(CBaseChainParams::UNITTEST);
    noui_connect();
#ifdef ENABLE_WALLET
    bitdb.MakeMock();
#endif
    ClearDatadirCache();
    pathTemp = GetTempPath() / strprintf("test_bitgreen_%lu_%i", (unsigned long)GetTime(), (int)(GetRand(100000)));
    boost::filesystem::create_directories(pathTemp);
    mapArgs["-datadir"] = pathTemp.string();
    pblocktree = new CBlockTreeDB(1 << 20, true);
    pcoinsdbview = new CCoinsViewDB(1 << 23, true);
    pcoinsTip = new CCoinsViewCache(pcoinsdbview);
    InitBlockIndex();
#ifdef ENABLE_WALLET
    bool fFirstRun;
    pwalletMain = new CWallet("wallet.dat");
    pwalletMain->LoadWallet(fFirstRun);
    RegisterValidationInterface(pwalletMain);
#endif
    nScriptCheckThreads = 3;
    for (int i=0; i < nScriptCheckThreads-1; i++)
            threadGroup.create_thread(&ThreadScriptCheck);
    RegisterNodeSignals(GetNodeSignals());
}
TestingSetup::~TestingSetup()
{
    UnregisterNodeSignals(GetNodeSignals());
    threadGroup.interrupt_all();
    threadGroup.join_all();
#ifdef ENABLE_WALLET
    UnregisterValidationInterface(pwalletMain);
    delete pwalletMain;
    pwalletMain = nullptr;
#endif
    UnloadBlockIndex();
    delete pcoinsTip;
    delete pcoinsdbview;
    delete pblocktree;
#ifdef ENABLE_WALLET
    bitdb.Flush(true);
    bitdb.Reset();
#endif
    boost::filesystem::remove_all(pathTemp);
    ECC_Stop();
}

void Shutdown(void* parg)
{
  exit(0);
}

void StartShutdown()
{
  exit(0);
}

bool ShutdownRequested()
{
  return false;
}
