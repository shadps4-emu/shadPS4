// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <shared_mutex>
#include <SDL3/SDL_audio.h>
#include "core/libraries/audio/audioout.h"

namespace Audio {

class SDLAudio {
public:
    SDLAudio() = default;
    virtual ~SDLAudio() = default;

    s32 AudioOutOpen(int type, u32 samples_num, u32 freq,
                     Libraries::AudioOut::OrbisAudioOutParamFormat format);
    s32 AudioOutOutput(s32 handle, const void* ptr);
    s32 AudioOutSetVolume(s32 handle, s32 bitflag, s32* volume);
    s32 AudioOutGetStatus(s32 handle, int* type, int* channels_num);

private:
    struct PortOut {
        SDL_AudioStream* stream = nullptr;
        u32 samples_num = 0;
        u32 freq = 0;
        u32 format = -1;
        int type = 0;
        int channels_num = 0;
        int volume[8] = {};
        u8 sample_size = 0;
        bool isOpen = false;
    };
    std::shared_mutex m_mutex;
    std::array<PortOut, Libraries::AudioOut::SCE_AUDIO_OUT_NUM_PORTS> portsOut;
};

} // namespace Audio
