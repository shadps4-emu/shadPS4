// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "gui_settings.h"

gui_settings::gui_settings(QObject* parent) : settings(parent) {
    m_settings = std::make_unique<QSettings>(ComputeSettingsDir() + "qt_ui.ini",
                                             QSettings::Format::IniFormat, parent);
}
