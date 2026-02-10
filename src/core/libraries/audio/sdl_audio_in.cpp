// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <SDL3/SDL.h>
#include <common/config.h>
#include <common/logging/log.h>
#include "audioin.h"
#include "audioin_backend.h"

namespace Libraries::AudioIn {

class SDLInPortBackend : public PortInBackend {
public:
    explicit SDLInPortBackend(const PortIn& port) : port(port) {
        SDL_AudioFormat sampleFormat = SDL_AUDIO_S16; // PS4 uses S16 format

        SDL_AudioSpec fmt;
        SDL_zero(fmt);
        fmt.format = sampleFormat;
        fmt.channels = static_cast<Uint8>(port.channels_num);
        fmt.freq = static_cast<int>(port.freq);

        std::string micDevStr = Config::getMicDevice();
        uint32_t devId = 0;
        if (micDevStr == "None") {
            nullDevice = true;
            LOG_INFO(Lib_AudioIn, "Audio input disabled by configuration");
        } else if (micDevStr == "Default Device") {
            devId = SDL_AUDIO_DEVICE_DEFAULT_RECORDING;
            LOG_INFO(Lib_AudioIn, "Using default audio input device");
        } else {
            try {
                devId = static_cast<uint32_t>(std::stoul(micDevStr));
                LOG_INFO(Lib_AudioIn, "Using audio input device ID: {}", devId);
            } catch (const std::exception& e) {
                nullDevice = true;
                LOG_WARNING(Lib_AudioIn, "Invalid device ID '{}', disabling input", micDevStr);
            }
        }

        if (!nullDevice) {
            stream = SDL_OpenAudioDeviceStream(devId, &fmt, nullptr, nullptr);
            if (stream) {
                if (SDL_ResumeAudioStreamDevice(stream)) {
                    LOG_INFO(Lib_AudioIn, "Audio input opened: {} Hz, {} channels, format {}",
                             port.freq, port.channels_num, static_cast<u32>(port.format));
                } else {
                    SDL_DestroyAudioStream(stream);
                    stream = nullptr;
                    LOG_ERROR(Lib_AudioIn, "Failed to resume audio input stream");
                }
            } else {
                LOG_ERROR(Lib_AudioIn, "Failed to open audio input device: {}", SDL_GetError());
            }
        }

        // Allocate internal buffer for null device simulation
        if (!stream) {
            const size_t bufferSize = port.samples_num * port.sample_size * port.channels_num;
            internal_buffer = std::malloc(bufferSize);
            if (internal_buffer) {
                // Fill with silence
                std::memset(internal_buffer, 0, bufferSize);
                LOG_INFO(Lib_AudioIn, "Created null audio input buffer of {} bytes", bufferSize);
            }
        }
    }

    ~SDLInPortBackend() override {
        if (stream) {
            SDL_DestroyAudioStream(stream);
        }
        if (internal_buffer) {
            std::free(internal_buffer);
            internal_buffer = nullptr;
        }
    }

    int Read(void* out_buffer) override {
        const int bytesToRead = port.samples_num * port.sample_size * port.channels_num;

        if (stream) {
            // Read from actual audio device
            int attempts = 0;
            while (SDL_GetAudioStreamAvailable(stream) < bytesToRead) {
                SDL_Delay(1);
                if (++attempts > 1000) {
                    return 0;
                }
            }

            const int bytesRead = SDL_GetAudioStreamData(stream, out_buffer, bytesToRead);
            if (bytesRead < 0) {
                LOG_ERROR(Lib_AudioIn, "Audio input read error: {}", SDL_GetError());
                return 0;
            }

            const int framesRead = bytesRead / (port.sample_size * port.channels_num);
            return framesRead;
        } else if (internal_buffer) {
            // Return silence from null device buffer
            std::memcpy(out_buffer, internal_buffer, bytesToRead);
            return port.samples_num;
        } else {
            // No device available
            return 0;
        }
    }

    void Clear() override {
        if (stream) {
            SDL_ClearAudioStream(stream);
        }
    }
    bool IsAvailable() override {
        if (nullDevice) {
            return false;
        } else {
            return true;
        }
    }

private:
    const PortIn& port;
    SDL_AudioStream* stream = nullptr;
    void* internal_buffer = nullptr;
    bool nullDevice = false;
};

std::unique_ptr<PortInBackend> SDLAudioIn::Open(PortIn& port) {
    return std::make_unique<SDLInPortBackend>(port);
}

} // namespace Libraries::AudioIn