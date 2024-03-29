// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QHeaderView>
#include <QWidget>
#include "pkg_viewer.h"

PKGViewer::PKGViewer(std::shared_ptr<GameInfoClass> game_info_get,
                     std::shared_ptr<GuiSettings> m_gui_settings,
                     std::function<void(std::string, int, int)> InstallDragDropPkg)
    : QMainWindow() {
    this->resize(1280, 720);
    m_gui_settings_ = m_gui_settings;
    m_game_info = game_info_get;
    dir_list = m_gui_settings->GetValue(gui::m_pkg_viewer).toStringList();

    treeWidget = new QTreeWidget(this);
    treeWidget->setColumnCount(9);
    QStringList headers;
    headers << "Name"
            << "Serial"
            << "Size"
            << "Installed"
            << "Category"
            << "Type"
            << "App Ver"
            << "FW"
            << "Region"
            << "Flags"
            << "Path";
    treeWidget->setHeaderLabels(headers);
    treeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    treeWidget->setColumnWidth(8, 170);
    this->setCentralWidget(treeWidget);
    QMenuBar* menuBar = new QMenuBar(this);
    menuBar->setContextMenuPolicy(Qt::PreventContextMenu);
    QMenu* fileMenu = menuBar->addMenu(tr("&File"));
    QAction* openFolderAct = new QAction(tr("Open Folder"), this);
    fileMenu->addAction(openFolderAct);
    this->setMenuBar(menuBar);
    CheckPKGFolders(); // Check for new PKG files in existing folders.
    if (!m_pkg_list.isEmpty())
        ProcessPKGInfo();

    for (int column = 0; column < treeWidget->columnCount() - 2; ++column) {
        // Resize the column to fit its contents
        treeWidget->resizeColumnToContents(column);
    }

    connect(openFolderAct, &QAction::triggered, this, &PKGViewer::OpenPKGFolder);

    connect(treeWidget, &QTreeWidget::customContextMenuRequested, this,
            [=, this](const QPoint& pos) {
                m_gui_context_menus.RequestGameMenuPKGViewer(pos, m_full_pkg_list, treeWidget,
                                                             InstallDragDropPkg);
            });
}

PKGViewer::~PKGViewer() {}

void PKGViewer::OpenPKGFolder() {
    QString folderPath =
        QFileDialog::getExistingDirectory(this, tr("Open Folder"), QDir::homePath());
    if (!dir_list.contains(folderPath)) {

        dir_list.append(folderPath);
        if (!folderPath.isEmpty()) {
            QDir directory(folderPath);
            for (const auto& dir : std::filesystem::directory_iterator(folderPath.toStdString())) {
                if (std::filesystem::is_regular_file(dir.path())) {
                    m_pkg_list.append(QString::fromStdString(dir.path().string()));
                }
            }
            std::sort(m_pkg_list.begin(), m_pkg_list.end());
            ProcessPKGInfo();
            m_gui_settings_->SetValue(gui::m_pkg_viewer, dir_list);
            m_gui_settings_->SetValue(gui::m_pkg_viewer_pkg_list, m_pkg_list);
        }
    } else {
        // qDebug() << "Folder selection canceled.";
    }
}

void PKGViewer::CheckPKGFolders() { // Check for new PKG file additions.
    m_pkg_list.clear();
    for (const QString& paths : dir_list) {
        for (const auto& dir : std::filesystem::directory_iterator(paths.toStdString())) {
            if (std::filesystem::is_regular_file(dir.path())) {
                m_pkg_list.append(QString::fromStdString(dir.path().string()));
            }
        }
    }
    std::sort(m_pkg_list.begin(), m_pkg_list.end());
}

void PKGViewer::ProcessPKGInfo() {
    treeWidget->clear();
    map_strings.clear();
    for (int i = 0; i < m_pkg_list.size(); i++) {
        Common::FS::IOFile file(m_pkg_list[i].toStdString(), Common::FS::FileAccessMode::Read);
        if (!file.IsOpen()) {
            // return false;
        }

        file.ReadRaw<u8>(&pkgheader, sizeof(PKGHeader));
        file.Seek(0);
        pkgSize = file.GetSize();
        pkg.resize(pkgheader.pkg_promote_size);
        file.Read(pkg);

        u32 offset = pkgheader.pkg_table_entry_offset;
        u32 n_files = pkgheader.pkg_table_entry_count;

        for (int i = 0; i < n_files; i++) {
            std::memcpy(&entry, &pkg[offset + i * 0x20], sizeof(entry));
            const auto name = GetEntryNameByType(entry.id);
            if (name == "param.sfo") {
                psf.resize(entry.size);
                int seek = entry.offset;
                file.Seek(seek);
                file.Read(psf);
                std::memcpy(&header, psf.data(), sizeof(header));
                auto future = std::async(std::launch::async, [&]() {
                    for (u32 i = 0; i < header.index_table_entries; i++) {
                        PSFEntry psfentry;
                        std::memcpy(&psfentry, &psf[sizeof(PSFHeader) + i * sizeof(PSFEntry)],
                                    sizeof(psfentry));
                        const std::string key =
                            (char*)&psf[header.key_table_offset + psfentry.key_offset];
                        if (psfentry.param_fmt == PSFEntry::Fmt::TextRaw ||
                            psfentry.param_fmt == PSFEntry::Fmt::TextNormal) {
                            map_strings[key] =
                                (char*)&psf[header.data_table_offset + psfentry.data_offset];
                        }
                        if (psfentry.param_fmt == PSFEntry::Fmt::Integer) {
                            u32 value;
                            std::memcpy(&value,
                                        &psf[header.data_table_offset + psfentry.data_offset],
                                        sizeof(value));
                            map_integers[key] = value;
                        }
                    }
                });
                future.wait();
            }
        }
        QString title_name = GetString("TITLE");
        QString title_id = GetString("TITLE_ID");
        QString app_type = GetAppType(GetInteger("APP_TYPE"));
        QString app_version = GetString("APP_VER");
        QString title_category = GetString("CATEGORY");
        QString pkg_size = game_list_util.FormatSize(pkgheader.pkg_size);
        pkg_content_flag = pkgheader.pkg_content_flags;
        QString flagss = "";
        for (const auto& flag : flagNames) {
            if (isFlagSet(pkg_content_flag, flag.first)) {
                if (!flagss.isEmpty())
                    flagss.append(", ");
                flagss.append(flag.second);
            }
        }

        u32 fw_int = GetInteger("SYSTEM_VER");
        QString fw = QString::number(fw_int, 16);
        QString fw_ = fw.length() > 7 ? QString::number(fw_int, 16).left(3).insert(2, '.')
                                      : fw.left(3).insert(1, '.');
        fw_ = (fw_int == 0) ? "0.00" : fw_;
        char region = pkgheader.pkg_content_id[0];
        QString pkg_info = "";
        if (title_category == "gd") {
            title_category = "App";
            pkg_info = title_name + ";" + title_id + ";" + pkg_size + ";" + title_category + ";" +
                       app_type + ";" + app_version + ";" + fw_ + ";" + GetRegion(region) + ";" +
                       flagss + ";" + m_pkg_list[i];
            m_pkg_app_list.append(pkg_info);
        } else {
            title_category = "Patch";
            pkg_info = title_name + ";" + title_id + ";" + pkg_size + ";" + title_category + ";" +
                       app_type + ";" + app_version + ";" + fw_ + ";" + GetRegion(region) + ";" +
                       flagss + ";" + m_pkg_list[i];
            m_pkg_patch_list.append(pkg_info);
        }
    }
    std::sort(m_pkg_app_list.begin(), m_pkg_app_list.end());
    for (int i = 0; i < m_pkg_app_list.size(); i++) {
        QTreeWidgetItem* treeItem = new QTreeWidgetItem(treeWidget);
        QStringList pkg_app_ = m_pkg_app_list[i].split(";");
        m_full_pkg_list.append(m_pkg_app_list[i]);
        treeItem->setExpanded(true);
        treeItem->setText(0, pkg_app_[0]);
        treeItem->setText(1, pkg_app_[1]);
        treeItem->setText(3, pkg_app_[2]);
        treeItem->setTextAlignment(3, Qt::AlignCenter);
        treeItem->setText(4, pkg_app_[3]);
        treeItem->setTextAlignment(4, Qt::AlignCenter);
        treeItem->setText(5, pkg_app_[4]);
        treeItem->setTextAlignment(5, Qt::AlignCenter);
        treeItem->setText(6, pkg_app_[5]);
        treeItem->setTextAlignment(6, Qt::AlignCenter);
        treeItem->setText(7, pkg_app_[6]);
        treeItem->setTextAlignment(7, Qt::AlignCenter);
        treeItem->setText(8, pkg_app_[7]);
        treeItem->setTextAlignment(8, Qt::AlignCenter);
        treeItem->setText(9, pkg_app_[8]);
        treeItem->setText(10, pkg_app_[9]);
        for (const GameInfo& info : m_game_info->m_games) { // Check if game is installed.
            if (info.name == pkg_app_[0].toStdString()) {
                treeItem->setText(2, QChar(0x2713));
                treeItem->setTextAlignment(2, Qt::AlignCenter);
            }
        }
        for (const QString& item : m_pkg_patch_list) {
            QStringList pkg_patch_ = item.split(";");
            if (pkg_patch_[0] == pkg_app_[0]) { // check patches.
                m_full_pkg_list.append(item);
                QTreeWidgetItem* childItem = new QTreeWidgetItem(treeItem);
                childItem->setText(0, pkg_patch_[0]);
                childItem->setText(1, pkg_patch_[1]);
                childItem->setText(3, pkg_patch_[2]);
                childItem->setTextAlignment(3, Qt::AlignCenter);
                childItem->setText(4, pkg_patch_[3]);
                childItem->setTextAlignment(4, Qt::AlignCenter);
                childItem->setText(5, pkg_patch_[4]);
                childItem->setTextAlignment(5, Qt::AlignCenter);
                childItem->setText(6, pkg_patch_[5]);
                childItem->setTextAlignment(6, Qt::AlignCenter);
                childItem->setText(7, pkg_patch_[6]);
                childItem->setTextAlignment(7, Qt::AlignCenter);
                childItem->setText(8, pkg_patch_[7]);
                childItem->setTextAlignment(8, Qt::AlignCenter);
                childItem->setText(9, pkg_patch_[8]);
                childItem->setText(10, pkg_patch_[9]);
            }
        }
    }
    std::sort(m_full_pkg_list.begin(), m_full_pkg_list.end());
}

QString PKGViewer::GetString(const std::string& key) {
    if (map_strings.find(key) != map_strings.end()) {
        return QString::fromStdString(map_strings.at(key));
    }
    return "";
}

u32 PKGViewer::GetInteger(const std::string& key) {
    if (map_integers.find(key) != map_integers.end()) {
        return map_integers.at(key);
    }
    return 0;
}