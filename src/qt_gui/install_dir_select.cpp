// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCheckBox>
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
    auto install_dirs = Config::getGameInstallDirs();
    selected_dir = install_dirs.empty() ? "" : install_dirs.front();

    if (!install_dirs.empty() && install_dirs.size() == 1) {
        accept();
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

    auto checkbox = new QCheckBox(tr("Install All Queued to Selected Folder"));
    connect(checkbox, &QCheckBox::toggled, this, &InstallDirSelect::setUseForAllQueued);
    vlayout->addWidget(checkbox);

    auto checkbox2 = new QCheckBox(tr("Delete PKG File on Install"));
    connect(checkbox2, &QCheckBox::toggled, this, &InstallDirSelect::setDeleteFileOnInstall);
    vlayout->addWidget(checkbox2);

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

void InstallDirSelect::setUseForAllQueued(bool enabled) {
    use_for_all_queued = enabled;
}

void InstallDirSelect::setDeleteFileOnInstall(bool enabled) {
    delete_file_on_install = enabled;
}

QWidget* InstallDirSelect::SetupDialogActions() {
    auto actions = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(actions, &QDialogButtonBox::accepted, this, &InstallDirSelect::accept);
    connect(actions, &QDialogButtonBox::rejected, this, &InstallDirSelect::reject);

    return actions;
}
