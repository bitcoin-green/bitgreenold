// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALDESCRIPTIONDIALOG_H
#define BITCOIN_QT_PROPOSALDESCRIPTIONDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class ProposalDescriptionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProposalDescriptionDialog(const QModelIndex& idx, QWidget* parent = 0);

private Q_SLOTS:
    void closeButton();
};

#endif // BITCOIN_QT_PROPOSALDESCRIPTIONDIALOG_H
