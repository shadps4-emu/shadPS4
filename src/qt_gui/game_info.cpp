// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QProgressDialog>

#include "common/path_util.h"
#include "compatibility_info.h"
#include "game_info.h"

// Maximum depth to search for games in subdirectories
const int max_recursion_depth = 5;

void ScanDirectoryRecursively(const QString& dir, QStringList& filePaths, int current_depth = 0) {
    // Stop recursion if we've reached the maximum depth
    if (current_depth >= max_recursion_depth) {
        return;
    }

    QDir directory(dir);
    QFileInfoList entries = directory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const auto& entry : entries) {
        if (entry.fileName().endsWith("-UPDATE")) {
            continue;
        }

        // Check if this directory contains a PS4 game (has sce_sys/param.sfo)
        if (QFile::exists(entry.filePath() + "/sce_sys/param.sfo")) {
            filePaths.append(entry.absoluteFilePath());
        } else {
            // If not a game directory, recursively scan it with increased depth
            ScanDirectoryRecursively(entry.absoluteFilePath(), filePaths, current_depth + 1);
        }
    }
}

GameInfoClass::GameInfoClass() = default;
GameInfoClass::~GameInfoClass() = default;

void GameInfoClass::GetGameInfo(QWidget* parent) {
    QStringList filePaths;
    for (const auto& installLoc : Config::getGameInstallDirs()) {
        QString installDir;
        Common::FS::PathToQString(installDir, installLoc);
        ScanDirectoryRecursively(installDir, filePaths, 0);
    }

    m_games = QtConcurrent::mapped(filePaths, [&](const QString& path) {
                  return readGameInfo(Common::FS::PathFromQString(path));
              }).results();

    // Progress bar, please be patient :)
    QProgressDialog dialog(tr("Loading game list, please wait :3"), tr("Cancel"), 0, 0, parent);
    dialog.setWindowTitle(tr("Loading..."));

    QFutureWatcher<void> futureWatcher;
    GameListUtils game_util;
    bool finished = false;
    futureWatcher.setFuture(QtConcurrent::map(m_games, game_util.GetFolderSize));
    connect(&futureWatcher, &QFutureWatcher<void>::finished, [&]() {
        dialog.reset();
        std::sort(m_games.begin(), m_games.end(), CompareStrings);
    });
    connect(&dialog, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
    dialog.setRange(0, m_games.size());
    connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged, &dialog,
            &QProgressDialog::setValue);

    dialog.exec();
}
