// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALCOMMUNITYFILTERPROXY_H
#define BITCOIN_QT_PROPOSALCOMMUNITYFILTERPROXY_H

#include "amount.h"

#include <QSortFilterProxyModel>

class ProposalCommunityFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ProposalCommunityFilterProxy(QObject *parent = 0);

    void setProposalEnd(const CAmount& block);
    void setProposal(const QString &proposal);
    void setMinPercentage(const CAmount& minimum);
    void setMinYesVotes(const CAmount& minimum);
    void setMinNoVotes(const CAmount& minimum);
    void setMinAbsoluteYesVotes(const CAmount& minimum);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const;

private:
    qint64 endBlock;
    QString proposalName;
    CAmount minPercentage;
    CAmount minYesVotes;
    CAmount minNoVotes;
    CAmount minAbsoluteYesVotes;
};

#endif // BITCOIN_QT_PROPOSALCOMMUNITYFILTERPROXY_H
