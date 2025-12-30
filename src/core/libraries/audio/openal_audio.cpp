// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <ranges>
#include <thread>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include <common/config.h>
#include "audioout.h"
#include "audioout_backend.h"

namespace Libraries::AudioOut {

// ------------------------------------------------------------
// Global OpenAL device/context
// ------------------------------------------------------------
struct OpenALGlobal {
    ALCdevice* device{};
    ALCcontext* context{};
    bool initialized{false};
    std::mutex mutex; // for context switching
};

static OpenALGlobal g_openal;

// Initialize global device/context once
static bool InitGlobalOpenAL(int sample_rate) {
    std::lock_guard<std::mutex> lock(g_openal.mutex);
    if (g_openal.initialized)
        return true;

    g_openal.device = alcOpenDevice(nullptr);
    if (!g_openal.device)
        return false;

    ALCint attrs[] = {ALC_FREQUENCY, sample_rate, ALC_SYNC, ALC_FALSE, 0};
    g_openal.context = alcCreateContext(g_openal.device, attrs);
    if (!g_openal.context)
        g_openal.context = alcCreateContext(g_openal.device, nullptr);

    if (!g_openal.context || !alcMakeContextCurrent(g_openal.context)) {
        alcCloseDevice(g_openal.device);
        g_openal.device = nullptr;
        g_openal.context = nullptr;
        return false;
    }

    g_openal.initialized = true;
    return true;
}

// Shutdown global OpenAL (call at emulator exit)
static void ShutdownGlobalOpenAL() {
    std::lock_guard<std::mutex> lock(g_openal.mutex);
    if (!g_openal.initialized)
        return;

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(g_openal.context);
    alcCloseDevice(g_openal.device);

    g_openal.device = nullptr;
    g_openal.context = nullptr;
    g_openal.initialized = false;
}

// ------------------------------------------------------------
// OpenALPortBackend
// ------------------------------------------------------------
class OpenALPortBackend : public PortBackend {
public:
    explicit OpenALPortBackend(const PortOut& port)
        : frame_size(port.format_info.FrameSize()), guest_buffer_size(port.BufferSize()),
          sample_rate(static_cast<int>(port.sample_rate)), channels(port.format_info.num_channels),
          is_float(port.format_info.is_float) {

        if (!InitGlobalOpenAL(sample_rate)) {
            LOG_ERROR(Lib_AudioOut, "Failed to initialize global OpenAL device");
            return;
        }

        // Select global context for this port
        {
            std::lock_guard<std::mutex> lock(g_openal.mutex);
            alcMakeContextCurrent(g_openal.context);
        }

        // Check for extensions
        has_float32 = alIsExtensionPresent("AL_EXT_float32");
        has_multichannel = alIsExtensionPresent("AL_EXT_MCFORMATS");
        has_direct_channels = alcIsExtensionPresent(g_openal.device, "AL_SOFT_direct_channels");

        // Generate source for this port
        alGenSources(1, &source);
        if (alGetError() != AL_NO_ERROR) {
            LOG_ERROR(Lib_AudioOut, "Failed to create OpenAL source");
            return;
        }
        alSourcef(source, AL_GAIN, 1.0f);
        alSourcei(source, AL_LOOPING, AL_FALSE);
        if (has_direct_channels)
            alSourcei(source, AL_DIRECT_CHANNELS_SOFT, AL_TRUE);

        // Determine AL format
        al_format = GetALFormat(channels, is_float);
        if (!al_format) {
            LOG_WARNING(Lib_AudioOut, "Unsupported 8CH format, will downmix to stereo");
            al_format = GetALFormat(2, true);
            downmix_needed = true;
            stereo_buffer.resize(CalculateStereoByteSize());
        }

        // Buffers
        CalculateBufferCount();
        buffers.resize(buffer_count);
        alGenBuffers(static_cast<ALsizei>(buffer_count), buffers.data());

        bytes_per_buffer = downmix_needed ? CalculateStereoByteSize() : CalculateByteSize();

        std::vector<std::byte> silence(bytes_per_buffer);
        std::memset(silence.data(), 0, silence.size());
        for (ALuint b : buffers)
            alBufferData(b, al_format, silence.data(), static_cast<ALsizei>(silence.size()),
                         sample_rate);

        alSourceQueueBuffers(source, static_cast<ALsizei>(buffers.size()), buffers.data());
        alSourcePlay(source);

        running = true;
        processing_thread = std::thread(&OpenALPortBackend::ProcessBuffers, this);

        LOG_INFO(Lib_AudioOut, "OpenAL port initialized: channels={}, rate={}, buffers={}",
                 channels, sample_rate, buffer_count);
    }

    ~OpenALPortBackend() override {
        running = false;
        buffer_cv.notify_all();
        if (processing_thread.joinable())
            processing_thread.join();

        std::lock_guard<std::mutex> lock(g_openal.mutex);
        if (source) {
            alSourceStop(source);
            alDeleteSources(1, &source);
            if (!buffers.empty())
                alDeleteBuffers(static_cast<ALsizei>(buffers.size()), buffers.data());
        }
    }

    void SetVolume(const std::array<int, 8>& ch_volumes) override {
        if (!source)
            return;

        int max = *std::ranges::max_element(ch_volumes);
        float gain =
            static_cast<float>(max) / SCE_AUDIO_OUT_VOLUME_0DB * Config::getVolumeSlider() / 100.0f;
        alSourcef(source, AL_GAIN, std::clamp(gain, 0.0f, 1.0f));
    }

    void Output(void* ptr) override {
        if (!running)
            return;

        std::vector<std::byte> data(guest_buffer_size);
        std::memcpy(data.data(), ptr, guest_buffer_size);

        if (downmix_needed)
            Downmix8CHToStereo(reinterpret_cast<const float*>(data.data()), stereo_buffer);

        {
            std::lock_guard<std::mutex> lock(buffer_mutex);
            queued_data.emplace_back(downmix_needed ? stereo_buffer : data);
        }
        buffer_cv.notify_one();
    }

private:
    void ProcessBuffers() {
        std::vector<ALuint> free_buffers;
        while (running) {
            {
                std::unique_lock<std::mutex> lock(buffer_mutex);
                buffer_cv.wait_for(lock, std::chrono::milliseconds(5),
                                   [&] { return !queued_data.empty() || !running; });
            }
            if (!running)
                break;

            ALint processed = 0;
            alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
            if (processed > 0) {
                free_buffers.resize(processed);
                alSourceUnqueueBuffers(source, processed, free_buffers.data());
            }

            while (!free_buffers.empty()) {
                ALint queued = 0;
                alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
                if (queued >= static_cast<ALint>(buffer_count))
                    break;

                std::vector<std::byte> data;
                {
                    std::lock_guard<std::mutex> lock(buffer_mutex);
                    if (queued_data.empty())
                        break;
                    data = std::move(queued_data.front());
                    queued_data.pop_front();
                }

                ALuint buffer = free_buffers.back();
                free_buffers.pop_back();
                alBufferData(buffer, al_format, data.data(), static_cast<ALsizei>(data.size()),
                             sample_rate);
                alSourceQueueBuffers(source, 1, &buffer);
            }

            ALint state = 0;
            alGetSourcei(source, AL_SOURCE_STATE, &state);
            if (state != AL_PLAYING)
                alSourcePlay(source);
        }
    }

    ALenum GetALFormat(int ch, bool f32) {
        if (f32 && has_float32) {
            switch (ch) {
            case 1:
                return AL_FORMAT_MONO_FLOAT32;
            case 2:
                return AL_FORMAT_STEREO_FLOAT32;
            case 4:
                return has_multichannel ? AL_FORMAT_QUAD32 : 0;
            case 6:
                return has_multichannel ? AL_FORMAT_51CHN32 : 0;
            case 7:
                return has_multichannel ? AL_FORMAT_61CHN32 : 0;
            case 8:
                return has_multichannel ? AL_FORMAT_71CHN32 : 0;
            default:
                return 0;
            }
        } else {
            switch (ch) {
            case 1:
                return AL_FORMAT_MONO16;
            case 2:
                return AL_FORMAT_STEREO16;
            case 4:
                return has_multichannel ? AL_FORMAT_QUAD16 : 0;
            case 6:
                return has_multichannel ? AL_FORMAT_51CHN16 : 0;
            case 7:
                return has_multichannel ? AL_FORMAT_61CHN16 : 0;
            case 8:
                return has_multichannel ? AL_FORMAT_71CHN16 : 0;
            default:
                return 0;
            }
        }
    }

    uint32_t CalculateByteSize() const {
        uint32_t bytes_per_sample = is_float ? 4 : 2;
        return guest_buffer_size / frame_size * bytes_per_sample * channels;
    }

    uint32_t CalculateStereoByteSize() const {
        uint32_t bytes_per_sample = 4; // float stereo
        return guest_buffer_size / frame_size * bytes_per_sample * 2;
    }

    void CalculateBufferCount() {
        buffer_count = std::clamp<size_t>(12, 12, 16); // adjust as needed
    }

    void Downmix8CHToStereo(const float* src, std::vector<std::byte>& dst) {
        // convert 8CH float audio to stereo
        size_t frames = guest_buffer_size / frame_size;
        float* dst_float = reinterpret_cast<float*>(dst.data());

        for (size_t i = 0; i < frames; i++) {
            dst_float[i * 2 + 0] = 0.0f;
            dst_float[i * 2 + 1] = 0.0f;
            for (int ch = 0; ch < 8; ch++) {
                float sample = reinterpret_cast<const float*>(src)[i * 8 + ch];
                if (ch % 2 == 0)
                    dst_float[i * 2 + 0] += sample * 0.5f; // left
                else
                    dst_float[i * 2 + 1] += sample * 0.5f; // right
            }
        }
    }

private:
    // Port parameters
    size_t frame_size{};
    size_t guest_buffer_size{};
    int sample_rate{};
    int channels{};
    bool is_float{};
    bool downmix_needed{false};

    size_t buffer_count{};
    uint32_t bytes_per_buffer{};
    bool has_float32{};
    bool has_multichannel{};
    bool has_direct_channels{};

    ALuint source{};
    std::vector<ALuint> buffers;
    ALenum al_format{};

    std::atomic<bool> running{false};
    std::thread processing_thread;
    std::mutex buffer_mutex;
    std::condition_variable buffer_cv;
    std::deque<std::vector<std::byte>> queued_data;
    std::vector<std::byte> stereo_buffer;
};
std::unique_ptr<PortBackend> OpenALAudioOut::Open(PortOut& port) {
    return std::make_unique<OpenALPortBackend>(port);
}
} // namespace Libraries::AudioOut
