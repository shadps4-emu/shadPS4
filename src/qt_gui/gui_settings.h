// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QWindow>
#include "settings.h"

namespace gui {
// categories
const QString main_window = "main_window";

// main window settions
const gui_value mw_geometry = gui_value(main_window, "geometry", QByteArray());

} // namespace gui

class gui_settings : public settings {
    Q_OBJECT

public:
    explicit gui_settings(QObject* parent = nullptr);
};
