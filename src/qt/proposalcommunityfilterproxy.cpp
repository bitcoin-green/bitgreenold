// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "proposalcommunityfilterproxy.h"
#include "proposalcommunitytablemodel.h"

#include <cstdlib>

ProposalCommunityFilterProxy::ProposalCommunityFilterProxy(QObject *parent) :
    QSortFilterProxyModel(parent),
    endBlock(0),
    proposalName(),
    minPercentage(-100),
    minYesVotes(0),
    minNoVotes(0),
    minAbsoluteYesVotes(INT_MIN)
{
}

bool ProposalCommunityFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    int proposalEndBlock = index.data(ProposalCommunityTableModel::EndBlockRole).toInt();
    QString propName = index.data(ProposalCommunityTableModel::ProposalRole).toString();
    int yesVotes = index.data(ProposalCommunityTableModel::YesVotesRole).toInt();
    int noVotes = index.data(ProposalCommunityTableModel::NoVotesRole).toInt();
    int absoluteYesVotes = index.data(ProposalCommunityTableModel::AbsoluteYesVotesRole).toInt();
    int percentage = index.data(ProposalCommunityTableModel::PercentageRole).toInt();

    if(proposalEndBlock < endBlock)
       return false;
    if(!propName.contains(proposalName, Qt::CaseInsensitive))
        return false;
    if(yesVotes < minYesVotes)
        return false;
    if(noVotes < minNoVotes)
        return false;
    if(absoluteYesVotes < minAbsoluteYesVotes)
        return false;
    if(percentage < minPercentage)
        return false;

    return true;
}

void ProposalCommunityFilterProxy::setProposalEnd(const CAmount& block)
{
    this->endBlock = block;
    invalidateFilter();
}

void ProposalCommunityFilterProxy::setProposal(const QString &proposal)
{
    this->proposalName = proposal;
    invalidateFilter();
}

void ProposalCommunityFilterProxy::setMinPercentage(const CAmount& minimum)
{
    this->minPercentage = minimum;
    invalidateFilter();
}

void ProposalCommunityFilterProxy::setMinYesVotes(const CAmount& minimum)
{
    this->minYesVotes = minimum;
    invalidateFilter();
}

void ProposalCommunityFilterProxy::setMinNoVotes(const CAmount& minimum)
{
    this->minNoVotes = minimum;
    invalidateFilter();
}

void ProposalCommunityFilterProxy::setMinAbsoluteYesVotes(const CAmount& minimum)
{
    this->minAbsoluteYesVotes = minimum;
    invalidateFilter();
}

int ProposalCommunityFilterProxy::rowCount(const QModelIndex &parent) const
{
    return QSortFilterProxyModel::rowCount(parent);
}