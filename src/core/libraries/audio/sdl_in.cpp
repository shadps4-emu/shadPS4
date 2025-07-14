// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include "sdl_in.h"

int SDLAudioIn::AudioInit() {
    return SDL_InitSubSystem(SDL_INIT_AUDIO);
}

int SDLAudioIn::AudioInOpen(int type, uint32_t samples_num, uint32_t freq, uint32_t format) {
    using Libraries::AudioIn::OrbisAudioInParam;
    std::scoped_lock lock{m_mutex};

    for (int id = 0; id < portsIn.size(); id++) {
        auto& port = portsIn[id];
        if (!port.isOpen) {
            port.isOpen = true;
            port.type = type;
            port.samples_num = samples_num;
            port.freq = freq;
            port.format = format;

            SDL_AudioFormat sampleFormat;
            switch (format) {
            case 0:
                sampleFormat = SDL_AUDIO_S16;
                port.channels_num = 1;
                port.sample_size = 2;
                break;
            case 2:
                sampleFormat = SDL_AUDIO_S16;
                port.channels_num = 2;
                port.sample_size = 2;
                break;
            default:
                port.isOpen = false;
                return ORBIS_AUDIO_IN_ERROR_INVALID_PORT;
            }

            SDL_AudioSpec fmt;
            SDL_zero(fmt);
            fmt.format = sampleFormat;
            fmt.channels = port.channels_num;
            fmt.freq = port.freq;

            port.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &fmt,
                                                    nullptr, nullptr);
            if (!port.stream) {
                port.isOpen = false;
                return -1;
            }

            SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(port.stream));
            return id + 1;
        }
    }

    return -1;
}

int SDLAudioIn::AudioInInput(int handle, void* out_buffer) {
    std::scoped_lock lock{m_mutex};
    if (handle < 1 || handle > portsIn.size())
        return ORBIS_AUDIO_IN_ERROR_INVALID_PORT;

    auto& port = portsIn[handle - 1];
    if (!port.isOpen || out_buffer == nullptr) {
        return ORBIS_AUDIO_IN_ERROR_INVALID_PORT;
    }

    const int bytesToRead = port.samples_num * port.sample_size * port.channels_num;

    if (out_buffer == nullptr) {
        int attempts = 0;
        while (SDL_GetAudioStreamAvailable(port.stream) > 0) {
            SDL_Delay(1);
            if (++attempts > 1000) {
                return ORBIS_AUDIO_IN_ERROR_TIMEOUT;
            }
        }
        return 0; // done
    }

    int attempts = 0;
    while (SDL_GetAudioStreamAvailable(port.stream) < bytesToRead) {
        SDL_Delay(1);
        if (++attempts > 1000) {
            return ORBIS_AUDIO_IN_ERROR_TIMEOUT;
        }
    }

    int result = SDL_GetAudioStreamData(port.stream, out_buffer, bytesToRead);
    if (result < 0) {
        // SDL_GetAudioStreamData failed
        SDL_Log("AudioInInput error: %s", SDL_GetError());
        return ORBIS_AUDIO_IN_ERROR_STREAM_FAIL;
    }

    return result;
}

void SDLAudioIn::AudioInClose(int handle) {
    std::scoped_lock lock{m_mutex};
    if (handle < 1 || handle > portsIn.size())
        return;

    auto& port = portsIn[handle - 1];
    if (!port.isOpen)
        return;

    SDL_DestroyAudioStream(port.stream);
    port.isOpen = false;
}
