// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QFutureWatcher>
#include <QGraphicsBlurEffect>
#include <QHeaderView>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent>
#include "custom_dock_widget.h"
#include "game_list_grid.h"
#include "game_list_item.h"
#include "game_list_table.h"
#include "game_list_utils.h"
#include "gui_settings.h"

class GameListFrame : public CustomDockWidget {
    Q_OBJECT
public:
    explicit GameListFrame(std::shared_ptr<GuiSettings> gui_settings, QWidget* parent = nullptr);
    ~GameListFrame();
    /** Fix columns with width smaller than the minimal section size */
    void FixNarrowColumns() const;

    /** Loads from settings. Public so that main frame can easily reset these settings if needed. */
    void LoadSettings();

    /** Saves settings. Public so that main frame can save this when a caching of column widths is
     * needed for settings backup */
    void SaveSettings();

    /** Resizes the columns to their contents and adds a small spacing */
    void ResizeColumnsToContents(int spacing = 20) const;

    /** Refresh the gamelist with/without loading game data from files. Public so that main frame
     * can refresh after vfs or install */
    void Refresh(const bool from_drive = false, const bool scroll_after = true);

    /** Repaint Gamelist Icons with new background color */
    void RepaintIcons(const bool& from_settings = false);

    /** Resize Gamelist Icons to size given by slider position */
    void ResizeIcons(const int& slider_pos);

public Q_SLOTS:
    void SetSearchText(const QString& text);
    void SetListMode(const bool& is_list);
private Q_SLOTS:
    void OnHeaderColumnClicked(int col);
    void OnRepaintFinished();
    void OnRefreshFinished();
    void RequestGameMenu(const QPoint& pos);
    void SetListBackgroundImage(QTableWidgetItem* item);
    void RefreshListBackgroundImage();

Q_SIGNALS:
    void GameListFrameClosed();
    void RequestIconSizeChange(const int& val);
    void ResizedWindow(QTableWidgetItem* item);

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QPixmap PaintedPixmap(const QPixmap& icon) const;
    void SortGameList() const;
    std::string CurrentSelectionPath();
    void PopulateGameList();
    void PopulateGameGrid(int maxCols, const QSize& image_size, const QColor& image_color);
    bool SearchMatchesApp(const QString& name, const QString& serial) const;
    bool IsEntryVisible(const game_info& game);
    static game_info GetGameInfoFromItem(const QTableWidgetItem* item);

    // Which widget we are displaying depends on if we are in grid or list mode.
    QMainWindow* m_game_dock = nullptr;
    QStackedWidget* m_central_widget = nullptr;

    // Game Grid
    GameListGrid* m_game_grid = nullptr;

    // Game List
    GameListTable* m_game_list = nullptr;
    QList<QAction*> m_columnActs;
    Qt::SortOrder m_col_sort_order;
    int m_sort_column;
    QMap<QString, QString> m_titles;

    // Game List Utils
    GameListUtils m_game_list_utils;

    // List Mode
    bool m_is_list_layout = true;
    bool m_old_layout_is_list = true;

    // data
    std::shared_ptr<GuiSettings> m_gui_settings;
    QList<game_info> m_game_data;
    std::vector<std::string> m_path_list;
    std::vector<game_info> m_games;
    QFutureWatcher<GameListItem*> m_repaint_watcher;
    QFutureWatcher<void> m_refresh_watcher;

    // Search
    QString m_search_text;

    // Icon Size
    int m_icon_size_index = 0;

    // Icons
    QSize m_icon_size;
    QColor m_icon_color;
    qreal m_margin_factor;
    qreal m_text_factor;

    // Background Image
    QImage backgroundImage;

    void SetTableItem(GameListTable* game_list, int row, int column, QString itemStr) {
        QWidget* widget = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout();
        QLabel* label = new QLabel(itemStr);
        QTableWidgetItem* item = new QTableWidgetItem();

        label->setStyleSheet("color: white; font-size: 15px; font-weight: bold;");

        // Create shadow effect
        QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
        shadowEffect->setBlurRadius(5);               // Set the blur radius of the shadow
        shadowEffect->setColor(QColor(0, 0, 0, 160)); // Set the color and opacity of the shadow
        shadowEffect->setOffset(2, 2);                // Set the offset of the shadow

        label->setGraphicsEffect(shadowEffect); // Apply shadow effect to the QLabel

        layout->addWidget(label);
        if (column != 7 && column != 1)
            layout->setAlignment(Qt::AlignCenter);
        widget->setLayout(layout);
        game_list->setItem(row, column, item);
        game_list->setCellWidget(row, column, widget);
    }
};
