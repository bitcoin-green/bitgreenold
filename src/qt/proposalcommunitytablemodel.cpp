// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "proposalcommunitytablemodel.h"

#include "masternode-vote.h"
#include "masternode-payments.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "masternode-helpers.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "proposalcommunityrecord.h"
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
        Qt::AlignRight|Qt::AlignVCenter
    };

ProposalCommunityTableModel::ProposalCommunityTableModel(CWallet* wallet, WalletModel* parent) : QAbstractTableModel(parent),
                                                                                     wallet(wallet),
                                                                                     walletModel(parent)
{
    columns << tr("Proposal") << tr("End Block") << tr("Yes") << tr("No") << tr("Funded");

    refreshProposals();
}

ProposalCommunityTableModel::~ProposalCommunityTableModel()
{
}

void ProposalCommunityTableModel::refreshProposals() {
    beginResetModel();
    proposalRecords.clear();

    int mnCount = mnodeman.CountEnabled(ActiveProtocol());

    std::vector<CCommunityProposal*> winningProps = communityVote.GetAllProposals();
    for (CCommunityProposal* pcommunityProposal : winningProps) {
        if (!pcommunityProposal->fValid) continue;

        int percentage = 0;
        int64_t absoluteYes = (int64_t)pcommunityProposal->GetYeas() - (int64_t)pcommunityProposal->GetNays();
        if(mnCount > 0) percentage = (int)min(floor(absoluteYes * 100 / (0.1 * mnCount)), 100.0);

        proposalRecords.append(new ProposalCommunityRecord(
                        QString::fromStdString(pcommunityProposal->GetHash().ToString()),
                        (int64_t)pcommunityProposal->GetBlockEnd(),
                        QString::fromStdString(pcommunityProposal->GetDescription()),
                        QString::fromStdString(pcommunityProposal->GetName()),
                        (int64_t)pcommunityProposal->GetYeas(),
                        (int64_t)pcommunityProposal->GetNays(),
                        absoluteYes,
                        percentage));
    }
    endResetModel();
}

int ProposalCommunityTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return proposalRecords.size();
}

int ProposalCommunityTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant ProposalCommunityTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    ProposalCommunityRecord *rec = static_cast<ProposalCommunityRecord*>(index.internalPointer());

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
        case EndBlock:
            return QVariant::fromValue(rec->end_block);
        case Percentage:
            return QString("%1\%").arg(rec->percentage);
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
        case EndBlock:
            return rec->end_block;
        case YesVotes:
            return QVariant::fromValue(rec->yesVotes);
        case NoVotes:
            return QVariant::fromValue(rec->noVotes);
        //case AbsoluteYesVotes:
        //    return QVariant::fromValue(rec->absoluteYesVotes);
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
    case ProposalDescriptionRole:
        return rec->description;
    case ProposalHashRole:
        return rec->hash;
    }
    return QVariant();
}

QVariant ProposalCommunityTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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
            case EndBlock:
                return tr("Block that the proposal ends.");
            case YesVotes:
                return tr("Obtained yes votes.");
            case NoVotes:
                return tr("Obtained no votes.");
            //case AbsoluteYesVotes:
            //    return tr("Obtained absolute yes votes.");
            case Percentage:
                return tr("Current vote percentage.");
            }
        }
    }
    return QVariant();
}

QModelIndex ProposalCommunityTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    if(row >= 0 && row < proposalRecords.size()) {
        ProposalCommunityRecord *rec = proposalRecords[row];
        return createIndex(row, column, rec);
    }

    return QModelIndex();
}