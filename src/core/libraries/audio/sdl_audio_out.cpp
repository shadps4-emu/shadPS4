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
constexpr float INV_VOLUME_0DB = 1.0f / VOLUME_0DB;
constexpr float VOLUME_EPSILON = 0.001f;

// Timing constants
constexpr u64 VOLUME_CHECK_INTERVAL_US = 50000; // Check every 50ms
constexpr u64 MIN_SLEEP_THRESHOLD_US = 10;

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

        if (!Initialize(port.type)) {
            LOG_ERROR(Lib_AudioOut, "Failed to initialize SDL audio backend");
        }
    }

    ~SDLPortBackend() override {
        Cleanup();
    }

    void Output(void* ptr) override {
        if (!stream || !internal_buffer || !convert) {
            return;
        }

        // Check for volume changes
        UpdateVolumeIfChanged();

        // Get current time
        const u64 current_time = Kernel::sceKernelGetProcessTime();

        if (ptr != nullptr) {
            // Convert audio format
            convert(ptr, internal_buffer, buffer_frames, nullptr);

            // Handle timing for consistent playback
            HandleTiming(current_time);

            // Check and manage audio queue
            ManageAudioQueue();

            // Send data to SDL
            if (!SDL_PutAudioStreamData(stream, internal_buffer, internal_buffer_size)) {
                LOG_ERROR(Lib_AudioOut, "Failed to output to SDL audio stream: {}", SDL_GetError());
            }

            last_output_time = current_time;
        }
    }

    void SetVolume(const std::array<int, 8>& ch_volumes) override {
        if (!stream) {
            return;
        }

        // Calculate maximum channel gain
        float max_channel_gain = 0.0f;
        const int channels_to_check = std::min(num_channels, 8u);

        for (int i = 0; i < channels_to_check; i++) {
            const float channel_gain = static_cast<float>(ch_volumes[i]) * INV_VOLUME_0DB;
            max_channel_gain = std::max(max_channel_gain, channel_gain);
        }

        // Combine with global volume slider
        const float slider_gain = Config::getVolumeSlider() / 100.0f;
        const float total_gain = max_channel_gain * slider_gain;

        // Apply volume change
        std::lock_guard<std::mutex> lock(volume_mutex);
        if (SDL_SetAudioStreamGain(stream, total_gain)) {
            current_gain.store(total_gain, std::memory_order_relaxed);
            LOG_DEBUG(Lib_AudioOut,
                      "Set combined audio gain to {:.3f} (channel: {:.3f}, slider: {:.3f})",
                      total_gain, max_channel_gain, slider_gain);
        } else {
            LOG_ERROR(Lib_AudioOut, "Failed to set audio stream gain: {}", SDL_GetError());
        }
    }

    u64 GetLastOutputTime() const {
        return last_output_time.load(std::memory_order_relaxed);
    }

private:
    bool Initialize(OrbisAudioOutPort type) {
        // Calculate timing parameters
        period_us = (1000000ULL * buffer_frames + sample_rate / 2) / sample_rate;

        // Allocate internal buffer
        internal_buffer_size = buffer_frames * sizeof(float) * num_channels;
        internal_buffer = std::malloc(internal_buffer_size);
        if (!internal_buffer) {
            LOG_ERROR(Lib_AudioOut, "Failed to allocate internal audio buffer of size {}",
                      internal_buffer_size);
            return false;
        }

        // Initialize current gain
        current_gain.store(Config::getVolumeSlider() / 100.0f, std::memory_order_relaxed);

        // Select converter function
        if (!SelectConverter()) {
            std::free(internal_buffer);
            internal_buffer = nullptr;
            return false;
        }

        // Open SDL device
        if (!OpenDevice(type)) {
            std::free(internal_buffer);
            internal_buffer = nullptr;
            return false;
        }

        CalculateQueueThreshold();
        return true;
    }

    void Cleanup() {
        if (stream) {
            SDL_DestroyAudioStream(stream);
            stream = nullptr;
        }
        if (internal_buffer) {
            std::free(internal_buffer);
            internal_buffer = nullptr;
        }
    }

    void UpdateVolumeIfChanged() {
        const u64 current_time = Kernel::sceKernelGetProcessTime();

        // Throttle volume checks to reduce overhead
        if (current_time - last_volume_check_time < VOLUME_CHECK_INTERVAL_US) {
            return;
        }

        last_volume_check_time = current_time;

        const float config_volume = Config::getVolumeSlider() / 100.0f;
        const float stored_gain = current_gain.load(std::memory_order_relaxed);

        // Only update if the difference is significant
        if (std::abs(config_volume - stored_gain) > VOLUME_EPSILON) {
            std::lock_guard<std::mutex> lock(volume_mutex);
            if (SDL_SetAudioStreamGain(stream, config_volume)) {
                current_gain.store(config_volume, std::memory_order_relaxed);
                LOG_DEBUG(Lib_AudioOut, "Updated audio gain to {:.3f}", config_volume);
            } else {
                LOG_ERROR(Lib_AudioOut, "Failed to set audio stream gain: {}", SDL_GetError());
            }
        }
    }

    void HandleTiming(u64 current_time) {
        if (next_output_time == 0) {
            // First output - set initial timing
            next_output_time = current_time + period_us;
        } else if (current_time > next_output_time) {
            // We're behind schedule - resync
            next_output_time = current_time + period_us;
        } else {
            // Wait for the next scheduled output time
            const u64 wait_until = next_output_time;
            next_output_time += period_us;

            const u64 time_to_wait = wait_until - current_time;
            if (time_to_wait > MIN_SLEEP_THRESHOLD_US) {
                // Sleep for most of the wait period, leaving a small margin
                const u64 sleep_duration = time_to_wait - MIN_SLEEP_THRESHOLD_US;
                std::this_thread::sleep_for(std::chrono::microseconds(sleep_duration));
            }
        }
    }

    void ManageAudioQueue() {
        const auto queued = SDL_GetAudioStreamQueued(stream);

        if (queued >= queue_threshold) {
            LOG_DEBUG(Lib_AudioOut, "Clearing backed up audio queue ({} >= {})", queued,
                      queue_threshold);
            SDL_ClearAudioStream(stream);
            CalculateQueueThreshold();
        }
    }

    bool OpenDevice(OrbisAudioOutPort type) {
        const SDL_AudioSpec fmt = {
            .format = SDL_AUDIO_F32LE,
            .channels = static_cast<u8>(num_channels),
            .freq = static_cast<int>(sample_rate),
        };

        // Determine device
        const std::string device_name = GetDeviceName(type);
        const SDL_AudioDeviceID dev_id = SelectAudioDevice(device_name, type);

        if (dev_id == SDL_INVALID_AUDIODEVICEID) {
            return false;
        }

        // Create audio stream
        stream = SDL_OpenAudioDeviceStream(dev_id, &fmt, nullptr, nullptr);
        if (!stream) {
            LOG_ERROR(Lib_AudioOut, "Failed to create SDL audio stream: {}", SDL_GetError());
            return false;
        }

        // Configure channel mapping
        if (!ConfigureChannelMap()) {
            SDL_DestroyAudioStream(stream);
            stream = nullptr;
            return false;
        }

        // Set initial volume
        const float initial_gain = current_gain.load(std::memory_order_relaxed);
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

    SDL_AudioDeviceID SelectAudioDevice(const std::string& device_name, OrbisAudioOutPort type) {
        if (device_name == "None") {
            LOG_INFO(Lib_AudioOut, "Audio device disabled for port type {}",
                     static_cast<int>(type));
            return SDL_INVALID_AUDIODEVICEID;
        }

        if (device_name.empty() || device_name == "Default Device") {
            return SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
        }

        // Search for specific device
        int num_devices = 0;
        SDL_AudioDeviceID* dev_array = SDL_GetAudioPlaybackDevices(&num_devices);

        if (!dev_array) {
            LOG_WARNING(Lib_AudioOut, "No audio devices found, using default");
            return SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
        }

        SDL_AudioDeviceID selected_device = SDL_INVALID_AUDIODEVICEID;

        for (int i = 0; i < num_devices; i++) {
            const char* dev_name = SDL_GetAudioDeviceName(dev_array[i]);
            if (dev_name && device_name == dev_name) {
                selected_device = dev_array[i];
                break;
            }
        }

        SDL_free(dev_array);

        if (selected_device == SDL_INVALID_AUDIODEVICEID) {
            LOG_WARNING(Lib_AudioOut, "Audio device '{}' not found, using default", device_name);
            return SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
        }

        return selected_device;
    }

    bool ConfigureChannelMap() {
        if (num_channels == 0) {
            return true;
        }

        std::vector<int> channel_map(num_channels);

        if (is_std && num_channels == 8) {
            // Standard 8CH layout requires remapping
            channel_map = {FL, FR, FC, LF, STD_SL, STD_SR, STD_BL, STD_BR};
        } else {
            // Use provided channel layout
            for (u32 i = 0; i < num_channels; i++) {
                channel_map[i] = channel_layout[i];
            }
        }

        if (!SDL_SetAudioStreamInputChannelMap(stream, channel_map.data(), num_channels)) {
            LOG_ERROR(Lib_AudioOut, "Failed to set channel map: {}", SDL_GetError());
            return false;
        }

        return true;
    }

    std::string GetDeviceName(OrbisAudioOutPort type) const {
        switch (type) {
        case OrbisAudioOutPort::Main:
        case OrbisAudioOutPort::Bgm:
            return Config::getMainOutputDevice();
        case OrbisAudioOutPort::PadSpk:
            return Config::getPadSpkOutputDevice();
        // Commented out cases for future implementation:
        // case OrbisAudioOutPort::Voice:
        // case OrbisAudioOutPort::Personal:
        //     return Config::getHeadphoneOutputDevice();
        // case OrbisAudioOutPort::Aux:
        //     return Config::getSpecialOutputDevice();
        default:
            return Config::getMainOutputDevice();
        }
    }

    bool SelectConverter() {
        if (is_float) {
            switch (num_channels) {
            case 1:
                convert = &ConvertF32Mono;
                break;
            case 2:
                convert = &ConvertF32Stereo;
                break;
            case 8:
                convert = is_std ? &ConvertF32Std8CH : &ConvertF32_8CH;
                break;
            default:
                LOG_ERROR(Lib_AudioOut, "Unsupported float channel count: {}", num_channels);
                return false;
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
                return false;
            }
        }

        return true;
    }

    void CalculateQueueThreshold() {
        if (!stream) {
            return;
        }

        SDL_AudioSpec discard;
        int sdl_buffer_frames = 0;

        if (!SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(stream), &discard,
                                      &sdl_buffer_frames)) {
            LOG_WARNING(Lib_AudioOut, "Failed to get SDL buffer size: {}", SDL_GetError());
        }

        const u32 sdl_buffer_size = sdl_buffer_frames * sizeof(float) * num_channels;
        queue_threshold = std::max(guest_buffer_size, sdl_buffer_size) * 4;

        LOG_DEBUG(Lib_AudioOut, "Audio queue threshold: {} bytes (SDL buffer: {} frames)",
                  queue_threshold, sdl_buffer_frames);
    }

    // Converter function type
    using ConverterFunc = void (*)(const void* src, void* dst, u32 frames, const float* volumes);

    // S16 converters - convert to float and normalize
    static void ConvertS16Mono(const void* src, void* dst, u32 frames, const float*) {
        const s16* s = static_cast<const s16*>(src);
        float* d = static_cast<float*>(dst);

        for (u32 i = 0; i < frames; i++) {
            d[i] = s[i] * INV_VOLUME_0DB;
        }
    }

    static void ConvertS16Stereo(const void* src, void* dst, u32 frames, const float*) {
        const s16* s = static_cast<const s16*>(src);
        float* d = static_cast<float*>(dst);

        const u32 num_samples = frames * 2;
        for (u32 i = 0; i < num_samples; i++) {
            d[i] = s[i] * INV_VOLUME_0DB;
        }
    }

    static void ConvertS16_8CH(const void* src, void* dst, u32 frames, const float*) {
        const s16* s = static_cast<const s16*>(src);
        float* d = static_cast<float*>(dst);

        const u32 num_samples = frames * 8;
        for (u32 i = 0; i < num_samples; i++) {
            d[i] = s[i] * INV_VOLUME_0DB;
        }
    }

    // Float converters - passthrough (no conversion needed)
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
            const u32 src_offset = i * 8;
            const u32 dst_offset = i * 8;

            d[dst_offset + FL] = s[src_offset + FL];
            d[dst_offset + FR] = s[src_offset + FR];
            d[dst_offset + FC] = s[src_offset + FC];
            d[dst_offset + LF] = s[src_offset + LF];
            d[dst_offset + SL] = s[src_offset + STD_SL];
            d[dst_offset + SR] = s[src_offset + STD_SR];
            d[dst_offset + BL] = s[src_offset + STD_BL];
            d[dst_offset + BR] = s[src_offset + STD_BR];
        }
    }

    // Audio format parameters
    const u32 frame_size;
    const u32 guest_buffer_size;
    const u32 buffer_frames;
    const u32 sample_rate;
    const u32 num_channels;
    const bool is_float;
    const bool is_std;
    const std::array<int, 8> channel_layout;

    // Timing
    u64 period_us{0};
    std::atomic<u64> last_output_time{0};
    u64 next_output_time{0};
    u64 last_volume_check_time{0};

    // Buffers
    u32 internal_buffer_size{0};
    void* internal_buffer{nullptr};

    // Converter function pointer
    ConverterFunc convert{nullptr};

    // Volume management
    std::atomic<float> current_gain{1.0f};
    mutable std::mutex volume_mutex;

    // SDL audio stream
    SDL_AudioStream* stream{nullptr};
    u32 queue_threshold{0};
};

std::unique_ptr<PortBackend> SDLAudioOut::Open(PortOut& port) {
    return std::make_unique<SDLPortBackend>(port);
}

} // namespace Libraries::AudioOut
