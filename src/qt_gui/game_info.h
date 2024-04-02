// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <QFuture>
#include <QPixmap>
#include <QtConcurrent/QtConcurrent>
#include "core/file_format/psf.h"
#include "game_list_utils.h"
#include "gui_settings.h"

struct GameInfo {
    std::string path; // root path of game directory (normaly directory that contains eboot.bin)
    std::string icon_path; // path of icon0.png
    std::string pic_path;  // path of pic1.png
    QPixmap icon;
    std::string size;
    // variables extracted from param.sfo
    std::string name = "Unknown";
    std::string serial = "Unknown";
    std::string version = "Unknown";
    std::string category = "Unknown";
    std::string fw = "Unknown";
};

class GameInfoClass {

public:
    void GetGameInfo();

    std::shared_ptr<GuiSettings> m_gui_settings = std::make_shared<GuiSettings>();
    QVector<GameInfo> m_games;

    static bool CompareStrings(GameInfo& a, GameInfo& b) {
        return a.name < b.name;
    }

    static GameInfo readGameInfo(const std::string& filePath) {
        GameInfo game;
        GameListUtils game_util;
        game.size = game_util.GetFolderSize(QDir(QString::fromStdString(filePath))).toStdString();
        game.path = filePath;

        PSF psf;
        if (psf.open(game.path + "/sce_sys/param.sfo")) {
            QString iconpath(QString::fromStdString(game.path) + "/sce_sys/icon0.png");
            QString picpath(QString::fromStdString(game.path) + "/sce_sys/pic1.png");
            game.icon_path = iconpath.toStdString();
            game.icon = QPixmap(iconpath);
            game.pic_path = picpath.toStdString();
            game.name = psf.GetString("TITLE");
            game.serial = psf.GetString("TITLE_ID");
            u32 fw_int = psf.GetInteger("SYSTEM_VER");
            QString fw = QString::number(fw_int, 16);
            QString fw_ = fw.length() > 7 ? QString::number(fw_int, 16).left(3).insert(2, '.')
                                          : fw.left(3).insert(1, '.');
            game.fw = (fw_int == 0) ? "0.00" : fw_.toStdString();
            game.version = psf.GetString("APP_VER");
            game.category = psf.GetString("CATEGORY");
        }
        return game;
    }
};