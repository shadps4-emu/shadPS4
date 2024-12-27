// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>
#include <SDL3/SDL_audio.h>

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
            return;
        }
        if (!SDL_ResumeAudioStreamDevice(stream)) {
            LOG_ERROR(Lib_AudioOut, "Failed to resume SDL audio stream: {}", SDL_GetError());
            SDL_DestroyAudioStream(stream);
            stream = nullptr;
            return;
        }
    }

    ~SDLPortBackend() override {
        if (!stream) {
            return;
        }
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }

    void Output(void* ptr, size_t size) override {
        if (!stream) {
            return;
        }
        SDL_PutAudioStreamData(stream, ptr, static_cast<int>(size));
        while (SDL_GetAudioStreamAvailable(stream) > AUDIO_STREAM_BUFFER_THRESHOLD) {
            // Yield to allow the stream to drain.
            std::this_thread::yield();
        }
    }

    void SetVolume(const std::array<int, 8>& ch_volumes) override {
        if (!stream) {
            return;
        }
        // SDL does not have per-channel volumes, for now just take the maximum of the channels.
        const auto vol = *std::ranges::max_element(ch_volumes);
        if (!SDL_SetAudioStreamGain(stream, static_cast<float>(vol) / SCE_AUDIO_OUT_VOLUME_0DB)) {
            LOG_WARNING(Lib_AudioOut, "Failed to change SDL audio stream volume: {}",
                        SDL_GetError());
        }
    }

private:
    SDL_AudioStream* stream;
};

std::unique_ptr<PortBackend> SDLAudioOut::Open(PortOut& port) {
    return std::make_unique<SDLPortBackend>(port);
}

} // namespace Libraries::AudioOut
