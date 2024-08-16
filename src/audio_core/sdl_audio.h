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

    int AudioOutOpen(int type, u32 samples_num, u32 freq,
                     Libraries::AudioOut::OrbisAudioOutParamFormat format);
    s32 AudioOutOutput(s32 handle, const void* ptr);
    bool AudioOutSetVolume(s32 handle, s32 bitflag, s32* volume);
    bool AudioOutGetStatus(s32 handle, int* type, int* channels_num);

private:
    struct PortOut {
        bool isOpen = false;
        int type = 0;
        u32 samples_num = 0;
        u8 sample_size = 0;
        u32 freq = 0;
        u32 format = -1;
        int channels_num = 0;
        int volume[8] = {};
        SDL_AudioStream* stream = nullptr;
    };
    std::shared_mutex m_mutex;
    std::array<PortOut, 22> portsOut; // main up to 8 ports , BGM 1 port , voice up to 4 ports ,
                                      // personal up to 4 ports , padspk up to 5 ports , aux 1 port
};

} // namespace Audio
