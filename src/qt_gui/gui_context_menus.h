// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QCheckBox>
#include <QClipboard>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFile>
#include <QGroupBox>
#include <QHeaderView>
#include <QImage>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTextStream>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include "game_info.h"
#include "trophy_viewer.h"

#ifdef Q_OS_WIN
#include <ShlObj.h>
#include <Windows.h>
#include <objbase.h>
#include <shlguid.h>
#include <shobjidl.h>
#endif
#include "common/path_util.h"

class GuiContextMenus : public QObject {
    Q_OBJECT
public:
    void RequestGameMenu(const QPoint& pos, QVector<GameInfo> m_games, QTableWidget* widget,
                         bool isList) {
        QPoint global_pos = widget->viewport()->mapToGlobal(pos);
        int itemID = 0;
        if (isList) {
            itemID = widget->currentRow();
        } else {
            itemID = widget->currentRow() * widget->columnCount() + widget->currentColumn();
        }

        // Setup menu.
        QMenu menu(widget);
        QAction createShortcut("Create Shortcut", widget);
        QAction openFolder("Open Game Folder", widget);
        QAction openCheats("Cheats/Patches", widget);
        QAction openSfoViewer("SFO Viewer", widget);
        QAction openTrophyViewer("Trophy Viewer", widget);

        menu.addAction(&openFolder);
        menu.addAction(&createShortcut);
        menu.addAction(&openCheats);
        menu.addAction(&openSfoViewer);
        menu.addAction(&openTrophyViewer);

        // "Copy" submenu.
        QMenu* copyMenu = new QMenu("Copy info", widget);
        QAction* copyName = new QAction("Copy Name", widget);
        QAction* copySerial = new QAction("Copy Serial", widget);
        QAction* copyNameAll = new QAction("Copy All", widget);

        copyMenu->addAction(copyName);
        copyMenu->addAction(copySerial);
        copyMenu->addAction(copyNameAll);

        menu.addMenu(copyMenu);

        // Show menu.
        auto selected = menu.exec(global_pos);
        if (!selected) {
            return;
        }

        if (selected == &openFolder) {
            QString folderPath = QString::fromStdString(m_games[itemID].path);
            QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
        }

        if (selected == &openSfoViewer) {
            PSF psf;
            if (psf.open(m_games[itemID].path + "/sce_sys/param.sfo", {})) {
                int rows = psf.map_strings.size() + psf.map_integers.size();
                QTableWidget* tableWidget = new QTableWidget(rows, 2);
                tableWidget->setAttribute(Qt::WA_DeleteOnClose);
                connect(widget->parent(), &QWidget::destroyed, tableWidget,
                        [widget, tableWidget]() { tableWidget->deleteLater(); });

                tableWidget->verticalHeader()->setVisible(false); // Hide vertical header
                int row = 0;

                for (const auto& pair : psf.map_strings) {
                    QTableWidgetItem* keyItem =
                        new QTableWidgetItem(QString::fromStdString(pair.first));
                    QTableWidgetItem* valueItem =
                        new QTableWidgetItem(QString::fromStdString(pair.second));

                    tableWidget->setItem(row, 0, keyItem);
                    tableWidget->setItem(row, 1, valueItem);
                    keyItem->setFlags(keyItem->flags() & ~Qt::ItemIsEditable);
                    valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
                    row++;
                }
                for (const auto& pair : psf.map_integers) {
                    QTableWidgetItem* keyItem =
                        new QTableWidgetItem(QString::fromStdString(pair.first));
                    QTableWidgetItem* valueItem = new QTableWidgetItem(
                        QString("0x").append(QString::number(pair.second, 16)));

                    tableWidget->setItem(row, 0, keyItem);
                    tableWidget->setItem(row, 1, valueItem);
                    keyItem->setFlags(keyItem->flags() & ~Qt::ItemIsEditable);
                    valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
                    row++;
                }
                tableWidget->resizeColumnsToContents();
                tableWidget->resizeRowsToContents();

                int width = tableWidget->horizontalHeader()->sectionSize(0) +
                            tableWidget->horizontalHeader()->sectionSize(1) + 2;
                int height = (rows + 1) * (tableWidget->rowHeight(0));
                tableWidget->setFixedSize(width, height);
                tableWidget->sortItems(0, Qt::AscendingOrder);
                tableWidget->horizontalHeader()->setVisible(false);

                tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
                tableWidget->setWindowTitle("SFO Viewer");
                tableWidget->show();
            }
        }

        if (selected == &openCheats) {
            const auto& CHEATS_DIR = Common::FS::GetUserPath(Common::FS::PathType::CheatsDir);
            QString CHEATS_DIR_QString = QString::fromStdString(CHEATS_DIR.string());
            const QString NameCheatJson = QString::fromStdString(m_games[itemID].serial) + "_" +
                                          QString::fromStdString(m_games[itemID].version) + ".json";
            const QString cheatFilePath = CHEATS_DIR_QString + "/" + NameCheatJson;

            QWidget* cheatWidget = new QWidget();
            cheatWidget->setAttribute(Qt::WA_DeleteOnClose);
            cheatWidget->setWindowTitle("Cheats/Patches");
            cheatWidget->resize(700, 300);

            QVBoxLayout* mainLayout = new QVBoxLayout(cheatWidget);
            QHBoxLayout* horizontalLayout = new QHBoxLayout();

            // GroupBox for game information (Left side)
            QGroupBox* gameInfoGroupBox = new QGroupBox();
            QVBoxLayout* leftLayout = new QVBoxLayout(gameInfoGroupBox);
            leftLayout->setAlignment(Qt::AlignTop);

            // Game image
            QLabel* gameImage = new QLabel();
            QPixmap pixmap(QString::fromStdString(m_games[itemID].icon_path));
            gameImage->setPixmap(pixmap.scaled(250, 250, Qt::KeepAspectRatio));
            gameImage->setAlignment(Qt::AlignCenter);
            leftLayout->addWidget(gameImage, 0, Qt::AlignCenter);

            // Game name
            QLabel* gameName = new QLabel(QString::fromStdString(m_games[itemID].name));
            gameName->setAlignment(Qt::AlignLeft);
            gameName->setWordWrap(true);
            leftLayout->addWidget(gameName);

            // Game serial
            QLabel* gameSerial =
                new QLabel("Serial: " + QString::fromStdString(m_games[itemID].serial));
            gameSerial->setAlignment(Qt::AlignLeft);
            leftLayout->addWidget(gameSerial);

            // Game version
            QLabel* gameVersion =
                new QLabel("Version: " + QString::fromStdString(m_games[itemID].version));
            gameVersion->setAlignment(Qt::AlignLeft);
            leftLayout->addWidget(gameVersion);

            // Game size
            QLabel* gameSize = new QLabel("Size: " + QString::fromStdString(m_games[itemID].size));
            gameSize->setAlignment(Qt::AlignLeft);
            leftLayout->addWidget(gameSize);

            // Check for credits and add QLabel if exists
            QFile file(cheatFilePath);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray jsonData = file.readAll();
                QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
                QJsonObject jsonObject = jsonDoc.object();

                if (jsonObject.contains("credits")) {
                    QJsonArray creditsArray = jsonObject["credits"].toArray();
                    if (!creditsArray.isEmpty()) {
                        QStringList creditsList;
                        for (const QJsonValue& value : creditsArray) {
                            creditsList.append(value.toString());
                        }
                        QString credits = creditsList.join(", ");
                        QLabel* creditsLabel = new QLabel("Author: " + credits);
                        creditsLabel->setAlignment(Qt::AlignLeft);
                        leftLayout->addWidget(creditsLabel);
                    }
                }
            }

            gameInfoGroupBox->setLayout(leftLayout);
            horizontalLayout->addWidget(gameInfoGroupBox);
            QScrollArea* scrollArea = new QScrollArea();
            scrollArea->setWidgetResizable(true);

            // Right
            QGroupBox* cheatsGroupBox = new QGroupBox();
            QVBoxLayout* rightLayout = new QVBoxLayout(cheatsGroupBox);
            QString checkBoxStyle = "QCheckBox { font-size: 19px; }";
            QString buttonStyle = "QPushButton { font-size: 19px; }";
            rightLayout->setAlignment(Qt::AlignTop);

            // Function to add checkboxes and buttons to the layout
            auto addMods = [=](const QJsonArray& modsArray) {
                // Limpar widgets existentes
                QLayoutItem* item;
                while ((item = rightLayout->takeAt(0)) != nullptr) {
                    delete item->widget();
                    delete item;
                }

                for (const QJsonValue& modValue : modsArray) {
                    QJsonObject modObject = modValue.toObject();
                    QString modName = modObject["name"].toString();
                    QString modType = modObject["type"].toString();

                    if (modType == "checkbox") {
                        bool isEnabled = modObject.contains("is_enabled")
                                             ? modObject["is_enabled"].toBool()
                                             : false;
                        QCheckBox* cheatCheckBox = new QCheckBox(modName);
                        cheatCheckBox->setStyleSheet(checkBoxStyle);
                        cheatCheckBox->setChecked(isEnabled);
                        rightLayout->addWidget(cheatCheckBox);

                        // Connect the toggled(bool) signal to handle state change
                        connect(cheatCheckBox, &QCheckBox::toggled, [=](bool checked) {
                            if (checked) {
                                // Implement action when checkbox is checked
                            } else {
                                // Implement action when checkbox is unchecked
                            }
                        });
                    } else if (modType == "button") {
                        QPushButton* cheatButton = new QPushButton(modName);
                        cheatButton->setStyleSheet(buttonStyle);
                        connect(cheatButton, &QPushButton::clicked, [=]() {
                            // Implementar a ação do botão !!!
                        });
                        rightLayout->addWidget(cheatButton);
                    }
                }
            };

            QNetworkAccessManager* manager = new QNetworkAccessManager(cheatWidget);

            auto loadCheats = [=](const QString& filePath) {
                QFile file(filePath);
                if (file.open(QIODevice::ReadOnly)) {
                    QByteArray jsonData = file.readAll();
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
                    QJsonObject jsonObject = jsonDoc.object();
                    QJsonArray modsArray = jsonObject["mods"].toArray();
                    addMods(modsArray);
                }
            };

            loadCheats(cheatFilePath);
            cheatsGroupBox->setLayout(rightLayout);
            scrollArea->setWidget(cheatsGroupBox);
            horizontalLayout->addWidget(scrollArea);
            mainLayout->addLayout(horizontalLayout);

            QHBoxLayout* buttonLayout = new QHBoxLayout();
            QPushButton* checkUpdateButton = new QPushButton("Check Update");
            QPushButton* cancelButton = new QPushButton("Cancel");
            QPushButton* saveButton = new QPushButton("Save");

            connect(checkUpdateButton, &QPushButton::clicked, [=]() {
                if (QFile::exists(cheatFilePath)) {
                    QMessageBox::StandardButton reply;
                    reply = QMessageBox::question(cheatWidget, "File Exists",
                                                  "File already exists. Do you want to replace it?",
                                                  QMessageBox::Yes | QMessageBox::No);
                    if (reply == QMessageBox::No) {
                        return;
                    }
                }

                // Cheats repository URL, replace with a synced fork of the repository
                const QString url = "https://raw.githubusercontent.com/GoldHEN/"
                                    "GoldHEN_Cheat_Repository/main/json/" +
                                    NameCheatJson;

                QNetworkRequest request(url);
                QNetworkReply* reply = manager->get(request);

                connect(reply, &QNetworkReply::finished, [=]() {
                    if (reply->error() == QNetworkReply::NoError) {
                        QByteArray jsonData = reply->readAll();

                        // Save the JSON file in the cheats folder
                        QFile cheatFile(cheatFilePath);
                        if (cheatFile.open(QIODevice::WriteOnly)) {
                            cheatFile.write(jsonData);
                            cheatFile.close();
                        }

                        // Reload and add new widgets
                        loadCheats(cheatFilePath);
                    } else {
                        QMessageBox::warning(
                            cheatWidget, "Cheats/Patches not found",
                            "No Cheats/Patches found for this game in this version.");
                    }

                    reply->deleteLater();
                });
            });

            connect(cancelButton, &QPushButton::clicked, [=]() { cheatWidget->close(); });

            connect(saveButton, &QPushButton::clicked, [=]() {
                QJsonDocument jsonDoc;
                QFile file(cheatFilePath);
                if (file.open(QIODevice::ReadOnly)) {
                    jsonDoc = QJsonDocument::fromJson(file.readAll());
                    file.close();
                }

                QJsonObject json = jsonDoc.object();
                QJsonArray modsArray = json["mods"].toArray();

                QMap<QString, bool> modMap;
                for (int i = 0; i < rightLayout->count(); ++i) {
                    QWidget* widget = rightLayout->itemAt(i)->widget();
                    if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget)) {
                        modMap[checkBox->text()] = checkBox->isChecked();
                    }
                    // Buttons don't need state saving, so we ignore them.
                }

                for (auto it = modMap.begin(); it != modMap.end(); ++it) {
                    bool found = false;
                    for (int i = 0; i < modsArray.size(); ++i) {
                        QJsonObject mod = modsArray[i].toObject();
                        if (mod["name"].toString() == it.key()) {
                            mod["is_enabled"] = it.value();
                            modsArray[i] = mod;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        QJsonObject newMod;
                        newMod["name"] = it.key();
                        newMod["is_enabled"] = it.value();
                        modsArray.append(newMod);
                    }
                }

                json["mods"] = modsArray;
                jsonDoc.setObject(json);

                if (file.open(QIODevice::WriteOnly)) {
                    file.write(jsonDoc.toJson());
                    file.close();
                    QMessageBox::information(cheatWidget, "Save", "Settings saved.");
                    cheatWidget->close();
                } else {
                    QMessageBox::warning(cheatWidget, "Error", "Could not open file for writing.");
                }
            });

            buttonLayout->addWidget(checkUpdateButton);
            buttonLayout->addWidget(cancelButton);
            buttonLayout->addWidget(saveButton);
            mainLayout->addLayout(buttonLayout);
            cheatWidget->setLayout(mainLayout);
            cheatWidget->show();
        }

        if (selected == &openTrophyViewer) {
            QString trophyPath = QString::fromStdString(m_games[itemID].serial);
            QString gameTrpPath = QString::fromStdString(m_games[itemID].path);
            TrophyViewer* trophyViewer = new TrophyViewer(trophyPath, gameTrpPath);
            trophyViewer->show();
            connect(widget->parent(), &QWidget::destroyed, trophyViewer,
                    [widget, trophyViewer]() { trophyViewer->deleteLater(); });
        }

        if (selected == &createShortcut) {
            QString targetPath = QString::fromStdString(m_games[itemID].path);
            QString ebootPath = targetPath + "/eboot.bin";

            // Get the full path to the icon
            QString iconPath = QString::fromStdString(m_games[itemID].icon_path);
            QFileInfo iconFileInfo(iconPath);
            QString icoPath = iconFileInfo.absolutePath() + "/" + iconFileInfo.baseName() + ".ico";

            // Path to shortcut/link
            QString linkPath;

            // Path to the shadps4.exe executable
            QString exePath;
#ifdef Q_OS_WIN
            linkPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" +
                       QString::fromStdString(m_games[itemID].name)
                           .remove(QRegularExpression("[\\\\/:*?\"<>|]")) +
                       ".lnk";

            exePath = QCoreApplication::applicationFilePath().replace("\\", "/");

#else
            linkPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" +
                       QString::fromStdString(m_games[itemID].name)
                           .remove(QRegularExpression("[\\\\/:*?\"<>|]")) +
                       ".desktop";
#endif

            // Convert the icon to .ico if necessary
            if (iconFileInfo.suffix().toLower() == "png") {
                // Convert icon from PNG to ICO
                if (convertPngToIco(iconPath, icoPath)) {

#ifdef Q_OS_WIN
                    if (createShortcutWin(linkPath, ebootPath, icoPath, exePath)) {
#else
                    if (createShortcutLinux(linkPath, ebootPath, iconPath)) {
#endif
                        QMessageBox::information(
                            nullptr, "Shortcut creation",
                            QString("Shortcut created successfully!\n %1").arg(linkPath));
                    } else {
                        QMessageBox::critical(
                            nullptr, "Error",
                            QString("Error creating shortcut!\n %1").arg(linkPath));
                    }
                } else {
                    QMessageBox::critical(nullptr, "Error", "Failed to convert icon.");
                }
            } else {
                // If the icon is already in ICO format, we just create the shortcut
#ifdef Q_OS_WIN
                if (createShortcutWin(linkPath, ebootPath, iconPath, exePath)) {
#else
                if (createShortcutLinux(linkPath, ebootPath, iconPath)) {
#endif
                    QMessageBox::information(
                        nullptr, "Shortcut creation",
                        QString("Shortcut created successfully!\n %1").arg(linkPath));
                } else {
                    QMessageBox::critical(nullptr, "Error",
                                          QString("Error creating shortcut!\n %1").arg(linkPath));
                }
            }
        }

        // Handle the "Copy" actions
        if (selected == copyName) {
            QClipboard* clipboard = QGuiApplication::clipboard();
            clipboard->setText(QString::fromStdString(m_games[itemID].name));
        }

        if (selected == copySerial) {
            QClipboard* clipboard = QGuiApplication::clipboard();
            clipboard->setText(QString::fromStdString(m_games[itemID].serial));
        }

        if (selected == copyNameAll) {
            QClipboard* clipboard = QGuiApplication::clipboard();
            QString combinedText = QString("Name:%1 | Serial:%2 | Version:%3 | Size:%4")
                                       .arg(QString::fromStdString(m_games[itemID].name))
                                       .arg(QString::fromStdString(m_games[itemID].serial))
                                       .arg(QString::fromStdString(m_games[itemID].version))
                                       .arg(QString::fromStdString(m_games[itemID].size));
            clipboard->setText(combinedText);
        }
    }

    int GetRowIndex(QTreeWidget* treeWidget, QTreeWidgetItem* item) {
        int row = 0;
        for (int i = 0; i < treeWidget->topLevelItemCount(); i++) { // check top level/parent items
            QTreeWidgetItem* currentItem = treeWidget->topLevelItem(i);
            if (currentItem == item) {
                return row;
            }
            row++;

            if (currentItem->childCount() > 0) { // check child items
                for (int j = 0; j < currentItem->childCount(); j++) {
                    QTreeWidgetItem* childItem = currentItem->child(j);
                    if (childItem == item) {
                        return row;
                    }
                    row++;
                }
            }
        }
        return -1;
    }

    void RequestGameMenuPKGViewer(
        const QPoint& pos, QStringList m_pkg_app_list, QTreeWidget* treeWidget,
        std::function<void(std::filesystem::path, int, int)> InstallDragDropPkg) {
        QPoint global_pos = treeWidget->viewport()->mapToGlobal(pos); // context menu position
        QTreeWidgetItem* currentItem = treeWidget->currentItem();     // current clicked item
        int itemIndex = GetRowIndex(treeWidget, currentItem);         // row

        QMenu menu(treeWidget);
        QAction installPackage("Install PKG", treeWidget);

        menu.addAction(&installPackage);

        auto selected = menu.exec(global_pos);
        if (!selected) {
            return;
        }

        if (selected == &installPackage) {
            QStringList pkg_app_ = m_pkg_app_list[itemIndex].split(";;");
            std::filesystem::path path(pkg_app_[9].toStdString());
#ifdef _WIN32
            path = std::filesystem::path(pkg_app_[9].toStdWString());
#endif
            InstallDragDropPkg(path, 1, 1);
        }
    }

private:
    bool convertPngToIco(const QString& pngFilePath, const QString& icoFilePath) {
        // Load the PNG image
        QImage image(pngFilePath);
        if (image.isNull()) {
            return false;
        }

        // Scale the image to the default icon size (256x256 pixels)
        QImage scaledImage =
            image.scaled(QSize(256, 256), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // Convert the image to QPixmap
        QPixmap pixmap = QPixmap::fromImage(scaledImage);

        // Save the pixmap as an ICO file
        if (pixmap.save(icoFilePath, "ICO")) {
            return true;
        } else {
            return false;
        }
    }

#ifdef Q_OS_WIN
    bool createShortcutWin(const QString& linkPath, const QString& targetPath,
                           const QString& iconPath, const QString& exePath) {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        // Create the ShellLink object
        IShellLink* pShellLink = nullptr;
        HRESULT hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                        IID_IShellLink, (LPVOID*)&pShellLink);
        if (SUCCEEDED(hres)) {
            // Defines the path to the program executable
            pShellLink->SetPath((LPCWSTR)exePath.utf16());

            // Sets the home directory ("Start in")
            pShellLink->SetWorkingDirectory((LPCWSTR)QFileInfo(exePath).absolutePath().utf16());

            // Set arguments, eboot.bin file location
            QString arguments = QString("\"%1\"").arg(targetPath);
            pShellLink->SetArguments((LPCWSTR)arguments.utf16());

            // Set the icon for the shortcut
            pShellLink->SetIconLocation((LPCWSTR)iconPath.utf16(), 0);

            // Save the shortcut
            IPersistFile* pPersistFile = nullptr;
            hres = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);
            if (SUCCEEDED(hres)) {
                hres = pPersistFile->Save((LPCWSTR)linkPath.utf16(), TRUE);
                pPersistFile->Release();
            }
            pShellLink->Release();
        }

        CoUninitialize();

        return SUCCEEDED(hres);
    }
#else
    bool createShortcutLinux(const QString& linkPath, const QString& targetPath,
                             const QString& iconPath) {
        QFile shortcutFile(linkPath);
        if (!shortcutFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(nullptr, "Error",
                                  QString("Error creating shortcut!\n %1").arg(linkPath));
            return false;
        }

        QTextStream out(&shortcutFile);
        out << "[Desktop Entry]\n";
        out << "Version=1.0\n";
        out << "Name=" << QFileInfo(linkPath).baseName() << "\n";
        out << "Exec=" << QCoreApplication::applicationFilePath() << " \"" << targetPath << "\"\n";
        out << "Icon=" << iconPath << "\n";
        out << "Terminal=false\n";
        out << "Type=Application\n";
        shortcutFile.close();

        return true;
    }
#endif
};
