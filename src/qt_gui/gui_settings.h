// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QWindow>
#include "settings.h"

namespace gui {
// categories
const QString general_settings = "general_settings";
const QString main_window = "main_window";
const QString game_list = "game_list";
const QString game_grid = "game_grid";
const QString favorites = "favorites";

// general
const gui_value gen_checkForUpdates = gui_value(general_settings, "checkForUpdates", false);
const gui_value gen_showChangeLog = gui_value(general_settings, "showChangeLog", false);
const gui_value gen_updateChannel = gui_value(general_settings, "updateChannel", "Release");
const gui_value gen_recentFiles =
    gui_value(main_window, "recentFiles", QVariant::fromValue(QList<QString>()));
const gui_value gen_guiLanguage = gui_value(general_settings, "guiLanguage", "en_US");
const gui_value gen_elfDirs =
    gui_value(main_window, "elfDirs", QVariant::fromValue(QList<QString>()));
const gui_value gen_theme = gui_value(general_settings, "theme", 0);

// main window settings
const gui_value mw_geometry = gui_value(main_window, "geometry", QByteArray());
const gui_value mw_showLabelsUnderIcons = gui_value(main_window, "showLabelsUnderIcons", true);

// game list settings
const gui_value gl_mode = gui_value(game_list, "tableMode", 0);
const gui_value gl_icon_size = gui_value(game_list, "icon_size", 36);
const gui_value gl_slider_pos = gui_value(game_list, "slider_pos", 0);
const gui_value gl_showBackgroundImage = gui_value(game_list, "showBackgroundImage", true);
const gui_value gl_backgroundImageOpacity = gui_value(game_list, "backgroundImageOpacity", 50);
const gui_value gl_playBackgroundMusic = gui_value(game_list, "playBackgroundMusic", true);
const gui_value gl_backgroundMusicVolume = gui_value(game_list, "backgroundMusicVolume", 50);
const gui_value gl_VolumeSlider = gui_value(game_list, "volumeSlider", 100);

// game grid settings
const gui_value gg_icon_size = gui_value(game_grid, "icon_size", 69);
const gui_value gg_slider_pos = gui_value(game_grid, "slider_pos", 0);

// favorites list
const gui_value favorites_list =
    gui_value(favorites, "favoritesList", QVariant::fromValue(QList<QString>()));

} // namespace gui

class gui_settings : public settings {
    Q_OBJECT

public:
    explicit gui_settings(QObject* parent = nullptr);
    ~gui_settings() override = default;
};
