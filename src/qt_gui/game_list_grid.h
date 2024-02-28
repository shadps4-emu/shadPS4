// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QCoreApplication>
#include "custom_dock_widget.h"
#include "game_list_table.h"
#include "game_list_utils.h"
#include "gui_settings.h"

class GameListGridDelegate;

class GameListGrid : public GameListTable {
    Q_OBJECT

    QSize m_icon_size;
    QColor m_icon_color;
    qreal m_margin_factor;
    qreal m_text_factor;
    bool m_text_enabled = true;

Q_SIGNALS:
    void ResizedWindowGrid(QTableWidgetItem* item);

protected:
    void resizeEvent(QResizeEvent* event) override;

public:
    explicit GameListGrid(const QSize& icon_size, QColor icon_color, const qreal& margin_factor,
                          const qreal& text_factor, const bool& showText);

    void enableText(const bool& enabled);
    void setIconSize(const QSize& size) const;
    GameListItem* addItem(const game_info& app, const QString& name, const int& row,
                          const int& col);

    [[nodiscard]] qreal getMarginFactor() const;

    game_info GetGameInfoFromItem(const QTableWidgetItem* item) {
        if (!item) {
            return nullptr;
        }

        const QVariant var = item->data(gui::game_role);
        if (!var.canConvert<game_info>()) {
            return nullptr;
        }

        return var.value<game_info>();
    }

private:
    void SetGridBackgroundImage(QTableWidgetItem* item);
    void RefreshBackgroundImage();

    GameListGridDelegate* grid_item_delegate;
    GameListUtils m_game_list_utils;

    // Background Image
    QImage backgroundImage;
};
