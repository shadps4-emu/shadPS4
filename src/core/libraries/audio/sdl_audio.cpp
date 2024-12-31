// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_hints.h>

#include "common/logging/log.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio/audioout_backend.h"

namespace Libraries::AudioOut {

class SDLPortBackend : public PortBackend {
public:
    explicit SDLPortBackend(const PortOut& port)
        : frame_size(port.format_info.FrameSize()), buffer_size(port.BufferSize()) {
        // We want the latency for delivering frames out to be as small as possible,
        // so set the sample frames hint to the number of frames per buffer.
        const auto samples_num_str = std::to_string(port.buffer_frames);
        if (!SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, samples_num_str.c_str())) {
            LOG_WARNING(Lib_AudioOut, "Failed to set SDL audio sample frames hint to {}: {}",
                        samples_num_str, SDL_GetError());
        }
        const SDL_AudioSpec fmt = {
            .format = port.format_info.is_float ? SDL_AUDIO_F32LE : SDL_AUDIO_S16LE,
            .channels = port.format_info.num_channels,
            .freq = static_cast<int>(port.sample_rate),
        };
        stream =
            SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &fmt, nullptr, nullptr);
        if (stream == nullptr) {
            LOG_ERROR(Lib_AudioOut, "Failed to create SDL audio stream: {}", SDL_GetError());
            return;
        }
        queue_threshold = CalculateQueueThreshold();
        if (!SDL_SetAudioStreamInputChannelMap(stream, port.format_info.channel_layout.data(),
                                               port.format_info.num_channels)) {
            LOG_ERROR(Lib_AudioOut, "Failed to configure SDL audio stream channel map: {}",
                      SDL_GetError());
            SDL_DestroyAudioStream(stream);
            stream = nullptr;
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

    void Output(void* ptr) override {
        if (!stream) {
            return;
        }
        // AudioOut library manages timing, but we still need to guard against the SDL
        // audio queue stalling, which may happen during device changes, for example.
        // Otherwise, latency may grow over time unbounded.
        if (const auto queued = SDL_GetAudioStreamQueued(stream); queued >= queue_threshold) {
            LOG_WARNING(Lib_AudioOut,
                        "SDL audio queue backed up ({} queued, {} threshold), clearing.", queued,
                        queue_threshold);
            SDL_ClearAudioStream(stream);
            // Recalculate the threshold in case this happened because of a device change.
            queue_threshold = CalculateQueueThreshold();
        }
        if (!SDL_PutAudioStreamData(stream, ptr, static_cast<int>(buffer_size))) {
            LOG_ERROR(Lib_AudioOut, "Failed to output to SDL audio stream: {}", SDL_GetError());
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
    [[nodiscard]] u32 CalculateQueueThreshold() const {
        SDL_AudioSpec discard;
        int sdl_buffer_frames;
        if (!SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(stream), &discard,
                                      &sdl_buffer_frames)) {
            LOG_WARNING(Lib_AudioOut, "Failed to get SDL audio stream buffer size: {}",
                        SDL_GetError());
            sdl_buffer_frames = 0;
        }
        return std::max<u32>(buffer_size, sdl_buffer_frames * frame_size) * 4;
    }

    u32 frame_size;
    u32 buffer_size;
    u32 queue_threshold;
    SDL_AudioStream* stream;
};

std::unique_ptr<PortBackend> SDLAudioOut::Open(PortOut& port) {
    return std::make_unique<SDLPortBackend>(port);
}

} // namespace Libraries::AudioOut
