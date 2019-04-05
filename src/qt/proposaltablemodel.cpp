// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "proposaltablemodel.h"

#include "masternode-budget.h"
#include "masternode-payments.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "masternode-helpers.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "proposalrecord.h"
#include "optionsmodel.h"
#include "utilmoneystr.h"
#include "walletmodel.h"

#include <univalue.h>

#include "main.h"
#include "sync.h"
#include "uint256.h"
#include "util.h"
#include "wallet.h"

#include <boost/foreach.hpp>


static int column_alignments[] = {
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
       // Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter
    };

ProposalTableModel::ProposalTableModel(CWallet* wallet, WalletModel* parent) : QAbstractTableModel(parent),
                                                                                     wallet(wallet),
                                                                                     walletModel(parent)
{
    columns << tr("Proposal") << tr("Budget") << tr("Start Block") << tr("End Block") << tr("Yes") << tr("No") << tr("Funded");

    refreshProposals();
}

ProposalTableModel::~ProposalTableModel()
{
}

void ProposalTableModel::refreshProposals() {
    beginResetModel();
    proposalRecords.clear();

    int mnCount = mnodeman.CountEnabled(ActiveProtocol());

    std::vector<CBudgetProposal*> winningProps = budget.GetAllProposals();
    BOOST_FOREACH (CBudgetProposal* pbudgetProposal, winningProps) {
        if(!pbudgetProposal->fValid) continue;

        int percentage = 0;
        int64_t absoluteYes = (int64_t)pbudgetProposal->GetYeas() - (int64_t)pbudgetProposal->GetNays();
        if(mnCount > 0) percentage = (int)min(floor(absoluteYes * 100 / (0.1 * mnCount)), 100.0);

        proposalRecords.append(new ProposalRecord(
                        QString::fromStdString(pbudgetProposal->GetHash().ToString()),
                        (int64_t)pbudgetProposal->GetBlockStart(),
                        (int64_t)pbudgetProposal->GetBlockEnd(),
                        QString::fromStdString(pbudgetProposal->GetURL()),
                        QString::fromStdString(pbudgetProposal->GetName()),
                        (int64_t)pbudgetProposal->GetYeas(),
                        (int64_t)pbudgetProposal->GetNays(),
                        absoluteYes,
                        pbudgetProposal->GetAmount(),
                        percentage));
    }
    endResetModel();
}

int ProposalTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return proposalRecords.size();
}

int ProposalTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant ProposalTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    ProposalRecord *rec = static_cast<ProposalRecord*>(index.internalPointer());

    int mnCount = mnodeman.CountEnabled(ActiveProtocol());

    switch(role)
    {
    case Qt::DisplayRole:
        switch(index.column())
        {
        case Proposal:
            return rec->name;
        case YesVotes:
            return QVariant::fromValue(rec->yesVotes);
        case NoVotes:
            return QVariant::fromValue(rec->noVotes);
        //case AbsoluteYesVotes:
        //    return QVariant::fromValue(rec->absoluteYesVotes);
        case StartBlock:
            return QVariant::fromValue(rec->start_block);
        case EndBlock:
            return QVariant::fromValue(rec->end_block);
        case Percentage:
            if(rec->percentage < 100) return tr("%1\%").arg(rec->percentage);
            return QString("%1\%").arg(rec->percentage);
        case Amount:
            return QString("%1 / Superblock").arg(BitcoinUnits::floorWithUnit(BitcoinUnits::BITG, rec->amount));
        }
        break;
    case Qt::ToolTipRole:
        switch(index.column())
        {
            case Percentage:
                if(rec->percentage < 100) return tr("%2 yes votes missing").arg(QString::number(ceil((0.1 * mnCount) - rec->absoluteYesVotes)));
            default:
                return QVariant();
        }
        break;
    case Qt::EditRole:
        switch(index.column())
        {
        case Proposal:
            return rec->name;
        case StartBlock:
            return rec->start_block;
        case EndBlock:
            return rec->end_block;
        case YesVotes:
            return QVariant::fromValue(rec->yesVotes);
        case NoVotes:
            return QVariant::fromValue(rec->noVotes);
        //case AbsoluteYesVotes:
        //    return QVariant::fromValue(rec->absoluteYesVotes);
        case Amount:
            return qint64(rec->amount);
        case Percentage:
            return QVariant::fromValue(rec->percentage);
        }
        break;
    case Qt::TextAlignmentRole:
        return column_alignments[index.column()];
    case Qt::ForegroundRole:
        if(index.column() == Percentage) {
            if(rec->percentage < 100) {
                return COLOR_NEGATIVE;
            } else {
                return QColor(23, 168, 26);
            }
        }

        return COLOR_BAREADDRESS;
        break;
    case ProposalRole:
        return rec->name;
    case AmountRole:
        return QVariant::fromValue(rec->amount);
    case StartBlockRole:
        return rec->start_block;
    case EndBlockRole:
        return rec->end_block;
    case YesVotesRole:
        return QVariant::fromValue(rec->yesVotes);
    case NoVotesRole:
        return QVariant::fromValue(rec->noVotes);
    case AbsoluteYesVotesRole:
        return QVariant::fromValue(rec->absoluteYesVotes);
    case PercentageRole:
        return QVariant::fromValue(rec->percentage);
    case ProposalUrlRole:
        return rec->url;
    case ProposalHashRole:
        return rec->hash;
    }
    return QVariant();
}

QVariant ProposalTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole)
        {
            return columns[section];
        }
        else if (role == Qt::TextAlignmentRole)
        {
            return Qt::AlignCenter;
        }
        else if (role == Qt::ToolTipRole)
        {
            switch(section)
            {
            case Proposal:
                return tr("Proposal Name");
            case StartBlock:
                return tr("Block that the proposal starts.");
            case EndBlock:
                return tr("Block that the proposal ends.");
            case YesVotes:
                return tr("Obtained yes votes.");
            case NoVotes:
                return tr("Obtained no votes.");
            //case AbsoluteYesVotes:
            //    return tr("Obtained absolute yes votes.");
            case Amount:
                return tr("Proposed amount.");
            case Percentage:
                return tr("Current vote percentage.");
            }
        }
    }
    return QVariant();
}

QModelIndex ProposalTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    if(row >= 0 && row < proposalRecords.size()) {
        ProposalRecord *rec = proposalRecords[row];
        return createIndex(row, column, rec);
    }

    return QModelIndex();
}