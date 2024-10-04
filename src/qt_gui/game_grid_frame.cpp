// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/path_util.h"
#include "game_grid_frame.h"

GameGridFrame::GameGridFrame(std::shared_ptr<GameInfoClass> game_info_get, QWidget* parent)
    : QTableWidget(parent), m_game_info(game_info_get) {
    icon_size = Config::getIconSizeGrid();
    windowWidth = parent->width();
    this->setShowGrid(false);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setSelectionBehavior(QAbstractItemView::SelectItems);
    this->setSelectionMode(QAbstractItemView::SingleSelection);
    this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->verticalScrollBar()->installEventFilter(this);
    this->verticalScrollBar()->setSingleStep(20);
    this->horizontalScrollBar()->setSingleStep(20);
    this->horizontalHeader()->setVisible(false);
    this->verticalHeader()->setVisible(false);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    PopulateGameGrid(m_game_info->m_games, false);

    connect(this, &QTableWidget::currentCellChanged, this, &GameGridFrame::onCurrentCellChanged);

    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this,
            &GameGridFrame::RefreshGridBackgroundImage);
    connect(this->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            &GameGridFrame::RefreshGridBackgroundImage);
    connect(this, &QTableWidget::customContextMenuRequested, this, [=, this](const QPoint& pos) {
        m_gui_context_menus.RequestGameMenu(pos, m_game_info->m_games, this, false);
    });
}

void GameGridFrame::onCurrentCellChanged(int currentRow, int currentColumn, int previousRow,
                                         int previousColumn) {
    cellClicked = true;
    crtRow = currentRow;
    crtColumn = currentColumn;
    columnCnt = this->columnCount();

    auto itemID = (crtRow * columnCnt) + currentColumn;
    if (itemID > m_game_info->m_games.count() - 1) {
        validCellSelected = false;
        BackgroundMusicPlayer::getInstance().stopMusic();
        return;
    }
    validCellSelected = true;
    SetGridBackgroundImage(crtRow, crtColumn);
    auto snd0Path = QString::fromStdString(m_game_info->m_games[itemID].snd0_path.string());
    PlayBackgroundMusic(snd0Path);
}

void GameGridFrame::PlayBackgroundMusic(QString path) {
    if (path.isEmpty() || !Config::getPlayBGM()) {
        BackgroundMusicPlayer::getInstance().stopMusic();
        return;
    }
    BackgroundMusicPlayer::getInstance().playMusic(path);
}

void GameGridFrame::PopulateGameGrid(QVector<GameInfo> m_games_search, bool fromSearch) {
    QVector<GameInfo> m_games_;
    this->clearContents();
    if (fromSearch)
        m_games_ = m_games_search;
    else
        m_games_ = m_game_info->m_games;
    m_games_shared = std::make_shared<QVector<GameInfo>>(m_games_);
    icon_size = Config::getIconSizeGrid(); // update icon size for resize event.

    int gamesPerRow = windowWidth / (icon_size + 20); // 2 x cell widget border size.
    int row = 0;
    int gameCounter = 0;
    int rowCount = m_games_.size() / gamesPerRow;
    if (m_games_.size() % gamesPerRow != 0) {
        rowCount += 1; // Add an extra row for the remainder
    }

    int column = 0;
    this->setColumnCount(gamesPerRow);
    this->setRowCount(rowCount);
    for (int i = 0; i < m_games_.size(); i++) {
        QWidget* widget = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout();
        QLabel* image_label = new QLabel();
        QImage icon = m_games_[gameCounter].icon.scaled(
            QSize(icon_size, icon_size), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        image_label->setFixedSize(icon.width(), icon.height());
        image_label->setPixmap(QPixmap::fromImage(icon));
        QLabel* name_label = new QLabel(QString::fromStdString(m_games_[gameCounter].serial));
        name_label->setAlignment(Qt::AlignHCenter);
        layout->addWidget(image_label);
        layout->addWidget(name_label);

        name_label->setStyleSheet("color: white; font-size: 12px; font-weight: bold;");
        QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
        shadowEffect->setBlurRadius(5);               // Set the blur radius of the shadow
        shadowEffect->setColor(QColor(0, 0, 0, 160)); // Set the color and opacity of the shadow
        shadowEffect->setOffset(2, 2);                // Set the offset of the shadow

        name_label->setGraphicsEffect(shadowEffect);
        widget->setLayout(layout);
        QString tooltipText = QString::fromStdString(m_games_[gameCounter].name + " (" +
                                                     m_games_[gameCounter].version + ", " +
                                                     m_games_[gameCounter].region + ")");
        widget->setToolTip(tooltipText);
        QString tooltipStyle = QString("QToolTip {"
                                       "background-color: #ffffff;"
                                       "color: #000000;"
                                       "border: 1px solid #000000;"
                                       "padding: 2px;"
                                       "font-size: 12px; }");
        widget->setStyleSheet(tooltipStyle);
        this->setCellWidget(row, column, widget);

        column++;
        if (column == gamesPerRow) {
            column = 0;
            row++;
        }

        gameCounter++;
        if (gameCounter >= m_games_.size()) {
            break;
        }
    }
    m_games_.clear();
    this->resizeRowsToContents();
    this->resizeColumnsToContents();
}

void GameGridFrame::SetGridBackgroundImage(int row, int column) {

    int itemID = (row * this->columnCount()) + column;
    QWidget* item = this->cellWidget(row, column);
    if (item) {
        QString pic1Path;
        Common::FS::PathToQString(pic1Path, (*m_games_shared)[itemID].pic_path);
        const auto blurredPic1Path = Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) /
                                     (*m_games_shared)[itemID].serial / "pic1.png";
        QString blurredPic1PathQt;
        Common::FS::PathToQString(blurredPic1PathQt, blurredPic1Path);

        backgroundImage = QImage(blurredPic1PathQt);
        if (backgroundImage.isNull()) {
            QImage image(pic1Path);
            backgroundImage = m_game_list_utils.BlurImage(image, image.rect(), 16);

            std::filesystem::path img_path =
                Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) /
                (*m_games_shared)[itemID].serial;
            std::filesystem::create_directories(img_path);
            if (!backgroundImage.save(blurredPic1PathQt, "PNG")) {
                // qDebug() << "Error: Unable to save image.";
            }
        }
        RefreshGridBackgroundImage();
    }
}

void GameGridFrame::RefreshGridBackgroundImage() {
    if (!backgroundImage.isNull()) {
        QPalette palette;
        palette.setBrush(QPalette::Base,
                         QBrush(backgroundImage.scaled(size(), Qt::IgnoreAspectRatio)));
        QColor transparentColor = QColor(135, 206, 235, 40);
        palette.setColor(QPalette::Highlight, transparentColor);
        this->setPalette(palette);
    }
}

bool GameGridFrame::IsValidCellSelected() {
    return validCellSelected;
}
