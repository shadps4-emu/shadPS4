// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_hints.h>
#include <common/config.h>

#include "common/logging/log.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio/audioout_backend.h"

namespace Libraries::AudioOut {

class SDLPortBackend : public PortBackend {
public:
    explicit SDLPortBackend(const PortOut& port)
        : frame_size(port.format_info.FrameSize()), guest_buffer_size(port.BufferSize()) {
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
        CalculateQueueThreshold();
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
        SDL_SetAudioStreamGain(stream, Config::getVolumeSlider() / 100.0f);
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
            CalculateQueueThreshold();
        }
        if (!SDL_PutAudioStreamData(stream, ptr, static_cast<int>(guest_buffer_size))) {
            LOG_ERROR(Lib_AudioOut, "Failed to output to SDL audio stream: {}", SDL_GetError());
        }
    }

    void SetVolume(const std::array<int, 8>& ch_volumes) override {
        if (!stream) {
            return;
        }
        // SDL does not have per-channel volumes, for now just take the maximum of the channels.
        const auto vol = *std::ranges::max_element(ch_volumes);
        if (!SDL_SetAudioStreamGain(stream, static_cast<float>(vol) / SCE_AUDIO_OUT_VOLUME_0DB *
                                                Config::getVolumeSlider() / 100.0f)) {
            LOG_WARNING(Lib_AudioOut, "Failed to change SDL audio stream volume: {}",
                        SDL_GetError());
        }
    }

private:
    void CalculateQueueThreshold() {
        SDL_AudioSpec discard;
        int sdl_buffer_frames;
        if (!SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(stream), &discard,
                                      &sdl_buffer_frames)) {
            LOG_WARNING(Lib_AudioOut, "Failed to get SDL audio stream buffer size: {}",
                        SDL_GetError());
            sdl_buffer_frames = 0;
        }
        const auto sdl_buffer_size = sdl_buffer_frames * frame_size;
        const auto new_threshold = std::max(guest_buffer_size, sdl_buffer_size) * 4;
        if (host_buffer_size != sdl_buffer_size || queue_threshold != new_threshold) {
            host_buffer_size = sdl_buffer_size;
            queue_threshold = new_threshold;
            LOG_INFO(Lib_AudioOut,
                     "SDL audio buffers: guest = {} bytes, host = {} bytes, threshold = {} bytes",
                     guest_buffer_size, host_buffer_size, queue_threshold);
        }
    }

    u32 frame_size;
    u32 guest_buffer_size;
    u32 host_buffer_size{};
    u32 queue_threshold{};
    SDL_AudioStream* stream{};
};

std::unique_ptr<PortBackend> SDLAudioOut::Open(PortOut& port) {
    return std::make_unique<SDLPortBackend>(port);
}

} // namespace Libraries::AudioOut
