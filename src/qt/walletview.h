// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2017-2018 The PIVX developers
// Copyright (c) 2017-2018 The Bitcoin Green developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_WALLETVIEW_H
#define BITCOIN_QT_WALLETVIEW_H

#include "amount.h"
#include "askpassphrasedialog.h"
#include "masternodelist.h"
#include "proposallist.h"

#include <QStackedWidget>
#include <ui_interface.h>

class BitcoinGUI;
class ClientModel;
class OverviewPage;
class ReceiveCoinsDialog;
class SendCoinsDialog;
class SendCoinsRecipient;
class TransactionView;
class WalletModel;
class BlockExplorer;

QT_BEGIN_NAMESPACE
class QLabel;
class QModelIndex;
class QProgressDialog;
QT_END_NAMESPACE

/*
  WalletView class. This class represents the view to a single wallet.
  It was added to support multiple wallet functionality. Each wallet gets its own WalletView instance.
  It communicates with both the client and the wallet models to give the user an up-to-date view of the
  current core state.
*/
class WalletView : public QStackedWidget
{
    Q_OBJECT

public:
    explicit WalletView(QWidget* parent);
    ~WalletView();

    void setBitcoinGUI(BitcoinGUI* gui);
    /** Set the client model.
        The client model represents the part of the core that communicates with the P2P network, and is wallet-agnostic.
    */
    void setClientModel(ClientModel* clientModel);
    /** Set the wallet model.
        The wallet model represents a bitcoin wallet, and offers access to the list of transactions, address book and sending
        functionality.
    */
    void setWalletModel(WalletModel* walletModel);

    bool handlePaymentRequest(const SendCoinsRecipient& recipient);

    void showOutOfSyncWarning(bool fShow);

private:
    ClientModel* clientModel;
    WalletModel* walletModel;

    OverviewPage* overviewPage;
    QWidget* transactionsPage;
    ReceiveCoinsDialog* receiveCoinsPage;
    SendCoinsDialog* sendCoinsPage;
    BlockExplorer* explorerWindow;
    MasternodeList* masternodeListPage;

    TransactionView* transactionView;
    ProposalList *proposalList;

    QProgressDialog* progressDialog;
    QLabel* transactionSum;

public slots:
    /** Switch to overview (home) page */
    void gotoOverviewPage();
    /** Switch to history (transactions) page */
    void gotoHistoryPage();
    /** Switch to masternode page */
    void gotoMasternodePage();
    /** Switch to proposal page */
    void gotoProposalPage();
    /** Switch to explorer page */
    void gotoBlockExplorerPage();
    /** Switch to receive coins page */
    void gotoReceiveCoinsPage();
    /** Switch to send coins page */
    void gotoSendCoinsPage(QString addr = "");

    /** Show Sign/Verify Message dialog and switch to sign message tab */
    void gotoSignMessageTab(QString addr = "");
    /** Show Sign/Verify Message dialog and switch to verify message tab */
    void gotoVerifyMessageTab(QString addr = "");
    /** Show MultiSend Dialog */
    void gotoMultiSendDialog();
    /** Show a multisig tab **/
    void gotoMultisigDialog(int index);
    /** Show BIP 38 tool - default to Encryption tab */
    void gotoBip38Tool();

    /** Show incoming transaction notification for new transactions.

        The new items are those between start and end inclusive, under the given parent item.
    */
    void processNewTransaction(const QModelIndex& parent, int start, int /*end*/);
    /** Encrypt the wallet */
    void encryptWallet(bool status);
    /** Backup the wallet */
    void backupWallet();
    /** Change encrypted wallet passphrase */
    void changePassphrase();
    /** Ask for passphrase to unlock wallet temporarily */
    void unlockWallet(AskPassphraseDialog::Context context);
    /** Lock wallet */
    void lockWallet();
    /** Toggle wallet lock state */
    void toggleLockWallet();

    /** Show used sending addresses */
    void usedSendingAddresses();
    /** Show used receiving addresses */
    void usedReceivingAddresses();

    /** Re-emit encryption status signal */
    void updateEncryptionStatus();

    /** Show progress dialog e.g. for rescan */
    void showProgress(const QString& title, int nProgress);

    /** Update selected BITG amount from transactionview */
    void trxAmount(QString amount);

signals:
    /** Signal that we want to show the main window */
    void showNormalIfMinimized();
    /**  Fired when a message should be reported to the user */
    void message(const QString& title, const QString& message, unsigned int style);
    /** Encryption status of wallet changed */
    void encryptionStatusChanged(int status);
    /** Notify that a new transaction appeared */
    void incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address);
};

#endif // BITCOIN_QT_WALLETVIEW_H
