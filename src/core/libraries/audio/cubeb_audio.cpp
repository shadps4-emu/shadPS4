// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <condition_variable>
#include <mutex>
#include <cubeb/cubeb.h>

#include "common/assert.h"
#include "common/ringbuffer.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio/audioout_backend.h"

namespace Libraries::AudioOut {

constexpr int AUDIO_STREAM_BUFFER_THRESHOLD = 65536; // Define constant for buffer threshold

class CubebPortBackend : public PortBackend {
public:
    CubebPortBackend(cubeb* ctx, const PortOut& port)
        : frame_size(port.frame_size), buffer(static_cast<int>(port.buffer_size) * 4) {
        if (!ctx) {
            return;
        }
        const auto get_channel_layout = [&port] -> cubeb_channel_layout {
            switch (port.channels_num) {
            case 1:
                return CUBEB_LAYOUT_MONO;
            case 2:
                return CUBEB_LAYOUT_STEREO;
            case 8:
                return CUBEB_LAYOUT_3F4_LFE;
            default:
                UNREACHABLE();
            }
        };
        cubeb_stream_params stream_params = {
            .format = port.is_float ? CUBEB_SAMPLE_FLOAT32LE : CUBEB_SAMPLE_S16LE,
            .rate = port.freq,
            .channels = port.channels_num,
            .layout = get_channel_layout(),
            .prefs = CUBEB_STREAM_PREF_NONE,
        };
        u32 latency_frames = 512;
        if (const auto ret = cubeb_get_min_latency(ctx, &stream_params, &latency_frames);
            ret != CUBEB_OK) {
            LOG_WARNING(Lib_AudioOut,
                        "Could not get minimum cubeb audio latency, falling back to default: {}",
                        ret);
        }
        char stream_name[64];
        snprintf(stream_name, sizeof(stream_name), "shadPS4 stream %p", this);
        if (const auto ret = cubeb_stream_init(ctx, &stream, stream_name, nullptr, nullptr, nullptr,
                                               &stream_params, latency_frames, &DataCallback,
                                               &StateCallback, this);
            ret != CUBEB_OK) {
            LOG_ERROR(Lib_AudioOut, "Failed to create cubeb stream: {}", ret);
            return;
        }
        if (const auto ret = cubeb_stream_start(stream); ret != CUBEB_OK) {
            LOG_ERROR(Lib_AudioOut, "Failed to start cubeb stream: {}", ret);
            return;
        }
    }

    ~CubebPortBackend() override {
        if (!stream) {
            return;
        }
        if (const auto ret = cubeb_stream_stop(stream); ret != CUBEB_OK) {
            LOG_WARNING(Lib_AudioOut, "Failed to stop cubeb stream: {}", ret);
        }
        cubeb_stream_destroy(stream);
        stream = nullptr;
    }

    void Output(void* ptr, size_t size) override {
        auto* data = static_cast<u8*>(ptr);

        std::unique_lock lock{buffer_mutex};
        buffer_cv.wait(lock, [&] { return buffer.available_write() >= size; });
        buffer.enqueue(data, static_cast<int>(size));
    }

    void SetVolume(const std::array<int, 8>& ch_volumes) override {
        if (!stream) {
            return;
        }
        // Cubeb does not have per-channel volumes, for now just take the maximum of the channels.
        const auto vol = *std::ranges::max_element(ch_volumes);
        if (const auto ret =
                cubeb_stream_set_volume(stream, static_cast<float>(vol) / SCE_AUDIO_OUT_VOLUME_0DB);
            ret != CUBEB_OK) {
            LOG_WARNING(Lib_AudioOut, "Failed to change cubeb stream volume: {}", ret);
        }
    }

private:
    static long DataCallback(cubeb_stream* stream, void* user_data, const void* in, void* out,
                             long num_frames) {
        auto* stream_data = static_cast<CubebPortBackend*>(user_data);
        const auto out_data = static_cast<u8*>(out);
        const auto requested_size = static_cast<int>(num_frames * stream_data->frame_size);

        std::unique_lock lock{stream_data->buffer_mutex};
        const auto dequeued_size = stream_data->buffer.dequeue(out_data, requested_size);
        lock.unlock();
        stream_data->buffer_cv.notify_one();

        if (dequeued_size < requested_size) {
            // Need to fill remaining space with silence.
            std::memset(out_data + dequeued_size, 0, requested_size - dequeued_size);
        }
        return num_frames;
    }

    static void StateCallback(cubeb_stream* stream, void* user_data, cubeb_state state) {
        switch (state) {
        case CUBEB_STATE_STARTED:
            LOG_INFO(Lib_AudioOut, "Cubeb stream started");
            break;
        case CUBEB_STATE_STOPPED:
            LOG_INFO(Lib_AudioOut, "Cubeb stream stopped");
            break;
        case CUBEB_STATE_DRAINED:
            LOG_INFO(Lib_AudioOut, "Cubeb stream drained");
            break;
        case CUBEB_STATE_ERROR:
            LOG_ERROR(Lib_AudioOut, "Cubeb stream encountered an error");
            break;
        }
    }

    size_t frame_size;
    RingBuffer<u8> buffer;
    std::mutex buffer_mutex;
    std::condition_variable buffer_cv;
    cubeb_stream* stream{};
};

CubebAudioOut::CubebAudioOut() {
    if (const auto ret = cubeb_init(&ctx, "shadPS4", nullptr); ret != CUBEB_OK) {
        LOG_CRITICAL(Lib_AudioOut, "Failed to create cubeb context: {}", ret);
    }
}

CubebAudioOut::~CubebAudioOut() {
    if (!ctx) {
        return;
    }
    cubeb_destroy(ctx);
    ctx = nullptr;
}

std::unique_ptr<PortBackend> CubebAudioOut::Open(PortOut& port) {
    return std::make_unique<CubebPortBackend>(ctx, port);
}

} // namespace Libraries::AudioOut
