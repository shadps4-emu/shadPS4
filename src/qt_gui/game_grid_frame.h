// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QPainter>
#include <QScrollBar>

#include "background_music_player.h"
#include "common/config.h"
#include "game_info.h"
#include "game_list_utils.h"
#include "gui_context_menus.h"
#include "gui_settings.h"
#include "qt_gui/compatibility_info.h"

class GameGridFrame : public QTableWidget {
    Q_OBJECT

Q_SIGNALS:
    void GameGridFrameClosed();

public Q_SLOTS:
    void SetGridBackgroundImage(int row, int column);
    void RefreshGridBackgroundImage();
    void resizeEvent(QResizeEvent* event);
    void PlayBackgroundMusic(QString path);
    void onCurrentCellChanged(int currentRow, int currentColumn, int previousRow,
                              int previousColumn);

private:
    QImage backgroundImage;
    GameListUtils m_game_list_utils;
    GuiContextMenus m_gui_context_menus;
    std::shared_ptr<GameInfoClass> m_game_info;
    std::shared_ptr<CompatibilityInfoClass> m_compat_info;
    std::shared_ptr<QVector<GameInfo>> m_games_shared;
    bool validCellSelected = false;
    int m_last_opacity = -1; // Track last opacity to avoid unnecessary recomputation
    std::filesystem::path m_current_game_path; // Track current game path to detect changes
    std::shared_ptr<gui_settings> m_gui_settings;
    void SetFavoriteIcon(QWidget* parentWidget, QVector<GameInfo> m_games_, int gameCounter);
    void SetGameConfigIcon(QWidget* parentWidget, QVector<GameInfo> m_games_, int gameCounter);
    bool CompareWithFavorite(GameInfo a, GameInfo b);

public:
    explicit GameGridFrame(std::shared_ptr<gui_settings> gui_settings,
                           std::shared_ptr<GameInfoClass> game_info_get,
                           std::shared_ptr<CompatibilityInfoClass> compat_info_get,
                           QWidget* parent = nullptr);
    void PopulateGameGrid(QVector<GameInfo> m_games, bool fromSearch);
    bool IsValidCellSelected();
    void SortByFavorite(QVector<GameInfo>* game_list);

    bool cellClicked = false;
    int icon_size;
    int windowWidth;
    int crtRow;
    int crtColumn;
    int columnCnt;
};
