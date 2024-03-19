// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL.h>
#include "sdl_audio.h"
#include <common/assert.h>

namespace Audio {

int SDLAudio::AudioInit() {
    return SDL_InitSubSystem(SDL_INIT_AUDIO);
}

int SDLAudio::AudioOutOpen(int type, u32 samples_num, u32 freq,
                           Libraries::AudioOut::OrbisAudioOutParam format) {
    using Libraries::AudioOut::OrbisAudioOutParam;
    std::scoped_lock lock{m_mutex};
    for (int id = 0; id < portsOut.size(); id++) {
        auto& port = portsOut[id];
        if (!port.isOpen) {
            port.isOpen = true;
            port.type = type;
            port.samples_num = samples_num;
            port.freq = freq;
            port.format = format;

            switch (format) {
            case OrbisAudioOutParam::ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_MONO:
            case OrbisAudioOutParam::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_MONO:
                port.channels_num = 1;
                break;
            case OrbisAudioOutParam::ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO:
            case OrbisAudioOutParam::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_STEREO:
                port.channels_num = 2;
                break;
            case OrbisAudioOutParam::ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_8CH:
            case OrbisAudioOutParam::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH:
            case OrbisAudioOutParam::ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_8CH_STD:
            case OrbisAudioOutParam::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH_STD:
                port.channels_num = 8;
                break;
            default:
                UNREACHABLE_MSG("Unknown format");
            }

            for (int i = 0; i < port.channels_num; i++) {
                port.volume[i] = Libraries::AudioOut::SCE_AUDIO_OUT_VOLUME_0DB;
            }
            return id + 1;
        }
       
    }

    return -1; // all ports are used
}

} // namespace Audio
