// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QFutureWatcher>
#include <QtConcurrent>

#include "common/config.h"
#include "core/file_format/psf.h"
#include "game_list_utils.h"

class GameInfoClass : public QObject {
    Q_OBJECT
public:
    GameInfoClass();
    ~GameInfoClass();
    void GetGameInfo(QWidget* parent = nullptr);
    QVector<GameInfo> m_games;

    static bool CompareStrings(GameInfo& a, GameInfo& b) {
        return a.name < b.name;
    }

    static GameInfo readGameInfo(const std::string& filePath) {
        GameInfo game;
        game.path = filePath;

        PSF psf;
        if (psf.Open(std::filesystem::path(game.path) / "sce_sys" / "param.sfo")) {
            game.icon_path = game.path + "/sce_sys/icon0.png";
            QString iconpath = QString::fromStdString(game.icon_path);
            game.icon = QImage(iconpath);
            game.pic_path = game.path + "/sce_sys/pic1.png";
            game.name = *psf.GetString("TITLE");
            game.serial = *psf.GetString("TITLE_ID");
            game.region =
                GameListUtils::GetRegion(psf.GetString("CONTENT_ID")->at(0)).toStdString();
            u32 fw_int = *psf.GetInteger("SYSTEM_VER");
            QString fw = QString::number(fw_int, 16);
            QString fw_ = fw.length() > 7 ? QString::number(fw_int, 16).left(3).insert(2, '.')
                                          : fw.left(3).insert(1, '.');
            game.fw = (fw_int == 0) ? "0.00" : fw_.toStdString();
            game.version = *psf.GetString("APP_VER");
        }
        return game;
    }
};