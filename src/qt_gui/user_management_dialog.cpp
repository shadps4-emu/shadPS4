// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDesktopServices>
#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QScreen>
#include "user_management_dialog.h"

user_manager_dialog::user_manager_dialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("User Manager"));
    setMinimumSize(QSize(500, 400));
    setModal(true);

    Init();
}

void user_manager_dialog::Init() {
    // Table
    m_table = new QTableWidget(this);
    m_table->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_table->setColumnCount(2);
    m_table->setCornerButtonEnabled(false);
    m_table->setAlternatingRowColors(true);
    m_table->setHorizontalHeaderLabels(QStringList() << tr("User ID") << tr("User Name"));
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setDefaultSectionSize(150);
    m_table->installEventFilter(this);

    QPushButton* push_remove_user = new QPushButton(tr("&Delete User"), this);
    push_remove_user->setAutoDefault(false);

    QPushButton* push_create_user = new QPushButton(tr("&Create User"), this);
    push_create_user->setAutoDefault(false);

    QPushButton* push_edit_user = new QPushButton(tr("&Edit User"), this);
    push_edit_user->setAutoDefault(false);

    QPushButton* push_close = new QPushButton(tr("&Close"), this);
    push_close->setAutoDefault(false);

    // Button Layout
    QHBoxLayout* hbox_buttons = new QHBoxLayout();
    hbox_buttons->addWidget(push_create_user);
    hbox_buttons->addWidget(push_edit_user);
    hbox_buttons->addWidget(push_remove_user);
    hbox_buttons->addStretch();
    hbox_buttons->addWidget(push_close);

    // Main Layout
    QVBoxLayout* vbox_main = new QVBoxLayout();
    vbox_main->setAlignment(Qt::AlignCenter);
    vbox_main->addWidget(m_table);
    vbox_main->addLayout(hbox_buttons);
    setLayout(vbox_main);

    // get active user
    m_active_user = Config::getActiveUserId();

}