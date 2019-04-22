// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "proposallist.h"
#include "guiutil.h"
#include "platformstyle.h"
#include "optionsmodel.h"
#include "proposalfilterproxy.h"
#include "proposalcommunityfilterproxy.h"
#include "proposaldescriptiondialog.h"
#include "proposalrecord.h"
#include "proposaltablemodel.h"
#include "proposalcommunityrecord.h"
#include "proposalcommunitytablemodel.h"
#include "masternodeman.h"
#include "masternode-budget.h"
#include "masternode-helpers.h"
#include "masternode-vote.h"
#include "masternodeconfig.h"
#include "masternode.h"
#include "util.h"

#include "ui_interface.h"

#include <QComboBox>
#include <QDateTimeEdit>
#include <QDesktopServices>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QMenu>
#include <QPoint>
#include <QScrollBar>
#include <QSettings>
#include <QTableView>
#include <QUrl>
#include <QVBoxLayout>

/** Date format for persistence */
static const char* PERSISTENCE_DATE_FORMAT = "yyyy-MM-dd";

ProposalList::ProposalList(QWidget* parent) : QWidget(parent), proposalProxyModel(0), proposalCommunityProxyModel(0),
    proposalList(0), proposalCommunityList(0), columnResizingFixer(0), columnCommunityResizingFixer(0)
{
    QSettings settings;
    showBudgetProposals = settings.value("fShowBudgetProposalsTab").toBool();
    showCommunityProposals = settings.value("fShowCommunityProposalsTab").toBool();
    /*  Note: the ProposalList is only created if at least one of these 2 sub-tabs is enabled */

    setContentsMargins(0,0,0,0);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setSpacing(0);
    QLabel *logo = new QLabel();
    logo->setObjectName("labelProposalListHeaderRight");
    QHBoxLayout *hlayout_logo = new QHBoxLayout(this);
    hlayout_logo->addStretch();
    hlayout_logo->addWidget(logo);
    logo->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    logo->setMinimumSize(464,60);
    vlayout->addLayout(hlayout_logo);
    vlayout->addSpacing(10);

    QVBoxLayout *vlayout_tabs = new QVBoxLayout(this);
    vlayout_tabs->setSpacing(0);


    QAction *voteYesAction = new QAction(tr("Vote yes"), this);
    QAction *voteAbstainAction = new QAction(tr("Vote abstain"), this);
    QAction *voteNoAction = new QAction(tr("Vote no"), this);

    QTabWidget *tabWidget = new QTabWidget(this);

    /** BUDGET PROPOSAL **/
    if(showBudgetProposals)
    {
        QWidget *budgetView = new QWidget(this);
        QVBoxLayout *vlayout_budget = new QVBoxLayout(this);
        vlayout_budget->setSpacing(0);
        budgetView->setLayout(vlayout_budget);

        tabWidget->addTab(budgetView,"Budget Proposals");


        hlayout = new ColumnAlignedLayout();
        hlayout->setContentsMargins(0,0,0,0);
        hlayout->setSpacing(0);

        proposalWidget = new QLineEdit(this);
        proposalWidget->setPlaceholderText(tr("Enter proposal name"));
        proposalWidget->setObjectName("proposalWidget");
        hlayout->addWidget(proposalWidget);

        amountWidget = new QLineEdit(this);
        amountWidget->setPlaceholderText(tr("Min amount"));
        amountWidget->setValidator(new QDoubleValidator(0, 1e20, 8, this));
        amountWidget->setObjectName("amountWidget");
        hlayout->addWidget(amountWidget);

        startBlockWidget = new QLineEdit(this);
        startBlockWidget->setPlaceholderText(tr("Start Block"));
        startBlockWidget->setValidator(new QIntValidator(0, INT_MAX, this));
        startBlockWidget->setObjectName("startBlockWidget");
        hlayout->addWidget(startBlockWidget);

        endBlockWidget = new QLineEdit(this);
        endBlockWidget->setPlaceholderText(tr("End Block"));
        endBlockWidget->setValidator(new QIntValidator(0, INT_MAX, this));
        endBlockWidget->setObjectName("endBlockWidget");
        hlayout->addWidget(endBlockWidget);

        yesVotesWidget = new QLineEdit(this);
        yesVotesWidget->setPlaceholderText(tr("Min yes votes"));
        yesVotesWidget->setValidator(new QIntValidator(0, INT_MAX, this));
        yesVotesWidget->setObjectName("yesVotesWidget");
        hlayout->addWidget(yesVotesWidget);

        noVotesWidget = new QLineEdit(this);
        noVotesWidget->setPlaceholderText(tr("Min no votes"));
        noVotesWidget->setValidator(new QIntValidator(0, INT_MAX, this));
        noVotesWidget->setObjectName("noVotesWidget");
        hlayout->addWidget(noVotesWidget);

        //absoluteYesVotesWidget = new QLineEdit(this);
        //absoluteYesVotesWidget->setPlaceholderText(tr("Min abs. yes votes"));
        //absoluteYesVotesWidget->setValidator(new QIntValidator(INT_MIN, INT_MAX, this));
        //absoluteYesVotesWidget->setObjectName("absoluteYesVotesWidget");
        //hlayout->addWidget(absoluteYesVotesWidget);

        percentageWidget = new QLineEdit(this);
        percentageWidget->setPlaceholderText(tr("Min percentage"));
        percentageWidget->setValidator(new QIntValidator(-100, 100, this));
        percentageWidget->setObjectName("percentageWidget");
        hlayout->addWidget(percentageWidget);

        QTableView *view = new QTableView(this);
        vlayout_budget->addLayout(hlayout);
        vlayout_budget->addWidget(view);
        vlayout_budget->setSpacing(0);
        int width = view->verticalScrollBar()->sizeHint().width();
        hlayout->addSpacing(width);
        hlayout->setTableColumnsToTrack(view->horizontalHeader());

        connect(view->horizontalHeader(), SIGNAL(sectionResized(int,int,int)), SLOT(invalidateAlignedLayout()));
        connect(view->horizontalScrollBar(), SIGNAL(valueChanged(int)), SLOT(invalidateAlignedLayout()));

        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        view->setTabKeyNavigation(false);
        view->setContextMenuPolicy(Qt::CustomContextMenu);

        proposalList = view;

        QHBoxLayout *actionBar = new QHBoxLayout();
        actionBar->setSpacing(11);
        actionBar->setContentsMargins(0,20,0,20);

        QPushButton *voteYesButton = new QPushButton(tr("Vote Yes"), this);
        voteYesButton->setToolTip(tr("Vote Yes on the selected proposal"));
        actionBar->addWidget(voteYesButton);

        QPushButton *voteAbstainButton = new QPushButton(tr("Vote Abstain"), this);
        voteAbstainButton->setToolTip(tr("Vote Abstain on the selected proposal"));
        actionBar->addWidget(voteAbstainButton);

        QPushButton *voteNoButton = new QPushButton(tr("Vote No"), this);
        voteNoButton->setToolTip(tr("Vote No on the selected proposal"));
        actionBar->addWidget(voteNoButton);

        secondsLabel = new QLabel();
        actionBar->addWidget(secondsLabel);
        actionBar->addStretch();

        vlayout_budget->addLayout(actionBar);

        QAction *openUrlAction = new QAction(tr("Visit proposal website"), this);

        contextMenu = new QMenu(this);
        contextMenu->addAction(voteYesAction);
        contextMenu->addAction(voteAbstainAction);
        contextMenu->addAction(voteNoAction);
        contextMenu->addSeparator();
        contextMenu->addAction(openUrlAction);

        connect(voteYesButton, SIGNAL(clicked()), this, SLOT(voteYes()));
        connect(voteAbstainButton, SIGNAL(clicked()), this, SLOT(voteAbstain()));
        connect(voteNoButton, SIGNAL(clicked()), this, SLOT(voteNo()));

        connect(proposalWidget, SIGNAL(textChanged(QString)), this, SLOT(changedProposal(QString)));

        connect(startBlockWidget, SIGNAL(textChanged(QString)), this, SLOT(changedStartBlock(QString)));
        connect(endBlockWidget, SIGNAL(textChanged(QString)), this, SLOT(changedEndBlock(QString)));
        connect(yesVotesWidget, SIGNAL(textChanged(QString)), this, SLOT(changedYesVotes(QString)));
        connect(noVotesWidget, SIGNAL(textChanged(QString)), this, SLOT(changedNoVotes(QString)));
        //connect(absoluteYesVotesWidget, SIGNAL(textChanged(QString)), this, SLOT(changedAbsoluteYesVotes(QString)));
        connect(yesVotesWidget, SIGNAL(textChanged(QString)), this, SLOT(changedYesVotes(QString)));
        connect(amountWidget, SIGNAL(textChanged(QString)), this, SLOT(changedAmount(QString)));
        connect(percentageWidget, SIGNAL(textChanged(QString)), this, SLOT(changedPercentage(QString)));

        connect(view, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openProposalUrl()));
        connect(view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

        connect(voteYesAction, SIGNAL(triggered()), this, SLOT(voteYes()));
        connect(voteNoAction, SIGNAL(triggered()), this, SLOT(voteNo()));
        connect(voteAbstainAction, SIGNAL(triggered()), this, SLOT(voteAbstain()));

        connect(openUrlAction, SIGNAL(triggered()), this, SLOT(openProposalUrl()));
    }



    /** COMMUNITY PROPOSAL **/
    if(showCommunityProposals)
    {
        QWidget *communityView = new QWidget(this);
        QVBoxLayout *vlayout_community = new QVBoxLayout(this);
        vlayout_community->setSpacing(0);
        communityView->setLayout(vlayout_community);

        tabWidget->addTab(communityView,"Community Proposals");


        hlayout_community = new ColumnAlignedLayout();
        hlayout_community->setContentsMargins(0,0,0,0);
        hlayout_community->setSpacing(0);

        proposalCommunityWidget = new QLineEdit(this);
        proposalCommunityWidget->setPlaceholderText(tr("Enter proposal name"));
        proposalCommunityWidget->setObjectName("proposalCommunityWidget");
        hlayout_community->addWidget(proposalCommunityWidget);


        endBlockCommunityWidget = new QLineEdit(this);
        endBlockCommunityWidget->setPlaceholderText(tr("End Block"));
        endBlockCommunityWidget->setValidator(new QIntValidator(0, INT_MAX, this));
        endBlockCommunityWidget->setObjectName("endBlockCommunityWidget");
        hlayout_community->addWidget(endBlockCommunityWidget);

        yesVotesCommunityWidget = new QLineEdit(this);
        yesVotesCommunityWidget->setPlaceholderText(tr("Min yes votes"));
        yesVotesCommunityWidget->setValidator(new QIntValidator(0, INT_MAX, this));
        yesVotesCommunityWidget->setObjectName("yesVotesCommunityWidget");
        hlayout_community->addWidget(yesVotesCommunityWidget);

        noVotesCommunityWidget = new QLineEdit(this);
        noVotesCommunityWidget->setPlaceholderText(tr("Min no votes"));
        noVotesCommunityWidget->setValidator(new QIntValidator(0, INT_MAX, this));
        noVotesCommunityWidget->setObjectName("noVotesCommunityWidget");
        hlayout_community->addWidget(noVotesCommunityWidget);


        //absoluteYesVotesCommunityWidget = new QLineEdit(this);
        //absoluteYesVotesCommunityWidget->setPlaceholderText(tr("Min abs. yes votes"));
        //absoluteYesVotesCommunityWidget->setValidator(new QIntValidator(INT_MIN, INT_MAX, this));
        //absoluteYesVotesCommunityWidget->setObjectName("absoluteYesVotesCommunityWidget");
        //hlayout_community->addWidget(absoluteYesVotesCommunityWidget);

        percentageCommunityWidget = new QLineEdit(this);
        percentageCommunityWidget->setPlaceholderText(tr("Min percentage"));
        percentageCommunityWidget->setValidator(new QIntValidator(-100, 100, this));
        percentageCommunityWidget->setObjectName("percentageCommunityWidget");
        hlayout_community->addWidget(percentageCommunityWidget);

        QTableView *view_community = new QTableView(this);
        vlayout_community->addLayout(hlayout_community);
        vlayout_community->addWidget(view_community);
        vlayout_community->setSpacing(0);
        int widthCommunity = view_community->verticalScrollBar()->sizeHint().width();
        hlayout_community->addSpacing(widthCommunity);
        hlayout_community->setTableColumnsToTrack(view_community->horizontalHeader());

        connect(view_community->horizontalHeader(), SIGNAL(sectionResized(int,int,int)), SLOT(invalidateAlignedLayout()));
        connect(view_community->horizontalScrollBar(), SIGNAL(valueChanged(int)), SLOT(invalidateAlignedLayout()));

        view_community->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        view_community->setTabKeyNavigation(false);
        view_community->setContextMenuPolicy(Qt::CustomContextMenu);

        proposalCommunityList = view_community;

        QHBoxLayout *actionCommunityBar = new QHBoxLayout();
        actionCommunityBar->setSpacing(11);
        actionCommunityBar->setContentsMargins(0,20,0,20);

        QPushButton *voteYesCommunityButton = new QPushButton(tr("Vote Yes"), this);
        voteYesCommunityButton->setToolTip(tr("Vote Yes on the selected proposal"));
        actionCommunityBar->addWidget(voteYesCommunityButton);

        QPushButton *voteAbstainCommunityButton = new QPushButton(tr("Vote Abstain"), this);
        voteAbstainCommunityButton->setToolTip(tr("Vote Abstain on the selected proposal"));
        actionCommunityBar->addWidget(voteAbstainCommunityButton);

        QPushButton *voteNoCommunityButton = new QPushButton(tr("Vote No"), this);
        voteNoCommunityButton->setToolTip(tr("Vote No on the selected proposal"));
        actionCommunityBar->addWidget(voteNoCommunityButton);

        secondsCommunityLabel = new QLabel();
        actionCommunityBar->addWidget(secondsCommunityLabel);
        actionCommunityBar->addStretch();

        vlayout_community->addLayout(actionCommunityBar);

        QAction *voteYesCommunityAction = new QAction(tr("Vote yes"), this);
        QAction *voteAbstainCommunityAction = new QAction(tr("Vote abstain"), this);
        QAction *voteNoCommunityAction = new QAction(tr("Vote no"), this);
        QAction *openDescriptionAction = new QAction(tr("Read description"), this);

        contextCommunityMenu = new QMenu(this);
        contextCommunityMenu->addAction(voteYesCommunityAction);
        contextCommunityMenu->addAction(voteAbstainCommunityAction);
        contextCommunityMenu->addAction(voteNoCommunityAction);
        contextCommunityMenu->addSeparator();
        contextCommunityMenu->addAction(openDescriptionAction);

        connect(voteYesCommunityButton, SIGNAL(clicked()), this, SLOT(voteYesCommunity()));
        connect(voteAbstainCommunityButton, SIGNAL(clicked()), this, SLOT(voteAbstainCommunity()));
        connect(voteNoCommunityButton, SIGNAL(clicked()), this, SLOT(voteNoCommunity()));

        connect(proposalCommunityWidget, SIGNAL(textChanged(QString)), this, SLOT(changedProposalCommunity(QString)));

        connect(endBlockCommunityWidget, SIGNAL(textChanged(QString)), this, SLOT(changedEndBlockCommunity(QString)));
        connect(yesVotesCommunityWidget, SIGNAL(textChanged(QString)), this, SLOT(changedYesVotesCommunity(QString)));
        connect(noVotesCommunityWidget, SIGNAL(textChanged(QString)), this, SLOT(changedNoVotesCommunity(QString)));
        //connect(absoluteYesVotesCommunityWidget, SIGNAL(textChanged(QString)), this, SLOT(changedAbsoluteYesVotesCommunity(QString)));
        connect(yesVotesCommunityWidget, SIGNAL(textChanged(QString)), this, SLOT(changedYesVotesCommunity(QString)));
        connect(percentageCommunityWidget, SIGNAL(textChanged(QString)), this, SLOT(changedPercentageCommunity(QString)));

        connect(view_community, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openDescription()));
        connect(view_community, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenuCommunity(QPoint)));

        connect(voteYesAction, SIGNAL(triggered()), this, SLOT(voteYesCommunity()));
        connect(voteNoAction, SIGNAL(triggered()), this, SLOT(voteNoCommunity()));
        connect(voteAbstainAction, SIGNAL(triggered()), this, SLOT(voteAbstainCommunity()));

        connect(openDescriptionAction, SIGNAL(triggered()), this, SLOT(openDescription()));
    }

    vlayout_tabs->addWidget(tabWidget);
    vlayout->addLayout(vlayout_tabs);

    setLayout(vlayout);
}

void ProposalList::invalidateAlignedLayout() {
    if(showBudgetProposals)
    {
        hlayout->invalidate();
    }

    if(showCommunityProposals)
    {
        hlayout_community->invalidate();
    }
}

void ProposalList::setModel(WalletModel* model) {
    this->model = model;
    if(model) {

        if(showBudgetProposals)
        {
            proposalProxyModel = new ProposalFilterProxy(this);
            proposalProxyModel->setSourceModel(model->getProposalTableModel());
            proposalProxyModel->setDynamicSortFilter(true);
            proposalProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
            proposalProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

            proposalProxyModel->setSortRole(Qt::EditRole);

            proposalList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            proposalList->setModel(proposalProxyModel);
            proposalList->setAlternatingRowColors(true);
            proposalList->setSelectionBehavior(QAbstractItemView::SelectRows);
            proposalList->setSortingEnabled(true);
            proposalList->sortByColumn(ProposalTableModel::StartBlock, Qt::DescendingOrder);
            proposalList->verticalHeader()->hide();

            proposalList->setColumnWidth(ProposalTableModel::Proposal, PROPOSAL_COLUMN_WIDTH);
            proposalList->setColumnWidth(ProposalTableModel::StartBlock, START_DATE_COLUMN_WIDTH);
            proposalList->setColumnWidth(ProposalTableModel::EndBlock, END_DATE_COLUMN_WIDTH);
            proposalList->setColumnWidth(ProposalTableModel::YesVotes, YES_VOTES_COLUMN_WIDTH);
            proposalList->setColumnWidth(ProposalTableModel::NoVotes, NO_VOTES_COLUMN_WIDTH);
            //proposalList->setColumnWidth(ProposalTableModel::AbsoluteYesVotes, ABSOLUTE_YES_COLUMN_WIDTH);
            proposalList->setColumnWidth(ProposalTableModel::Amount, AMOUNT_COLUMN_WIDTH);
            proposalList->setColumnWidth(ProposalTableModel::Percentage, PERCENTAGE_COLUMN_WIDTH);

            columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(proposalList, PERCENTAGE_COLUMN_WIDTH, MINIMUM_COLUMN_WIDTH);
        }

        if(showCommunityProposals)
        {
            proposalCommunityProxyModel = new ProposalCommunityFilterProxy(this);
            proposalCommunityProxyModel->setSourceModel(model->getProposalCommunityTableModel());
            proposalCommunityProxyModel->setDynamicSortFilter(true);
            proposalCommunityProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
            proposalCommunityProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

            proposalCommunityProxyModel->setSortRole(Qt::EditRole);

            proposalCommunityList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            proposalCommunityList->setModel(proposalCommunityProxyModel);
            proposalCommunityList->setAlternatingRowColors(true);
            proposalCommunityList->setSelectionBehavior(QAbstractItemView::SelectRows);
            proposalCommunityList->setSortingEnabled(true);
            proposalCommunityList->sortByColumn(ProposalTableModel::EndBlock, Qt::DescendingOrder);
            proposalCommunityList->verticalHeader()->hide();

            proposalCommunityList->setColumnWidth(ProposalCommunityTableModel::Proposal, PROPOSAL_COLUMN_WIDTH);
            proposalCommunityList->setColumnWidth(ProposalCommunityTableModel::EndBlock, END_DATE_COLUMN_WIDTH);
            proposalCommunityList->setColumnWidth(ProposalCommunityTableModel::YesVotes, YES_VOTES_COLUMN_WIDTH);
            proposalCommunityList->setColumnWidth(ProposalCommunityTableModel::NoVotes, NO_VOTES_COLUMN_WIDTH);
            //proposalCommunityList->setColumnWidth(ProposalCommunityTableModel::AbsoluteYesVotes, ABSOLUTE_YES_COLUMN_WIDTH);
            proposalCommunityList->setColumnWidth(ProposalCommunityTableModel::Percentage, PERCENTAGE_COLUMN_WIDTH);

            columnCommunityResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(proposalCommunityList, PERCENTAGE_COLUMN_WIDTH, MINIMUM_COLUMN_WIDTH);
        }

        nLastUpdate = GetTime();

        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(refreshProposals()));
        timer->start(1000);

        if(showBudgetProposals)
        {
            proposalList->horizontalHeader()->setStretchLastSection(false);
            proposalList->horizontalHeader()->setSectionResizeMode(ProposalTableModel::Percentage, QHeaderView::Stretch);
        }

        if(showCommunityProposals)
        {
            proposalCommunityList->horizontalHeader()->setStretchLastSection(false);
            proposalCommunityList->horizontalHeader()->setSectionResizeMode(ProposalCommunityTableModel::Percentage, QHeaderView::Stretch);
        }
    }
}

void ProposalList::refreshProposals(bool force) {
    int64_t secondsRemaining = nLastUpdate - GetTime() + PROPOSALLIST_UPDATE_SECONDS;

    QString secOrMinutes = ((((double)(secondsRemaining / 60))) >= 1) ? tr("minute(s)") : tr("second(s)");
    if(showBudgetProposals)
    {
        secondsLabel->setText(tr("List will be updated in %1 %2").arg((secondsRemaining >= 60) ? QString::number(secondsRemaining / 60) : QString::number(secondsRemaining), secOrMinutes));
    }

    if(showCommunityProposals)
    {
        secondsCommunityLabel->setText(tr("List will be updated in %1 %2").arg((secondsRemaining >= 60) ? QString::number(secondsRemaining / 60) : QString::number(secondsRemaining), secOrMinutes));
    }

    if(secondsRemaining > 0 && !force) return;
    nLastUpdate = GetTime();


    if(showBudgetProposals)
    {
        model->getProposalTableModel()->refreshProposals();
        secondsLabel->setText(tr("List will be updated in 0 second(s)"));
    }

    if(showCommunityProposals)
    {
        model->getProposalCommunityTableModel()->refreshProposals();
        secondsCommunityLabel->setText(tr("List will be updated in 0 second(s)"));
    }
}

void ProposalList::changedStartBlock(const QString &minAmount)
{
    if(!proposalProxyModel)
        return;

    proposalProxyModel->setProposalStart(minAmount.toInt());
}

void ProposalList::changedEndBlock(const QString &minAmount)
{
    if(!proposalProxyModel)
        return;

    proposalProxyModel->setProposalEnd(minAmount.toInt());
}

void ProposalList::changedAmount(const QString &minAmount)
{
    if(!proposalProxyModel)
        return;

    proposalProxyModel->setMinAmount(minAmount.toLongLong());
}

void ProposalList::changedPercentage(const QString &minPercentage)
{
    if(!proposalProxyModel)
        return;

    int value = minPercentage == "" ? -100 : minPercentage.toInt();

    proposalProxyModel->setMinPercentage(value);
}

void ProposalList::changedProposal(const QString &proposal)
{
    if(!proposalProxyModel)
        return;

    proposalProxyModel->setProposal(proposal);
}

void ProposalList::changedYesVotes(const QString &minYesVotes)
{
    if(!proposalProxyModel)
        return;

    proposalProxyModel->setMinYesVotes(minYesVotes.toInt());
}

void ProposalList::changedNoVotes(const QString &minNoVotes)
{
    if(!proposalProxyModel)
        return;

    proposalProxyModel->setMinNoVotes(minNoVotes.toInt());
}

void ProposalList::changedAbsoluteYesVotes(const QString &minAbsoluteYesVotes)
{
    if(!proposalProxyModel)
        return;

    int value = minAbsoluteYesVotes == "" ? INT_MIN : minAbsoluteYesVotes.toInt();

    proposalProxyModel->setMinAbsoluteYesVotes(value);
}

void ProposalList::contextualMenu(const QPoint &point)
{
    QModelIndex index = proposalList->indexAt(point);
    QModelIndexList selection = proposalList->selectionModel()->selectedRows(0);
    if (selection.empty())
        return;

    if(index.isValid())
        contextMenu->exec(QCursor::pos());
}

void ProposalList::voteYes()
{
    vote_click_handler("yes");
}

void ProposalList::voteNo()
{
    vote_click_handler("no");
}

void ProposalList::voteAbstain()
{
    vote_click_handler("abstain");
}

void ProposalList::changedEndBlockCommunity(const QString &minAmount)
{
    if(!proposalCommunityProxyModel)
        return;

    proposalCommunityProxyModel->setProposalEnd(minAmount.toInt());
}

void ProposalList::changedPercentageCommunity(const QString &minPercentage)
{
    if(!proposalCommunityProxyModel)
        return;

    int value = minPercentage == "" ? -100 : minPercentage.toInt();

    proposalCommunityProxyModel->setMinPercentage(value);
}

void ProposalList::changedProposalCommunity(const QString &proposal)
{
    if(!proposalCommunityProxyModel)
        return;

    proposalCommunityProxyModel->setProposal(proposal);
}

void ProposalList::changedYesVotesCommunity(const QString &minYesVotes)
{
    if(!proposalCommunityProxyModel)
        return;

    proposalCommunityProxyModel->setMinYesVotes(minYesVotes.toInt());
}

void ProposalList::changedNoVotesCommunity(const QString &minNoVotes)
{
    if(!proposalCommunityProxyModel)
        return;

    proposalCommunityProxyModel->setMinNoVotes(minNoVotes.toInt());
}

void ProposalList::changedAbsoluteYesVotesCommunity(const QString &minAbsoluteYesVotes)
{
    if(!proposalCommunityProxyModel)
        return;

    int value = minAbsoluteYesVotes == "" ? INT_MIN : minAbsoluteYesVotes.toInt();

    proposalCommunityProxyModel->setMinAbsoluteYesVotes(value);
}

void ProposalList::contextualMenuCommunity(const QPoint &point)
{
    QModelIndex index = proposalCommunityList->indexAt(point);
    QModelIndexList selection = proposalCommunityList->selectionModel()->selectedRows(0);
    if (selection.empty())
        return;

    if(index.isValid())
        contextCommunityMenu->exec(QCursor::pos());
}

void ProposalList::voteYesCommunity()
{
    vote_community_click_handler("yes");
}

void ProposalList::voteNoCommunity()
{
    vote_community_click_handler("no");
}

void ProposalList::voteAbstainCommunity()
{
    vote_community_click_handler("abstain");
}

void ProposalList::vote_community_click_handler(const std::string voteString)
{
    if(!showCommunityProposals)
    {
        return;
    }

    if(!proposalCommunityList->selectionModel())
        return;

    QModelIndexList selection = proposalCommunityList->selectionModel()->selectedRows();
    if(selection.empty())
        return;

    QString proposalName = selection.at(0).data(ProposalCommunityTableModel::ProposalRole).toString();

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm vote"),
        tr("Are you sure you want to vote <strong>%1</strong> on the proposal <strong>%2</strong>?").arg(QString::fromStdString(voteString), proposalName),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) return;

    uint256 hash;
    hash.SetHex(selection.at(0).data(ProposalCommunityTableModel::ProposalHashRole).toString().toStdString());

    int nVote = VOTE_ABSTAIN;
    if(voteString == "yes") nVote = VOTE_YES;
    else if(voteString == "no") nVote = VOTE_NO;

    int nSuccessful = 0;
    int nFailed = 0;

    BOOST_FOREACH (CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
        std::string errorMessage;
        CPubKey pubKeyMasternode;
        CKey keyMasternode;

        if(!masternodeSigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)) {
            nFailed++;
            continue;
        }

        CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
        if(pmn == nullptr) {
            nFailed++;
            continue;
        }

        CCommunityVote vote(pmn->vin, hash, nVote);
        if(!vote.Sign(keyMasternode, pubKeyMasternode)) {
            nFailed++;
            continue;
        }

        std::string strError = "";
        if(communityVote.UpdateProposal(vote, nullptr, strError)) {
            communityVote.mapSeenMasternodeCommunityVotes.insert(make_pair(vote.GetHash(), vote));
            vote.Relay();
            nSuccessful++;
        } else {
            nFailed++;
        }
    }

    QMessageBox::information(this, tr("Voting"),
        tr("You voted %1 %2 time(s) successfully and failed %3 time(s) on %4").arg(QString::fromStdString(voteString), QString::number(nSuccessful), QString::number(nFailed), proposalName));

    refreshProposals(true);
}

void ProposalList::vote_click_handler(const std::string voteString)
{
    if(!showBudgetProposals)
    {
        return;
    }

    if(!proposalList->selectionModel())
        return;

    QModelIndexList selection = proposalList->selectionModel()->selectedRows();
    if(selection.empty())
        return;

    QString proposalName = selection.at(0).data(ProposalTableModel::ProposalRole).toString();

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm vote"),
        tr("Are you sure you want to vote <strong>%1</strong> on the proposal <strong>%2</strong>?").arg(QString::fromStdString(voteString), proposalName),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) return;

    uint256 hash;
    hash.SetHex(selection.at(0).data(ProposalTableModel::ProposalHashRole).toString().toStdString());

    int nVote = VOTE_ABSTAIN;
    if(voteString == "yes") nVote = VOTE_YES;
    else if(voteString == "no") nVote = VOTE_NO;

    int nSuccessful = 0;
    int nFailed = 0;

    BOOST_FOREACH (CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
        std::string errorMessage;
        CPubKey pubKeyMasternode;
        CKey keyMasternode;

        if(!masternodeSigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)) {
            nFailed++;
            continue;
        }

        CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
        if(pmn == nullptr) {
            nFailed++;
            continue;
        }

        CBudgetVote vote(pmn->vin, hash, nVote);
        if(!vote.Sign(keyMasternode, pubKeyMasternode)) {
            nFailed++;
            continue;
        }

        std::string strError = "";
        if(budget.UpdateProposal(vote, nullptr, strError)) {
            budget.mapSeenMasternodeBudgetVotes.insert(make_pair(vote.GetHash(), vote));
            vote.Relay();
            nSuccessful++;
        } else {
            nFailed++;
        }
    }

    QMessageBox::information(this, tr("Voting"),
        tr("You voted %1 %2 time(s) successfully and failed %3 time(s) on %4").arg(QString::fromStdString(voteString), QString::number(nSuccessful), QString::number(nFailed), proposalName));

    refreshProposals(true);
}

void ProposalList::openProposalUrl()
{
    if(!proposalList || !proposalList->selectionModel())
        return;

    QModelIndexList selection = proposalList->selectionModel()->selectedRows(0);
    if(!selection.isEmpty()) {
        QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Visit proposal website"),
        tr("Are you sure you want to visit the proposal website \"%1\" ?").arg(selection.at(0).data(ProposalTableModel::ProposalUrlRole).toString()),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

        if(retval != QMessageBox::Yes) return;
        QDesktopServices::openUrl(selection.at(0).data(ProposalTableModel::ProposalUrlRole).toString());
    }
}

void ProposalList::openDescription()
{
    if(!proposalCommunityList || !proposalCommunityList->selectionModel())
        return;

    QModelIndexList selection = proposalCommunityList->selectionModel()->selectedRows(0);

    if(!selection.isEmpty()) {
        ProposalDescriptionDialog *propDesc = new ProposalDescriptionDialog(selection.at(0));
        propDesc->exec();
    }
}

void ProposalList::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    if(showBudgetProposals)
    {
        columnResizingFixer->stretchColumnWidth(ProposalTableModel::Proposal);
    }

    if(showCommunityProposals)
    {
        columnCommunityResizingFixer->stretchColumnWidth(ProposalCommunityTableModel::Proposal);
    }
}