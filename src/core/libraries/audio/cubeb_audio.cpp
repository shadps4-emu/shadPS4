// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cubeb/cubeb.h>

#include "common/assert.h"
#include "common/ringbuffer.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio/cubeb_audio.h"

namespace Libraries::AudioOut {

constexpr int AUDIO_STREAM_BUFFER_THRESHOLD = 65536; // Define constant for buffer threshold

static cubeb_channel_layout GetCubebChannelLayout(int num_channels) {
    switch (num_channels) {
    case 1:
        return CUBEB_LAYOUT_MONO;
    case 2:
        return CUBEB_LAYOUT_STEREO;
    case 8:
        return CUBEB_LAYOUT_3F4_LFE;
    default:
        UNREACHABLE();
    }
}

struct CubebStream {
    cubeb_stream* stream{};
    size_t frame_size;
    ring_buffer_base<u8> buffer;

    CubebStream(cubeb* ctx, const PortOut& port)
        : frame_size(port.channels_num * port.sample_size),
          buffer(static_cast<int>(port.samples_num * frame_size) * 4) {
        cubeb_stream_params stream_params = {
            .format = port.is_float ? CUBEB_SAMPLE_FLOAT32LE : CUBEB_SAMPLE_S16LE,
            .rate = port.freq,
            .channels = static_cast<u32>(port.channels_num),
            .layout = GetCubebChannelLayout(port.channels_num),
        };
        u32 latency_frames = 512;
        if (const auto ret = cubeb_get_min_latency(ctx, &stream_params, &latency_frames);
            ret != CUBEB_OK) {
            LOG_WARNING(Lib_AudioOut, "Could not get minimum cubeb audio latency: {}", ret);
        }
        if (const auto ret = cubeb_stream_init(ctx, &stream, "shadPS4", nullptr, nullptr, nullptr,
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

    ~CubebStream() {
        if (const auto ret = cubeb_stream_stop(stream); ret != CUBEB_OK) {
            LOG_WARNING(Lib_AudioOut, "Failed to stop cubeb stream: {}", ret);
        }
        cubeb_stream_destroy(stream);
    }

    void Output(void* ptr, size_t size) {
        auto* data = static_cast<u8*>(ptr);
        while (size > 0) {
            const auto queued = buffer.enqueue(data, static_cast<int>(size));
            size -= queued;
            data += queued;
            if (size > 0) {
                // If the data is too large for the ring buffer, yield execution and give it a
                // chance to drain.
                std::this_thread::yield();
            }
        }
    }

    void SetVolume(const std::array<int, 8>& ch_volumes) {
        // Cubeb does not have per-channel volumes, for now just take the maximum of the channels.
        const auto vol = *std::ranges::max_element(ch_volumes);
        if (const auto ret =
                cubeb_stream_set_volume(stream, static_cast<float>(vol) / SCE_AUDIO_OUT_VOLUME_0DB);
            ret != CUBEB_OK) {
            LOG_WARNING(Lib_AudioOut, "Failed to change cubeb stream volume: {}", ret);
        }
    }

    static long DataCallback(cubeb_stream* stream, void* user_data, const void* in, void* out,
                             long num_frames) {
        auto* stream_data = static_cast<CubebStream*>(user_data);
        const auto requested_size = num_frames * stream_data->frame_size;
        const auto dequeued_size = stream_data->buffer.dequeue(
            static_cast<u8*>(out), static_cast<int>(num_frames * stream_data->frame_size));
        if (dequeued_size < requested_size) {
            // Need to fill remaining space with silence.
            std::memset(static_cast<u8*>(out) + dequeued_size, 0, requested_size - dequeued_size);
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
};

CubebAudioOut::CubebAudioOut() {
    if (const auto ret = cubeb_init(&ctx, "shadPS4", nullptr); ret != CUBEB_OK) {
        LOG_CRITICAL(Lib_AudioOut, "Failed to create cubeb context: {}", ret);
        return;
    }
}

CubebAudioOut::~CubebAudioOut() {
    cubeb_destroy(ctx);
}

void* CubebAudioOut::Open(PortOut& port) {
    return new CubebStream(ctx, port);
}

void CubebAudioOut::Close(void* impl) {
    delete static_cast<CubebStream*>(impl);
}

void CubebAudioOut::Output(void* impl, void* ptr, size_t size) {
    static_cast<CubebStream*>(impl)->Output(ptr, size);
}

void CubebAudioOut::SetVolume(void* impl, const std::array<int, 8>& ch_volumes) {
    static_cast<CubebStream*>(impl)->SetVolume(ch_volumes);
}

} // namespace Libraries::AudioOut
