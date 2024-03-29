// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "game_list_frame.h"

GameListFrame::GameListFrame(std::shared_ptr<GameInfoClass> game_info_get,
                             std::shared_ptr<GuiSettings> m_gui_settings, QWidget* parent)
    : QTableWidget(parent) {
    m_game_info = game_info_get;
    icon_size = m_gui_settings->GetValue(gui::m_icon_size).toInt();

    this->setShowGrid(false);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->setSelectionMode(QAbstractItemView::SingleSelection);
    this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->verticalScrollBar()->installEventFilter(this);
    this->verticalScrollBar()->setSingleStep(20);
    this->horizontalScrollBar()->setSingleStep(20);
    this->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    this->verticalHeader()->setVisible(false);
    this->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    this->horizontalHeader()->setHighlightSections(false);
    this->horizontalHeader()->setSortIndicatorShown(true);
    this->horizontalHeader()->setStretchLastSection(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setColumnCount(8);
    this->setColumnWidth(1, 250);
    this->setColumnWidth(2, 110);
    this->setColumnWidth(3, 80);
    this->setColumnWidth(4, 90);
    this->setColumnWidth(5, 80);
    this->setColumnWidth(6, 80);
    QStringList headers;
    headers << "Icon"
            << "Name"
            << "Serial"
            << "Firmware"
            << "Size"
            << "Version"
            << "Category"
            << "Path";
    this->setHorizontalHeaderLabels(headers);
    this->horizontalHeader()->setSortIndicatorShown(true);
    this->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    this->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    this->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    PopulateGameList();

    connect(this, &QTableWidget::itemClicked, this, &GameListFrame::SetListBackgroundImage);
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this,
            &GameListFrame::RefreshListBackgroundImage);
    connect(this->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            &GameListFrame::RefreshListBackgroundImage);

    this->horizontalHeader()->setSortIndicatorShown(true);
    this->horizontalHeader()->setSectionsClickable(true);
    QObject::connect(
        this->horizontalHeader(), &QHeaderView::sectionClicked, this, [this](int columnIndex) {
            if (ListSortedAsc) {
                SortNameDescending(columnIndex);
                this->horizontalHeader()->setSortIndicator(columnIndex, Qt::DescendingOrder);
                ListSortedAsc = false;
            } else {
                SortNameAscending(columnIndex);
                this->horizontalHeader()->setSortIndicator(columnIndex, Qt::AscendingOrder);
                ListSortedAsc = true;
            }
            this->clearContents();
            PopulateGameList();
        });

    connect(this, &QTableWidget::customContextMenuRequested, this, [=, this](const QPoint& pos) {
        m_gui_context_menus.RequestGameMenu(pos, m_game_info->m_games, this, true);
    });
}

void GameListFrame::PopulateGameList() {
    this->setRowCount(m_game_info->m_games.size());
    ResizeIcons(icon_size);

    for (int i = 0; i < m_game_info->m_games.size(); i++) {
        SetTableItem(this, i, 1, QString::fromStdString(m_game_info->m_games[i].name));
        SetTableItem(this, i, 2, QString::fromStdString(m_game_info->m_games[i].serial));
        SetTableItem(this, i, 3, QString::fromStdString(m_game_info->m_games[i].fw));
        SetTableItem(this, i, 4, QString::fromStdString(m_game_info->m_games[i].size));
        SetTableItem(this, i, 5, QString::fromStdString(m_game_info->m_games[i].version));
        SetTableItem(this, i, 6, QString::fromStdString(m_game_info->m_games[i].category));
        SetTableItem(this, i, 7, QString::fromStdString(m_game_info->m_games[i].path));
    }
}

void GameListFrame::SetListBackgroundImage(QTableWidgetItem* item) {
    if (!item) {
        // handle case where no item was clicked
        return;
    }

    QString pic1Path = QString::fromStdString(m_game_info->m_games[item->row()].pic_path);
    QString blurredPic1Path =
        qApp->applicationDirPath() +
        QString::fromStdString("/game_data/" + m_game_info->m_games[item->row()].serial +
                               "/pic1.png");

    backgroundImage = QImage(blurredPic1Path);
    if (backgroundImage.isNull()) {
        QImage image(pic1Path);
        backgroundImage = m_game_list_utils.BlurImage(image, image.rect(), 16);

        std::filesystem::path img_path =
            std::filesystem::path("game_data/") / m_game_info->m_games[item->row()].serial;
        std::filesystem::create_directories(img_path);
        if (!backgroundImage.save(blurredPic1Path, "PNG")) {
            // qDebug() << "Error: Unable to save image.";
        }
    }
    RefreshListBackgroundImage();
}

void GameListFrame::RefreshListBackgroundImage() {
    QPixmap blurredPixmap = QPixmap::fromImage(backgroundImage);
    QPalette palette;
    palette.setBrush(QPalette::Base, QBrush(blurredPixmap.scaled(size(), Qt::IgnoreAspectRatio)));
    QColor transparentColor = QColor(135, 206, 235, 40);
    palette.setColor(QPalette::Highlight, transparentColor);
    this->setPalette(palette);
}

void GameListFrame::SortNameAscending(int columnIndex) {
    std::sort(m_game_info->m_games.begin(), m_game_info->m_games.end(),
              [columnIndex](const GameInfo& a, const GameInfo& b) {
                  return CompareStringsAscending(a, b, columnIndex);
              });
}

void GameListFrame::SortNameDescending(int columnIndex) {
    std::sort(m_game_info->m_games.begin(), m_game_info->m_games.end(),
              [columnIndex](const GameInfo& a, const GameInfo& b) {
                  return CompareStringsDescending(a, b, columnIndex);
              });
}

void GameListFrame::ResizeIcons(int iconSize) {
    QList<int> indices;
    for (int i = 0; i < m_game_info->m_games.size(); i++) {
        indices.append(i);
    }
    std::future<void> future = std::async(std::launch::async, [=, this]() {
        for (int index : indices) {
            QPixmap scaledPixmap = m_game_info->m_games[index].icon.scaled(
                QSize(iconSize, iconSize), Qt::KeepAspectRatio, Qt::SmoothTransformation);

            QTableWidgetItem* iconItem = new QTableWidgetItem();
            this->verticalHeader()->resizeSection(index, scaledPixmap.height());
            this->horizontalHeader()->resizeSection(0, scaledPixmap.width());
            iconItem->setData(Qt::DecorationRole, scaledPixmap);
            this->setItem(index, 0, iconItem);
        }
    });
    future.wait();
    this->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
}