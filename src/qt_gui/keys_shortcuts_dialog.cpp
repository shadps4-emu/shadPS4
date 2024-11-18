// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QImage>
#include <QLabel>
#include <QPixmap>
#include "keys_shortcuts_dialog.h"
#include "ui_keys_shortcuts_dialog.h"

KeysShortcutsDialog::KeysShortcutsDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::KeysShortcutsDialog) {
    ui->setupUi(this);
    ui->text->setText(tr("KeysShortcutsDialog_MSG").replace("\\n", "\n"));
}

KeysShortcutsDialog::~KeysShortcutsDialog() {
    delete ui;
}