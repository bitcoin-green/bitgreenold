// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALCOMMUNITYTABLEMODEL_H
#define BITCOIN_QT_PROPOSALCOMMUNITYTABLEMODEL_H

#include "bitcoinunits.h"

#include <QAbstractTableModel>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

class PlatformStyle;
class ProposalCommunityRecord;
class ProposalCommunityTablePriv;
class WalletModel;

class CWallet;

class ProposalCommunityTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ProposalCommunityTableModel(CWallet* wallet, WalletModel* parent = 0);
    ~ProposalCommunityTableModel();

    enum ColumnIndex {
        Proposal = 0,
        EndBlock = 1,
        YesVotes = 2,
        NoVotes = 3,
        //AbsoluteYesVotes = 4,
        Percentage = 4
    };

    enum RoleIndex {
        ProposalRole = Qt::UserRole,
        EndBlockRole,
        YesVotesRole,
        NoVotesRole,
        AbsoluteYesVotesRole,
        PercentageRole,
        ProposalDescriptionRole,
        ProposalHashRole,
    };

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    void refreshProposals();
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;

private:

    CWallet* wallet;
    WalletModel* walletModel;
    QList<ProposalCommunityRecord*> proposalRecords;
    QStringList columns;
};

#endif // BITCOIN_QT_PROPOSALCOMMUNITYTABLEMODEL_H
