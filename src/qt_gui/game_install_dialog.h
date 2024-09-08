// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>
#include <QLineEdit>
#include "common/config.h"
#include "common/path_util.h"

class GameInstallDialog : public QDialog {
    Q_OBJECT // This macro is necessary for Qt's meta-object system

        public : explicit GameInstallDialog(QWidget* parent = nullptr);
    ~GameInstallDialog() override;

private slots:
    void Browse();
    void Save();

private:
    QWidget* SetupGamesDirectory();
    QWidget* SetupDialogActions();

    QLineEdit* m_gamesDirectory; // Pointer to the QLineEdit widget
};