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
        if (columnIndex == 1) {
            return a.name < b.name;
        } else if (columnIndex == 2) {
            return a.serial < b.serial;
        } else if (columnIndex == 3) {
            return a.fw < b.fw;
        }
        return false;
    }

    static bool CompareStringsDescending(GameInfo a, GameInfo b, int columnIndex) {
        if (columnIndex == 1) {
            return a.name > b.name;
        } else if (columnIndex == 2) {
            return a.serial > b.serial;
        } else if (columnIndex == 3) {
            return a.fw > b.fw;
        }
        return false;
    }
};