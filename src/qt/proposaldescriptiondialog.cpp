// Copyright (c) 2011-2013 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2017 The PIVX developers
// Copyright (c) 2017-2018 The Bitcoin Green developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "proposaldescriptiondialog.h"
#include "guiutil.h"

#include "proposalcommunitytablemodel.h"

#include <QModelIndex>
#include <QSettings>
#include <QString>

ProposalDescriptionDialog::ProposalDescriptionDialog(const QModelIndex& idx, QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
    this->setStyleSheet(GUIUtil::loadStyleSheet());

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    QLabel *name = new QLabel("<strong>"+idx.data(ProposalCommunityTableModel::ProposalRole).toString()+"</strong>");
    vlayout->addWidget(name);
    QLabel *description = new QLabel(idx.data(ProposalCommunityTableModel::ProposalDescriptionRole).toString());
    vlayout->addWidget(description);
    QHBoxLayout *hlayout = new QHBoxLayout(this);
    hlayout->addStretch();
    QPushButton *closeButton = new QPushButton(tr("Close"), this);
    hlayout->addWidget(closeButton);
    vlayout->addLayout(hlayout);

    connect(closeButton, SIGNAL(clicked()), this, SLOT(closeButton()));

    setLayout(vlayout);
}

void ProposalDescriptionDialog::closeButton() {
    close();
}