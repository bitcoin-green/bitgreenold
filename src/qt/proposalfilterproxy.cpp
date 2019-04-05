// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "proposalfilterproxy.h"
#include "proposaltablemodel.h"


#include "guiutil.h"
#include "util.h"

#include <cstdlib>

ProposalFilterProxy::ProposalFilterProxy(QObject *parent) :
    QSortFilterProxyModel(parent),
    startBlock(0),
    endBlock(0),
    proposalName(),
    minAmount(0),
    minPercentage(-100),
    minYesVotes(0),
    minNoVotes(0),
    minAbsoluteYesVotes(INT_MIN)
{
}

bool ProposalFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    int proposalStartBlock = index.data(ProposalTableModel::StartBlockRole).toInt();
    int proposalEndBlock = index.data(ProposalTableModel::EndBlockRole).toInt();
    QString propName = index.data(ProposalTableModel::ProposalRole).toString();
    qint64 amount = llabs(index.data(ProposalTableModel::AmountRole).toLongLong());
    int yesVotes = index.data(ProposalTableModel::YesVotesRole).toInt();
    int noVotes = index.data(ProposalTableModel::NoVotesRole).toInt();
    int absoluteYesVotes = index.data(ProposalTableModel::AbsoluteYesVotesRole).toInt();
    int percentage = index.data(ProposalTableModel::PercentageRole).toInt();

    CAmount nAmount = amount / BitcoinUnits::factor(BitcoinUnits::BITG);
    if(proposalStartBlock < startBlock)
       return false;
    if(proposalEndBlock < endBlock)
       return false;
    if(!propName.contains(proposalName, Qt::CaseInsensitive))
        return false;
    if(nAmount < minAmount)
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

void ProposalFilterProxy::setProposalStart(const CAmount& block)
{
    this->startBlock = block;
    invalidateFilter();
}

void ProposalFilterProxy::setProposalEnd(const CAmount& block)
{
    this->endBlock = block;
    invalidateFilter();
}

void ProposalFilterProxy::setProposal(const QString &proposal)
{
    this->proposalName = proposal;
    invalidateFilter();
}

void ProposalFilterProxy::setMinAmount(const CAmount& minimum)
{
    this->minAmount = minimum;
    invalidateFilter();
}

void ProposalFilterProxy::setMinPercentage(const CAmount& minimum)
{
    this->minPercentage = minimum;
    invalidateFilter();
}

void ProposalFilterProxy::setMinYesVotes(const CAmount& minimum)
{
    this->minYesVotes = minimum;
    invalidateFilter();
}

void ProposalFilterProxy::setMinNoVotes(const CAmount& minimum)
{
    this->minNoVotes = minimum;
    invalidateFilter();
}

void ProposalFilterProxy::setMinAbsoluteYesVotes(const CAmount& minimum)
{
    this->minAbsoluteYesVotes = minimum;
    invalidateFilter();
}

int ProposalFilterProxy::rowCount(const QModelIndex &parent) const
{
    return QSortFilterProxyModel::rowCount(parent);
}