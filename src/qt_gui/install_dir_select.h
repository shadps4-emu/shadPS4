// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>
#include <QListWidget>

#include "common/config.h"
#include "common/path_util.h"

class QLineEdit;

class InstallDirSelect final : public QDialog {
public:
    InstallDirSelect();
    ~InstallDirSelect();

    std::filesystem::path getSelectedDirectory() {
        return selected_dir;
    }

private slots:
    void BrowseGamesDirectory();

private:
    QWidget* SetupInstallDirList();
    QWidget* SetupDialogActions();
    void setSelectedDirectory(QListWidgetItem* item);
    std::filesystem::path selected_dir;
};
