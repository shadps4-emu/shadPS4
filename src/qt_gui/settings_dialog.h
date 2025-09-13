// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <span>
#include <QDialog>
#include <QGroupBox>
#include <QPushButton>

#include "common/config.h"
#include "common/path_util.h"
#include "gui_settings.h"
#include "qt_gui/compatibility_info.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(std::shared_ptr<gui_settings> gui_settings,
                            std::shared_ptr<CompatibilityInfoClass> m_compat_info,
                            QWidget* parent = nullptr, bool is_game_specific = false,
                            std::string gsc_serial = "");
    ~SettingsDialog();

    bool eventFilter(QObject* obj, QEvent* event) override;
    void updateNoteTextEdit(const QString& groupName);

    int exec() override;

signals:
    void LanguageChanged(const QString& locale);
    void CompatibilityChanged();
    void BackgroundOpacityChanged(int opacity);

private:
    void LoadValuesFromConfig();
    void UpdateSettings(bool game_specific = false);
    void SyncRealTimeWidgetstoConfig();
    void InitializeEmulatorLanguages();
    void OnLanguageChanged(int index);
    void OnCursorStateChanged(s16 index);
    void closeEvent(QCloseEvent* event) override;
    void setDefaultValues();
    void VolumeSliderChange(int value);

    std::unique_ptr<Ui::SettingsDialog> ui;

    std::map<std::string, int> languages;

    QString defaultTextEdit;

    int initialHeight;
    bool is_saving = false;
    bool game_specific;
    std::string gs_serial;

    std::shared_ptr<gui_settings> m_gui_settings;
};
