// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMouseEvent>
#include <QTableWidget>

#include "game_info.h"
#include "game_list_item.h"

struct GuiGameInfo {
    GameInfo info{};
    QPixmap icon;
    QPixmap pxmap;
    GameListItem* item = nullptr;
};

typedef std::shared_ptr<GuiGameInfo> game_info;
Q_DECLARE_METATYPE(game_info)

class GameListTable : public QTableWidget {
public:
    void clear_list();

protected:
    void mousePressEvent(QMouseEvent* event) override;
};