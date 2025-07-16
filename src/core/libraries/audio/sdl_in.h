// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <mutex>
#include <SDL3/SDL.h>

namespace Libraries::AudioIn {
enum OrbisAudioInParam {
    ORBIS_AUDIO_IN_PARAM_FORMAT_S16_MONO = 0,
    ORBIS_AUDIO_IN_PARAM_FORMAT_S16_STEREO = 2
};
}

#define ORBIS_AUDIO_IN_ERROR_INVALID_PORT -1
#define ORBIS_AUDIO_IN_ERROR_TIMEOUT -2
#define ORBIS_AUDIO_IN_ERROR_STREAM_FAIL -3

class SDLAudioIn {
public:
    int AudioInit();
    int AudioInOpen(int type, uint32_t samples_num, uint32_t freq, uint32_t format);
    int AudioInInput(int handle, void* out_buffer);
    void AudioInClose(int handle);

private:
    struct AudioInPort {
        bool isOpen = false;
        int type = 0;
        uint32_t samples_num = 0;
        uint32_t freq = 0;
        int channels_num = 0;
        int sample_size = 0;
        uint32_t format = 0;
        SDL_AudioStream* stream = nullptr;
    };

    std::array<AudioInPort, 8> portsIn;
    std::mutex m_mutex;
};
