// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "sdl_audio.h"
#include "common/assert.h"
#include "core/libraries/error_codes.h"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>

#include <mutex>

namespace Audio {

int SDLAudio::AudioOutOpen(int type, u32 samples_num, u32 freq,
                           Libraries::AudioOut::OrbisAudioOutParamFormat format) {
    using Libraries::AudioOut::OrbisAudioOutParamFormat;
    std::unique_lock lock{m_mutex};

    for (int id = 0; id < portsOut.size(); ++id) {
        auto& port = portsOut[id];
        if (!port.isOpen) {
            port.isOpen = true;
            port.type = type;
            port.samples_num = samples_num;
            port.freq = freq;
            port.format = format;

            SDL_AudioFormat sampleFormat;
            switch (format) {
                case OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_MONO:
                case OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO:
                case OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_8CH:
                case OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_8CH_STD:
                    sampleFormat = SDL_AUDIO_S16;
                    port.sample_size = 2;
                    break;
                case OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_MONO:
                case OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_STEREO:
                case OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH:
                case OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH_STD:
                    sampleFormat = SDL_AUDIO_F32;
                    port.sample_size = 4;
                    break;
                default:
                    UNREACHABLE_MSG("Unknown format");
            }

            port.channels_num = (format == OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_MONO ||
                                 format == OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_MONO) ? 1 :
                                (format == OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO ||
                                 format == OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_STEREO) ? 2 : 8;

            for (int i = 0; i < port.channels_num; ++i) {
                port.volume[i] = Libraries::AudioOut::SCE_AUDIO_OUT_VOLUME_0DB;
            }

            SDL_AudioSpec fmt{};
            fmt.format = sampleFormat;
            fmt.channels = port.channels_num;
            fmt.freq = 48000;

            port.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &fmt, nullptr, nullptr);
            if (port.stream != nullptr) {
                SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(port.stream));
                return id + 1;
            }

            port.isOpen = false; // Cleanup if stream open fails
            return -1;
        }
    }

    return -1; // all ports are used
}

s32 SDLAudio::AudioOutOutput(s32 handle, const void* ptr) {
    std::shared_lock lock{m_mutex};
    auto& port = portsOut[handle - 1];

    if (!port.isOpen || ptr == nullptr) {
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT;
    }

    int result = SDL_PutAudioStreamData(port.stream, ptr, port.samples_num * port.sample_size * port.channels_num);

    // Wait until the buffer has sufficient space
    while (SDL_GetAudioStreamAvailable(port.stream) > 65536) {
        SDL_Delay(0);
    }

    return result;
}

bool SDLAudio::AudioOutSetVolume(s32 handle, s32 bitflag, s32* volume) {
    using Libraries::AudioOut::OrbisAudioOutParamFormat;
    std::shared_lock lock{m_mutex};
    auto& port = portsOut[handle - 1];

    if (!port.isOpen) {
        return false;
    }

    for (int i = 0; i < port.channels_num; ++i, bitflag >>= 1u) {
        if (bitflag & 0x1u) {
            int src_index = i;
            if (port.format == OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH_STD ||
                port.format == OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_8CH_STD) {
                switch (i) {
                    case 4: src_index = 6; break;
                    case 5: src_index = 7; break;
                    case 6: src_index = 4; break;
                    case 7: src_index = 5; break;
                    default: break;
                }
            }
            port.volume[i] = volume[src_index];
        }
    }

    return true;
}

bool SDLAudio::AudioOutGetStatus(s32 handle, int* type, int* channels_num) {
    std::shared_lock lock{m_mutex};
    auto& port = portsOut[handle - 1];

    if (!port.isOpen) {
        return false;
    }

    *type = port.type;
    *channels_num = port.channels_num;

    return true;
}

} // namespace Audio
