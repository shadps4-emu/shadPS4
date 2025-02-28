// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <QCheckBox>
#include <QDockWidget>
#include <QMessageBox>
#include <cmrc/cmrc.hpp>
#include <common/config.h>
#include "common/path_util.h"
#include "main_window_themes.h"
#include "trophy_viewer.h"

namespace fs = std::filesystem;

CMRC_DECLARE(res);

// true: European format; false: American format
bool useEuropeanDateFormat = true;

void TrophyViewer::updateTrophyInfo() {
    int total = 0;
    int unlocked = 0;

    // Cycles through each tab (table) of the QTabWidget
    for (int i = 0; i < tabWidget->count(); i++) {
        QTableWidget* table = qobject_cast<QTableWidget*>(tabWidget->widget(i));
        if (table) {
            total += table->rowCount();
            for (int row = 0; row < table->rowCount(); ++row) {
                QString cellText;
                // The "Unlocked" column can be a widget or a simple item
                QWidget* widget = table->cellWidget(row, 0);
                if (widget) {
                    // Looks for the QLabel inside the widget (as defined in SetTableItem)
                    QLabel* label = widget->findChild<QLabel*>();
                    if (label) {
                        cellText = label->text();
                    }
                } else {
                    QTableWidgetItem* item = table->item(row, 0);
                    if (item) {
                        cellText = item->text();
                    }
                }
                if (cellText == "unlocked")
                    unlocked++;
            }
        }
    }
    int progress = (total > 0) ? (unlocked * 100 / total) : 0;
    trophyInfoLabel->setText(
        QString(tr("Progress") + ": %1% (%2/%3)").arg(progress).arg(unlocked).arg(total));
}

void TrophyViewer::updateTableFilters() {
    bool showEarned = showEarnedCheck->isChecked();
    bool showNotEarned = showNotEarnedCheck->isChecked();
    bool showHidden = showHiddenCheck->isChecked();

    // Cycles through each tab of the QTabWidget
    for (int i = 0; i < tabWidget->count(); ++i) {
        QTableWidget* table = qobject_cast<QTableWidget*>(tabWidget->widget(i));
        if (!table)
            continue;
        for (int row = 0; row < table->rowCount(); ++row) {
            QString unlockedText;
            // Gets the text of the "Unlocked" column (index 0)
            QWidget* widget = table->cellWidget(row, 0);
            if (widget) {
                QLabel* label = widget->findChild<QLabel*>();
                if (label)
                    unlockedText = label->text();
            } else {
                QTableWidgetItem* item = table->item(row, 0);
                if (item)
                    unlockedText = item->text();
            }

            QString hiddenText;
            // Gets the text of the "Hidden" column (index 7)
            QWidget* hiddenWidget = table->cellWidget(row, 7);
            if (hiddenWidget) {
                QLabel* label = hiddenWidget->findChild<QLabel*>();
                if (label)
                    hiddenText = label->text();
            } else {
                QTableWidgetItem* item = table->item(row, 7);
                if (item)
                    hiddenText = item->text();
            }

            bool visible = true;
            if (unlockedText == "unlocked" && !showEarned)
                visible = false;
            if (unlockedText == "locked" && !showNotEarned)
                visible = false;
            if (hiddenText.toLower() == "yes" && !showHidden)
                visible = false;

            table->setRowHidden(row, !visible);
        }
    }
}

TrophyViewer::TrophyViewer(QString trophyPath, QString gameTrpPath) : QMainWindow() {
    this->setWindowTitle(tr("Trophy Viewer"));
    this->setAttribute(Qt::WA_DeleteOnClose);
    tabWidget = new QTabWidget(this);

    auto lan = Config::getEmulatorLanguage();
    if (lan == "en_US" || lan == "zh_CN" || lan == "zh_TW" || lan == "ja_JP" || lan == "ko_KR" ||
        lan == "lt_LT" || lan == "nb_NO" || lan == "nl_NL") {
        useEuropeanDateFormat = false;
    }

    gameTrpPath_ = gameTrpPath;
    headers << "Unlocked"
            << "Trophy"
            << "Name"
            << "Description"
            << "Time Unlocked"
            << "Type"
            << "ID"
            << "Hidden"
            << "PID";
    PopulateTrophyWidget(trophyPath);

    QDockWidget* trophyInfoDock = new QDockWidget("", this);
    QWidget* dockWidget = new QWidget(trophyInfoDock);
    QVBoxLayout* dockLayout = new QVBoxLayout(dockWidget);
    dockLayout->setAlignment(Qt::AlignTop);

    trophyInfoLabel = new QLabel(tr("Progress") + ": 0% (0/0)", dockWidget);
    trophyInfoLabel->setStyleSheet(
        "font-weight: bold; font-size: 16px; color: white; background: #333; padding: 5px;");
    dockLayout->addWidget(trophyInfoLabel);

    // Creates QCheckBox to filter trophies
    showEarnedCheck = new QCheckBox(tr("Show Earned Trophies"), dockWidget);
    showNotEarnedCheck = new QCheckBox(tr("Show Not Earned Trophies"), dockWidget);
    showHiddenCheck = new QCheckBox(tr("Show Hidden Trophies"), dockWidget);

    // Defines the initial states (all checked)
    showEarnedCheck->setChecked(true);
    showNotEarnedCheck->setChecked(true);
    showHiddenCheck->setChecked(false);

    // Adds checkboxes to the layout
    dockLayout->addWidget(showEarnedCheck);
    dockLayout->addWidget(showNotEarnedCheck);
    dockLayout->addWidget(showHiddenCheck);

    dockWidget->setLayout(dockLayout);
    trophyInfoDock->setWidget(dockWidget);

    // Adds the dock to the left area
    this->addDockWidget(Qt::LeftDockWidgetArea, trophyInfoDock);

    // Connects checkbox signals to update trophy display
#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 0))
    connect(showEarnedCheck, &QCheckBox::stateChanged, this, &TrophyViewer::updateTableFilters);
    connect(showNotEarnedCheck, &QCheckBox::stateChanged, this, &TrophyViewer::updateTableFilters);
    connect(showHiddenCheck, &QCheckBox::stateChanged, this, &TrophyViewer::updateTableFilters);
#else
    connect(showEarnedCheck, &QCheckBox::checkStateChanged, this,
            &TrophyViewer::updateTableFilters);
    connect(showNotEarnedCheck, &QCheckBox::checkStateChanged, this,
            &TrophyViewer::updateTableFilters);
    connect(showHiddenCheck, &QCheckBox::checkStateChanged, this,
            &TrophyViewer::updateTableFilters);
#endif

    updateTrophyInfo();
    updateTableFilters();
}

void TrophyViewer::PopulateTrophyWidget(QString title) {
    const auto trophyDir = Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) /
                           Common::FS::PathFromQString(title) / "TrophyFiles";
    QString trophyDirQt;
    Common::FS::PathToQString(trophyDirQt, trophyDir);

    QDir dir(trophyDirQt);
    if (!dir.exists()) {
        std::filesystem::path path = Common::FS::PathFromQString(gameTrpPath_);
        if (!trp.Extract(path, title.toStdString())) {
            QMessageBox::critical(this, "Trophy Data Extraction Error",
                                  "Unable to extract Trophy data, please ensure you have "
                                  "inputted a trophy key in the settings menu.");
            QWidget::close();
            return;
        }
    }
    QFileInfoList dirList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (dirList.isEmpty())
        return;

    for (const QFileInfo& dirInfo : dirList) {
        QString tabName = dirInfo.fileName();
        QString trpDir = trophyDirQt + "/" + tabName;

        QString iconsPath = trpDir + "/Icons";
        QDir iconsDir(iconsPath);
        QFileInfoList iconDirList = iconsDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
        std::vector<QImage> icons;

        for (const QFileInfo& iconInfo : iconDirList) {
            QImage icon =
                QImage(iconInfo.absoluteFilePath())
                    .scaled(QSize(128, 128), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            icons.push_back(icon);
        }

        QStringList trpId;
        QStringList trpHidden;
        QStringList trpUnlocked;
        QStringList trpType;
        QStringList trpPid;
        QStringList trophyNames;
        QStringList trophyDetails;
        QStringList trpTimeUnlocked;

        QString xmlPath = trpDir + "/Xml/TROP.XML";
        QFile file(xmlPath);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            return;
        }

        QXmlStreamReader reader(&file);

        while (!reader.atEnd() && !reader.hasError()) {
            reader.readNext();
            if (reader.isStartElement() && reader.name().toString() == "trophy") {
                trpId.append(reader.attributes().value("id").toString());
                trpHidden.append(reader.attributes().value("hidden").toString());
                trpType.append(reader.attributes().value("ttype").toString());
                trpPid.append(reader.attributes().value("pid").toString());

                if (reader.attributes().hasAttribute("unlockstate")) {
                    if (reader.attributes().value("unlockstate").toString() == "true") {
                        trpUnlocked.append("unlocked");
                    } else {
                        trpUnlocked.append("locked");
                    }
                    if (reader.attributes().hasAttribute("timestamp")) {
                        QString ts = reader.attributes().value("timestamp").toString();
                        if (ts.length() > 10)
                            trpTimeUnlocked.append("unknown");
                        else {
                            bool ok;
                            qint64 timestampInt = ts.toLongLong(&ok);
                            if (ok) {
                                QDateTime dt = QDateTime::fromSecsSinceEpoch(timestampInt);
                                QString format = useEuropeanDateFormat ? "dd/MM/yyyy HH:mm:ss"
                                                                       : "MM/dd/yyyy HH:mm:ss";
                                trpTimeUnlocked.append(dt.toString(format));
                            } else {
                                trpTimeUnlocked.append("unknown");
                            }
                        }
                    } else {
                        trpTimeUnlocked.append("");
                    }
                } else {
                    trpUnlocked.append("locked");
                    trpTimeUnlocked.append("");
                }
            }

            if (reader.name().toString() == "name" && !trpId.isEmpty()) {
                trophyNames.append(reader.readElementText());
            }

            if (reader.name().toString() == "detail" && !trpId.isEmpty()) {
                trophyDetails.append(reader.readElementText());
            }
        }
        QTableWidget* tableWidget = new QTableWidget(this);
        tableWidget->setShowGrid(false);
        tableWidget->setColumnCount(9);
        tableWidget->setHorizontalHeaderLabels(headers);
        tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        tableWidget->horizontalHeader()->setStretchLastSection(true);
        tableWidget->verticalHeader()->setVisible(false);
        tableWidget->setRowCount(icons.size());
        tableWidget->setSortingEnabled(true);

        for (int row = 0; auto& icon : icons) {
            QTableWidgetItem* item = new QTableWidgetItem();
            item->setData(Qt::DecorationRole, icon);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            tableWidget->setItem(row, 1, item);

            const std::string filename = GetTrpType(trpType[row].at(0));
            QTableWidgetItem* typeitem = new QTableWidgetItem();

            const auto CustomTrophy_Dir =
                Common::FS::GetUserPath(Common::FS::PathType::CustomTrophy);
            std::string customPath;

            if (fs::exists(CustomTrophy_Dir / filename)) {
                customPath = (CustomTrophy_Dir / filename).string();
            }

            std::vector<char> imgdata;

            if (!customPath.empty()) {
                std::ifstream file(customPath, std::ios::binary);
                if (file) {
                    imgdata = std::vector<char>(std::istreambuf_iterator<char>(file),
                                                std::istreambuf_iterator<char>());
                }
            } else {
                auto resource = cmrc::res::get_filesystem();
                std::string resourceString = "src/images/" + filename;
                auto file = resource.open(resourceString);
                imgdata = std::vector<char>(file.begin(), file.end());
            }

            QImage type_icon = QImage::fromData(imgdata).scaled(
                QSize(100, 100), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            typeitem->setData(Qt::DecorationRole, type_icon);
            typeitem->setFlags(typeitem->flags() & ~Qt::ItemIsEditable);
            tableWidget->setItem(row, 5, typeitem);

            std::string detailString = trophyDetails[row].toStdString();
            std::size_t newline_pos = 0;
            while ((newline_pos = detailString.find("\n", newline_pos)) != std::string::npos) {
                detailString.replace(newline_pos, 1, " ");
                ++newline_pos;
            }

            if (!trophyNames.isEmpty() && !trophyDetails.isEmpty()) {
                SetTableItem(tableWidget, row, 0, trpUnlocked[row]);
                SetTableItem(tableWidget, row, 2, trophyNames[row]);
                SetTableItem(tableWidget, row, 3, QString::fromStdString(detailString));
                SetTableItem(tableWidget, row, 4, trpTimeUnlocked[row]);
                SetTableItem(tableWidget, row, 6, trpId[row]);
                SetTableItem(tableWidget, row, 7, trpHidden[row]);
                SetTableItem(tableWidget, row, 8, trpPid[row]);
            }
            tableWidget->verticalHeader()->resizeSection(row, icon.height());
            row++;
        }
        tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        int width = 16;
        for (int i = 0; i < 9; i++) {
            width += tableWidget->horizontalHeader()->sectionSize(i);
        }
        tableWidget->resize(width, 720);
        tabWidget->addTab(tableWidget,
                          tabName.insert(6, " ").replace(0, 1, tabName.at(0).toUpper()));

        this->showMaximized();

        tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
        tableWidget->setColumnWidth(3, 650);
    }
    this->setCentralWidget(tabWidget);
}

void TrophyViewer::SetTableItem(QTableWidget* parent, int row, int column, QString str) {
    QTableWidgetItem* item = new QTableWidgetItem(str);

    if (column != 1 && column != 2 && column != 3)
        item->setTextAlignment(Qt::AlignCenter);
    item->setFont(QFont("Arial", 12, QFont::Bold));

    Theme theme = static_cast<Theme>(Config::getMainWindowTheme());

    if (theme == Theme::Light) {
        item->setForeground(QBrush(Qt::black));
    } else {
        item->setForeground(QBrush(Qt::white));
    }

    parent->setItem(row, column, item);
}
