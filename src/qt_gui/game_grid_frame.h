#pragma once

#include <QFutureWatcher>
#include <QGraphicsBlurEffect>
#include <QHeaderView>
#include <QLabel>
#include <QPixmap>
#include <QScrollBar>
#include <QStyleOptionViewItem>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>
#include "game_info.h"
#include "game_list_utils.h"
#include "gui_context_menus.h"

class GameGridFrame : public QTableWidget {
    Q_OBJECT

Q_SIGNALS:
    void GameGridFrameClosed();

public Q_SLOTS:
    void SetGridBackgroundImage(int row, int column);
    void RefreshGridBackgroundImage();

private:
    QImage backgroundImage;
    GameListUtils m_game_list_utils;
    GuiContextMenus m_gui_context_menus;
    std::shared_ptr<GameInfoClass> m_game_info;
    std::shared_ptr<GuiSettings> m_gui_settings_;
    std::shared_ptr<QVector<GameInfo>> m_games_shared;

public:
    explicit GameGridFrame(std::shared_ptr<GameInfoClass> game_info_get,
                           std::shared_ptr<GuiSettings> m_gui_settings, QWidget* parent = nullptr);
    void PopulateGameGrid(QVector<GameInfo> m_games, bool fromSearch);

    int icon_size;
    int windowWidth;
};
