// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>
#include <QListWidget>

#include "common/config.h"
#include "common/path_util.h"

class QLineEdit;

class InstallDirSelect final : public QDialog {
    Q_OBJECT

public:
    InstallDirSelect();
    ~InstallDirSelect();

    std::filesystem::path getSelectedDirectory() {
        return selected_dir;
    }

    bool useForAllQueued() {
        return use_for_all_queued;
    }

    bool deleteFileOnInstall() {
        return delete_file_on_install;
    }

private:
    QWidget* SetupInstallDirList();
    QWidget* SetupDialogActions();
    void setSelectedDirectory(QListWidgetItem* item);
    void setDeleteFileOnInstall(bool enabled);
    void setUseForAllQueued(bool enabled);
    std::filesystem::path selected_dir;
    bool delete_file_on_install = false;
    bool use_for_all_queued = false;
};
