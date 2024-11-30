// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>

#include "common/assert.h"
#include "core/libraries/audio/audioout_error.h"
#include "core/libraries/audio/sdl_audio.h"

namespace Libraries::AudioOut {

constexpr int AUDIO_STREAM_BUFFER_THRESHOLD = 65536; // Define constant for buffer threshold

s32 SDLAudioOut::Open(OrbisAudioOutPort type, u32 samples_num, u32 freq,
                      OrbisAudioOutParamFormat format) {
    std::scoped_lock lock{m_mutex};
    const auto port = std::ranges::find(ports_out, false, &PortOut::is_open);
    if (port == ports_out.end()) {
        LOG_ERROR(Lib_AudioOut, "Audio ports are full");
        return ORBIS_AUDIO_OUT_ERROR_PORT_FULL;
    }

    port->is_open = true;
    port->type = type;
    port->samples_num = samples_num;
    port->freq = freq;
    port->format = format;
    SDL_AudioFormat sampleFormat;
    switch (format) {
    case OrbisAudioOutParamFormat::S16Mono:
        sampleFormat = SDL_AUDIO_S16;
        port->channels_num = 1;
        port->sample_size = 2;
        break;
    case OrbisAudioOutParamFormat::FloatMono:
        sampleFormat = SDL_AUDIO_F32;
        port->channels_num = 1;
        port->sample_size = 4;
        break;
    case OrbisAudioOutParamFormat::S16Stereo:
        sampleFormat = SDL_AUDIO_S16;
        port->channels_num = 2;
        port->sample_size = 2;
        break;
    case OrbisAudioOutParamFormat::FloatStereo:
        sampleFormat = SDL_AUDIO_F32;
        port->channels_num = 2;
        port->sample_size = 4;
        break;
    case OrbisAudioOutParamFormat::S16_8CH:
        sampleFormat = SDL_AUDIO_S16;
        port->channels_num = 8;
        port->sample_size = 2;
        break;
    case OrbisAudioOutParamFormat::Float_8CH:
        sampleFormat = SDL_AUDIO_F32;
        port->channels_num = 8;
        port->sample_size = 4;
        break;
    case OrbisAudioOutParamFormat::S16_8CH_Std:
        sampleFormat = SDL_AUDIO_S16;
        port->channels_num = 8;
        port->sample_size = 2;
        break;
    case OrbisAudioOutParamFormat::Float_8CH_Std:
        sampleFormat = SDL_AUDIO_F32;
        port->channels_num = 8;
        port->sample_size = 4;
        break;
    default:
        UNREACHABLE_MSG("Unknown format");
    }

    port->volume.fill(Libraries::AudioOut::SCE_AUDIO_OUT_VOLUME_0DB);

    SDL_AudioSpec fmt;
    SDL_zero(fmt);
    fmt.format = sampleFormat;
    fmt.channels = port->channels_num;
    fmt.freq = freq;
    port->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &fmt, NULL, NULL);
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(port->stream));
    return std::distance(ports_out.begin(), port) + 1;
}

s32 SDLAudioOut::Output(s32 handle, const void* ptr) {
    auto& port = ports_out.at(handle - 1);
    if (!port.is_open) {
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT;
    }

    const size_t data_size = port.samples_num * port.sample_size * port.channels_num;
    bool result = SDL_PutAudioStreamData(port.stream, ptr, data_size);
    while (SDL_GetAudioStreamAvailable(port.stream) > AUDIO_STREAM_BUFFER_THRESHOLD) {
        SDL_Delay(0);
    }
    return result ? ORBIS_OK : -1;
}

s32 SDLAudioOut::SetVolume(s32 handle, s32 bitflag, s32* volume) {
    using Libraries::AudioOut::OrbisAudioOutParamFormat;
    auto& port = ports_out.at(handle - 1);
    if (!port.is_open) {
        return ORBIS_AUDIO_OUT_ERROR_INVALID_PORT;
    }

    for (int i = 0; i < port.channels_num; i++, bitflag >>= 1u) {
        auto bit = bitflag & 0x1u;

        if (bit == 1) {
            int src_index = i;
            if (port.format == OrbisAudioOutParamFormat::Float_8CH_Std ||
                port.format == OrbisAudioOutParamFormat::S16_8CH_Std) {
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

} // namespace Libraries::AudioOut
