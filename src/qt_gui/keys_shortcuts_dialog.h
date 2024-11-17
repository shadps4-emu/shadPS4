// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDesktopServices>
#include <QDialog>
#include <QLabel>
#include <QPixmap>
#include <QUrl>

namespace Ui {
class KeysShortcutsDialog;
}

class KeysShortcutsDialog : public QDialog {
    Q_OBJECT

public:
    explicit KeysShortcutsDialog(QWidget* parent = nullptr);
    ~KeysShortcutsDialog();

private:
    Ui::KeysShortcutsDialog* ui;
};
