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
constexpr ALsizei NUM_BUFFERS = 6;
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
        if (!source || !convert) [[unlikely]] {
            return;
        }
        if (ptr == nullptr) [[unlikely]] {
            return;
        }
        if (!device_context->MakeCurrent()) {
            return;
        }

        UpdateVolumeIfChanged();
        const u64 current_time = Kernel::sceKernelGetProcessTime();

        // Convert audio data ONCE per call
        if (use_native_float) {
            convert(ptr, al_buffer_float.data(), buffer_frames, nullptr);
        } else {
            convert(ptr, al_buffer_s16.data(), buffer_frames, nullptr);
        }

        // Reclaim processed buffers
        ALint processed = 0;
        alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

        while (processed > 0) {
            ALuint buffer_id;
            alSourceUnqueueBuffers(source, 1, &buffer_id);
            if (alGetError() == AL_NO_ERROR) {
                available_buffers.push_back(buffer_id);
                processed--;
            } else {
                break;
            }
        }

        // Queue buffer
        if (!available_buffers.empty()) {
            ALuint buffer_id = available_buffers.back();
            available_buffers.pop_back();

            if (use_native_float) {
                alBufferData(buffer_id, format, al_buffer_float.data(), buffer_size_bytes,
                             sample_rate);
            } else {
                alBufferData(buffer_id, format, al_buffer_s16.data(), buffer_size_bytes,
                             sample_rate);
            }
            alSourceQueueBuffers(source, 1, &buffer_id);
        }

        // Check state and queue health
        ALint state = 0;
        ALint queued = 0;
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);

        if (state != AL_PLAYING && queued > 0) {
            LOG_DEBUG(Lib_AudioOut, "Audio underrun detected (queued: {}), restarting source",
                      queued);
            alSourcePlay(source);
        }

        // Only sleep if we have healthy buffer queue
        if (queued >= 2) {
            HandleTiming(current_time);
        } else {
            next_output_time = current_time + period_us;
        }

        last_output_time.store(current_time, std::memory_order_release);
        output_count++;
    }
    void SetVolume(const std::array<int, 8>& ch_volumes) override {
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

        const float current = current_gain.load(std::memory_order_acquire);
        if (std::abs(total_gain - current) < VOLUME_EPSILON) {
            return;
        }

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
        const std::string device_name = GetDeviceName(type);

        if (!OpenALDevice::GetInstance().SelectDevice(device_name)) {
            if (device_name == "None") {
                LOG_INFO(Lib_AudioOut, "Audio device disabled for port type {}",
                         static_cast<int>(type));
            } else {
                LOG_ERROR(Lib_AudioOut, "Failed to open OpenAL device '{}'", device_name);
            }
            return false;
        }

        device_context = &OpenALDevice::GetInstance();

        if (!device_context->MakeCurrent()) {
            LOG_ERROR(Lib_AudioOut, "Failed to make OpenAL context current");
            return false;
        }

        // Calculate timing parameters
        period_us = (1000000ULL * buffer_frames + sample_rate / 2) / sample_rate;

        // Check for AL_EXT_FLOAT32 extension
        has_float_ext = alIsExtensionPresent("AL_EXT_FLOAT32");
        if (has_float_ext && is_float) {
            LOG_INFO(Lib_AudioOut, "AL_EXT_FLOAT32 extension detected - using native float format");
        }

        // Determine OpenAL format
        if (!DetermineOpenALFormat()) {
            LOG_ERROR(Lib_AudioOut, "Unsupported audio format for OpenAL");
            return false;
        }

        // Allocate buffers based on format
        if (use_native_float) {
            al_buffer_float.resize(buffer_frames * num_channels);
            buffer_size_bytes = buffer_frames * num_channels * sizeof(float);
        } else {
            al_buffer_s16.resize(buffer_frames * num_channels);
            buffer_size_bytes = buffer_frames * num_channels * sizeof(s16);
        }

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
        alSourcef(source, AL_GAIN, current_gain.load(std::memory_order_relaxed));

        // Prime buffers with silence
        if (use_native_float) {
            std::vector<float> silence(buffer_frames * num_channels, 0.0f);
            for (size_t i = 0; i < buffers.size() - 1; i++) {
                ALuint buffer_id = available_buffers.back();
                available_buffers.pop_back();
                alBufferData(buffer_id, format, silence.data(), buffer_size_bytes, sample_rate);
                alSourceQueueBuffers(source, 1, &buffer_id);
            }
        } else {
            std::vector<s16> silence(buffer_frames * num_channels, 0);
            for (size_t i = 0; i < buffers.size() - 1; i++) {
                ALuint buffer_id = available_buffers.back();
                available_buffers.pop_back();
                alBufferData(buffer_id, format, silence.data(), buffer_size_bytes, sample_rate);
                alSourceQueueBuffers(source, 1, &buffer_id);
            }
        }

        alSourcePlay(source);

        LOG_INFO(Lib_AudioOut, "Initialized OpenAL backend ({} Hz, {} ch, {} format, {})",
                 sample_rate, num_channels, is_float ? "float" : "int16",
                 use_native_float ? "native" : "converted");
        return true;
    }

    std::string GetDeviceName(OrbisAudioOutPort type) const {
        switch (type) {
        case OrbisAudioOutPort::Main:
        case OrbisAudioOutPort::Bgm:
            return Config::getMainOutputDevice();
        case OrbisAudioOutPort::PadSpk:
            return Config::getPadSpkOutputDevice();
        default:
            return Config::getMainOutputDevice();
        }
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
            next_output_time = current_time + period_us;
            return;
        }

        const s64 time_diff = static_cast<s64>(current_time - next_output_time);

        if (time_diff > static_cast<s64>(TIMING_RESYNC_THRESHOLD_US)) [[unlikely]] {
            next_output_time = current_time + period_us;
        } else if (time_diff < 0) {
            const u64 time_to_wait = static_cast<u64>(-time_diff);
            next_output_time += period_us;

            if (time_to_wait > MIN_SLEEP_THRESHOLD_US) {
                const u64 sleep_duration = time_to_wait - MIN_SLEEP_THRESHOLD_US;
                std::this_thread::sleep_for(std::chrono::microseconds(sleep_duration));
            }
        } else {
            next_output_time += period_us;
        }
    }

    bool DetermineOpenALFormat() {
        // Try to use native float formats if extension is available
        if (is_float && has_float_ext) {
            switch (num_channels) {
            case 1:
                format = AL_FORMAT_MONO_FLOAT32;
                use_native_float = true;
                return true;
            case 2:
                format = AL_FORMAT_STEREO_FLOAT32;
                use_native_float = true;
                return true;
            case 4:
                format = alGetEnumValue("AL_FORMAT_QUAD32");
                if (format != 0 && alGetError() == AL_NO_ERROR) {
                    use_native_float = true;
                    return true;
                }
                break;
            case 6:
                format = alGetEnumValue("AL_FORMAT_51CHN32");
                if (format != 0 && alGetError() == AL_NO_ERROR) {
                    use_native_float = true;
                    return true;
                }
                break;
            case 8:
                format = alGetEnumValue("AL_FORMAT_71CHN32");
                if (format != 0 && alGetError() == AL_NO_ERROR) {
                    use_native_float = true;
                    return true;
                }
                break;
            }

            LOG_WARNING(
                Lib_AudioOut,
                "Float format for {} channels not supported, falling back to S16 conversion",
                num_channels);
        }

        // Fall back to S16 formats (with conversion if needed)
        use_native_float = false;

        if (is_float) {
            // Will need to convert float to S16
            format = AL_FORMAT_MONO16;

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
            // Native 16-bit integer formats
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
        alGenSources(1, &source);
        if (alGetError() != AL_NO_ERROR) {
            LOG_ERROR(Lib_AudioOut, "Failed to generate OpenAL source");
            return false;
        }

        buffers.resize(NUM_BUFFERS);
        alGenBuffers(static_cast<ALsizei>(buffers.size()), buffers.data());
        if (alGetError() != AL_NO_ERROR) {
            LOG_ERROR(Lib_AudioOut, "Failed to generate OpenAL buffers");
            alDeleteSources(1, &source);
            source = 0;
            return false;
        }

        available_buffers = buffers;

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
        if (is_float && use_native_float) {
            // Native float - just copy/remap if needed
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
        } else if (is_float && !use_native_float) {
            // Float to S16 conversion needed
            switch (num_channels) {
            case 1:
                convert = &ConvertF32ToS16Mono;
                break;
            case 2:
#ifdef HAS_SSE2
                convert = &ConvertF32ToS16StereoSIMD;
#else
                convert = &ConvertF32ToS16Stereo;
#endif
                break;
            case 8:
#ifdef HAS_SSE2
                convert = is_std ? &ConvertF32ToS16Std8CH : &ConvertF32ToS16_8CH_SIMD;
#else
                convert = is_std ? &ConvertF32ToS16Std8CH : &ConvertF32ToS16_8CH;
#endif
                break;
            default:
                LOG_ERROR(Lib_AudioOut, "Unsupported float channel count: {}", num_channels);
                return false;
            }
        } else {
            // S16 native - just copy
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

    static inline s16 OrbisFloatToS16(float v) {
        if (std::abs(v) < 1.0e-20f)
            v = 0.0f;

        // Sony behavior: +1.0f -> 32767, -1.0f -> -32768
        const float scaled = v * 32768.0f;

        if (scaled >= 32767.0f)
            return 32767;
        if (scaled <= -32768.0f)
            return -32768;

        return static_cast<s16>(scaled + (scaled >= 0 ? 0.5f : -0.5f));
    }
    static void ConvertS16Mono(const void* src, void* dst, u32 frames, const float*) {
        const s16* s = static_cast<const s16*>(src);
        s16* d = static_cast<s16*>(dst);
        std::memcpy(d, s, frames * sizeof(s16));
    }

    static void ConvertS16Stereo(const void* src, void* dst, u32 frames, const float*) {
        const s16* s = static_cast<const s16*>(src);
        s16* d = static_cast<s16*>(dst);

        const u32 num_samples = frames << 1;
        std::memcpy(d, s, num_samples * sizeof(s16));
    }

    static void ConvertS16_8CH(const void* src, void* dst, u32 frames, const float*) {
        const s16* s = static_cast<const s16*>(src);
        s16* d = static_cast<s16*>(dst);

        const u32 num_samples = frames << 3;
        std::memcpy(d, s, num_samples * sizeof(s16));
    }

    // Float passthrough converters (for AL_EXT_FLOAT32)
    static void ConvertF32Mono(const void* src, void* dst, u32 frames, const float*) {
        const float* s = static_cast<const float*>(src);
        float* d = static_cast<float*>(dst);
        std::memcpy(d, s, frames * sizeof(float));
    }

    static void ConvertF32Stereo(const void* src, void* dst, u32 frames, const float*) {
        const float* s = static_cast<const float*>(src);
        float* d = static_cast<float*>(dst);
        std::memcpy(d, s, frames * 2 * sizeof(float));
    }

    static void ConvertF32_8CH(const void* src, void* dst, u32 frames, const float*) {
        const float* s = static_cast<const float*>(src);
        float* d = static_cast<float*>(dst);
        std::memcpy(d, s, frames * 8 * sizeof(float));
    }

    static void ConvertF32Std8CH(const void* src, void* dst, u32 frames, const float*) {
        const float* s = static_cast<const float*>(src);
        float* d = static_cast<float*>(dst);

        for (u32 i = 0; i < frames; i++) {
            const u32 offset = i << 3;
            d[offset + FL] = s[offset + FL];
            d[offset + FR] = s[offset + FR];
            d[offset + FC] = s[offset + FC];
            d[offset + LF] = s[offset + LF];
            d[offset + SL] = s[offset + STD_SL];
            d[offset + SR] = s[offset + STD_SR];
            d[offset + BL] = s[offset + STD_BL];
            d[offset + BR] = s[offset + STD_BR];
        }
    }

    // Float to S16 converters for OpenAL
    static void ConvertF32ToS16Mono(const void* src, void* dst, u32 frames, const float*) {
        const float* s = static_cast<const float*>(src);
        s16* d = static_cast<s16*>(dst);

        for (u32 i = 0; i < frames; i++)
            d[i] = OrbisFloatToS16(s[i]);
    }
#ifdef HAS_SSE2
    static void ConvertF32ToS16StereoSIMD(const void* src, void* dst, u32 frames, const float*) {
        const float* s = static_cast<const float*>(src);
        s16* d = static_cast<s16*>(dst);

        const __m128 scale = _mm_set1_ps(32768.0f);
        const __m128 min_val = _mm_set1_ps(-32768.0f);
        const __m128 max_val = _mm_set1_ps(32767.0f);

        const u32 num_samples = frames << 1;
        u32 i = 0;

        // Process 8 samples at a time
        for (; i + 8 <= num_samples; i += 8) {
            // Load 8 floats
            __m128 f1 = _mm_loadu_ps(&s[i]);
            __m128 f2 = _mm_loadu_ps(&s[i + 4]);

            // Scale and clamp
            f1 = _mm_mul_ps(f1, scale);
            f2 = _mm_mul_ps(f2, scale);
            f1 = _mm_max_ps(f1, min_val);
            f2 = _mm_max_ps(f2, min_val);
            f1 = _mm_min_ps(f1, max_val);
            f2 = _mm_min_ps(f2, max_val);

            // Convert to int32
            __m128i i1 = _mm_cvtps_epi32(f1);
            __m128i i2 = _mm_cvtps_epi32(f2);

            // Pack to int16
            __m128i packed = _mm_packs_epi32(i1, i2);

            // Store
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&d[i]), packed);
        }

        // Handle remaining samples
        for (; i < num_samples; i++) {
            d[i] = OrbisFloatToS16(s[i]);
        }
    }
#elif
    static void ConvertF32ToS16Stereo(const void* src, void* dst, u32 frames, const float*) {
        const float* s = static_cast<const float*>(src);
        s16* d = static_cast<s16*>(dst);

        const u32 num_samples = frames << 1;
        for (u32 i = 0; i < num_samples; i++)
            d[i] = OrbisFloatToS16(s[i]);
    }
#endif

#ifdef HAS_SSE2
    static void ConvertF32ToS16_8CH_SIMD(const void* src, void* dst, u32 frames, const float*) {
        const float* s = static_cast<const float*>(src);
        s16* d = static_cast<s16*>(dst);

        const __m128 scale = _mm_set1_ps(32768.0f);
        const __m128 min_val = _mm_set1_ps(-32768.0f);
        const __m128 max_val = _mm_set1_ps(32767.0f);

        const u32 num_samples = frames << 3;
        u32 i = 0;

        // Process 8 samples at a time (1 frame of 8CH audio)
        for (; i + 8 <= num_samples; i += 8) {
            __m128 f1 = _mm_loadu_ps(&s[i]);
            __m128 f2 = _mm_loadu_ps(&s[i + 4]);

            f1 = _mm_mul_ps(f1, scale);
            f2 = _mm_mul_ps(f2, scale);
            f1 = _mm_max_ps(_mm_min_ps(f1, max_val), min_val);
            f2 = _mm_max_ps(_mm_min_ps(f2, max_val), min_val);

            __m128i i1 = _mm_cvtps_epi32(f1);
            __m128i i2 = _mm_cvtps_epi32(f2);
            __m128i packed = _mm_packs_epi32(i1, i2);

            _mm_storeu_si128(reinterpret_cast<__m128i*>(&d[i]), packed);
        }

        for (; i < num_samples; i++) {
            d[i] = OrbisFloatToS16(s[i]);
        }
    }
#elif
    static void ConvertF32ToS16_8CH(const void* src, void* dst, u32 frames, const float*) {
        const float* s = static_cast<const float*>(src);
        s16* d = static_cast<s16*>(dst);

        const u32 num_samples = frames << 3;
        for (u32 i = 0; i < num_samples; i++)
            d[i] = OrbisFloatToS16(s[i]);
    }
#endif
    static void ConvertF32ToS16Std8CH(const void* src, void* dst, u32 frames, const float*) {
        const float* s = static_cast<const float*>(src);
        s16* d = static_cast<s16*>(dst);

        for (u32 i = 0; i < frames; i++) {
            const u32 offset = i << 3;

            d[offset + FL] = OrbisFloatToS16(s[offset + FL]);
            d[offset + FR] = OrbisFloatToS16(s[offset + FR]);
            d[offset + FC] = OrbisFloatToS16(s[offset + FC]);
            d[offset + LF] = OrbisFloatToS16(s[offset + LF]);
            d[offset + SL] = OrbisFloatToS16(s[offset + STD_SL]);
            d[offset + SR] = OrbisFloatToS16(s[offset + STD_SR]);
            d[offset + BL] = OrbisFloatToS16(s[offset + STD_BL]);
            d[offset + BR] = OrbisFloatToS16(s[offset + STD_BR]);
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
    std::vector<s16> al_buffer_s16;     // For S16 formats
    std::vector<float> al_buffer_float; // For float formats

    // Extension support
    bool has_float_ext{false};
    bool use_native_float{false};

    // Converter function pointer
    ConverterFunc convert{nullptr};

    // Volume management
    alignas(64) std::atomic<float> current_gain{1.0f};
};

std::unique_ptr<PortBackend> OpenALAudioOut::Open(PortOut& port) {
    return std::make_unique<OpenALPortBackend>(port);
}

} // namespace Libraries::AudioOut