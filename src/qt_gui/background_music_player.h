// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>

class BackgroundMusicPlayer : public QObject {
    Q_OBJECT

public:
    static BackgroundMusicPlayer& getInstance() {
        static BackgroundMusicPlayer instance;
        return instance;
    }

    void playMusic(const QString& snd0path);
    void stopMusic();


private:
    BackgroundMusicPlayer(QObject* parent = nullptr);
    ~BackgroundMusicPlayer();

    QMediaPlayer* m_mediaPlayer;
    QAudioOutput* m_audioOutput;
    QUrl m_currentMusic;
};