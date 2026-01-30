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
#include <common/logging/log.h>
#include <magic_enum/magic_enum.hpp>
#include "audioout.h"
#include "audioout_backend.h"
#include "openal_manager.h"

namespace Libraries::AudioOut {

// ------------------------------------------------------------
// OpenALPortBackend
// ------------------------------------------------------------
class OpenALPortBackend : public PortBackend {
public:
    explicit OpenALPortBackend(const PortOut& port)
        : frame_size(port.format_info.FrameSize()), guest_buffer_size(port.BufferSize()),
          sample_rate(static_cast<int>(port.sample_rate)), channels(port.format_info.num_channels),
          is_float(port.format_info.is_float) {

        LOG_DEBUG(Lib_AudioOut,
                  "OpenALPortBackend constructor: channels={}, rate={}, float={}, buffer_size={}",
                  channels, sample_rate, is_float, guest_buffer_size);

        if (!OpenALManager::Instance().Initialize(sample_rate)) {
            LOG_ERROR(Lib_AudioOut, "Failed to initialize OpenAL device");
            return;
        }

        // Select context for this port
        OpenALManager::Instance().MakeContextCurrent();
        LOG_DEBUG(Lib_AudioOut, "OpenAL context made current");

        // Check for extensions
        has_float32 = alIsExtensionPresent("AL_EXT_float32");
        has_multichannel = alIsExtensionPresent("AL_EXT_MCFORMATS");
        has_direct_channels = OpenALManager::Instance().HasExtension("AL_SOFT_direct_channels");

        LOG_DEBUG(Lib_AudioOut,
                  "OpenAL extensions: float32={}, multichannel={}, direct_channels={}", has_float32,
                  has_multichannel, has_direct_channels);

        // Generate source for this port
        alGenSources(1, &source);
        ALenum al_error = alGetError();
        if (al_error != AL_NO_ERROR) {
            LOG_ERROR(Lib_AudioOut, "Failed to create OpenAL source, error: {}", al_error);
            return;
        }
        LOG_DEBUG(Lib_AudioOut, "OpenAL source created: {}", source);

        alSourcef(source, AL_GAIN, 1.0f);
        alSourcei(source, AL_LOOPING, AL_FALSE);
        if (has_direct_channels) {
            alSourcei(source, AL_DIRECT_CHANNELS_SOFT, AL_TRUE);
            LOG_DEBUG(Lib_AudioOut, "Direct channels enabled");
        }

        // Determine AL format
        al_format = GetALFormat(channels, is_float);
        LOG_DEBUG(Lib_AudioOut, "Requested format: channels={}, float={}, al_format={}", channels,
                  is_float, al_format);

        if (!al_format) {
            // Downmix to stereo if unsupported 8CH
            if (channels == 8) {
                LOG_WARNING(Lib_AudioOut, "8CH input, downmixing to stereo");
                downmix_needed = true;
                channels = 2;
            } else {
                downmix_needed = false;
            }
            frames_per_buffer = guest_buffer_size / frame_size;
            al_format = has_float32 ? AL_FORMAT_STEREO_FLOAT32 : AL_FORMAT_STEREO16;
            if (has_float32) {
                al_format = AL_FORMAT_STEREO_FLOAT32;
                bytes_per_buffer = static_cast<uint32_t>(frames_per_buffer * sizeof(float) * 2);
                LOG_DEBUG(Lib_AudioOut, "Using float stereo format, bytes_per_buffer={}",
                          bytes_per_buffer);
            } else {
                al_format = AL_FORMAT_STEREO16;
                bytes_per_buffer = static_cast<uint32_t>(frames_per_buffer * sizeof(int16_t) * 2);
                LOG_DEBUG(Lib_AudioOut, "Using int16 stereo format, bytes_per_buffer={}",
                          bytes_per_buffer);
            }
            stereo_buffer.resize(bytes_per_buffer);
        } else {
            bytes_per_buffer = CalculateBufferSize(channels, is_float);
            LOG_DEBUG(Lib_AudioOut, "Using native format, bytes_per_buffer={}", bytes_per_buffer);
        }

        // Buffers
        CalculateBufferCount();
        LOG_DEBUG(Lib_AudioOut, "Allocating {} buffers", buffer_count);

        buffers.resize(buffer_count);
        alGenBuffers(static_cast<ALsizei>(buffer_count), buffers.data());
        al_error = alGetError();
        if (al_error != AL_NO_ERROR) {
            LOG_ERROR(Lib_AudioOut, "Failed to generate OpenAL buffers, error: {}", al_error);
            return;
        }

        for (size_t i = 0; i < buffers.size(); i++) {
            if (!alIsBuffer(buffers[i])) {
                LOG_ERROR(Lib_AudioOut, "Invalid OpenAL buffer generated at index {}", i);
            } else {
                LOG_TRACE(Lib_AudioOut, "Buffer {} created: {}", i, buffers[i]);
            }
        }

        // Fill buffers with silence
        std::vector<std::byte> silence(bytes_per_buffer, std::byte{0});
        LOG_DEBUG(Lib_AudioOut, "Filling buffers with silence, size={} bytes", silence.size());

        for (ALuint b : buffers) {
            alBufferData(b, al_format, silence.data(), static_cast<ALsizei>(silence.size()),
                         sample_rate);
        }

        al_error = alGetError();
        if (al_error != AL_NO_ERROR) {
            LOG_ERROR(Lib_AudioOut, "Failed to fill buffers with silence, error: {}", al_error);
        }

        alSourceQueueBuffers(source, static_cast<ALsizei>(buffers.size()), buffers.data());
        alSourcePlay(source);

        al_error = alGetError();
        if (al_error != AL_NO_ERROR) {
            LOG_ERROR(Lib_AudioOut, "Failed to start playback, error: {}", al_error);
        } else {
            LOG_DEBUG(Lib_AudioOut, "Playback started successfully");
        }

        running = true;
        processing_thread = std::thread(&OpenALPortBackend::ProcessBuffers, this);

        LOG_INFO(Lib_AudioOut,
                 "OpenAL port initialized: channels={}, rate={}, buffers={}, format={}", channels,
                 sample_rate, buffer_count, al_format);
    }

    ~OpenALPortBackend() override {
        LOG_DEBUG(Lib_AudioOut, "OpenALPortBackend destructor called");
        {
            std::lock_guard<std::mutex> lock(buffer_mutex);
            running = false;
        }
        buffer_cv.notify_all();
        if (processing_thread.joinable()) {
            LOG_DEBUG(Lib_AudioOut, "Joining processing thread...");
            processing_thread.join();
            LOG_DEBUG(Lib_AudioOut, "Processing thread joined");
        }

        // Clean up OpenAL resources
        OpenALManager::Instance().MakeContextCurrent();
        if (source) {
            LOG_DEBUG(Lib_AudioOut, "Stopping and deleting source: {}", source);
            alSourceStop(source);
            alDeleteSources(1, &source);
            ALenum al_error = alGetError();
            if (al_error != AL_NO_ERROR) {
                LOG_ERROR(Lib_AudioOut, "Error deleting source, error: {}", al_error);
            }
            source = 0;
        }
        if (!buffers.empty()) {
            LOG_DEBUG(Lib_AudioOut, "Deleting {} buffers", buffers.size());
            alDeleteBuffers(static_cast<ALsizei>(buffers.size()), buffers.data());
            ALenum al_error = alGetError();
            if (al_error != AL_NO_ERROR) {
                LOG_ERROR(Lib_AudioOut, "Error deleting buffers, error: {}", al_error);
            }
            buffers.clear();
        }
        LOG_DEBUG(Lib_AudioOut, "OpenALPortBackend destruction complete");
    }

    void SetVolume(const std::array<int, 8>& ch_volumes) override {
        LOG_TRACE(Lib_AudioOut, "SetVolume called, source={}", source);
        if (!source) {
            LOG_WARNING(Lib_AudioOut, "SetVolume called but source is invalid");
            return;
        }

        // Calculate average volume for active channels only
        int total = 0;
        int count = 0;
        for (int i = 0; i < channels && i < 8; i++) {
            total += ch_volumes[i];
            count++;
            LOG_TRACE(Lib_AudioOut, "Channel {} volume: {}", i, ch_volumes[i]);
        }

        if (count == 0) {
            LOG_WARNING(Lib_AudioOut, "No active channels for volume calculation");
            return;
        }

        int avg = total / count;
        LOG_DEBUG(Lib_AudioOut, "Volume calculation: total={}, count={}, avg={}", total, count,
                  avg);

        float normalized = static_cast<float>(avg) / SCE_AUDIO_OUT_VOLUME_0DB;
        float config_volume = Config::getVolumeSlider();
        float gain = normalized * config_volume / 100.0f;

        LOG_DEBUG(Lib_AudioOut, "Volume: normalized={:.3f}, config={}, gain={:.3f}", normalized,
                  config_volume, gain);

        OpenALManager::Instance().MakeContextCurrent();
        alSourcef(source, AL_GAIN, std::clamp(gain, 0.0f, 1.0f));
        ALenum al_error = alGetError();
        if (al_error != AL_NO_ERROR) {
            LOG_ERROR(Lib_AudioOut, "Error setting gain, error: {}", al_error);
        } else {
            LOG_TRACE(Lib_AudioOut, "Gain set to {:.3f}", std::clamp(gain, 0.0f, 1.0f));
        }
    }

    void Output(void* ptr) override {
        bool is_running = running.load();
        LOG_TRACE(Lib_AudioOut, "Output called, running={}, ptr={}", is_running, fmt::ptr(ptr));
        if (!is_running || !ptr) {
            LOG_WARNING(Lib_AudioOut, "Output called but not running or null pointer");
            return;
        }

        std::vector<std::byte> audio_data;

        if (downmix_needed) {
            // 8-channel input
            const size_t input_channels = 8;
            const size_t bytes_per_sample = is_float ? 4 : 2;
            const size_t frames = guest_buffer_size / frame_size;

            LOG_DEBUG(Lib_AudioOut,
                      "Downmixing: frames={}, input_channels={}, output_channels=2, "
                      "bytes_per_sample={}, bytes_per_buffer={}",
                      frames, input_channels, bytes_per_sample, bytes_per_buffer);

            // Resize buffer exactly to expected OpenAL size
            audio_data.resize(bytes_per_buffer);

            // Downmix directly into buffer
            bool use_float_output = (al_format == AL_FORMAT_STEREO_FLOAT32);
            LOG_DEBUG(Lib_AudioOut, "Downmix: use_float_output={}", use_float_output);

            Downmix8CHToStereo(ptr, is_float, frames, use_float_output, audio_data);

            LOG_DEBUG(Lib_AudioOut, "Downmix complete: output_buffer_size={}", audio_data.size());

        } else {
            // Copy input directly
            audio_data.resize(guest_buffer_size);
            std::memcpy(audio_data.data(), ptr, guest_buffer_size);
            LOG_DEBUG(Lib_AudioOut, "Direct copy: {} bytes from {}", guest_buffer_size,
                      fmt::ptr(ptr));
        }

        // Verify buffer size matches expected
        if (audio_data.size() != bytes_per_buffer) {
            LOG_WARNING(Lib_AudioOut, "Buffer size mismatch: expected={}, actual={}",
                        bytes_per_buffer, audio_data.size());
        }

        // Queue the buffer
        size_t queue_size;
        {
            std::lock_guard<std::mutex> lock(buffer_mutex);
            queued_data.emplace_back(std::move(audio_data));
            queue_size = queued_data.size();
        }

        LOG_TRACE(Lib_AudioOut, "Queued buffer: size={}, queue_size={}", audio_data.size(),
                  queue_size);

        buffer_cv.notify_one();
    }

private:
    const char* GetALStateString(ALint state) {
        switch (state) {
        case AL_INITIAL:
            return "AL_INITIAL";
        case AL_PLAYING:
            return "AL_PLAYING";
        case AL_PAUSED:
            return "AL_PAUSED";
        case AL_STOPPED:
            return "AL_STOPPED";
        default:
            return "UNKNOWN";
        }
    }
    void ProcessBuffers() {
        LOG_DEBUG(Lib_AudioOut, "ProcessBuffers thread started");

        // Ensure OpenAL context is current for this thread
        OpenALManager::Instance().MakeContextCurrent();
        LOG_DEBUG(Lib_AudioOut, "OpenAL context set in processing thread");

        std::vector<ALuint> free_buffers;
        int loop_counter = 0;
        int processed_total = 0;
        int underruns = 0; // Track underruns

        while (running.load()) {
            loop_counter++;

            // Check for processed buffers first
            ALint processed = 0;
            alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
            if (processed > 0) {
                LOG_TRACE(Lib_AudioOut, "ProcessBuffers: {} buffers processed", processed);
                free_buffers.resize(processed);
                alSourceUnqueueBuffers(source, processed, free_buffers.data());
                processed_total += processed;

                ALenum al_error = alGetError();
                if (al_error != AL_NO_ERROR) {
                    LOG_ERROR(Lib_AudioOut, "Error unqueueing buffers, error: {}", al_error);
                }
            }

            // Fill any free buffers with queued data
            size_t buffers_filled = 0;
            while (!free_buffers.empty()) {
                ALint queued = 0;
                alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);

                std::vector<std::byte> data;
                {
                    std::lock_guard<std::mutex> lock(buffer_mutex);
                    if (queued_data.empty()) {
                        // No data available, break and wait
                        break;
                    }
                    data = std::move(queued_data.front());
                    queued_data.pop_front();
                }

                ALuint buffer = free_buffers.back();
                free_buffers.pop_back();

                LOG_TRACE(Lib_AudioOut, "Filling buffer {} with {} bytes", buffer, data.size());

                alBufferData(buffer, al_format, data.data(), static_cast<ALsizei>(data.size()),
                             sample_rate);
                alSourceQueueBuffers(source, 1, &buffer);

                buffers_filled++;

                ALenum al_error = alGetError();
                if (al_error != AL_NO_ERROR) {
                    LOG_ERROR(Lib_AudioOut, "Error queueing buffer, error: {}", al_error);
                }
            }

            if (buffers_filled > 0) {
                LOG_TRACE(Lib_AudioOut, "Filled {} buffers", buffers_filled);
            }

            // Check source state
            ALint state = 0;
            alGetSourcei(source, AL_SOURCE_STATE, &state);

            ALint queued = 0;
            alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);

            if (state != AL_PLAYING) {
                if (queued > 0) {
                    // We have buffers but source stopped - restart it
                    underruns++;
                    LOG_WARNING(Lib_AudioOut,
                                "Source stopped (state={}), restarting. Queued={}, Underruns={}",
                                GetALStateString(state), queued, underruns);
                    alSourcePlay(source);
                    ALenum al_error = alGetError();
                    if (al_error != AL_NO_ERROR) {
                        LOG_ERROR(Lib_AudioOut, "Error restarting playback, error: {}", al_error);
                    }
                } else {
                    // No buffers queued, this is expected
                    LOG_TRACE(Lib_AudioOut,
                              "Source stopped, no buffers queued (state={}, queued={})",
                              GetALStateString(state), queued);
                }
            }

            // If we have very few buffers queued, wait shorter time
            int wait_time_ms = 5;
            if (queued < buffer_count / 2) {
                wait_time_ms = 2; // Wait shorter if low on buffers
            }

            // Wait for more data or timeout
            {
                std::unique_lock<std::mutex> lock(buffer_mutex);
                if (queued_data.empty()) {
                    buffer_cv.wait_for(lock, std::chrono::milliseconds(wait_time_ms),
                                       [&] { return !queued_data.empty() || !running.load(); });
                }
            }

            if (!running.load()) {
                LOG_DEBUG(Lib_AudioOut, "ProcessBuffers: exiting due to !running");
                break;
            }

            // Log statistics every 500 iterations
            if (loop_counter % 500 == 0) {
                ALint queued_now = 0;
                alGetSourcei(source, AL_BUFFERS_QUEUED, &queued_now);
                ALint state_now = 0;
                alGetSourcei(source, AL_SOURCE_STATE, &state_now);

                LOG_DEBUG(Lib_AudioOut,
                          "ProcessBuffers stats: loops={}, processed={}, queued={}, state={}, "
                          "queue_size={}, underruns={}",
                          loop_counter, processed_total, queued_now, GetALStateString(state_now),
                          queued_data.size(), underruns);
            }
        }

        LOG_DEBUG(Lib_AudioOut,
                  "ProcessBuffers thread exiting, total loops={}, total processed={}, underruns={}",
                  loop_counter, processed_total, underruns);
    }

    ALenum GetALFormat(int ch, bool f32) {
        ALenum format = 0;

        if (f32 && has_float32) {
            switch (ch) {
            case 1:
                format = AL_FORMAT_MONO_FLOAT32;
                break;
            case 2:
                format = AL_FORMAT_STEREO_FLOAT32;
                break;
            case 4:
                format = has_multichannel ? AL_FORMAT_QUAD32 : 0;
                break;
            case 6:
                format = has_multichannel ? AL_FORMAT_51CHN32 : 0;
                break;
            case 7:
                format = has_multichannel ? AL_FORMAT_61CHN32 : 0;
                break;
            case 8:
                format = has_multichannel ? AL_FORMAT_71CHN32 : 0;
                break;
            default:
                format = 0;
                break;
            }
        } else {
            switch (ch) {
            case 1:
                format = AL_FORMAT_MONO16;
                break;
            case 2:
                format = AL_FORMAT_STEREO16;
                break;
            case 4:
                format = has_multichannel ? AL_FORMAT_QUAD16 : 0;
                break;
            case 6:
                format = has_multichannel ? AL_FORMAT_51CHN16 : 0;
                break;
            case 7:
                format = has_multichannel ? AL_FORMAT_61CHN16 : 0;
                break;
            case 8:
                format = has_multichannel ? AL_FORMAT_71CHN16 : 0;
                break;
            default:
                format = 0;
                break;
            }
        }

        LOG_DEBUG(Lib_AudioOut, "GetALFormat: ch={}, f32={}, has_multichannel={}, format={}", ch,
                  f32, has_multichannel, format);
        return format;
    }

    uint32_t CalculateBufferSize(int out_channels, bool use_float) const {
        uint32_t bytes_per_sample = use_float ? 4 : 2;
        size_t frames = guest_buffer_size / frame_size;
        uint32_t size = static_cast<uint32_t>(frames * bytes_per_sample * out_channels);

        LOG_DEBUG(Lib_AudioOut,
                  "CalculateBufferSize: frames={}, bytes_per_sample={}, out_channels={}, size={}",
                  frames, bytes_per_sample, out_channels, size);
        return size;
    }

    void CalculateBufferCount() {
        // Calculate frames from guest buffer size and frame size
        const size_t frames = guest_buffer_size / frame_size;
        const float buffer_duration_ms = (frames * 1000.0f) / sample_rate;

        // Target more buffers for safety (120ms total latency)
        const size_t target_buffers = static_cast<size_t>(std::ceil(120.0f / buffer_duration_ms));

        // Increase minimum buffers
        buffer_count = std::clamp<size_t>(target_buffers, 4, 12);

        LOG_DEBUG(Lib_AudioOut, "Buffer count: frames={}, duration={:.1f}ms, target={}, final={}",
                  frames, buffer_duration_ms, target_buffers, buffer_count);
    }

    void Downmix8CHToStereo(const void* src, bool is_float_input, size_t frames, bool output_float,
                            std::vector<std::byte>& dst) {
        LOG_TRACE(Lib_AudioOut,
                  "Downmix8CHToStereo: frames={}, is_float_input={}, output_float={}, dst_size={}",
                  frames, is_float_input, output_float, dst.size());

        const float* in;
        std::vector<float> temp;

        if (is_float_input) {
            in = static_cast<const float*>(src);
            LOG_TRACE(Lib_AudioOut, "Input is float32");
        } else {
            const int16_t* s = static_cast<const int16_t*>(src);
            temp.resize(frames * 8);
            for (size_t i = 0; i < temp.size(); i++)
                temp[i] = s[i] / 32768.0f;
            in = temp.data();
            LOG_TRACE(Lib_AudioOut, "Input is int16, converted to float");
        }

        float* out = reinterpret_cast<float*>(dst.data());

        for (size_t i = 0; i < frames; i++) {
            float fl = in[i * 8 + 0];
            float fr = in[i * 8 + 1];
            float c = in[i * 8 + 2];
            float lfe = in[i * 8 + 3];
            float sl = in[i * 8 + 4];
            float sr = in[i * 8 + 5];
            float bl = in[i * 8 + 6];
            float br = in[i * 8 + 7];

            out[i * 2 + 0] =
                std::clamp(fl + 0.707f * c + 0.707f * sl + 0.5f * bl + 0.1f * lfe, -1.f, 1.f);
            out[i * 2 + 1] =
                std::clamp(fr + 0.707f * c + 0.707f * sr + 0.5f * br + 0.1f * lfe, -1.f, 1.f);

            // Log first few frames for debugging
            if (i < 3) {
                LOG_TRACE(Lib_AudioOut,
                          "Frame {}: FL={:.3f}, FR={:.3f}, C={:.3f}, LFE={:.3f}, "
                          "SL={:.3f}, SR={:.3f}, BL={:.3f}, BR={:.3f} -> L={:.3f}, R={:.3f}",
                          i, fl, fr, c, lfe, sl, sr, bl, br, out[i * 2 + 0], out[i * 2 + 1]);
            }
        }

        LOG_DEBUG(Lib_AudioOut, "Downmix complete: processed {} frames", frames);
    }

private:
    // Port parameters
    size_t frame_size{};
    size_t guest_buffer_size{};
    size_t frames_per_buffer{};
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

// ------------------------------------------------------------
// OpenALAudioOut
// ------------------------------------------------------------
std::unique_ptr<PortBackend> OpenALAudioOut::Open(PortOut& port) {
    LOG_DEBUG(Lib_AudioOut, "OpenALAudioOut::Open called for port type {}",
              magic_enum::enum_name(port.type));
    return std::make_unique<OpenALPortBackend>(port);
}

} // namespace Libraries::AudioOut