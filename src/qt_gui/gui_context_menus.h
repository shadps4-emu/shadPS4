// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QClipboard>
#include <QDesktopServices>
#include <QMenu>
#include <QMessageBox>
#include <QTreeWidgetItem>

#include <qt_gui/background_music_player.h>
#include "cheats_patches.h"
#include "common/config.h"
#include "common/path_util.h"
#include "common/scm_rev.h"
#include "compatibility_info.h"
#include "game_info.h"
#include "gui_settings.h"
#include "trophy_viewer.h"

#ifdef Q_OS_WIN
#include <ShlObj.h>
#include <Windows.h>
#include <objbase.h>
#include <shlguid.h>
#include <shobjidl.h>
#include <wrl/client.h>
#endif

class GuiContextMenus : public QObject {
    Q_OBJECT
public:
    int RequestGameMenu(const QPoint& pos, QVector<GameInfo>& m_games,
                        std::shared_ptr<CompatibilityInfoClass> m_compat_info,
                        std::shared_ptr<gui_settings> settings, QTableWidget* widget, bool isList) {
        QPoint global_pos = widget->viewport()->mapToGlobal(pos);
        std::shared_ptr<gui_settings> m_gui_settings = std::move(settings);
        int itemID = 0;
        int changedFavorite = 0;
        if (isList) {
            itemID = widget->currentRow();
        } else {
            itemID = widget->currentRow() * widget->columnCount() + widget->currentColumn();
        }

        // Do not show the menu if no item is selected
        if (itemID < 0 || itemID >= m_games.size()) {
            return changedFavorite;
        }

        // Setup menu.
        QMenu menu(widget);

        // "Open Folder..." submenu
        QMenu* openFolderMenu = new QMenu(tr("Open Folder..."), widget);
        QAction* openGameFolder = new QAction(tr("Open Game Folder"), widget);
        QAction* openUpdateFolder = new QAction(tr("Open Update Folder"), widget);
        QAction* openSaveDataFolder = new QAction(tr("Open Save Data Folder"), widget);
        QAction* openLogFolder = new QAction(tr("Open Log Folder"), widget);

        openFolderMenu->addAction(openGameFolder);
        openFolderMenu->addAction(openUpdateFolder);
        openFolderMenu->addAction(openSaveDataFolder);
        openFolderMenu->addAction(openLogFolder);

        menu.addMenu(openFolderMenu);

        QString serialStr = QString::fromStdString(m_games[itemID].serial);
        QList<QString> list = gui_settings::Var2List(m_gui_settings->GetValue(gui::favorites_list));
        bool isFavorite = list.contains(serialStr);
        QAction* toggleFavorite;

        if (isFavorite) {
            toggleFavorite = new QAction(tr("Remove from Favorites"), widget);
        } else {
            toggleFavorite = new QAction(tr("Add to Favorites"), widget);
        }
        QAction createShortcut(tr("Create Shortcut"), widget);
        QAction openCheats(tr("Cheats / Patches"), widget);
        QAction openSfoViewer(tr("SFO Viewer"), widget);
        QAction openTrophyViewer(tr("Trophy Viewer"), widget);

        menu.addAction(toggleFavorite);
        menu.addAction(&createShortcut);
        menu.addAction(&openCheats);
        menu.addAction(&openSfoViewer);
        menu.addAction(&openTrophyViewer);

        // "Copy" submenu.
        QMenu* copyMenu = new QMenu(tr("Copy info..."), widget);
        QAction* copyName = new QAction(tr("Copy Name"), widget);
        QAction* copySerial = new QAction(tr("Copy Serial"), widget);
        QAction* copyVersion = new QAction(tr("Copy Version"), widget);
        QAction* copySize = new QAction(tr("Copy Size"), widget);
        QAction* copyNameAll = new QAction(tr("Copy All"), widget);

        copyMenu->addAction(copyName);
        copyMenu->addAction(copySerial);
        copyMenu->addAction(copyVersion);
        if (Config::GetLoadGameSizeEnabled()) {
            copyMenu->addAction(copySize);
        }
        copyMenu->addAction(copyNameAll);

        menu.addMenu(copyMenu);

        // "Delete..." submenu.
        QMenu* deleteMenu = new QMenu(tr("Delete..."), widget);
        QAction* deleteGame = new QAction(tr("Delete Game"), widget);
        QAction* deleteUpdate = new QAction(tr("Delete Update"), widget);
        QAction* deleteSaveData = new QAction(tr("Delete Save Data"), widget);
        QAction* deleteDLC = new QAction(tr("Delete DLC"), widget);
        QAction* deleteTrophy = new QAction(tr("Delete Trophy"), widget);

        deleteMenu->addAction(deleteGame);
        deleteMenu->addAction(deleteUpdate);
        deleteMenu->addAction(deleteSaveData);
        deleteMenu->addAction(deleteDLC);
        deleteMenu->addAction(deleteTrophy);

        menu.addMenu(deleteMenu);

        // Compatibility submenu.
        QMenu* compatibilityMenu = new QMenu(tr("Compatibility..."), widget);
        QAction* updateCompatibility = new QAction(tr("Update database"), widget);
        QAction* viewCompatibilityReport = new QAction(tr("View report"), widget);
        QAction* submitCompatibilityReport = new QAction(tr("Submit a report"), widget);

        compatibilityMenu->addAction(updateCompatibility);
        compatibilityMenu->addAction(viewCompatibilityReport);
        if (Common::g_is_release) {
            compatibilityMenu->addAction(submitCompatibilityReport);
        }

        menu.addMenu(compatibilityMenu);

        compatibilityMenu->setEnabled(Config::getCompatibilityEnabled());
        viewCompatibilityReport->setEnabled(m_games[itemID].compatibility.status !=
                                            CompatibilityStatus::Unknown);

        // Show menu.
        auto selected = menu.exec(global_pos);
        if (!selected) {
            return changedFavorite;
        }

        if (selected == openGameFolder) {
            QString folderPath;
            Common::FS::PathToQString(folderPath, m_games[itemID].path);
            QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
        }

        if (selected == openUpdateFolder) {
            QString open_update_path;
            Common::FS::PathToQString(open_update_path, m_games[itemID].path);
            open_update_path += "-UPDATE";
            if (std::filesystem::exists(Common::FS::PathFromQString(open_update_path))) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(open_update_path));
            } else {
                Common::FS::PathToQString(open_update_path, m_games[itemID].path);
                open_update_path += "-patch";
                if (std::filesystem::exists(Common::FS::PathFromQString(open_update_path))) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(open_update_path));
                } else {
                    QMessageBox::critical(nullptr, tr("Error"),
                                          QString(tr("This game has no update folder to open!")));
                }
            }
        }

        if (selected == openSaveDataFolder) {
            QString saveDataPath;
            Common::FS::PathToQString(saveDataPath,
                                      Config::GetSaveDataPath() / "1" / m_games[itemID].save_dir);
            QDir(saveDataPath).mkpath(saveDataPath);
            QDesktopServices::openUrl(QUrl::fromLocalFile(saveDataPath));
        }

        if (selected == openLogFolder) {
            QString logPath;
            Common::FS::PathToQString(logPath,
                                      Common::FS::GetUserPath(Common::FS::PathType::LogDir));
            if (!Config::getSeparateLogFilesEnabled()) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(logPath));
            } else {
                QString fileName = QString::fromStdString(m_games[itemID].serial) + ".log";
                QString filePath = logPath + "/" + fileName;
                QStringList arguments;
                if (QFile::exists(filePath)) {
#ifdef Q_OS_WIN
                    arguments << "/select," << filePath.replace("/", "\\");
                    QProcess::startDetached("explorer", arguments);

#elif defined(Q_OS_MAC)
                    arguments << "-R" << filePath;
                    QProcess::startDetached("open", arguments);

#elif defined(Q_OS_LINUX)
                    QStringList arguments;
                    arguments << "--select" << filePath;
                    if (!QProcess::startDetached("nautilus", arguments)) {
                        // Failed to open Nautilus to select file
                        arguments.clear();
                        arguments << logPath;
                        if (!QProcess::startDetached("xdg-open", arguments)) {
                            // Failed to open directory on Linux
                        }
                    }
#else
                    QDesktopServices::openUrl(QUrl::fromLocalFile(logPath));
#endif
                } else {
                    QMessageBox msgBox;
                    msgBox.setIcon(QMessageBox::Information);
                    msgBox.setText(tr("No log file found for this game!"));

                    QPushButton* okButton = msgBox.addButton(QMessageBox::Ok);
                    QPushButton* openFolderButton =
                        msgBox.addButton(tr("Open Log Folder"), QMessageBox::ActionRole);

                    msgBox.exec();

                    if (msgBox.clickedButton() == openFolderButton) {
                        QDesktopServices::openUrl(QUrl::fromLocalFile(logPath));
                    }
                }
            }
        }

        if (selected == &openSfoViewer) {
            PSF psf;
            QString gameName = QString::fromStdString(m_games[itemID].name);
            std::filesystem::path game_folder_path = m_games[itemID].path;
            std::filesystem::path game_update_path = game_folder_path;
            game_update_path += "-UPDATE";
            if (std::filesystem::exists(game_update_path)) {
                game_folder_path = game_update_path;
            } else {
                game_update_path = game_folder_path;
                game_update_path += "-patch";
                if (std::filesystem::exists(game_update_path)) {
                    game_folder_path = game_update_path;
                }
            }
            if (psf.Open(game_folder_path / "sce_sys" / "param.sfo")) {
                int rows = psf.GetEntries().size();
                QTableWidget* tableWidget = new QTableWidget(rows, 2);
                tableWidget->setAttribute(Qt::WA_DeleteOnClose);
                connect(widget->parent(), &QWidget::destroyed, tableWidget,
                        [tableWidget]() { tableWidget->deleteLater(); });

                tableWidget->verticalHeader()->setVisible(false); // Hide vertical header
                int row = 0;

                for (const auto& entry : psf.GetEntries()) {
                    QTableWidgetItem* keyItem =
                        new QTableWidgetItem(QString::fromStdString(entry.key));
                    QTableWidgetItem* valueItem;
                    switch (entry.param_fmt) {
                    case PSFEntryFmt::Binary: {
                        const auto bin = psf.GetBinary(entry.key);
                        if (!bin.has_value()) {
                            valueItem = new QTableWidgetItem(QString("Unknown"));
                        } else {
                            std::string text;
                            text.reserve(bin->size() * 2);
                            for (const auto& c : *bin) {
                                static constexpr char hex[] = "0123456789ABCDEF";
                                text.push_back(hex[c >> 4 & 0xF]);
                                text.push_back(hex[c & 0xF]);
                            }
                            valueItem = new QTableWidgetItem(QString::fromStdString(text));
                        }
                    } break;
                    case PSFEntryFmt::Text: {
                        auto text = psf.GetString(entry.key);
                        if (!text.has_value()) {
                            valueItem = new QTableWidgetItem(QString("Unknown"));
                        } else {
                            valueItem =
                                new QTableWidgetItem(QString::fromStdString(std::string{*text}));
                        }
                    } break;
                    case PSFEntryFmt::Integer: {
                        auto integer = psf.GetInteger(entry.key);
                        if (!integer.has_value()) {
                            valueItem = new QTableWidgetItem(QString("Unknown"));
                        } else {
                            valueItem =
                                new QTableWidgetItem(QString("0x") + QString::number(*integer, 16));
                        }
                    } break;
                    }

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
                tableWidget->setWindowTitle(tr("SFO Viewer for ") + gameName);
                tableWidget->show();
            }
        }

        if (selected == toggleFavorite) {
            if (isFavorite) {
                list.removeOne(serialStr);
            } else {
                list.append(serialStr);
            }
            m_gui_settings->SetValue(gui::favorites_list, gui_settings::List2Var(list));
            changedFavorite = 1;
        }

        if (selected == &openCheats) {
            QString gameName = QString::fromStdString(m_games[itemID].name);
            QString gameSerial = QString::fromStdString(m_games[itemID].serial);
            QString gameVersion = QString::fromStdString(m_games[itemID].version);
            QString gameSize = QString::fromStdString(m_games[itemID].size);
            QString iconPath;
            Common::FS::PathToQString(iconPath, m_games[itemID].icon_path);
            QPixmap gameImage(iconPath);
            CheatsPatches* cheatsPatches =
                new CheatsPatches(gameName, gameSerial, gameVersion, gameSize, gameImage);
            cheatsPatches->show();
            connect(widget->parent(), &QWidget::destroyed, cheatsPatches,
                    [cheatsPatches]() { cheatsPatches->deleteLater(); });
        }

        if (selected == &openTrophyViewer) {
            QString trophyPath, gameTrpPath;
            Common::FS::PathToQString(trophyPath, m_games[itemID].serial);
            Common::FS::PathToQString(gameTrpPath, m_games[itemID].path);
            auto game_update_path = Common::FS::PathFromQString(gameTrpPath);
            game_update_path += "-UPDATE";
            if (std::filesystem::exists(game_update_path)) {
                Common::FS::PathToQString(gameTrpPath, game_update_path);
            } else {
                game_update_path = Common::FS::PathFromQString(gameTrpPath);
                game_update_path += "-patch";
                if (std::filesystem::exists(game_update_path)) {
                    Common::FS::PathToQString(gameTrpPath, game_update_path);
                }
            }

            // Array with all games and their trophy information
            QVector<TrophyGameInfo> allTrophyGames;
            for (const auto& game : m_games) {
                TrophyGameInfo gameInfo;
                gameInfo.name = QString::fromStdString(game.name);
                Common::FS::PathToQString(gameInfo.trophyPath, game.serial);
                Common::FS::PathToQString(gameInfo.gameTrpPath, game.path);

                auto update_path = Common::FS::PathFromQString(gameInfo.gameTrpPath);
                update_path += "-UPDATE";
                if (std::filesystem::exists(update_path)) {
                    Common::FS::PathToQString(gameInfo.gameTrpPath, update_path);
                } else {
                    update_path = Common::FS::PathFromQString(gameInfo.gameTrpPath);
                    update_path += "-patch";
                    if (std::filesystem::exists(update_path)) {
                        Common::FS::PathToQString(gameInfo.gameTrpPath, update_path);
                    }
                }

                allTrophyGames.append(gameInfo);
            }

            QString gameName = QString::fromStdString(m_games[itemID].name);
            TrophyViewer* trophyViewer =
                new TrophyViewer(m_gui_settings, trophyPath, gameTrpPath, gameName, allTrophyGames);
            trophyViewer->show();
            connect(widget->parent(), &QWidget::destroyed, trophyViewer,
                    [trophyViewer]() { trophyViewer->deleteLater(); });
        }

        if (selected == &createShortcut) {
            QString targetPath;
            Common::FS::PathToQString(targetPath, m_games[itemID].path);
            QString ebootPath = targetPath + "/eboot.bin";

            // Get the full path to the icon
            QString iconPath;
            Common::FS::PathToQString(iconPath, m_games[itemID].icon_path);
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
                    if (createShortcutLinux(linkPath, m_games[itemID].name, ebootPath, iconPath)) {
#endif
                        QMessageBox::information(
                            nullptr, tr("Shortcut creation"),
                            QString(tr("Shortcut created successfully!") + "\n%1").arg(linkPath));
                    } else {
                        QMessageBox::critical(
                            nullptr, tr("Error"),
                            QString(tr("Error creating shortcut!") + "\n%1").arg(linkPath));
                    }
                } else {
                    QMessageBox::critical(nullptr, tr("Error"), tr("Failed to convert icon."));
                }
            } else {
                // If the icon is already in ICO format, we just create the shortcut
#ifdef Q_OS_WIN
                if (createShortcutWin(linkPath, ebootPath, iconPath, exePath)) {
#else
                if (createShortcutLinux(linkPath, m_games[itemID].name, ebootPath, iconPath)) {
#endif
                    QMessageBox::information(
                        nullptr, tr("Shortcut creation"),
                        QString(tr("Shortcut created successfully!") + "\n%1").arg(linkPath));
                } else {
                    QMessageBox::critical(
                        nullptr, tr("Error"),
                        QString(tr("Error creating shortcut!") + "\n%1").arg(linkPath));
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

        if (selected == copyVersion) {
            QClipboard* clipboard = QGuiApplication::clipboard();
            clipboard->setText(QString::fromStdString(m_games[itemID].version));
        }

        if (selected == copySize) {
            QClipboard* clipboard = QGuiApplication::clipboard();
            clipboard->setText(QString::fromStdString(m_games[itemID].size));
        }

        if (selected == copyNameAll) {
            QString GameSizeEnabled;
            if (Config::GetLoadGameSizeEnabled()) {
                GameSizeEnabled = " | Size:" + QString::fromStdString(m_games[itemID].size);
            }

            QClipboard* clipboard = QGuiApplication::clipboard();
            QString combinedText = QString("Name:%1 | Serial:%2 | Version:%3%4")
                                       .arg(QString::fromStdString(m_games[itemID].name))
                                       .arg(QString::fromStdString(m_games[itemID].serial))
                                       .arg(QString::fromStdString(m_games[itemID].version))
                                       .arg(GameSizeEnabled);

            clipboard->setText(combinedText);
        }

        if (selected == deleteGame || selected == deleteUpdate || selected == deleteDLC ||
            selected == deleteSaveData || selected == deleteTrophy) {
            bool error = false;
            QString folder_path, game_update_path, dlc_path, save_data_path, trophy_data_path;
            Common::FS::PathToQString(folder_path, m_games[itemID].path);
            game_update_path = folder_path + "-UPDATE";
            if (!std::filesystem::exists(Common::FS::PathFromQString(game_update_path))) {
                game_update_path = folder_path + "-patch";
            }
            Common::FS::PathToQString(
                dlc_path, Config::getAddonInstallDir() /
                              Common::FS::PathFromQString(folder_path).parent_path().filename());
            Common::FS::PathToQString(save_data_path,
                                      Config::GetSaveDataPath() / "1" / m_games[itemID].save_dir);

            Common::FS::PathToQString(trophy_data_path,
                                      Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) /
                                          m_games[itemID].serial / "TrophyFiles");

            QString message_type;

            if (selected == deleteGame) {
                BackgroundMusicPlayer::getInstance().stopMusic();
                message_type = tr("Game");
            } else if (selected == deleteUpdate) {
                if (!std::filesystem::exists(Common::FS::PathFromQString(game_update_path))) {
                    QMessageBox::critical(nullptr, tr("Error"),
                                          QString(tr("This game has no update to delete!")));
                    error = true;
                } else {
                    folder_path = game_update_path;
                    message_type = tr("Update");
                }
            } else if (selected == deleteDLC) {
                if (!std::filesystem::exists(Common::FS::PathFromQString(dlc_path))) {
                    QMessageBox::critical(nullptr, tr("Error"),
                                          QString(tr("This game has no DLC to delete!")));
                    error = true;
                } else {
                    folder_path = dlc_path;
                    message_type = tr("DLC");
                }
            } else if (selected == deleteSaveData) {
                if (!std::filesystem::exists(Common::FS::PathFromQString(save_data_path))) {
                    QMessageBox::critical(nullptr, tr("Error"),
                                          QString(tr("This game has no save data to delete!")));
                    error = true;
                } else {
                    folder_path = save_data_path;
                    message_type = tr("Save Data");
                }
            } else if (selected == deleteTrophy) {
                if (!std::filesystem::exists(Common::FS::PathFromQString(trophy_data_path))) {
                    QMessageBox::critical(
                        nullptr, tr("Error"),
                        QString(tr("This game has no saved trophies to delete!")));
                    error = true;
                } else {
                    folder_path = trophy_data_path;
                    message_type = tr("Trophy");
                }
            }
            if (!error) {
                QString gameName = QString::fromStdString(m_games[itemID].name);
                QDir dir(folder_path);
                QMessageBox::StandardButton reply = QMessageBox::question(
                    nullptr, QString(tr("Delete %1")).arg(message_type),
                    QString(tr("Are you sure you want to delete %1's %2 directory?"))
                        .arg(gameName, message_type),
                    QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    dir.removeRecursively();
                    if (selected == deleteGame) {
                        widget->removeRow(itemID);
                        m_games.removeAt(itemID);
                    }
                }
            }
        }

        if (selected == updateCompatibility) {
            m_compat_info->UpdateCompatibilityDatabase(widget, true);
        }

        if (selected == viewCompatibilityReport) {
            if (m_games[itemID].compatibility.issue_number != "") {
                auto url_issues =
                    "https://github.com/shadps4-compatibility/shadps4-game-compatibility/issues/";
                QDesktopServices::openUrl(
                    QUrl(url_issues + m_games[itemID].compatibility.issue_number));
            }
        }

        if (selected == submitCompatibilityReport) {
            if (m_games[itemID].compatibility.issue_number == "") {
                QUrl url = QUrl("https://github.com/shadps4-compatibility/"
                                "shadps4-game-compatibility/issues/new");
                QUrlQuery query;
                query.addQueryItem("template", QString("game_compatibility.yml"));
                query.addQueryItem(
                    "title", QString("%1 - %2").arg(QString::fromStdString(m_games[itemID].serial),
                                                    QString::fromStdString(m_games[itemID].name)));
                query.addQueryItem("game-name", QString::fromStdString(m_games[itemID].name));
                query.addQueryItem("game-serial", QString::fromStdString(m_games[itemID].serial));
                query.addQueryItem("game-version", QString::fromStdString(m_games[itemID].version));
                query.addQueryItem("emulator-version", QString(Common::g_version));
                url.setQuery(query);

                QDesktopServices::openUrl(url);
            } else {
                auto url_issues =
                    "https://github.com/shadps4-compatibility/shadps4-game-compatibility/issues/";
                QDesktopServices::openUrl(
                    QUrl(url_issues + m_games[itemID].compatibility.issue_number));
            }
        }
        return changedFavorite;
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
        Microsoft::WRL::ComPtr<IShellLink> pShellLink;
        HRESULT hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                        IID_PPV_ARGS(&pShellLink));
        if (SUCCEEDED(hres)) {
            // Defines the path to the program executable
            pShellLink->SetPath((LPCWSTR)exePath.utf16());

            // Sets the home directory ("Start in")
            pShellLink->SetWorkingDirectory((LPCWSTR)QFileInfo(exePath).absolutePath().utf16());

            // Set arguments, eboot.bin file location
            QString arguments = QString("-g \"%1\"").arg(targetPath);
            pShellLink->SetArguments((LPCWSTR)arguments.utf16());

            // Set the icon for the shortcut
            pShellLink->SetIconLocation((LPCWSTR)iconPath.utf16(), 0);

            // Save the shortcut
            Microsoft::WRL::ComPtr<IPersistFile> pPersistFile;
            hres = pShellLink.As(&pPersistFile);
            if (SUCCEEDED(hres)) {
                hres = pPersistFile->Save((LPCWSTR)linkPath.utf16(), TRUE);
            }
        }

        CoUninitialize();

        return SUCCEEDED(hres);
    }
#else
    bool createShortcutLinux(const QString& linkPath, const std::string& name,
                             const QString& targetPath, const QString& iconPath) {
        QFile shortcutFile(linkPath);
        if (!shortcutFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(nullptr, "Error",
                                  QString("Error creating shortcut!\n %1").arg(linkPath));
            return false;
        }

        QTextStream out(&shortcutFile);
        out << "[Desktop Entry]\n";
        out << "Version=1.0\n";
        out << "Name=" << QString::fromStdString(name) << "\n";
        out << "Exec=" << QCoreApplication::applicationFilePath() << " \"" << targetPath << "\"\n";
        out << "Icon=" << iconPath << "\n";
        out << "Terminal=false\n";
        out << "Type=Application\n";
        shortcutFile.close();

        return true;
    }
#endif
};
