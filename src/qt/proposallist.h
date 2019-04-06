// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALLIST_H
#define BITCOIN_QT_PROPOSALLIST_H

#include "guiutil.h"
#include "proposaltablemodel.h"
#include "proposalcommunitytablemodel.h"
#include "columnalignedlayout.h"
#include "walletmodel.h"
#include <QWidget>
#include <QKeyEvent>
#include <QTimer>

class PlatformStyle;
class ProposalFilterProxy;
class ProposalCommunityFilterProxy;

QT_BEGIN_NAMESPACE
class QComboBox;
class QDateTimeEdit;
class QFrame;
class QItemSelectionModel;
class QLineEdit;
class QMenu;
class QModelIndex;
class QSignalMapper;
class QTableView;
QT_END_NAMESPACE

#define PROPOSALLIST_UPDATE_SECONDS 300

class ProposalList : public QWidget
{
    Q_OBJECT

public:
    explicit ProposalList(QWidget* parent = 0);

    void setModel(WalletModel* model);

    enum DateEnum
    {
        All,
        Today,
        ThisWeek,
        ThisMonth,
        LastMonth,
        ThisYear,
        Range
    };

    enum ColumnWidths {
        PROPOSAL_COLUMN_WIDTH = 580,
        START_DATE_COLUMN_WIDTH = 110,
        END_DATE_COLUMN_WIDTH = 110,
        YES_VOTES_COLUMN_WIDTH = 60,
        NO_VOTES_COLUMN_WIDTH = 60,
        AMOUNT_COLUMN_WIDTH = 80,
        ABSOLUTE_YES_COLUMN_WIDTH = 60,
        PERCENTAGE_COLUMN_WIDTH = 80,
        MINIMUM_COLUMN_WIDTH = 23
    };

private:
    bool showBudgetProposals;
    bool showCommunityProposals;

    ProposalFilterProxy *proposalProxyModel;
    ProposalCommunityFilterProxy *proposalCommunityProxyModel;
    QTableView *proposalList;
    QTableView *proposalCommunityList;
    int64_t nLastUpdate = 0;
    WalletModel* model;

    QLineEdit *proposalWidget;
    QLineEdit *startBlockWidget;
    QLineEdit *endBlockWidget;
    QTimer *timer;
    QTabWidget *tabWidget;

    QLineEdit *yesVotesWidget;
    QLineEdit *noVotesWidget;
    QLineEdit *absoluteYesVotesWidget;
    QLineEdit *amountWidget;
    QLineEdit *percentageWidget;

    QLineEdit *proposalCommunityWidget;
    QLineEdit *endBlockCommunityWidget;
    QLineEdit *yesVotesCommunityWidget;
    QLineEdit *noVotesCommunityWidget;
    QLineEdit *absoluteYesVotesCommunityWidget;
    QLineEdit *percentageCommunityWidget;

    QLabel *secondsLabel;
    QLabel *secondsCommunityLabel;

    QMenu *contextMenu;
    QMenu *contextCommunityMenu;

    ColumnAlignedLayout *hlayout;
    ColumnAlignedLayout *hlayout_community;

    void vote_click_handler(const std::string voteString);
    void vote_community_click_handler(const std::string voteString);

    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;
    GUIUtil::TableViewLastColumnResizingFixer *columnCommunityResizingFixer;

    virtual void resizeEvent(QResizeEvent* event);

private Q_SLOTS:
    void contextualMenu(const QPoint &);
    void contextualMenuCommunity(const QPoint &);
    void voteYes();
    void voteNo();
    void voteAbstain();
    void voteYesCommunity();
    void voteNoCommunity();
    void voteAbstainCommunity();
    void openProposalUrl();
    void openDescription();
    void invalidateAlignedLayout();

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);

public Q_SLOTS:
    void refreshProposals(bool force = false);
    void changedProposal(const QString &proposal);
    void changedStartBlock(const QString &minYesVotes);
    void changedEndBlock(const QString &minNoVotes);
    void changedYesVotes(const QString &minYesVotes);
    void changedNoVotes(const QString &minNoVotes);
    void changedAbsoluteYesVotes(const QString &minAbsoluteYesVotes);
    void changedPercentage(const QString &minPercentage);
    void changedAmount(const QString &minAmount);
    void changedProposalCommunity(const QString &proposal);
    void changedEndBlockCommunity(const QString &minNoVotes);
    void changedYesVotesCommunity(const QString &minYesVotes);
    void changedNoVotesCommunity(const QString &minNoVotes);
    void changedAbsoluteYesVotesCommunity(const QString &minAbsoluteYesVotes);
    void changedPercentageCommunity(const QString &minPercentage);

};

#endif // BITCOIN_QT_PROPOSALLIST_H
