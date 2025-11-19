// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <common/config.h>
#include <common/logging/log.h>
#include "sdl_in.h"

int SDLAudioIn::AudioInit() {
    return SDL_InitSubSystem(SDL_INIT_AUDIO);
}

int SDLAudioIn::AudioInOpen(int type, uint32_t samples_num, uint32_t freq, uint32_t format) {
    std::scoped_lock lock{m_mutex};

    for (int id = 0; id < static_cast<int>(portsIn.size()); ++id) {
        auto& port = portsIn[id];
        if (!port.isOpen) {
            port.isOpen = true;
            port.type = type;
            port.samples_num = samples_num;
            port.freq = freq;
            port.format = format;

            SDL_AudioFormat sampleFormat;
            switch (format) {
            case Libraries::AudioIn::ORBIS_AUDIO_IN_PARAM_FORMAT_S16_MONO:
                sampleFormat = SDL_AUDIO_S16;
                port.channels_num = 1;
                port.sample_size = 2;
                break;
            case Libraries::AudioIn::ORBIS_AUDIO_IN_PARAM_FORMAT_S16_STEREO:
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

            std::string micDevStr = Config::getMicDevice();
            uint32_t devId;

            bool nullDevice = false;
            if (micDevStr == "None") {
                nullDevice = true;
            } else if (micDevStr == "Default Device") {
                devId = SDL_AUDIO_DEVICE_DEFAULT_RECORDING;
            } else {
                try {
                    devId = static_cast<uint32_t>(std::stoul(micDevStr));
                } catch (const std::exception& e) {
                    nullDevice = true;
                }
            }

            port.stream =
                nullDevice ? nullptr : SDL_OpenAudioDeviceStream(devId, &fmt, nullptr, nullptr);

            if (!port.stream) {
                // if stream is null, either due to configuration disabling the input,
                // or no input devices present in the system, still return a valid id
                // as some games require that (e.g. L.A. Noire)
                return id + 1;
            }

            if (SDL_ResumeAudioStreamDevice(port.stream) == false) {
                SDL_DestroyAudioStream(port.stream);
                port = {};
                return ORBIS_AUDIO_IN_ERROR_STREAM_FAIL;
            }

            return id + 1;
        }
    }

    return ORBIS_AUDIO_IN_ERROR_INVALID_PORT;
}

int SDLAudioIn::AudioInInput(int handle, void* out_buffer) {
    std::scoped_lock lock{m_mutex};

    if (handle < 1 || handle > static_cast<int>(portsIn.size()) || !out_buffer)
        return ORBIS_AUDIO_IN_ERROR_INVALID_PORT;

    auto& port = portsIn[handle - 1];
    if (!port.isOpen)
        return ORBIS_AUDIO_IN_ERROR_INVALID_PORT;

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

    const int bytesRead = SDL_GetAudioStreamData(port.stream, out_buffer, bytesToRead);
    if (bytesRead < 0) {
        // SDL_GetAudioStreamData failed
        LOG_ERROR(Lib_AudioIn, "AudioInInput error: {}", SDL_GetError());
        return ORBIS_AUDIO_IN_ERROR_STREAM_FAIL;
    }
    const int framesRead = bytesRead / (port.sample_size * port.channels_num);
    return framesRead;
}

void SDLAudioIn::AudioInClose(int handle) {
    std::scoped_lock lock{m_mutex};
    if (handle < 1 || handle > (int)portsIn.size())
        return;

    auto& port = portsIn[handle - 1];
    if (!port.isOpen)
        return;

    SDL_DestroyAudioStream(port.stream);
    port = {};
}