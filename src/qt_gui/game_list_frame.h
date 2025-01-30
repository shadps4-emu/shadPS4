// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm> // std::transform
#include <cctype>    // std::tolower

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QScrollBar>

#include "background_music_player.h"
#include "compatibility_info.h"
#include "game_info.h"
#include "game_list_utils.h"
#include "gui_context_menus.h"

class GameListFrame : public QTableWidget {
    Q_OBJECT
public:
    explicit GameListFrame(std::shared_ptr<GameInfoClass> game_info_get,
                           std::shared_ptr<CompatibilityInfoClass> compat_info_get,
                           QWidget* parent = nullptr);
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
    void SetCompatibilityItem(int row, int column, CompatibilityEntry entry);
    QString GetPlayTime(const std::string& serial);
    QList<QAction*> m_columnActs;
    GameInfoClass* game_inf_get = nullptr;
    bool ListSortedAsc = true;

public:
    void PopulateGameList(bool isInitialPopulation = true);
    void ResizeIcons(int iconSize);

    QImage backgroundImage;
    GameListUtils m_game_list_utils;
    GuiContextMenus m_gui_context_menus;
    std::shared_ptr<GameInfoClass> m_game_info;
    std::shared_ptr<CompatibilityInfoClass> m_compat_info;

    int icon_size;

    static float parseAsFloat(const std::string& str, const int& offset) {
        return std::stof(str.substr(0, str.size() - offset));
    }

    static float parseSizeMB(const std::string& size) {
        float num = parseAsFloat(size, 3);
        return (size[size.size() - 2] == 'G') ? num * 1024 : num;
    }

    static bool CompareStringsAscending(GameInfo a, GameInfo b, int columnIndex) {
        switch (columnIndex) {
        case 1: {
            std::string name_a = a.name, name_b = b.name;
            std::transform(name_a.begin(), name_a.end(), name_a.begin(), ::tolower);
            std::transform(name_b.begin(), name_b.end(), name_b.begin(), ::tolower);
            return name_a < name_b;
        }
        case 2:
            return a.compatibility.status < b.compatibility.status;
        case 3:
            return a.serial.substr(4) < b.serial.substr(4);
        case 4:
            return a.region < b.region;
        case 5:
            return parseAsFloat(a.fw, 0) < parseAsFloat(b.fw, 0);
        case 6:
            return parseSizeMB(b.size) < parseSizeMB(a.size);
        case 7:
            return a.version < b.version;
        case 8:
            return a.play_time < b.play_time;
        case 9:
            return a.path < b.path;
        default:
            return false;
        }
    }

    static bool CompareStringsDescending(GameInfo a, GameInfo b, int columnIndex) {
        switch (columnIndex) {
        case 1: {
            std::string name_a = a.name, name_b = b.name;
            std::transform(name_a.begin(), name_a.end(), name_a.begin(), ::tolower);
            std::transform(name_b.begin(), name_b.end(), name_b.begin(), ::tolower);
            return name_a > name_b;
        }
        case 2:
            return a.compatibility.status > b.compatibility.status;
        case 3:
            return a.serial.substr(4) > b.serial.substr(4);
        case 4:
            return a.region > b.region;
        case 5:
            return parseAsFloat(a.fw, 0) > parseAsFloat(b.fw, 0);
        case 6:
            return parseSizeMB(b.size) > parseSizeMB(a.size);
        case 7:
            return a.version > b.version;
        case 8:
            return a.play_time > b.play_time;
        case 9:
            return a.path > b.path;
        default:
            return false;
        }
    }
};
