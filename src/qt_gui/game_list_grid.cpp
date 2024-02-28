// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QHeaderView>
#include <QScrollBar>
#include "game_list_grid.h"
#include "game_list_grid_delegate.h"
#include "game_list_item.h"

GameListGrid::GameListGrid(const QSize& icon_size, QColor icon_color, const qreal& margin_factor,
                           const qreal& text_factor, const bool& showText)
    : m_icon_size(icon_size), m_icon_color(std::move(icon_color)), m_margin_factor(margin_factor),
      m_text_factor(text_factor), m_text_enabled(showText) {
    setObjectName("game_grid");

    QSize item_size;
    if (m_text_enabled) {
        item_size =
            m_icon_size + QSize(m_icon_size.width() * m_margin_factor * 2,
                                m_icon_size.height() * m_margin_factor * (m_text_factor + 1));
    } else {
        item_size = m_icon_size + m_icon_size * m_margin_factor * 2;
    }

    grid_item_delegate = new GameListGridDelegate(item_size, m_margin_factor, m_text_factor, this);
    setItemDelegate(grid_item_delegate);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    verticalScrollBar()->setSingleStep(20);
    horizontalScrollBar()->setSingleStep(20);
    setContextMenuPolicy(Qt::CustomContextMenu);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setVisible(false);
    setShowGrid(false);
    QPalette palette;
    palette.setColor(QPalette::Base, QColor(230, 230, 230, 80));
    setPalette(palette);

    connect(this, &GameListTable::itemClicked, this, &GameListGrid::SetGridBackgroundImage);
    connect(this, &GameListGrid::ResizedWindowGrid, this, &GameListGrid::SetGridBackgroundImage);
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this,
            &GameListGrid::RefreshBackgroundImage);
    connect(this->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            &GameListGrid::RefreshBackgroundImage);
}

void GameListGrid::enableText(const bool& enabled) {
    m_text_enabled = enabled;
}

void GameListGrid::setIconSize(const QSize& size) const {
    if (m_text_enabled) {
        grid_item_delegate->setItemSize(
            size + QSize(size.width() * m_margin_factor * 2,
                         size.height() * m_margin_factor * (m_text_factor + 1)));
    } else {
        grid_item_delegate->setItemSize(size + size * m_margin_factor * 2);
    }
}

GameListItem* GameListGrid::addItem(const game_info& app, const QString& name, const int& row,
                                    const int& col) {
    GameListItem* item = new GameListItem;
    item->set_icon_func([this, app, item](int) {
        const qreal device_pixel_ratio = devicePixelRatioF();

        // define size of expanded image, which is raw image size + margins
        QSizeF exp_size_f;
        if (m_text_enabled) {
            exp_size_f =
                m_icon_size + QSizeF(m_icon_size.width() * m_margin_factor * 2,
                                     m_icon_size.height() * m_margin_factor * (m_text_factor + 1));
        } else {
            exp_size_f = m_icon_size + m_icon_size * m_margin_factor * 2;
        }

        // define offset for raw image placement
        QPoint offset(m_icon_size.width() * m_margin_factor,
                      m_icon_size.height() * m_margin_factor);
        const QSize exp_size = (exp_size_f * device_pixel_ratio).toSize();

        // create empty canvas for expanded image
        QImage exp_img(exp_size, QImage::Format_ARGB32);
        exp_img.setDevicePixelRatio(device_pixel_ratio);
        exp_img.fill(Qt::transparent);

        // create background for image
        QImage bg_img(app->pxmap.size(), QImage::Format_ARGB32);
        bg_img.setDevicePixelRatio(device_pixel_ratio);
        bg_img.fill(m_icon_color);

        // place raw image inside expanded image
        QPainter painter(&exp_img);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawImage(offset, bg_img);
        painter.drawPixmap(offset, app->pxmap);
        app->pxmap = {};
        painter.end();

        // create item with expanded image, title and position
        item->setData(Qt::ItemDataRole::DecorationRole, QPixmap::fromImage(exp_img));
    });
    if (m_text_enabled) {
        item->setData(Qt::ItemDataRole::DisplayRole, name);
    }

    setItem(row, col, item);
    return item;
}

qreal GameListGrid::getMarginFactor() const {
    return m_margin_factor;
}
void GameListGrid::RefreshBackgroundImage() {
    QPixmap blurredPixmap = QPixmap::fromImage(backgroundImage);
    QPalette palette;
    palette.setBrush(QPalette::Base, QBrush(blurredPixmap.scaled(size(), Qt::IgnoreAspectRatio)));
    QColor transparentColor = QColor(135, 206, 235, 40);
    palette.setColor(QPalette::Highlight, transparentColor);
    this->setPalette(palette);
}
void GameListGrid::SetGridBackgroundImage(QTableWidgetItem* item) {
    if (!item) {
        // handle case where icon item does not exist
        return;
    }
    QTableWidgetItem* iconItem = this->item(item->row(), item->column());

    if (!iconItem) {
        // handle case where icon item does not exist
        return;
    }
    game_info gameinfo = GetGameInfoFromItem(iconItem);
    QString pic1Path = QString::fromStdString(gameinfo->info.pic_path);
    QString blurredPic1Path =
        qApp->applicationDirPath() +
        QString::fromStdString("/game_data/" + gameinfo->info.serial + "/pic1.png");

    backgroundImage = QImage(blurredPic1Path);
    if (backgroundImage.isNull()) {
        QImage image(pic1Path);
        backgroundImage = m_game_list_utils.BlurImage(image, image.rect(), 18);

        std::filesystem::path img_path =
            std::filesystem::path("game_data/") / gameinfo->info.serial;
        std::filesystem::create_directories(img_path);
        if (!backgroundImage.save(blurredPic1Path, "PNG")) {
            // qDebug() << "Error: Unable to save image.";
        }
    }
    QPixmap blurredPixmap = QPixmap::fromImage(backgroundImage);
    QPalette palette;
    palette.setBrush(QPalette::Base, QBrush(blurredPixmap.scaled(size(), Qt::IgnoreAspectRatio)));
    QColor transparentColor = QColor(135, 206, 235, 40);
    palette.setColor(QPalette::Highlight, transparentColor);
    this->setPalette(palette);
}

void GameListGrid::resizeEvent(QResizeEvent* event) {
    Q_EMIT ResizedWindowGrid(this->currentItem());
}