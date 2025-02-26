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
    QVector<GameInfo> m_games_backup;

    static bool CompareStrings(GameInfo& a, GameInfo& b) {
        std::string name_a = a.name, name_b = b.name;
        std::transform(name_a.begin(), name_a.end(), name_a.begin(), ::tolower);
        std::transform(name_b.begin(), name_b.end(), name_b.begin(), ::tolower);
        return name_a < name_b;
    }

    static GameInfo readGameInfo(const std::filesystem::path& filePath) {
        GameInfo game;
        game.path = filePath;
        std::filesystem::path sce_folder_path = filePath / "sce_sys" / "param.sfo";
        std::filesystem::path game_update_path = filePath;
        game_update_path += "-UPDATE";
        if (std::filesystem::exists(game_update_path / "sce_sys" / "param.sfo")) {
            sce_folder_path = game_update_path / "sce_sys" / "param.sfo";
        }

        PSF psf;
        if (psf.Open(sce_folder_path)) {
            game.icon_path = game.path / "sce_sys" / "icon0.png";
            QString iconpath;
            Common::FS::PathToQString(iconpath, game.icon_path);
            game.icon = QImage(iconpath);
            game.pic_path = game.path / "sce_sys" / "pic1.png";
            game.snd0_path = game.path / "sce_sys" / "snd0.at9";

            if (const auto title = psf.GetString("TITLE"); title.has_value()) {
                game.name = *title;
            }
            if (const auto title_id = psf.GetString("TITLE_ID"); title_id.has_value()) {
                game.serial = *title_id;
            }
            if (const auto content_id = psf.GetString("CONTENT_ID");
                content_id.has_value() && !content_id->empty()) {
                game.region = GameListUtils::GetRegion(content_id->at(0)).toStdString();
            }
            if (const auto fw_int_opt = psf.GetInteger("SYSTEM_VER"); fw_int_opt.has_value()) {
                auto fw_int = *fw_int_opt;
                if (fw_int == 0) {
                    game.fw = "0.00";
                } else {
                    QString fw = QString::number(fw_int, 16);
                    QString fw_ = fw.length() > 7
                                      ? QString::number(fw_int, 16).left(3).insert(2, '.')
                                      : fw.left(3).insert(1, '.');
                    game.fw = fw_.toStdString();
                }
            }
            if (auto app_ver = psf.GetString("APP_VER"); app_ver.has_value()) {
                game.version = *app_ver;
            }
            if (const auto play_time = psf.GetString("PLAY_TIME"); play_time.has_value()) {
                game.play_time = *play_time;
            }
        }
        return game;
    }
};
