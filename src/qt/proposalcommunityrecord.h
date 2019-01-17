// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALCOMMUNITYRECORD_H
#define BITCOIN_QT_PROPOSALCOMMUNITYRECORD_H

#include "amount.h"
#include "uint256.h"

#include <QList>
#include <QString>

class CWallet;

class ProposalCommunityRecord
{
public:
    ProposalCommunityRecord():
            hash(""), end_block(0), description(""), name(""), yesVotes(0), noVotes(0), absoluteYesVotes(0), percentage(0)
    {
    }

    ProposalCommunityRecord(QString hash, qint64 end_block,
                QString description, QString name,
                const CAmount& yesVotes, const CAmount& noVotes, const CAmount& absoluteYesVotes,
                const CAmount& percentage):
            hash(hash), end_block(end_block), description(description), name(name), yesVotes(yesVotes), noVotes(noVotes),
            absoluteYesVotes(absoluteYesVotes), percentage(percentage)
    {
    }

    QString hash;
    qint64 end_block;
    QString description;
    QString name;
    CAmount yesVotes;
    CAmount noVotes;
    CAmount absoluteYesVotes;
    CAmount percentage;
};

#endif // BITCOIN_QT_PROPOSALCOMMUNITYRECORD_H
