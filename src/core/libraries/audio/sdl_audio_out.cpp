// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <atomic>
#include <cstring>
#include <thread>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_hints.h>

#include "common/config.h"
#include "common/logging/log.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio/audioout_backend.h"
#include "core/libraries/kernel/threads.h"

#define SDL_INVALID_AUDIODEVICEID 0

namespace Libraries::AudioOut {

// Volume constants
constexpr float VOLUME_0DB = 32768.0f; // 1 << 15

// Channel positions
enum ChannelPos {
    FL = 0,
    FR = 1,
    FC = 2,
    LF = 3,
    SL = 4,
    SR = 5,
    BL = 6,
    BR = 7,
    STD_SL = 6,
    STD_SR = 7,
    STD_BL = 4,
    STD_BR = 5
};

class SDLPortBackend : public PortBackend {
public:
    explicit SDLPortBackend(const PortOut& port)
        : frame_size(port.format_info.FrameSize()), guest_buffer_size(port.BufferSize()),
          buffer_frames(port.buffer_frames), sample_rate(port.sample_rate),
          num_channels(port.format_info.num_channels), is_float(port.format_info.is_float),
          is_std(port.format_info.is_std), channel_layout(port.format_info.channel_layout) {

        // Calculate timing
        period_us = (1000000ULL * buffer_frames + sample_rate / 2) / sample_rate;
        last_output_time = 0;
        next_output_time = 0;

        // Allocate internal buffer
        internal_buffer_size = buffer_frames * sizeof(float) * num_channels;
        internal_buffer = std::malloc(internal_buffer_size);
        if (!internal_buffer) {
            LOG_ERROR(Lib_AudioOut, "Failed to allocate internal audio buffer");
            return;
        }

        // Initialize current gain
        current_gain.store(Config::getVolumeSlider() / 100.0f);

        // Select converter function
        SelectConverter();

        // Open SDL device
        if (!OpenDevice(port.type)) {
            std::free(internal_buffer);
            internal_buffer = nullptr;
            return;
        }

        CalculateQueueThreshold();
    }

    ~SDLPortBackend() override {
        if (stream) {
            SDL_DestroyAudioStream(stream);
        }
        if (internal_buffer) {
            std::free(internal_buffer);
        }
    }

    void Output(void* ptr) override {
        if (!stream || !internal_buffer) {
            return;
        }

        // Check for volume changes and update if needed
        UpdateVolumeIfChanged();

        // Get current time in microseconds
        u64 current_time = Kernel::sceKernelGetProcessTime();

        if (ptr != nullptr) {
            // Simple format conversion (no volume application)
            convert(ptr, internal_buffer, buffer_frames, nullptr);

            if (next_output_time == 0) {
                next_output_time = current_time + period_us;
            } else if (current_time > next_output_time) {
                next_output_time = current_time + period_us;
            } else {
                u64 wait_until = next_output_time;
                next_output_time += period_us;

                if (current_time < wait_until) {
                    u64 sleep_us = wait_until - current_time;
                    if (sleep_us > 10) {
                        sleep_us -= 10;
                        std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));
                    }
                }
            }

            last_output_time = current_time;

            // Check queue and clear if backed up
            if (const auto queued = SDL_GetAudioStreamQueued(stream); queued >= queue_threshold) {
                LOG_DEBUG(Lib_AudioOut, "Clearing backed up audio queue ({} >= {})", queued,
                          queue_threshold);
                SDL_ClearAudioStream(stream);
                CalculateQueueThreshold();
            }

            if (!SDL_PutAudioStreamData(stream, internal_buffer, internal_buffer_size)) {
                LOG_ERROR(Lib_AudioOut, "Failed to output to SDL audio stream: {}", SDL_GetError());
            }
        }
    }

    void SetVolume(const std::array<int, 8>& ch_volumes) override {
        if (!stream) {
            return;
        }
        float max_channel_gain = 0.0f;
        for (int i = 0; i < num_channels && i < 8; i++) {
            float channel_gain = static_cast<float>(ch_volumes[i]) / VOLUME_0DB;
            max_channel_gain = std::max(max_channel_gain, channel_gain);
        }

        // Combine with global volume slider
        float total_gain = max_channel_gain * (Config::getVolumeSlider() / 100.0f);

        std::lock_guard<std::mutex> lock(volume_mutex);
        if (SDL_SetAudioStreamGain(stream, total_gain)) {
            current_gain.store(total_gain);
            LOG_DEBUG(Lib_AudioOut,
                      "Set combined audio gain to {:.3f} (channel: {:.3f}, slider: {:.3f})",
                      total_gain, max_channel_gain, Config::getVolumeSlider() / 100.0f);
        } else {
            LOG_ERROR(Lib_AudioOut, "Failed to set audio stream gain: {}", SDL_GetError());
        }
    }

    u64 GetLastOutputTime() const {
        return last_output_time;
    }

private:
    std::atomic<bool> volume_update_needed{false};
    u64 last_volume_check_time{0};
    static constexpr u64 VOLUME_CHECK_INTERVAL_US = 50000; // Check every 50ms

    void UpdateVolumeIfChanged() {
        u64 current_time = Kernel::sceKernelGetProcessTime();

        // Only check volume every 50ms to reduce overhead
        if (current_time - last_volume_check_time >= VOLUME_CHECK_INTERVAL_US) {
            last_volume_check_time = current_time;

            float config_volume = Config::getVolumeSlider() / 100.0f;
            float stored_gain = current_gain.load();

            if (std::abs(config_volume - stored_gain) > 0.001f) {
                if (SDL_SetAudioStreamGain(stream, config_volume)) {
                    current_gain.store(config_volume);
                    LOG_DEBUG(Lib_AudioOut, "Updated audio gain to {:.3f}", config_volume);
                } else {
                    LOG_ERROR(Lib_AudioOut, "Failed to set audio stream gain: {}", SDL_GetError());
                }
            }
        }
    }
    bool OpenDevice(OrbisAudioOutPort type) {
        const SDL_AudioSpec fmt = {
            .format = SDL_AUDIO_F32LE, // Always use float for internal processing
            .channels = static_cast<u8>(num_channels),
            .freq = static_cast<int>(sample_rate),
        };

        // Determine device name
        std::string device_name = GetDeviceName(type);
        SDL_AudioDeviceID dev_id = SDL_INVALID_AUDIODEVICEID;

        if (device_name == "None") {
            LOG_INFO(Lib_AudioOut, "Audio device disabled for port type {}",
                     static_cast<int>(type));
            return false;
        } else if (device_name.empty() || device_name == "Default Device") {
            dev_id = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
        } else {
            int num_devices = 0;
            SDL_AudioDeviceID* dev_array = SDL_GetAudioPlaybackDevices(&num_devices);

            if (dev_array) {
                bool found = false;
                for (int i = 0; i < num_devices; i++) {
                    const char* dev_name = SDL_GetAudioDeviceName(dev_array[i]);
                    if (dev_name && std::string(dev_name) == device_name) {
                        dev_id = dev_array[i];
                        found = true;
                        break;
                    }
                }
                SDL_free(dev_array);

                if (!found) {
                    LOG_WARNING(Lib_AudioOut, "Audio device '{}' not found, using default",
                                device_name);
                    dev_id = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
                }
            } else {
                LOG_WARNING(Lib_AudioOut, "No audio devices found, using default");
                dev_id = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
            }
        }

        // Create audio stream
        stream = SDL_OpenAudioDeviceStream(dev_id, &fmt, nullptr, nullptr);
        if (!stream) {
            LOG_ERROR(Lib_AudioOut, "Failed to create SDL audio stream: {}", SDL_GetError());
            return false;
        }

        // Set channel map
        if (num_channels > 0) {
            std::vector<int> channel_map(num_channels);

            if (is_std && num_channels == 8) {
                // Standard 8CH layout
                channel_map = {FL, FR, FC, LF, STD_SL, STD_SR, STD_BL, STD_BR};
            } else {
                // Use provided channel layout
                for (int i = 0; i < num_channels; i++) {
                    channel_map[i] = channel_layout[i];
                }
            }

            if (!SDL_SetAudioStreamInputChannelMap(stream, channel_map.data(), num_channels)) {
                LOG_ERROR(Lib_AudioOut, "Failed to set channel map: {}", SDL_GetError());
                SDL_DestroyAudioStream(stream);
                stream = nullptr;
                return false;
            }
        }

        // Set initial volume
        float initial_gain = current_gain.load();
        if (!SDL_SetAudioStreamGain(stream, initial_gain)) {
            LOG_WARNING(Lib_AudioOut, "Failed to set initial audio gain: {}", SDL_GetError());
        }

        // Start playback
        if (!SDL_ResumeAudioStreamDevice(stream)) {
            LOG_ERROR(Lib_AudioOut, "Failed to resume audio stream: {}", SDL_GetError());
            SDL_DestroyAudioStream(stream);
            stream = nullptr;
            return false;
        }

        LOG_INFO(Lib_AudioOut, "Opened audio device: {} ({} Hz, {} ch, gain: {:.3f})", device_name,
                 sample_rate, num_channels, initial_gain);
        return true;
    }

    std::string GetDeviceName(OrbisAudioOutPort type) {
        switch (type) {
        case OrbisAudioOutPort::Main:
        case OrbisAudioOutPort::Bgm:
            return Config::getMainOutputDevice();
        // case OrbisAudioOutPort::Voice:
        // case OrbisAudioOutPort::Personal:
        //     return Config::getHeadphoneOutputDevice();
        case OrbisAudioOutPort::PadSpk:
            return Config::getPadSpkOutputDevice();
        // case OrbisAudioOutPort::Aux:
        //     return Config::getSpecialOutputDevice();
        default:
            return Config::getMainOutputDevice();
        }
    }

    void SelectConverter() {
        if (is_float) {
            switch (num_channels) {
            case 1:
                convert = &ConvertF32Mono;
                break;
            case 2:
                convert = &ConvertF32Stereo;
                break;
            case 8:
                if (is_std) {
                    convert = &ConvertF32Std8CH;
                } else {
                    convert = &ConvertF32_8CH;
                }
                break;
            default:
                LOG_ERROR(Lib_AudioOut, "Unsupported float channel count: {}", num_channels);
                convert = nullptr;
            }
        } else {
            switch (num_channels) {
            case 1:
                convert = &ConvertS16Mono;
                break;
            case 2:
                convert = &ConvertS16Stereo;
                break;
            case 8:
                convert = &ConvertS16_8CH;
                break;
            default:
                LOG_ERROR(Lib_AudioOut, "Unsupported S16 channel count: {}", num_channels);
                convert = nullptr;
            }
        }
    }

    void CalculateQueueThreshold() {
        if (!stream)
            return;

        SDL_AudioSpec discard;
        int sdl_buffer_frames;
        if (!SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(stream), &discard,
                                      &sdl_buffer_frames)) {
            LOG_WARNING(Lib_AudioOut, "Failed to get SDL buffer size: {}", SDL_GetError());
            sdl_buffer_frames = 0;
        }

        u32 sdl_buffer_size = sdl_buffer_frames * sizeof(float) * num_channels;
        queue_threshold = std::max(guest_buffer_size, sdl_buffer_size) * 4;

        LOG_DEBUG(Lib_AudioOut, "Audio queue threshold: {} bytes (SDL buffer: {} frames)",
                  queue_threshold, sdl_buffer_frames);
    }

    using ConverterFunc = void (*)(const void* src, void* dst, u32 frames, const float* volumes);

    // Remove volume parameter and application from all converters
    static void ConvertS16Mono(const void* src, void* dst, u32 frames, const float*) {
        const s16* s = static_cast<const s16*>(src);
        float* d = static_cast<float*>(dst);

        constexpr float inv_scale = 1.0f / VOLUME_0DB;

        for (u32 i = 0; i < frames; i++) {
            d[i] = s[i] * inv_scale;
        }
    }

    static void ConvertS16Stereo(const void* src, void* dst, u32 frames, const float*) {
        const s16* s = static_cast<const s16*>(src);
        float* d = static_cast<float*>(dst);

        constexpr float inv_scale = 1.0f / VOLUME_0DB;

        for (u32 i = 0; i < frames; i++) {
            d[i * 2] = s[i * 2] * inv_scale;
            d[i * 2 + 1] = s[i * 2 + 1] * inv_scale;
        }
    }

    static void ConvertS16_8CH(const void* src, void* dst, u32 frames, const float*) {
        const s16* s = static_cast<const s16*>(src);
        float* d = static_cast<float*>(dst);

        constexpr float inv_scale = 1.0f / VOLUME_0DB;

        for (u32 i = 0; i < frames; i++) {
            for (int ch = 0; ch < 8; ch++) {
                d[i * 8 + ch] = s[i * 8 + ch] * inv_scale;
            }
        }
    }

    // Float converters become simple memcpy or passthrough
    static void ConvertF32Mono(const void* src, void* dst, u32 frames, const float*) {
        std::memcpy(dst, src, frames * sizeof(float));
    }

    static void ConvertF32Stereo(const void* src, void* dst, u32 frames, const float*) {
        std::memcpy(dst, src, frames * 2 * sizeof(float));
    }

    static void ConvertF32_8CH(const void* src, void* dst, u32 frames, const float*) {
        std::memcpy(dst, src, frames * 8 * sizeof(float));
    }

    static void ConvertF32Std8CH(const void* src, void* dst, u32 frames, const float*) {
        const float* s = static_cast<const float*>(src);
        float* d = static_cast<float*>(dst);

        for (u32 i = 0; i < frames; i++) {
            d[i * 8 + FL] = s[i * 8 + FL];
            d[i * 8 + FR] = s[i * 8 + FR];
            d[i * 8 + FC] = s[i * 8 + FC];
            d[i * 8 + LF] = s[i * 8 + LF];
            d[i * 8 + SL] = s[i * 8 + STD_SL]; // Channel remapping still needed
            d[i * 8 + SR] = s[i * 8 + STD_SR];
            d[i * 8 + BL] = s[i * 8 + STD_BL];
            d[i * 8 + BR] = s[i * 8 + STD_BR];
        }
    }

    // Member variables
    u32 frame_size;
    u32 guest_buffer_size;
    u32 buffer_frames;
    u32 sample_rate;
    u32 num_channels;
    bool is_float;
    bool is_std;
    std::array<int, 8> channel_layout;

    u64 period_us;
    u64 last_output_time;
    u64 next_output_time;

    // Buffers
    u32 internal_buffer_size;
    void* internal_buffer;

    // Converter function
    ConverterFunc convert;

    // Volume tracking
    std::atomic<float> current_gain{1.0f};
    mutable std::mutex volume_mutex;

    // SDL
    SDL_AudioStream* stream{};
    u32 queue_threshold{};
};

std::unique_ptr<PortBackend> SDLAudioOut::Open(PortOut& port) {
    return std::make_unique<SDLPortBackend>(port);
}

} // namespace Libraries::AudioOut