// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>
#include <QTableWidget>

class user_manager_dialog : public QDialog {
    Q_OBJECT

public:
    explicit user_manager_dialog(QWidget* parent = nullptr);

private:
    void Init();

    QTableWidget* m_table = nullptr;
    std::string m_active_user;
};