// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <QDialog>
#include <QGroupBox>
#include <QPushButton>

#include "common/config.h"
#include "common/path_util.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(std::span<const QString> physical_devices, QWidget* parent = nullptr);
    ~SettingsDialog();

    bool eventFilter(QObject* obj, QEvent* event) override;
    void updateNoteTextEdit(const QString& groupName);

    int exec() override;

signals:
    void LanguageChanged(const std::string& locale);

private:
    void LoadValuesFromConfig();
    void InitializeEmulatorLanguages();
    void OnLanguageChanged(int index);

    std::unique_ptr<Ui::SettingsDialog> ui;

    std::map<std::string, int> languages;

    QString defaultTextEdit;
};
