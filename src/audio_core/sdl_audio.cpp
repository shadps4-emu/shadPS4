// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "sdl_audio.h"

#include "common/assert.h"
#include "core/libraries/error_codes.h"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>

#include <mutex> // std::unique_lock

namespace Audio {

constexpr int AUDIO_STREAM_BUFFER_THRESHOLD = 65536; // Define constant for buffer threshold

s32 SDLAudio::AudioOutOpen(int type, u32 samples_num, u32 freq,
                           Libraries::AudioOut::OrbisAudioOutParamFormat format) {
    using Libraries::AudioOut::OrbisAudioOutParamFormat;
    std::unique_lock lock{m_mutex};
    for (int id = 0; id < portsOut.size(); id++) {
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
                sampleFormat = SDL_AUDIO_S16;
                port.channels_num = 1;
                port.sample_size = 2;
                break;
            case OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_MONO:
                sampleFormat = SDL_AUDIO_F32;
                port.channels_num = 1;
                port.sample_size = 4;
                break;
            case OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO:
                sampleFormat = SDL_AUDIO_S16;
                port.channels_num = 2;
                port.sample_size = 2;
                break;
            case OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_STEREO:
                sampleFormat = SDL_AUDIO_F32;
                port.channels_num = 2;
                port.sample_size = 4;
                break;
            case OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_8CH:
                sampleFormat = SDL_AUDIO_S16;
                port.channels_num = 8;
                port.sample_size = 2;
                break;
            case OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH:
                sampleFormat = SDL_AUDIO_F32;
                port.channels_num = 8;
                port.sample_size = 4;
                break;
            case OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_8CH_STD:
                sampleFormat = SDL_AUDIO_S16;
                port.channels_num = 8;
                port.sample_size = 2;
                break;
            case OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH_STD:
                sampleFormat = SDL_AUDIO_F32;
                port.channels_num = 8;
                port.sample_size = 4;
                break;
            default:
                UNREACHABLE_MSG("Unknown format");
            }

            for (int i = 0; i < port.channels_num; i++) {
                port.volume[i] = Libraries::AudioOut::SCE_AUDIO_OUT_VOLUME_0DB;
            }

            SDL_AudioSpec fmt;
            SDL_zero(fmt);
            fmt.format = sampleFormat;
            fmt.channels = port.channels_num;
            fmt.freq = freq; // Set frequency from the argument
            port.stream =
                SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &fmt, NULL, NULL);
            SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(port.stream));
            return id + 1;
        }
    }

    LOG_ERROR(Lib_AudioOut, "Audio ports are full");
    return ORBIS_AUDIO_OUT_ERROR_PORT_FULL; // all ports are used
}

s32 SDLAudio::AudioOutOutput(s32 handle, const void* ptr) {
    std::shared_lock lock{m_mutex};
    auto& port = portsOut[handle - 1];
    if (!port.isOpen) {
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT;
    }

    const size_t data_size = port.samples_num * port.sample_size * port.channels_num;

    SDL_bool result = SDL_PutAudioStreamData(port.stream, ptr, data_size);

    lock.unlock(); // Unlock only after necessary operations

    while (SDL_GetAudioStreamAvailable(port.stream) > AUDIO_STREAM_BUFFER_THRESHOLD) {
        SDL_Delay(0);
    }

    return result ? ORBIS_OK : -1;
}

s32 SDLAudio::AudioOutSetVolume(s32 handle, s32 bitflag, s32* volume) {
    using Libraries::AudioOut::OrbisAudioOutParamFormat;
    std::shared_lock lock{m_mutex};
    auto& port = portsOut[handle - 1];
    if (!port.isOpen) {
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT;
    }

    for (int i = 0; i < port.channels_num; i++, bitflag >>= 1u) {
        auto bit = bitflag & 0x1u;

        if (bit == 1) {
            int src_index = i;
            if (port.format ==
                    OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH_STD ||
                port.format == OrbisAudioOutParamFormat::ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_8CH_STD) {
                switch (i) {
                case 4:
                    src_index = 6;
                    break;
                case 5:
                    src_index = 7;
                    break;
                case 6:
                    src_index = 4;
                    break;
                case 7:
                    src_index = 5;
                    break;
                default:
                    break;
                }
            }
            port.volume[i] = volume[src_index];
        }
    }

    return ORBIS_OK;
}

s32 SDLAudio::AudioOutGetStatus(s32 handle, int* type, int* channels_num) {
    std::shared_lock lock{m_mutex};
    auto& port = portsOut[handle - 1];
    *type = port.type;
    *channels_num = port.channels_num;

    return ORBIS_OK;
}

} // namespace Audio
