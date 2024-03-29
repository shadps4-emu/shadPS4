#pragma once

#include <QFutureWatcher>
#include <QGraphicsBlurEffect>
#include <QHeaderView>
#include <QLabel>
#include <QMainWindow>
#include <QPixmap>
#include <QScrollBar>
#include <QStyleOptionViewItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>
#include "game_info.h"
#include "game_list_utils.h"
#include "gui_context_menus.h"

class GameListFrame : public QTableWidget {
    Q_OBJECT
public:
    explicit GameListFrame(std::shared_ptr<GameInfoClass> game_info_get,
                           std::shared_ptr<GuiSettings> m_gui_settings, QWidget* parent = nullptr);
Q_SIGNALS:
    void GameListFrameClosed();

public Q_SLOTS:
    void SetListBackgroundImage(QTableWidgetItem* item);
    void RefreshListBackgroundImage();
    void SortNameAscending(int columnIndex);
    void SortNameDescending(int columnIndex);

private:
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

    void SetTableItem(QTableWidget* game_list, int row, int column, QString itemStr) {
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