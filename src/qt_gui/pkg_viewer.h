// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <QFileDialog>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QtConcurrent/QtConcurrent>
#include "common/io_file.h"
#include "core/file_format/pkg.h"
#include "core/file_format/pkg_type.h"
#include "core/file_format/psf.h"
#include "game_info.h"
#include "game_list_utils.h"
#include "gui_context_menus.h"
#include "gui_settings.h"

class PKGViewer : public QMainWindow {
    Q_OBJECT
public:
    explicit PKGViewer(std::shared_ptr<GameInfoClass> game_info_get,
                       std::shared_ptr<GuiSettings> m_gui_settings,
                       std::function<void(std::string, int, int)> InstallDragDropPkg = nullptr);
    ~PKGViewer();
    void OpenPKGFolder();
    void CheckPKGFolders();
    void ProcessPKGInfo();
    QString GetString(const std::string& key);
    u32 GetInteger(const std::string& key);

private:
    GuiContextMenus m_gui_context_menus;
    PSF psf_;
    PKGHeader pkgheader;
    PKGEntry entry;
    PSFHeader header;
    PSFEntry psfentry;
    char pkgTitleID[9];
    std::vector<u8> pkg;
    std::vector<u8> psf;
    u64 pkgSize = 0;
    std::shared_ptr<GuiSettings> m_gui_settings_;
    std::unordered_map<std::string, std::string> map_strings;
    std::unordered_map<std::string, u32> map_integers;

    u32_be pkg_content_flag;
    std::shared_ptr<GameInfoClass> m_game_info;
    GameListUtils game_list_util;
    // Status bar
    QStatusBar* statusBar;

    std::vector<std::pair<PKGContentFlag, std::string>> flagNames = {
        {PKGContentFlag::FIRST_PATCH, "FIRST_PATCH"},
        {PKGContentFlag::PATCHGO, "PATCHGO"},
        {PKGContentFlag::REMASTER, "REMASTER"},
        {PKGContentFlag::PS_CLOUD, "PS_CLOUD"},
        {PKGContentFlag::GD_AC, "GD_AC"},
        {PKGContentFlag::NON_GAME, "NON_GAME"},
        {PKGContentFlag::UNKNOWN_0x8000000, "UNKNOWN_0x8000000"},
        {PKGContentFlag::SUBSEQUENT_PATCH, "SUBSEQUENT_PATCH"},
        {PKGContentFlag::DELTA_PATCH, "DELTA_PATCH"},
        {PKGContentFlag::CUMULATIVE_PATCH, "CUMULATIVE_PATCH"}};

    std::vector<std::pair<int, QString>> appTypes = {
        {0, "FULL APP"},
        {1, "UPGRADABLE"},
        {2, "DEMO"},
        {3, "FREEMIUM"},
    };

    bool isFlagSet(u32_be variable, PKGContentFlag flag) {
        return (variable) & static_cast<u32>(flag);
    }

    QString GetRegion(char region) {
        switch (region) {
        case 'U':
            return "USA";
        case 'E':
            return "Europe";
        case 'J':
            return "Japan";
        case 'H':
            return "Asia";
        case 'I':
            return "World";
        default:
            return "Unknown";
        }
    }

    QString GetAppType(int region) {
        switch (region) {
        case 0:
            return "Not Specified";
        case 1:
            return "FULL APP";
        case 2:
            return "UPGRADABLE";
        case 3:
            return "DEMO";
        case 4:
            return "FREEMIUM";
        default:
            return "Unknown";
        }
    }
    QStringList m_full_pkg_list;
    QStringList m_pkg_app_list;
    QStringList m_pkg_patch_list;
    QStringList m_pkg_list;
    QStringList dir_list;
    QTreeWidget* treeWidget = nullptr;
};