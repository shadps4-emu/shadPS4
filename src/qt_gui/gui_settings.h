// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QColor>

#include "settings.h"

namespace gui {
enum custom_roles {
    game_role = Qt::UserRole + 1337,
};

enum game_list_columns {
    column_icon,
    column_name,
    column_serial,
    column_firmware,
    column_size,
    column_version,
    column_category,
    column_path,
    column_count
};

inline QString get_game_list_column_name(game_list_columns col) {
    switch (col) {
    case column_icon:
        return "column_icon";
    case column_name:
        return "column_name";
    case column_serial:
        return "column_serial";
    case column_firmware:
        return "column_firmware";
    case column_size:
        return "column_size";
    case column_version:
        return "column_version";
    case column_category:
        return "column_category";
    case column_path:
        return "column_path";
    case column_count:
        return "";
    }

    throw std::runtime_error("get_game_list_column_name: Invalid column");
}

const QString main_window = "main_window";
const QString game_list = "GameList";
const QString settings = "Settings";
const QString themes = "Themes";

const GuiSave main_window_gamelist_visible = GuiSave(main_window, "gamelistVisible", true);
const GuiSave main_window_geometry = GuiSave(main_window, "geometry", QByteArray());
const GuiSave main_window_windowState = GuiSave(main_window, "windowState", QByteArray());
const GuiSave main_window_mwState = GuiSave(main_window, "mwState", QByteArray());

const GuiSave game_list_state = GuiSave(game_list, "state", QByteArray());
const GuiSave game_list_listMode = GuiSave(game_list, "listMode", true);
const GuiSave settings_install_dir = GuiSave(settings, "installDirectory", "");
const GuiSave mw_themes = GuiSave(themes, "Themes", 0);
const GuiSave m_icon_size = GuiSave(game_list, "iconSize", 36);
const GuiSave m_icon_size_grid = GuiSave(game_list, "iconSizeGrid", 69);
const GuiSave m_slide_pos = GuiSave(game_list, "sliderPos", 0);
const GuiSave m_slide_pos_grid = GuiSave(game_list, "sliderPosGrid", 0);
const GuiSave m_table_mode = GuiSave(main_window, "tableMode", 0);
const GuiSave m_window_size = GuiSave(main_window, "windowSize", QSize(1280, 720));
const GuiSave m_pkg_viewer = GuiSave("pkg_viewer", "pkgDir", QStringList());
const GuiSave m_pkg_viewer_pkg_list = GuiSave("pkg_viewer", "pkgList", QStringList());

} // namespace gui

class GuiSettings : public Settings {
    Q_OBJECT

public:
    explicit GuiSettings(QObject* parent = nullptr);

    bool GetGamelistColVisibility(int col) const;

public Q_SLOTS:
    void SetGamelistColVisibility(int col, bool val) const;
    static GuiSave GetGuiSaveForColumn(int col);
};
