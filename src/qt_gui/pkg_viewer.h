// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QFileDialog>
#include <QMenuBar>
#include <QStatusBar>

#include "common/io_file.h"
#include "core/file_format/pkg.h"
#include "core/file_format/pkg_type.h"
#include "core/file_format/psf.h"
#include "game_info.h"
#include "game_list_utils.h"
#include "gui_context_menus.h"

class PKGViewer : public QMainWindow {
    Q_OBJECT
public:
    explicit PKGViewer(
        std::shared_ptr<GameInfoClass> game_info_get, QWidget* parent,
        std::function<void(std::filesystem::path, int, int)> InstallDragDropPkg = nullptr);
    ~PKGViewer();
    void OpenPKGFolder();
    void CheckPKGFolders();
    void ProcessPKGInfo();

private:
    GuiContextMenus m_gui_context_menus;
    PKG package;
    PSF psf;
    PKGHeader pkgheader;
    PKGEntry entry;
    PSFHeader header;
    char pkgTitleID[9];
    std::vector<u8> pkg;
    u64 pkgSize = 0;
    std::unordered_map<std::string, std::string> map_strings;
    std::unordered_map<std::string, u32> map_integers;

    u32_be pkg_content_flag;
    std::shared_ptr<GameInfoClass> m_game_info;
    GameListUtils game_list_util;
    // Status bar
    QStatusBar* statusBar;

    std::vector<std::pair<int, QString>> appTypes = {
        {0, "FULL APP"},
        {1, "UPGRADABLE"},
        {2, "DEMO"},
        {3, "FREEMIUM"},
    };

    QStringList m_full_pkg_list;
    QStringList m_pkg_app_list;
    QStringList m_pkg_patch_list;
    QStringList m_pkg_list;
    QStringList dir_list;
    std::vector<std::string> dir_list_std;
    QTreeWidget* treeWidget = nullptr;
};