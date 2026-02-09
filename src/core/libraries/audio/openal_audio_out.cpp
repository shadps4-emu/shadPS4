// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <atomic>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>
#include <AL/al.h>
#include <AL/alc.h>
#include <alext.h>
#include <queue>

#include "common/config.h"
#include "common/logging/log.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio/audioout_backend.h"
#include "core/libraries/audio/openal_manager.h"
#include "core/libraries/kernel/threads.h"

// SIMD support detection
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define HAS_SSE2
#endif

namespace Libraries::AudioOut {

// Volume constants
constexpr float VOLUME_0DB = 32768.0f; // 1 << 15
constexpr float INV_VOLUME_0DB = 1.0f / VOLUME_0DB;
constexpr float VOLUME_EPSILON = 0.001f;
// Timing constants
constexpr u64 VOLUME_CHECK_INTERVAL_US = 50000; // Check every 50ms
constexpr u64 MIN_SLEEP_THRESHOLD_US = 10;
constexpr u64 TIMING_RESYNC_THRESHOLD_US = 100000; // Resync if >100ms behind

// OpenAL constants
constexpr ALsizei NUM_BUFFERS = 4;            // Triple buffering
constexpr ALsizei BUFFER_QUEUE_THRESHOLD = 2; // Queue more buffers when below this

// Channel positions
enum ChannelPos : u8 {
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

class OpenALPortBackend : public PortBackend {
public:
    explicit OpenALPortBackend(const PortOut& port)
        : frame_size(port.format_info.FrameSize()), guest_buffer_size(port.BufferSize()),
          buffer_frames(port.buffer_frames), sample_rate(port.sample_rate),
          num_channels(port.format_info.num_channels), is_float(port.format_info.is_float),
          is_std(port.format_info.is_std), channel_layout(port.format_info.channel_layout) {

        if (!Initialize(port.type)) {
            LOG_ERROR(Lib_AudioOut, "Failed to initialize OpenAL audio backend");
        }
    }

    ~OpenALPortBackend() override {
        Cleanup();
    }

    void Output(void* ptr) override {
        if (!source || buffers.empty() || !convert) [[unlikely]] {
            return;
        }

        if (ptr == nullptr) [[unlikely]] {
            return;
        }

        // Make context current before any OpenAL operations
        if (!device_context->MakeCurrent()) {
            return;
        }

        UpdateVolumeIfChanged();
        const u64 current_time = Kernel::sceKernelGetProcessTime();

        // Process audio data
        convert(ptr, al_buffer.data(), buffer_frames, nullptr);
        HandleTiming(current_time);

        // Manage buffer queue
        while (!available_buffers.empty() && available_buffers.size() < NUM_BUFFERS) {
            ALint processed = 0;
            alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

            if (processed <= 0)
                break;

            ALuint buffer_id;
            alSourceUnqueueBuffers(source, 1, &buffer_id); // One at a time

            if (alGetError() != AL_NO_ERROR) {
                break; // Stop on error
            }

            available_buffers.push_back(buffer_id);
        }

        // Check if we need to queue more buffers
        ALint queued = 0;
        alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);

        if (queued < BUFFER_QUEUE_THRESHOLD && !available_buffers.empty()) {
            ALuint buffer_id = available_buffers.back();
            available_buffers.pop_back();

            alBufferData(buffer_id, format, al_buffer.data(), buffer_size_bytes, sample_rate);

            alSourceQueueBuffers(source, 1, &buffer_id);
            queued++;
        }
        // Ensure the source is playing (OpenAL does NOT auto-start)
        ALint state = 0;
        alGetSourcei(source, AL_SOURCE_STATE, &state);

        if (state != AL_PLAYING && queued > 0) {
            alSourcePlay(source);
        }
        // Update timing
        last_output_time.store(current_time, std::memory_order_release);
        output_count++;
    }

    void SetVolume(const std::array<int, 8>& ch_volumes) override {
        // Make context current before any OpenAL operations
        if (!device_context->MakeCurrent()) {
            return;
        }

        if (!source) [[unlikely]] {
            return;
        }

        float max_channel_gain = 0.0f;
        const u32 channels_to_check = std::min(num_channels, 8u);

        for (u32 i = 0; i < channels_to_check; i++) {
            const float channel_gain = static_cast<float>(ch_volumes[i]) * INV_VOLUME_0DB;
            max_channel_gain = std::max(max_channel_gain, channel_gain);
        }

        const float slider_gain = Config::getVolumeSlider() * 0.01f;
        const float total_gain = max_channel_gain * slider_gain;

        // Only update if changed significantly
        const float current = current_gain.load(std::memory_order_acquire);
        if (std::abs(total_gain - current) < VOLUME_EPSILON) {
            return;
        }

        // Apply volume change to OpenAL source
        alSourcef(source, AL_GAIN, total_gain);

        ALenum error = alGetError();
        if (error == AL_NO_ERROR) {
            current_gain.store(total_gain, std::memory_order_release);
            LOG_DEBUG(Lib_AudioOut,
                      "Set combined audio gain to {:.3f} (channel: {:.3f}, slider: {:.3f})",
                      total_gain, max_channel_gain, slider_gain);
        } else {
            LOG_ERROR(Lib_AudioOut, "Failed to set OpenAL source gain: {}",
                      GetALErrorString(error));
        }
    }

    u64 GetLastOutputTime() const {
        return last_output_time.load(std::memory_order_acquire);
    }

private:
    bool Initialize(OrbisAudioOutPort type) {
        // Get OpenAL device and context
        if (!OpenALDevice::GetInstance().IsInitialized()) {
            LOG_ERROR(Lib_AudioOut, "OpenAL device not initialized");
            return false;
        }

        device_context = &OpenALDevice::GetInstance();

        // Make context current for initialization
        if (!device_context->MakeCurrent()) {
            LOG_ERROR(Lib_AudioOut, "Failed to make OpenAL context current");
            return false;
        }

        // Calculate timing parameters
        period_us = (1000000ULL * buffer_frames + sample_rate / 2) / sample_rate;

        // Determine OpenAL format
        if (!DetermineOpenALFormat()) {
            LOG_ERROR(Lib_AudioOut, "Unsupported audio format for OpenAL");
            return false;
        }

        // Calculate buffer size
        buffer_size_bytes = buffer_frames * frame_size;

        // Allocate current buffer
        current_buffer.resize(buffer_frames * num_channels); // float staging
        al_buffer.resize(buffer_frames * num_channels);      // int16 upload
        buffer_size_bytes = buffer_frames * num_channels * sizeof(s16);

        // Select optimal converter function
        if (!SelectConverter()) {
            return false;
        }

        // Generate OpenAL source and buffers
        if (!CreateOpenALObjects()) {
            return false;
        }

        // Initialize current gain
        current_gain.store(Config::getVolumeSlider() * 0.01f, std::memory_order_relaxed);

        // Apply initial volume
        alSourcef(source, AL_GAIN, current_gain.load(std::memory_order_relaxed));

        LOG_INFO(Lib_AudioOut, "Initialized OpenAL backend ({} Hz, {} ch, {} format)", sample_rate,
                 num_channels, is_float ? "float" : "int16");

        return true;
    }

    void Cleanup() {
        if (!device_context->MakeCurrent()) {
            return;
        }

        if (source) {
            alSourceStop(source);

            ALint queued = 0;
            alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
            while (queued-- > 0) {
                ALuint buf;
                alSourceUnqueueBuffers(source, 1, &buf);
            }

            alDeleteSources(1, &source);
            source = 0;
        }

        if (!buffers.empty()) {
            alDeleteBuffers(static_cast<ALsizei>(buffers.size()), buffers.data());
            buffers.clear();
        }
    }

    void UpdateVolumeIfChanged() {
        const u64 current_time = Kernel::sceKernelGetProcessTime();

        if (current_time - last_volume_check_time < VOLUME_CHECK_INTERVAL_US) {
            return;
        }

        last_volume_check_time = current_time;

        const float config_volume = Config::getVolumeSlider() * 0.01f;
        const float stored_gain = current_gain.load(std::memory_order_acquire);

        // Only update if the difference is significant
        if (std::abs(config_volume - stored_gain) > VOLUME_EPSILON) {
            alSourcef(source, AL_GAIN, config_volume);

            ALenum error = alGetError();
            if (error == AL_NO_ERROR) {
                current_gain.store(config_volume, std::memory_order_release);
                LOG_DEBUG(Lib_AudioOut, "Updated audio gain to {:.3f}", config_volume);
            } else {
                LOG_ERROR(Lib_AudioOut, "Failed to set audio gain: {}", GetALErrorString(error));
            }
        }
    }

    void HandleTiming(u64 current_time) {
        if (next_output_time == 0) [[unlikely]] {
            // First output - set initial timing
            next_output_time = current_time + period_us;
            return;
        }

        const s64 time_diff = static_cast<s64>(current_time - next_output_time);

        if (time_diff > static_cast<s64>(TIMING_RESYNC_THRESHOLD_US)) [[unlikely]] {
            // We're far behind - resync
            next_output_time = current_time + period_us;
        } else if (time_diff < 0) {
            // We're ahead of schedule - wait
            const u64 time_to_wait = static_cast<u64>(-time_diff);
            next_output_time += period_us;

            if (time_to_wait > MIN_SLEEP_THRESHOLD_US) {
                // Sleep for most of the wait period
                const u64 sleep_duration = time_to_wait - MIN_SLEEP_THRESHOLD_US;
                std::this_thread::sleep_for(std::chrono::microseconds(sleep_duration));
            }
        } else {
            // Slightly behind or on time - just advance
            next_output_time += period_us;
        }
    }

    bool DetermineOpenALFormat() {
        if (is_float) {
            // OpenAL doesn't natively support float formats, we need to use AL_EXT_FLOAT32
            // extension For simplicity, we'll convert to int16 in our converter functions
            format = AL_FORMAT_MONO16; // Default, will be overridden

            switch (num_channels) {
            case 1:
                format = AL_FORMAT_MONO16;
                break;
            case 2:
                format = AL_FORMAT_STEREO16;
                break;
            case 6:
                format = alGetEnumValue("AL_FORMAT_51CHN16");
                if (format == 0 || alGetError() != AL_NO_ERROR) {
                    LOG_WARNING(Lib_AudioOut, "5.1 format not supported, falling back to stereo");
                    format = AL_FORMAT_STEREO16;
                }
                break;
            case 8:
                format = alGetEnumValue("AL_FORMAT_71CHN16");
                if (format == 0 || alGetError() != AL_NO_ERROR) {
                    LOG_WARNING(Lib_AudioOut, "7.1 format not supported, falling back to stereo");
                    format = AL_FORMAT_STEREO16;
                }
                break;
            default:
                LOG_ERROR(Lib_AudioOut, "Unsupported float channel count: {}", num_channels);
                return false;
            }
        } else {
            // 16-bit integer formats
            switch (num_channels) {
            case 1:
                format = AL_FORMAT_MONO16;
                break;
            case 2:
                format = AL_FORMAT_STEREO16;
                break;
            case 6:
                format = alGetEnumValue("AL_FORMAT_51CHN16");
                if (format == 0 || alGetError() != AL_NO_ERROR) {
                    LOG_WARNING(Lib_AudioOut, "5.1 format not supported, falling back to stereo");
                    format = AL_FORMAT_STEREO16;
                }
                break;
            case 8:
                format = alGetEnumValue("AL_FORMAT_71CHN16");
                if (format == 0 || alGetError() != AL_NO_ERROR) {
                    LOG_WARNING(Lib_AudioOut, "7.1 format not supported, falling back to stereo");
                    format = AL_FORMAT_STEREO16;
                }
                break;
            default:
                LOG_ERROR(Lib_AudioOut, "Unsupported S16 channel count: {}", num_channels);
                return false;
            }
        }

        return true;
    }

    bool CreateOpenALObjects() {
        // Generate source
        alGenSources(1, &source);
        if (alGetError() != AL_NO_ERROR) {
            LOG_ERROR(Lib_AudioOut, "Failed to generate OpenAL source");
            return false;
        }

        // Generate buffers
        buffers.resize(NUM_BUFFERS);
        alGenBuffers(static_cast<ALsizei>(buffers.size()), buffers.data());
        if (alGetError() != AL_NO_ERROR) {
            LOG_ERROR(Lib_AudioOut, "Failed to generate OpenAL buffers");
            alDeleteSources(1, &source);
            source = 0;
            return false;
        }

        // All buffers are initially available
        available_buffers = buffers;

        // Configure source properties
        alSourcef(source, AL_PITCH, 1.0f);
        alSourcef(source, AL_GAIN, 1.0f);
        alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
        alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
        alSourcei(source, AL_LOOPING, AL_FALSE);
        alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);

        LOG_DEBUG(Lib_AudioOut, "Created OpenAL source {} with {} buffers", source, buffers.size());

        return true;
    }

    bool SelectConverter() {
        if (is_float) {
            // For OpenAL, we need to convert float to int16
            switch (num_channels) {
            case 1:
                convert = &ConvertF32ToS16Mono;
                break;
            case 2:
                convert = &ConvertF32ToS16Stereo;
                break;
            case 8:
                convert = is_std ? &ConvertF32ToS16Std8CH : &ConvertF32ToS16_8CH;
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
#if defined(HAS_SSE2)
                convert = &ConvertS16StereoSIMD;
#else
                convert = &ConvertS16Stereo;
#endif
                break;
            case 8:
#if defined(HAS_SSE2)
                convert = &ConvertS16_8CH_SIMD;
#else
                convert = &ConvertS16_8CH;
#endif
                break;
            default:
                LOG_ERROR(Lib_AudioOut, "Unsupported S16 channel count: {}", num_channels);
                return false;
            }
        }

        return true;
    }

    const char* GetALErrorString(ALenum error) {
        switch (error) {
        case AL_NO_ERROR:
            return "AL_NO_ERROR";
        case AL_INVALID_NAME:
            return "AL_INVALID_NAME";
        case AL_INVALID_ENUM:
            return "AL_INVALID_ENUM";
        case AL_INVALID_VALUE:
            return "AL_INVALID_VALUE";
        case AL_INVALID_OPERATION:
            return "AL_INVALID_OPERATION";
        case AL_OUT_OF_MEMORY:
            return "AL_OUT_OF_MEMORY";
        default:
            return "Unknown AL error";
        }
    }

    // Converter function type
    using ConverterFunc = void (*)(const void* src, void* dst, u32 frames, const float* volumes);

    // S16 converters (same as SDL backend)
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

        const u32 num_samples = frames << 1;
        for (u32 i = 0; i < num_samples; i++) {
            d[i] = s[i] * INV_VOLUME_0DB;
        }
    }

#ifdef HAS_SSE2
    static void ConvertS16StereoSIMD(const void* src, void* dst, u32 frames, const float*) {
        const s16* s = static_cast<const s16*>(src);
        float* d = static_cast<float*>(dst);

        const __m128 scale = _mm_set1_ps(INV_VOLUME_0DB);
        const u32 num_samples = frames << 1;
        u32 i = 0;

        for (; i + 8 <= num_samples; i += 8) {
            __m128i s16_vals = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&s[i]));
            __m128i s32_lo = _mm_cvtepi16_epi32(s16_vals);
            __m128i s32_hi = _mm_cvtepi16_epi32(_mm_srli_si128(s16_vals, 8));
            __m128 f_lo = _mm_mul_ps(_mm_cvtepi32_ps(s32_lo), scale);
            __m128 f_hi = _mm_mul_ps(_mm_cvtepi32_ps(s32_hi), scale);
            _mm_storeu_ps(&d[i], f_lo);
            _mm_storeu_ps(&d[i + 4], f_hi);
        }

        for (; i < num_samples; i++) {
            d[i] = s[i] * INV_VOLUME_0DB;
        }
    }
#endif

    static void ConvertS16_8CH(const void* src, void* dst, u32 frames, const float*) {
        const s16* s = static_cast<const s16*>(src);
        float* d = static_cast<float*>(dst);

        const u32 num_samples = frames << 3;
        for (u32 i = 0; i < num_samples; i++) {
            d[i] = s[i] * INV_VOLUME_0DB;
        }
    }

#ifdef HAS_SSE2
    static void ConvertS16_8CH_SIMD(const void* src, void* dst, u32 frames, const float*) {
        const s16* s = static_cast<const s16*>(src);
        float* d = static_cast<float*>(dst);

        const __m128 scale = _mm_set1_ps(INV_VOLUME_0DB);
        const u32 num_samples = frames << 3;
        u32 i = 0;

        for (; i + 8 <= num_samples; i += 8) {
            __m128i s16_vals = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&s[i]));
            __m128i s32_lo = _mm_cvtepi16_epi32(s16_vals);
            __m128i s32_hi = _mm_cvtepi16_epi32(_mm_srli_si128(s16_vals, 8));
            __m128 f_lo = _mm_mul_ps(_mm_cvtepi32_ps(s32_lo), scale);
            __m128 f_hi = _mm_mul_ps(_mm_cvtepi32_ps(s32_hi), scale);
            _mm_storeu_ps(&d[i], f_lo);
            _mm_storeu_ps(&d[i + 4], f_hi);
        }

        for (; i < num_samples; i++) {
            d[i] = s[i] * INV_VOLUME_0DB;
        }
    }
#endif

    // Float to S16 converters for OpenAL
    static void ConvertF32ToS16Mono(const void* src, void* dst, u32 frames, const float*) {
        const float* s = static_cast<const float*>(src);
        s16* d = static_cast<s16*>(dst);

        for (u32 i = 0; i < frames; i++) {
            const float sample = s[i] * VOLUME_0DB;
            d[i] = static_cast<s16>(std::clamp(sample, -32768.0f, 32767.0f));
        }
    }

    static void ConvertF32ToS16Stereo(const void* src, void* dst, u32 frames, const float*) {
        const float* s = static_cast<const float*>(src);
        s16* d = static_cast<s16*>(dst);

        const u32 num_samples = frames << 1;
        for (u32 i = 0; i < num_samples; i++) {
            const float sample = s[i] * VOLUME_0DB;
            d[i] = static_cast<s16>(std::clamp(sample, -32768.0f, 32767.0f));
        }
    }

    static void ConvertF32ToS16_8CH(const void* src, void* dst, u32 frames, const float*) {
        const float* s = static_cast<const float*>(src);
        s16* d = static_cast<s16*>(dst);

        const u32 num_samples = frames << 3;
        for (u32 i = 0; i < num_samples; i++) {
            const float sample = s[i] * VOLUME_0DB;
            d[i] = static_cast<s16>(std::clamp(sample, -32768.0f, 32767.0f));
        }
    }

    static void ConvertF32ToS16Std8CH(const void* src, void* dst, u32 frames, const float*) {
        const float* s = static_cast<const float*>(src);
        s16* d = static_cast<s16*>(dst);

        for (u32 i = 0; i < frames; i++) {
            const u32 offset = i << 3;

            d[offset + FL] =
                static_cast<s16>(std::clamp(s[offset + FL] * VOLUME_0DB, -32768.0f, 32767.0f));
            d[offset + FR] =
                static_cast<s16>(std::clamp(s[offset + FR] * VOLUME_0DB, -32768.0f, 32767.0f));
            d[offset + FC] =
                static_cast<s16>(std::clamp(s[offset + FC] * VOLUME_0DB, -32768.0f, 32767.0f));
            d[offset + LF] =
                static_cast<s16>(std::clamp(s[offset + LF] * VOLUME_0DB, -32768.0f, 32767.0f));
            d[offset + SL] =
                static_cast<s16>(std::clamp(s[offset + STD_SL] * VOLUME_0DB, -32768.0f, 32767.0f));
            d[offset + SR] =
                static_cast<s16>(std::clamp(s[offset + STD_SR] * VOLUME_0DB, -32768.0f, 32767.0f));
            d[offset + BL] =
                static_cast<s16>(std::clamp(s[offset + STD_BL] * VOLUME_0DB, -32768.0f, 32767.0f));
            d[offset + BR] =
                static_cast<s16>(std::clamp(s[offset + STD_BR] * VOLUME_0DB, -32768.0f, 32767.0f));
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

    alignas(64) u64 period_us{0};
    alignas(64) std::atomic<u64> last_output_time{0};
    u64 next_output_time{0};
    u64 last_volume_check_time{0};
    u32 output_count{0};

    // OpenAL objects
    OpenALDevice* device_context{nullptr};
    ALuint source{0};
    std::vector<ALuint> buffers;
    std::vector<ALuint> available_buffers;
    ALenum format{AL_FORMAT_STEREO16};

    // Buffer management
    u32 buffer_size_bytes{0};
    std::vector<float> current_buffer;
    std::vector<s16> al_buffer;

    // Converter function pointer
    ConverterFunc convert{nullptr};

    // Volume management
    alignas(64) std::atomic<float> current_gain{1.0f};
};

std::unique_ptr<PortBackend> OpenALAudioOut::Open(PortOut& port) {
    return std::make_unique<OpenALPortBackend>(port);
}

} // namespace Libraries::AudioOut