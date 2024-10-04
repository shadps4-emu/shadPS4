// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QScrollBar>

#include "background_music_player.h"
#include "game_info.h"
#include "game_list_utils.h"
#include "gui_context_menus.h"

class GameListFrame : public QTableWidget {
    Q_OBJECT
public:
    explicit GameListFrame(std::shared_ptr<GameInfoClass> game_info_get, QWidget* parent = nullptr);
Q_SIGNALS:
    void GameListFrameClosed();

public Q_SLOTS:
    void SetListBackgroundImage(QTableWidgetItem* item);
    void RefreshListBackgroundImage();
    void SortNameAscending(int columnIndex);
    void SortNameDescending(int columnIndex);
    void PlayBackgroundMusic(QTableWidgetItem* item);
    void onCurrentCellChanged(int currentRow, int currentColumn, int previousRow,
                              int previousColumn);

private:
    void SetTableItem(int row, int column, QString itemStr);
    void SetRegionFlag(int row, int column, QString itemStr);
    QList<QAction*> m_columnActs;
    GameInfoClass* game_inf_get = nullptr;
    bool ListSortedAsc = true;

public:
    void PopulateGameList();
    void ResizeIcons(int iconSize);

    QImage backgroundImage;
    GameListUtils m_game_list_utils;
    GuiContextMenus m_gui_context_menus;
    std::shared_ptr<GameInfoClass> m_game_info;

    int icon_size;

    static bool CompareStringsAscending(GameInfo a, GameInfo b, int columnIndex) {
        switch (columnIndex) {
        case 1:
            return a.name < b.name;
        case 2:
            return a.serial < b.serial;
        case 3:
            return a.region < b.region;
        case 4:
            return a.fw < b.fw;
        case 5:
            return a.size < b.size;
        case 6:
            return a.version < b.version;
        case 7:
            return a.path < b.path;
        default:
            return false;
        }
    }

    static bool CompareStringsDescending(GameInfo a, GameInfo b, int columnIndex) {
        switch (columnIndex) {
        case 1:
            return a.name > b.name;
        case 2:
            return a.serial > b.serial;
        case 3:
            return a.region > b.region;
        case 4:
            return a.fw > b.fw;
        case 5:
            return a.size > b.size;
        case 6:
            return a.version > b.version;
        case 7:
            return a.path > b.path;
        default:
            return false;
        }
    }
};