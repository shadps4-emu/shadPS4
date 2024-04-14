// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <SDL.h>

#include "core/libraries/audio/audioout.h"

namespace Audio {

class SDLAudio {
public:
    SDLAudio() = default;
    virtual ~SDLAudio() = default;

    int AudioInit();
    int AudioOutOpen(int type, u32 samples_num, u32 freq,
                     Libraries::AudioOut::OrbisAudioOutParam format);
    s32 AudioOutOutput(s32 handle, const void* ptr);
    bool AudioOutSetVolume(s32 handle, s32 bitflag, s32* volume);

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
    std::mutex m_mutex;
    std::array<PortOut, 22> portsOut; // main up to 8 ports , BGM 1 port , voice up to 4 ports ,
                                      // personal up to 4 ports , padspk up to 5 ports , aux 1 port
};

} // namespace Audio
