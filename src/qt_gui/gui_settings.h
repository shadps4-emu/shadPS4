// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QWindow>
#include "settings.h"

namespace gui {
// categories
const QString main_window = "main_window";
const QString game_list = "game_list";
const QString game_grid = "game_grid";

// main window settings
const gui_value mw_geometry = gui_value(main_window, "geometry", QByteArray());

// game list settings
const gui_value gl_mode = gui_value(game_list, "tableMode", 0);
const gui_value gl_icon_size = gui_value(game_list, "icon_size", 36);
const gui_value gl_slider_pos = gui_value(game_list, "slider_pos", 0);

// game grid settings
const gui_value gg_icon_size = gui_value(game_grid, "icon_size", 69);
const gui_value gg_slider_pos = gui_value(game_grid, "slider_pos", 0);

} // namespace gui

class gui_settings : public settings {
    Q_OBJECT

public:
    explicit gui_settings(QObject* parent = nullptr);
};
