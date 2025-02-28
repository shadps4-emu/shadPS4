// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "background_music_player.h"

BackgroundMusicPlayer::BackgroundMusicPlayer(QObject* parent) : QObject(parent) {
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
}

void BackgroundMusicPlayer::setVolume(int volume) {
    float linearVolume = QAudio::convertVolume(volume / 100.0f, QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);
    m_audioOutput->setVolume(linearVolume);
}

void BackgroundMusicPlayer::playMusic(const QString& snd0path, bool loops) {
    if (snd0path.isEmpty()) {
        stopMusic();
        return;
    }
    const auto newMusic = QUrl::fromLocalFile(snd0path);
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState &&
        m_currentMusic == newMusic) {
        // already playing the correct music
        return;
    }

    if (loops) {
        m_mediaPlayer->setLoops(QMediaPlayer::Infinite);
    } else {
        m_mediaPlayer->setLoops(1);
    }

    m_currentMusic = newMusic;
    m_mediaPlayer->setSource(newMusic);
    m_mediaPlayer->play();
}

void BackgroundMusicPlayer::stopMusic() {
    m_mediaPlayer->stop();
    m_mediaPlayer->setSource(QUrl(""));
}
