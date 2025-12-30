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

class OpenALPortBackend final : public PortBackend {
public:
    explicit OpenALPortBackend(const PortOut& port)
        : frame_size(port.format_info.FrameSize()), guest_buffer_size(port.BufferSize()),
          sample_rate(static_cast<int>(port.sample_rate)), channels(port.format_info.num_channels),
          is_float(port.format_info.is_float) {

        std::string port_name = (port.type == OrbisAudioOutPort::PadSpk)
                                    ? Config::getPadSpkOutputDevice()
                                    : Config::getMainOutputDevice();

        if (port_name == "None")
            return;

        // ------------------------------------------------------------
        // Device selection
        // ------------------------------------------------------------
        const ALCchar* device_name = nullptr;

        if (port_name == "Default Device") {
            device_name = alcGetString(nullptr, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
            if (!device_name)
                device_name = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
        } else {
            const ALCchar* list = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
            while (list && *list) {
                if (port_name == list) {
                    device_name = list;
                    break;
                }
                list += std::strlen(list) + 1;
            }
        }

        device = alcOpenDevice(device_name);
        if (!device) {
            LOG_ERROR(Lib_AudioOut, "Failed to open OpenAL device");
            return;
        }

        // ------------------------------------------------------------
        // Context
        // ------------------------------------------------------------
        ALCint attrs[] = {ALC_FREQUENCY, sample_rate, ALC_SYNC, ALC_FALSE, 0};

        context = alcCreateContext(device, attrs);
        if (!context)
            context = alcCreateContext(device, nullptr);

        if (!context || !alcMakeContextCurrent(context)) {
            LOG_ERROR(Lib_AudioOut, "Failed to create OpenAL context");
            Shutdown();
            return;
        }

        ALCint actual_rate{};
        alcGetIntegerv(device, ALC_FREQUENCY, 1, &actual_rate);
        LOG_INFO(Lib_AudioOut, "OpenAL device rate: {} Hz", actual_rate);

        // ------------------------------------------------------------
        // Extension detection
        // ------------------------------------------------------------
        has_float32 = alIsExtensionPresent("AL_EXT_float32");
        has_multichannel = alIsExtensionPresent("AL_EXT_MCFORMATS");
        has_direct_channels = alcIsExtensionPresent(device, "AL_SOFT_direct_channels");

        // ------------------------------------------------------------
        // Source
        // ------------------------------------------------------------
        alGenSources(1, &source);
        if (alGetError() != AL_NO_ERROR) {
            LOG_ERROR(Lib_AudioOut, "Failed to create OpenAL source");
            Shutdown();
            return;
        }

        alSourcef(source, AL_GAIN, 1.0f);
        alSourcei(source, AL_LOOPING, AL_FALSE);

        if (has_direct_channels)
            alSourcei(source, AL_DIRECT_CHANNELS_SOFT, AL_TRUE);

        al_format = GetALFormat(channels, is_float);
        if (!al_format) {
            LOG_ERROR(Lib_AudioOut, "Unsupported OpenAL format");
            Shutdown();
            return;
        }

        CalculateBufferCount();

        // ------------------------------------------------------------
        // Buffers
        // ------------------------------------------------------------
        buffers.resize(buffer_count);
        alGenBuffers(static_cast<ALsizei>(buffer_count), buffers.data());

        bytes_per_buffer = CalculateByteSize();

        std::vector<std::byte> silence(bytes_per_buffer);
        std::memset(silence.data(), 0, silence.size());

        for (ALuint b : buffers) {
            alBufferData(b, al_format, silence.data(), static_cast<ALsizei>(silence.size()),
                         sample_rate);
        }

        alSourceQueueBuffers(source, static_cast<ALsizei>(buffers.size()), buffers.data());
        alSourcePlay(source);

        // ------------------------------------------------------------
        // Worker thread
        // ------------------------------------------------------------
        running = true;
        processing_thread = std::thread(&OpenALPortBackend::ProcessBuffers, this);

        LOG_INFO(Lib_AudioOut, "OpenAL initialized: channels={}, rate={}, buffers={}", channels,
                 sample_rate, buffer_count);
    }

    ~OpenALPortBackend() override {
        Shutdown();
    }

    // ------------------------------------------------------------
    // Audio submission
    // ------------------------------------------------------------
    void Output(void* ptr) override {
        if (!running)
            return;

        {
            std::lock_guard<std::mutex> lock(buffer_mutex);

            if (queued_data.size() >= MAX_QUEUE_SIZE) {
                dropped_frames++;
                if ((dropped_frames % 100) == 0) {
                    LOG_WARNING(Lib_AudioOut, "OpenAL queue overflow, dropped {} frames",
                                dropped_frames.load());
                }
                return;
            }

            queued_data.emplace_back(static_cast<std::byte*>(ptr),
                                     static_cast<std::byte*>(ptr) + guest_buffer_size);
        }

        buffer_cv.notify_one();
    }

    void SetVolume(const std::array<int, 8>& ch_volumes) override {
        if (!source)
            return;

        int max = *std::ranges::max_element(ch_volumes);
        float gain =
            static_cast<float>(max) / SCE_AUDIO_OUT_VOLUME_0DB * Config::getVolumeSlider() / 100.0f;

        alSourcef(source, AL_GAIN, std::clamp(gain, 0.0f, 1.0f));
    }

private:
    // ------------------------------------------------------------
    // Worker thread
    // ------------------------------------------------------------
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

                if (data.size() != bytes_per_buffer) {
                    data.resize(bytes_per_buffer);
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

    // ------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------
    void Shutdown() {
        running = false;
        buffer_cv.notify_all();

        if (processing_thread.joinable())
            processing_thread.join();

        if (context) {
            alSourceStop(source);
            alDeleteSources(1, &source);
            if (!buffers.empty())
                alDeleteBuffers(static_cast<ALsizei>(buffers.size()), buffers.data());
            alcDestroyContext(context);
        }

        if (device)
            alcCloseDevice(device);

        context = nullptr;
        device = nullptr;
    }

    void CalculateBufferCount() {
        static constexpr uint32_t BUFFER_MS = 10;
        static constexpr uint32_t TARGET_LATENCY_MS = 120;

        buffer_count = TARGET_LATENCY_MS / BUFFER_MS;
        buffer_count = std::clamp<uint32_t>(buffer_count, 12u, 16u);
    }

    uint32_t CalculateByteSize() const {
        uint32_t bytes_per_sample = 0;

        switch (al_format) {
        case AL_FORMAT_MONO16:
            bytes_per_sample = 2;
            break;
        case AL_FORMAT_STEREO16:
            bytes_per_sample = 4;
            break;
        case AL_FORMAT_MONO_FLOAT32:
            bytes_per_sample = 4;
            break;
        case AL_FORMAT_STEREO_FLOAT32:
            bytes_per_sample = 8;
            break;
        default:
            return 0;
        }

        const uint32_t frames = guest_buffer_size / frame_size;
        return frames * bytes_per_sample;
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
            }
        }
        return 0;
    }

private:
    static constexpr size_t MAX_QUEUE_SIZE = 64;

    u32 frame_size{};
    u32 guest_buffer_size{};
    uint32_t bytes_per_buffer{};
    int sample_rate{};
    int channels{};
    bool is_float{};

    size_t buffer_count{};

    bool has_float32{};
    bool has_multichannel{};
    bool has_direct_channels{};

    ALCdevice* device{};
    ALCcontext* context{};
    ALuint source{};
    std::vector<ALuint> buffers;
    ALenum al_format{};

    std::atomic<bool> running{false};
    std::atomic<int> dropped_frames{0};

    std::thread processing_thread;
    std::mutex buffer_mutex;
    std::condition_variable buffer_cv;
    std::deque<std::vector<std::byte>> queued_data;
};

std::unique_ptr<PortBackend> OpenALAudioOut::Open(PortOut& port) {
    return std::make_unique<OpenALPortBackend>(port);
}

} // namespace Libraries::AudioOut
