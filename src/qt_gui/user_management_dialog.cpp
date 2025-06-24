#include "user_management_dialog.h"
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
#include <common/path_util.h>
#include "common/config.h"
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
    RefreshTable();
}
void user_manager_dialog::RefreshTable() {

    // For indicating logged-in user.
    QFont bold_font;
    bold_font.setBold(true);

    m_user_list.clear();
    const auto& home_dir = Common::FS::GetUserPathString(Common::FS::PathType::HomeDir);
    m_user_list = user_account::GetUserAccounts(home_dir);

    // Clear and then repopulate the table with the list gathered above.
    m_table->setRowCount(static_cast<int>(m_user_list.size()));

    int row = 0;
    for (auto& [id, account] : m_user_list) {
        QTableWidgetItem* user_id_item =
            new QTableWidgetItem(QString::fromStdString(account.GetUserId()));
        user_id_item->setData(Qt::UserRole, id);
        user_id_item->setFlags(user_id_item->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, 0, user_id_item);

        QTableWidgetItem* username_item =
            new QTableWidgetItem(QString::fromStdString(account.GetUsername()));
        username_item->setData(Qt::UserRole, id);
        username_item->setFlags(username_item->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, 1, username_item);

        // make bold the user that is active
        if (m_active_user.starts_with(account.GetUserId())) {
            user_id_item->setFont(bold_font);
            username_item->setFont(bold_font);
        }
        ++row;
    }

    // GUI resizing
    m_table->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    m_table->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);

    const QSize table_size(m_table->verticalHeader()->width() +
                               m_table->horizontalHeader()->length() + m_table->frameWidth() * 2,
                           m_table->horizontalHeader()->height() +
                               m_table->verticalHeader()->length() + m_table->frameWidth() * 2);

    const QSize preferred_size =
        minimumSize().expandedTo(sizeHint() - m_table->sizeHint() + table_size).expandedTo(size());
    const QSize max_size(preferred_size.width(),
                         static_cast<int>(QGuiApplication::primaryScreen()->size().height() * 0.6));

    resize(preferred_size.boundedTo(max_size));
}
