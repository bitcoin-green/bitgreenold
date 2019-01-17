// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALTABLEMODEL_H
#define BITCOIN_QT_PROPOSALTABLEMODEL_H

#include "bitcoinunits.h"

#include <QAbstractTableModel>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

class PlatformStyle;
class ProposalRecord;
class ProposalTablePriv;
class WalletModel;

class CWallet;

class ProposalTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ProposalTableModel(CWallet* wallet, WalletModel* parent = 0);
    ~ProposalTableModel();

    enum ColumnIndex {
        Proposal = 0,
        Amount = 1,
        StartBlock = 2,
        EndBlock = 3,
        YesVotes = 4,
        NoVotes = 5,
        //AbsoluteYesVotes = 6,
        Percentage = 6
    };

    enum RoleIndex {
        ProposalRole = Qt::UserRole,
        AmountRole,
        StartBlockRole,
        EndBlockRole,
        YesVotesRole,
        NoVotesRole,
        AbsoluteYesVotesRole,
        PercentageRole,
        ProposalUrlRole,
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
    QList<ProposalRecord*> proposalRecords;
    QStringList columns;
};

#endif // BITCOIN_QT_PROPOSALTABLEMODEL_H
