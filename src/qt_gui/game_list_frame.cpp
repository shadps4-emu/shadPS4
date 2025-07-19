// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QToolTip>
#include "common/config.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/string_util.h"
#include "game_list_frame.h"
#include "game_list_utils.h"

GameListFrame::GameListFrame(std::shared_ptr<gui_settings> gui_settings,
                             std::shared_ptr<GameInfoClass> game_info_get,
                             std::shared_ptr<CompatibilityInfoClass> compat_info_get,
                             QWidget* parent)
    : QTableWidget(parent), m_gui_settings(std::move(gui_settings)), m_game_info(game_info_get),
      m_compat_info(compat_info_get) {
    icon_size = m_gui_settings->GetValue(gui::gl_icon_size).toInt();
    last_favorite = "";
    this->setShowGrid(false);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->setSelectionMode(QAbstractItemView::SingleSelection);
    this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->verticalScrollBar()->installEventFilter(this);
    this->verticalScrollBar()->setSingleStep(20);
    this->horizontalScrollBar()->setSingleStep(20);
    this->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->verticalHeader()->setVisible(false);
    this->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    this->horizontalHeader()->setHighlightSections(false);
    this->horizontalHeader()->setSortIndicatorShown(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setColumnCount(11);
    this->setColumnWidth(1, 300); // Name
    this->setColumnWidth(2, 140); // Compatibility
    this->setColumnWidth(3, 120); // Serial
    this->setColumnWidth(4, 90);  // Region
    this->setColumnWidth(5, 90);  // Firmware
    this->setColumnWidth(6, 90);  // Size
    this->setColumnWidth(7, 90);  // Version
    this->setColumnWidth(8, 120); // Play Time
    this->setColumnWidth(10, 90); // Favorite
    QStringList headers;
    headers << tr("Icon") << tr("Name") << tr("Compatibility") << tr("Serial") << tr("Region")
            << tr("Firmware") << tr("Size") << tr("Version") << tr("Play Time") << tr("Path")
            << tr("Favorite");
    this->setHorizontalHeaderLabels(headers);
    this->horizontalHeader()->setSortIndicatorShown(true);
    this->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    this->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    this->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    this->horizontalHeader()->setSectionResizeMode(9, QHeaderView::Stretch);
    this->horizontalHeader()->setSectionResizeMode(10, QHeaderView::Fixed);
    PopulateGameList();

    connect(this, &QTableWidget::currentCellChanged, this, &GameListFrame::onCurrentCellChanged);
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
                sortColumn = columnIndex;
            } else {
                SortNameAscending(columnIndex);
                this->horizontalHeader()->setSortIndicator(columnIndex, Qt::AscendingOrder);
                ListSortedAsc = true;
                sortColumn = columnIndex;
            }
            this->clearContents();
            PopulateGameList(false);
        });

    connect(this, &QTableWidget::customContextMenuRequested, this, [=, this](const QPoint& pos) {
        int changedFavorite = m_gui_context_menus.RequestGameMenu(
            pos, m_game_info->m_games, m_compat_info, m_gui_settings, this, true);
        if (changedFavorite) {
            last_favorite = m_game_info->m_games[this->currentRow()].serial;
            PopulateGameList(false);
        }
    });

    connect(this, &QTableWidget::cellClicked, this, [=, this](int row, int column) {
        if (column == 2 && m_game_info->m_games[row].compatibility.issue_number != "") {
            auto url_issues =
                "https://github.com/shadps4-compatibility/shadps4-game-compatibility/issues/";
            QDesktopServices::openUrl(
                QUrl(url_issues + m_game_info->m_games[row].compatibility.issue_number));
        } else if (column == 10) {
            last_favorite = m_game_info->m_games[row].serial;
            QString serialStr = QString::fromStdString(last_favorite);
            QList<QString> list =
                gui_settings::Var2List(m_gui_settings->GetValue(gui::favorites_list));
            bool isFavorite = list.contains(serialStr);
            if (isFavorite) {
                list.removeOne(serialStr);
            } else {
                list.append(serialStr);
            }
            m_gui_settings->SetValue(gui::favorites_list, gui_settings::List2Var(list));
            PopulateGameList(false);
        }
    });
}

void GameListFrame::onCurrentCellChanged(int currentRow, int currentColumn, int previousRow,
                                         int previousColumn) {
    QTableWidgetItem* item = this->item(currentRow, currentColumn);
    if (!item) {
        return;
    }
    m_current_item = item; // Store current item
    SetListBackgroundImage(item);
    PlayBackgroundMusic(item);
}

void GameListFrame::PlayBackgroundMusic(QTableWidgetItem* item) {
    if (!item || !m_gui_settings->GetValue(gui::gl_playBackgroundMusic).toBool()) {
        BackgroundMusicPlayer::getInstance().stopMusic();
        return;
    }
    QString snd0path;
    Common::FS::PathToQString(snd0path, m_game_info->m_games[item->row()].snd0_path);
    BackgroundMusicPlayer::getInstance().playMusic(snd0path);
}

void GameListFrame::PopulateGameList(bool isInitialPopulation) {
    this->m_current_item = nullptr;
    // Do not show status column if it is not enabled
    this->setColumnHidden(2, !Config::getCompatibilityEnabled());
    this->setColumnHidden(6, !Config::GetLoadGameSizeEnabled());

    this->setRowCount(m_game_info->m_games.size());
    ResizeIcons(icon_size);

    ApplyLastSorting(isInitialPopulation);

    for (int i = 0; i < m_game_info->m_games.size(); i++) {
        SetTableItem(i, 1, QString::fromStdString(m_game_info->m_games[i].name));
        SetTableItem(i, 3, QString::fromStdString(m_game_info->m_games[i].serial));
        SetRegionFlag(i, 4, QString::fromStdString(m_game_info->m_games[i].region));
        SetTableItem(i, 5, QString::fromStdString(m_game_info->m_games[i].fw));
        SetTableItem(i, 6, QString::fromStdString(m_game_info->m_games[i].size));
        SetTableItem(i, 7, QString::fromStdString(m_game_info->m_games[i].version));
        SetFavoriteIcon(i, 10);

        if (m_game_info->m_games[i].serial == last_favorite && !isInitialPopulation) {
            this->setCurrentCell(i, 10);
        }

        m_game_info->m_games[i].compatibility =
            m_compat_info->GetCompatibilityInfo(m_game_info->m_games[i].serial);
        SetCompatibilityItem(i, 2, m_game_info->m_games[i].compatibility);

        QString playTime = GetPlayTime(m_game_info->m_games[i].serial);
        if (playTime.isEmpty()) {
            m_game_info->m_games[i].play_time = "0:00:00";
            SetTableItem(i, 8, tr("Never Played"));
        } else {
            QStringList timeParts = playTime.split(':');
            int hours = timeParts[0].toInt();
            int minutes = timeParts[1].toInt();
            int seconds = timeParts[2].toInt();

            QString formattedPlayTime;
            if (hours > 0) {
                formattedPlayTime += QString("%1").arg(hours) + tr("h");
            }
            if (minutes > 0) {
                formattedPlayTime += QString("%1").arg(minutes) + tr("m");
            }

            formattedPlayTime = formattedPlayTime.trimmed();
            m_game_info->m_games[i].play_time = playTime.toStdString();
            if (formattedPlayTime.isEmpty()) {
                SetTableItem(i, 8, QString("%1").arg(seconds) + tr("s"));
            } else {
                SetTableItem(i, 8, formattedPlayTime);
            }
        }

        QString path;
        Common::FS::PathToQString(path, m_game_info->m_games[i].path);
        SetTableItem(i, 9, path);
    }
}

void GameListFrame::SetListBackgroundImage(QTableWidgetItem* item) {
    if (!item) {
        // handle case where no item was clicked
        return;
    }

    // If background images are hidden, clear the background image
    if (!m_gui_settings->GetValue(gui::gl_showBackgroundImage).toBool()) {
        backgroundImage = QImage();
        m_last_opacity = -1;         // Reset opacity tracking when disabled
        m_current_game_path.clear(); // Reset current game path
        RefreshListBackgroundImage();
        return;
    }

    const auto& game = m_game_info->m_games[item->row()];
    const int opacity = m_gui_settings->GetValue(gui::gl_backgroundImageOpacity).toInt();

    // Recompute if opacity changed or we switched to a different game
    if (opacity != m_last_opacity || game.pic_path != m_current_game_path) {
        auto image_path = game.pic_path.u8string();
        QImage original_image(QString::fromStdString({image_path.begin(), image_path.end()}));
        if (!original_image.isNull()) {
            backgroundImage = m_game_list_utils.ChangeImageOpacity(
                original_image, original_image.rect(), opacity / 100.0f);
            m_last_opacity = opacity;
            m_current_game_path = game.pic_path;
        }
    }

    RefreshListBackgroundImage();
}

void GameListFrame::RefreshListBackgroundImage() {
    QPalette palette;
    if (!backgroundImage.isNull() &&
        m_gui_settings->GetValue(gui::gl_showBackgroundImage).toBool()) {
        QSize widgetSize = size();
        QPixmap scaledPixmap =
            QPixmap::fromImage(backgroundImage)
                .scaled(widgetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int x = (widgetSize.width() - scaledPixmap.width()) / 2;
        int y = (widgetSize.height() - scaledPixmap.height()) / 2;
        QPixmap finalPixmap(widgetSize);
        finalPixmap.fill(Qt::transparent);
        QPainter painter(&finalPixmap);
        painter.drawPixmap(x, y, scaledPixmap);
        palette.setBrush(QPalette::Base, QBrush(finalPixmap));
    }
    QColor transparentColor = QColor(135, 206, 235, 40);
    palette.setColor(QPalette::Highlight, transparentColor);
    this->setPalette(palette);
}

void GameListFrame::resizeEvent(QResizeEvent* event) {
    QTableWidget::resizeEvent(event);
    RefreshListBackgroundImage();
}

bool GameListFrame::CompareWithFavorite(GameInfo a, GameInfo b, int columnIndex, bool ascending) {
    std::string serial_a = a.serial;
    std::string serial_b = b.serial;
    QString serialStr_a = QString::fromStdString(a.serial);
    QString serialStr_b = QString::fromStdString(b.serial);
    QList<QString> list = gui_settings::Var2List(m_gui_settings->GetValue(gui::favorites_list));
    bool isFavorite_a = list.contains(serialStr_a);
    bool isFavorite_b = list.contains(serialStr_b);
    if (isFavorite_a != isFavorite_b) {
        return isFavorite_a;
    } else if (ascending) {
        return CompareStringsAscending(a, b, columnIndex);
    } else {
        return CompareStringsDescending(a, b, columnIndex);
    }
}

void GameListFrame::SortNameAscending(int columnIndex) {
    std::sort(m_game_info->m_games.begin(), m_game_info->m_games.end(),
              [this, columnIndex](const GameInfo& a, const GameInfo& b) {
                  return this->CompareWithFavorite(a, b, columnIndex, true);
              });
}

void GameListFrame::SortNameDescending(int columnIndex) {
    std::sort(m_game_info->m_games.begin(), m_game_info->m_games.end(),
              [this, columnIndex](const GameInfo& a, const GameInfo& b) {
                  return this->CompareWithFavorite(a, b, columnIndex, false);
              });
}

void GameListFrame::ApplyLastSorting(bool isInitialPopulation) {
    if (isInitialPopulation) {
        SortNameAscending(1); // Column 1 = Name
        ResizeIcons(icon_size);
    } else if (ListSortedAsc) {
        SortNameAscending(sortColumn);
        ResizeIcons(icon_size);
    } else {
        SortNameDescending(sortColumn);
        ResizeIcons(icon_size);
    }
}

void GameListFrame::ResizeIcons(int iconSize) {
    for (int index = 0; auto& game : m_game_info->m_games) {
        QImage scaledPixmap = game.icon.scaled(QSize(iconSize, iconSize), Qt::KeepAspectRatio,
                                               Qt::SmoothTransformation);
        QTableWidgetItem* iconItem = new QTableWidgetItem();
        this->verticalHeader()->resizeSection(index, scaledPixmap.height());
        this->horizontalHeader()->resizeSection(0, scaledPixmap.width());
        iconItem->setData(Qt::DecorationRole, scaledPixmap);
        this->setItem(index, 0, iconItem);
        index++;
    }
    this->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
}

void GameListFrame::SetCompatibilityItem(int row, int column, CompatibilityEntry entry) {
    QTableWidgetItem* item = new QTableWidgetItem();
    QWidget* widget = new QWidget(this);
    QGridLayout* layout = new QGridLayout(widget);

    widget->setStyleSheet("QToolTip {background-color: black; color: white;}");

    QColor color;
    QString status_explanation;

    switch (entry.status) {
    case CompatibilityStatus::Unknown:
        color = QStringLiteral("#000000");
        status_explanation = tr("Compatibility is untested");
        break;
    case CompatibilityStatus::Nothing:
        color = QStringLiteral("#212121");
        status_explanation = tr("Game does not initialize properly / crashes the emulator");
        break;
    case CompatibilityStatus::Boots:
        color = QStringLiteral("#828282");
        status_explanation = tr("Game boots, but only displays a blank screen");
        break;
    case CompatibilityStatus::Menus:
        color = QStringLiteral("#FF0000");
        status_explanation = tr("Game displays an image but does not go past the menu");
        break;
    case CompatibilityStatus::Ingame:
        color = QStringLiteral("#F2D624");
        status_explanation = tr("Game has game-breaking glitches or unplayable performance");
        break;
    case CompatibilityStatus::Playable:
        color = QStringLiteral("#47D35C");
        status_explanation =
            tr("Game can be completed with playable performance and no major glitches");
        break;
    }

    QString tooltip_string;

    if (entry.status == CompatibilityStatus::Unknown) {
        tooltip_string = status_explanation;
    } else {
        tooltip_string =
            "<p> <i>" + tr("Click to see details on github") + "</i>" + "<br>" +
            tr("Last updated") +
            QString(": %1 (%2)").arg(entry.last_tested.toString("yyyy-MM-dd"), entry.version) +
            "<br>" + status_explanation + "</p>";
    }

    QPixmap circle_pixmap(16, 16);
    circle_pixmap.fill(Qt::transparent);
    QPainter painter(&circle_pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(color);
    painter.setBrush(color);
    painter.drawEllipse({circle_pixmap.width() / 2.0, circle_pixmap.height() / 2.0}, 6.0, 6.0);

    QLabel* dotLabel = new QLabel("", widget);
    dotLabel->setPixmap(circle_pixmap);

    QLabel* label = new QLabel(m_compat_info->GetCompatStatusString(entry.status), widget);
    this->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    label->setStyleSheet("color: white; font-size: 16px; font-weight: bold;");

    // Create shadow effect
    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(5);               // Set the blur radius of the shadow
    shadowEffect->setColor(QColor(0, 0, 0, 160)); // Set the color and opacity of the shadow
    shadowEffect->setOffset(2, 2);                // Set the offset of the shadow

    label->setGraphicsEffect(shadowEffect); // Apply shadow effect to the QLabel

    layout->addWidget(dotLabel, 0, 0, -1, 1);
    layout->addWidget(label, 0, 1, 1, 1);
    layout->setAlignment(Qt::AlignLeft);
    widget->setLayout(layout);
    widget->setToolTip(tooltip_string);
    this->setItem(row, column, item);
    this->setCellWidget(row, column, widget);

    return;
}

void GameListFrame::SetTableItem(int row, int column, QString itemStr) {
    QTableWidgetItem* item = new QTableWidgetItem();
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    QLabel* label = new QLabel(itemStr, widget);

    label->setStyleSheet("color: white; font-size: 16px; font-weight: bold;");

    // Create shadow effect
    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(5);               // Set the blur radius of the shadow
    shadowEffect->setColor(QColor(0, 0, 0, 160)); // Set the color and opacity of the shadow
    shadowEffect->setOffset(2, 2);                // Set the offset of the shadow

    label->setGraphicsEffect(shadowEffect); // Apply shadow effect to the QLabel

    layout->addWidget(label);
    if (column != 8 && column != 1)
        layout->setAlignment(Qt::AlignCenter);
    widget->setLayout(layout);
    this->setItem(row, column, item);
    this->setCellWidget(row, column, widget);
}

void GameListFrame::SetRegionFlag(int row, int column, QString itemStr) {
    QTableWidgetItem* item = new QTableWidgetItem();
    QImage scaledPixmap;
    if (itemStr == "Japan") {
        scaledPixmap = QImage(":images/flag_jp.png");
    } else if (itemStr == "Europe") {
        scaledPixmap = QImage(":images/flag_eu.png");
    } else if (itemStr == "USA") {
        scaledPixmap = QImage(":images/flag_us.png");
    } else if (itemStr == "Asia") {
        scaledPixmap = QImage(":images/flag_china.png");
    } else if (itemStr == "World") {
        scaledPixmap = QImage(":images/flag_world.png");
    } else {
        scaledPixmap = QImage(":images/flag_unk.png");
    }
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    QLabel* label = new QLabel(widget);
    label->setPixmap(QPixmap::fromImage(scaledPixmap));
    layout->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    widget->setLayout(layout);
    this->setItem(row, column, item);
    this->setCellWidget(row, column, widget);
}

void GameListFrame::SetFavoriteIcon(int row, int column) {

    QString serialStr = QString::fromStdString(m_game_info->m_games[row].serial);
    QList<QString> list = gui_settings::Var2List(m_gui_settings->GetValue(gui::favorites_list));
    bool isFavorite = list.contains(serialStr);

    QTableWidgetItem* item = new QTableWidgetItem();
    QImage scaledPixmap = QImage(":images/favorite_icon.png");

    scaledPixmap = scaledPixmap.scaledToHeight(this->columnWidth(column) / 2.5);
    scaledPixmap = scaledPixmap.scaledToWidth(this->columnWidth(column) / 2.5);
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    QLabel* label = new QLabel(widget);
    label->setPixmap(QPixmap::fromImage(scaledPixmap));
    label->setObjectName("favoriteIcon");
    label->setVisible(isFavorite);

    layout->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    widget->setLayout(layout);
    this->setItem(row, column, item);
    this->setCellWidget(row, column, widget);

    if (column > 0) {
        this->horizontalHeader()->setSectionResizeMode(column - 1, QHeaderView::Stretch);
    }
}

QString GameListFrame::GetPlayTime(const std::string& serial) {
    QString playTime;
    const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    QString filePath = QString::fromStdString((user_dir / "play_time.txt").string());

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return playTime;
    }

    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        QString lineStr = QString::fromUtf8(line).trimmed();

        QStringList parts = lineStr.split(' ');
        if (parts.size() >= 2) {
            QString fileSerial = parts[0];
            QString time = parts[1];

            if (fileSerial == QString::fromStdString(serial)) {
                playTime = time;
                break;
            }
        }
    }

    file.close();
    return playTime;
}

QTableWidgetItem* GameListFrame::GetCurrentItem() {
    return m_current_item;
}
