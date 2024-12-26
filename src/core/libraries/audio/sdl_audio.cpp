// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_init.h>

#include "common/logging/log.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio/audioout_backend.h"

namespace Libraries::AudioOut {

constexpr int AUDIO_STREAM_BUFFER_THRESHOLD = 65536; // Define constant for buffer threshold

class SDLPortBackend : public PortBackend {
public:
    explicit SDLPortBackend(const PortOut& port) {
        const SDL_AudioSpec fmt = {
            .format = port.is_float ? SDL_AUDIO_F32 : SDL_AUDIO_S16,
            .channels = port.channels_num,
            .freq = static_cast<int>(port.freq),
        };
        stream =
            SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &fmt, nullptr, nullptr);
        if (stream == nullptr) {
            LOG_ERROR(Lib_AudioOut, "Failed to create SDL audio stream: {}", SDL_GetError());
        }
        SDL_ResumeAudioStreamDevice(stream);
    }

    ~SDLPortBackend() override {
        if (stream) {
            SDL_DestroyAudioStream(stream);
            stream = nullptr;
        }
    }

    void Output(void* ptr, size_t size) override {
        SDL_PutAudioStreamData(stream, ptr, static_cast<int>(size));
        while (SDL_GetAudioStreamAvailable(stream) > AUDIO_STREAM_BUFFER_THRESHOLD) {
            // Yield to allow the stream to drain.
            std::this_thread::yield();
        }
    }

    void SetVolume(const std::array<int, 8>& ch_volumes) override {
        // TODO: Not yet implemented
    }

private:
    SDL_AudioStream* stream;
};

std::unique_ptr<PortBackend> SDLAudioOut::Open(PortOut& port) {
    return std::make_unique<SDLPortBackend>(port);
}

} // namespace Libraries::AudioOut
