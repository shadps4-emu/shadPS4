// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "install_dir_select.h"

InstallDirSelect::InstallDirSelect() : selected_dir() {
    selected_dir = Config::getGameInstallDirs().empty() ? "" : Config::getGameInstallDirs().front();

    if (!Config::getGameInstallDirs().empty() && Config::getGameInstallDirs().size() == 1) {
        reject();
    }

    auto layout = new QVBoxLayout(this);

    layout->addWidget(SetupInstallDirList());
    layout->addStretch();
    layout->addWidget(SetupDialogActions());

    setWindowTitle(tr("shadPS4 - Choose directory"));
    setWindowIcon(QIcon(":images/shadps4.ico"));
}

InstallDirSelect::~InstallDirSelect() {}

QWidget* InstallDirSelect::SetupInstallDirList() {
    auto group = new QGroupBox(tr("Select which directory you want to install to."));
    auto vlayout = new QVBoxLayout();

    auto m_path_list = new QListWidget();
    QList<QString> qt_list;
    for (const auto& str : Config::getGameInstallDirs()) {
        QString installDirPath;
        Common::FS::PathToQString(installDirPath, str);
        qt_list.append(installDirPath);
    }
    m_path_list->insertItems(0, qt_list);
    m_path_list->setSpacing(1);

    connect(m_path_list, &QListWidget::itemClicked, this, &InstallDirSelect::setSelectedDirectory);
    connect(m_path_list, &QListWidget::itemActivated, this,
            &InstallDirSelect::setSelectedDirectory);

    vlayout->addWidget(m_path_list);

    group->setLayout(vlayout);
    return group;
}

void InstallDirSelect::setSelectedDirectory(QListWidgetItem* item) {
    if (item) {
        const auto highlighted_path = Common::FS::PathFromQString(item->text());
        if (!highlighted_path.empty()) {
            selected_dir = highlighted_path;
        }
    }
}

QWidget* InstallDirSelect::SetupDialogActions() {
    auto actions = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(actions, &QDialogButtonBox::accepted, this, &InstallDirSelect::accept);
    connect(actions, &QDialogButtonBox::rejected, this, &InstallDirSelect::reject);

    return actions;
}
