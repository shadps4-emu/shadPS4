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

const QSize game_list_icon_size_min = QSize(28, 28);
const QSize game_list_icon_size_small = QSize(56, 56);
const QSize game_list_icon_size_medium = QSize(128, 128);
const QSize game_list_icon_size_max =
    QSize(256, 256); // let's do 256, 512 is too big (that's what she said)

const int game_list_max_slider_pos = 100;

inline int get_Index(const QSize& current) {
    const int size_delta = game_list_icon_size_max.width() - game_list_icon_size_min.width();
    const int current_delta = current.width() - game_list_icon_size_min.width();
    return game_list_max_slider_pos * current_delta / size_delta;
}

const QString main_window = "main_window";
const QString game_list = "GameList";
const QString settings = "Settings";
const QString themes = "Themes";

const QColor game_list_icon_color = QColor(240, 240, 240, 255);

const GuiSave main_window_gamelist_visible = GuiSave(main_window, "gamelistVisible", true);
const GuiSave main_window_geometry = GuiSave(main_window, "geometry", QByteArray());
const GuiSave main_window_windowState = GuiSave(main_window, "windowState", QByteArray());
const GuiSave main_window_mwState = GuiSave(main_window, "mwState", QByteArray());

const GuiSave game_list_sortAsc = GuiSave(game_list, "sortAsc", true);
const GuiSave game_list_sortCol = GuiSave(game_list, "sortCol", 1);
const GuiSave game_list_state = GuiSave(game_list, "state", QByteArray());
const GuiSave game_list_iconSize =
    GuiSave(game_list, "iconSize", get_Index(game_list_icon_size_small));
const GuiSave game_list_iconSizeGrid =
    GuiSave(game_list, "iconSizeGrid", get_Index(game_list_icon_size_small));
const GuiSave game_list_iconColor = GuiSave(game_list, "iconColor", game_list_icon_color);
const GuiSave game_list_listMode = GuiSave(game_list, "listMode", true);
const GuiSave game_list_textFactor = GuiSave(game_list, "textFactor", qreal{2.0});
const GuiSave game_list_marginFactor = GuiSave(game_list, "marginFactor", qreal{0.09});
const GuiSave settings_install_dir = GuiSave(settings, "installDirectory", "");
const GuiSave mw_themes = GuiSave(themes, "Themes", 0);

} // namespace gui

class GuiSettings : public Settings {
    Q_OBJECT

public:
    explicit GuiSettings(QObject* parent = nullptr);

    bool GetGamelistColVisibility(int col) const;

public Q_SLOTS:
    void SetGamelistColVisibility(int col, bool val) const;
    static GuiSave GetGuiSaveForColumn(int col);
    static QSize SizeFromSlider(int pos);
};
