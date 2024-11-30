// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <shared_mutex>
#include <SDL3/SDL_audio.h>
#include "core/libraries/audio/audioout.h"

namespace Libraries::AudioOut {

class SDLAudioOut {
public:
    explicit SDLAudioOut() = default;
    ~SDLAudioOut() = default;

    s32 Open(OrbisAudioOutPort type, u32 samples_num, u32 freq, OrbisAudioOutParamFormat format);
    s32 Output(s32 handle, const void* ptr);
    s32 SetVolume(s32 handle, s32 bitflag, s32* volume);

    constexpr std::pair<OrbisAudioOutPort, int> GetStatus(s32 handle) const {
        const auto& port = ports_out.at(handle - 1);
        return std::make_pair(port.type, port.channels_num);
    }

private:
    struct PortOut {
        SDL_AudioStream* stream;
        u32 samples_num;
        u32 freq;
        OrbisAudioOutParamFormat format;
        OrbisAudioOutPort type;
        int channels_num;
        std::array<int, 8> volume;
        u8 sample_size;
        bool is_open;
    };
    std::shared_mutex m_mutex;
    std::array<PortOut, Libraries::AudioOut::SCE_AUDIO_OUT_NUM_PORTS> ports_out{};
};

} // namespace Libraries::AudioOut
