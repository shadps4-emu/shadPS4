// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QProgressDialog>

#include "game_info.h"

GameInfoClass::GameInfoClass() = default;
GameInfoClass::~GameInfoClass() = default;

void GameInfoClass::GetGameInfo(QWidget* parent) {
    QString installDir = QString::fromStdString(Config::getGameInstallDir());
    QStringList filePaths;
    QDir parentFolder(installDir);
    QFileInfoList fileList = parentFolder.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto& fileInfo : fileList) {
        if (fileInfo.isDir()) {
            filePaths.append(fileInfo.absoluteFilePath());
        }
    }
    m_games = QtConcurrent::mapped(filePaths, [&](const QString& path) {
                  return readGameInfo(path.toStdString());
              }).results();

    // Progress bar, please be patient :)
    QProgressDialog dialog("Loading game list, please wait :3", "Cancel", 0, 0, parent);
    dialog.setWindowTitle("Loading...");

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
