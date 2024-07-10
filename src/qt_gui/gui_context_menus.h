// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QCoreApplication>
#include <QDesktopServices>
#include <QFile>
#include <QHeaderView>
#include <QImage>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTextStream>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "game_info.h"
#include "trophy_viewer.h"

#ifdef Q_OS_WIN
#include <ShlObj.h>
#include <Windows.h>
#include <objbase.h>
#include <shlguid.h>
#include <shobjidl.h>
#endif

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
        QAction openSfoViewer("SFO Viewer", widget);
        QAction openTrophyViewer("Trophy Viewer", widget);

        menu.addAction(&openFolder);
        menu.addAction(&createShortcut);
        menu.addAction(&openSfoViewer);
        menu.addAction(&openTrophyViewer);

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
                            nullptr, "Shortcut Creation",
                            QString("Shortcut created successfully:\n %1").arg(linkPath));
                    } else {
                        QMessageBox::critical(
                            nullptr, "Error",
                            QString("Error creating shortcut:\n %1").arg(linkPath));
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
                        nullptr, "Shortcut Creation",
                        QString("Shortcut created successfully:\n %1").arg(linkPath));
                } else {
                    QMessageBox::critical(nullptr, "Error",
                                          QString("Error creating shortcut:\n %1").arg(linkPath));
                }
            }
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
                                  QString("Error creating shortcut:\n %1").arg(linkPath));
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
