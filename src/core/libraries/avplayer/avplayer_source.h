// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string_view>

#include "common/assert.h"
#include "core/libraries/avplayer/avplayer.h"
#include "core/libraries/avplayer/avplayer_common.h"
#include "core/libraries/avplayer/avplayer_data_streamer.h"
#include "core/libraries/kernel/threads.h"

struct AVCodecContext;
struct AVFormatContext;
struct AVFrame;
struct AVIOContext;
struct AVPacket;
struct SwrContext;
struct SwsContext;

namespace Libraries::AvPlayer {

class AvPlayerStateCallback {
public:
    virtual ~AvPlayerStateCallback() = default;

    virtual AvPlayerAvSyncMode GetSyncMode() = 0;

    virtual void OnWarning(u32 id) = 0;
    virtual void OnError() = 0;
    virtual void OnEOF() = 0;
};

class GuestBuffer {
public:
    GuestBuffer(const AvPlayerMemAllocator& memory_replacement, u32 align, u32 size,
                bool is_texture) noexcept
        : m_memory_replacement(memory_replacement),
          m_data(is_texture ? AllocateTexture(memory_replacement, align, size)
                            : Allocate(memory_replacement, align, size)),
          m_is_texture(is_texture) {
        ASSERT_MSG(m_data, "Could not allocate frame buffer.");
    }

    ~GuestBuffer() {
        if (m_data != nullptr) {
            if (m_is_texture) {
                DeallocateTexture(m_memory_replacement, m_data);
            } else {
                Deallocate(m_memory_replacement, m_data);
            }
            m_data = {};
        }
    }

    GuestBuffer(const GuestBuffer&) noexcept = delete;
    GuestBuffer& operator=(const GuestBuffer&) noexcept = delete;

    GuestBuffer(GuestBuffer&& r) noexcept
        : m_memory_replacement(r.m_memory_replacement), m_data(r.m_data),
          m_is_texture(r.m_is_texture) {
        r.m_data = nullptr;
    };

    GuestBuffer& operator=(GuestBuffer&& r) noexcept {
        std::swap(m_data, r.m_data);
        return *this;
    }

    u8* GetBuffer() const noexcept {
        return m_data;
    }

private:
    static u8* Allocate(const AvPlayerMemAllocator& memory_replacement, u32 align, u32 size) {
        return reinterpret_cast<u8*>(
            memory_replacement.allocate(memory_replacement.object_ptr, align, size));
    }

    static void Deallocate(const AvPlayerMemAllocator& memory_replacement, void* ptr) {
        memory_replacement.deallocate(memory_replacement.object_ptr, ptr);
    }

    static u8* AllocateTexture(const AvPlayerMemAllocator& memory_replacement, u32 align,
                               u32 size) {
        return reinterpret_cast<u8*>(
            memory_replacement.allocate_texture(memory_replacement.object_ptr, align, size));
    }

    static void DeallocateTexture(const AvPlayerMemAllocator& memory_replacement, void* ptr) {
        memory_replacement.deallocate_texture(memory_replacement.object_ptr, ptr);
    }

    const AvPlayerMemAllocator& m_memory_replacement;
    u8* m_data = nullptr;
    bool m_is_texture = false;
};

struct Frame {
    GuestBuffer buffer;
    AvPlayerFrameInfoEx info;
};

class EventCV {
public:
    template <class Pred>
    void Wait(Pred pred) {
        std::unique_lock lock(m_mutex);
        m_cv.wait(lock, std::move(pred));
    }

    template <class Pred>
    bool Wait(std::stop_token stop, Pred pred) {
        std::unique_lock lock(m_mutex);
        return m_cv.wait(lock, std::move(stop), std::move(pred));
    }

    template <class Pred, class Rep, class Period>
    bool WaitFor(std::chrono::duration<Rep, Period> timeout, Pred pred) {
        std::unique_lock lock(m_mutex);
        return m_cv.wait_for(lock, timeout, std::move(pred));
    }

    void Notify() {
        std::unique_lock lock(m_mutex);
        m_cv.notify_all();
    }

private:
    std::mutex m_mutex{};
    std::condition_variable_any m_cv{};
};

class AvPlayerSource {
public:
    AvPlayerSource(AvPlayerStateCallback& state, bool use_vdec2);
    ~AvPlayerSource();

    bool Init(const AvPlayerInitData& init_data, std::string_view path);
    bool FindStreamInfo();
    s32 GetStreamCount();
    bool GetStreamInfo(u32 stream_index, AvPlayerStreamInfo& info);
    bool EnableStream(u32 stream_index);
    void SetLooping(bool is_looping);
    std::optional<bool> HasFrames(u32 num_frames);
    bool Start();
    bool Stop();
    void Pause();
    void Resume();
    bool GetAudioData(AvPlayerFrameInfo& audio_info);
    bool GetVideoData(AvPlayerFrameInfo& video_info);
    bool GetVideoData(AvPlayerFrameInfoEx& video_info);
    u64 CurrentTime();
    bool IsActive();

private:
    static void ReleaseAVPacket(AVPacket* packet);
    static void ReleaseAVFrame(AVFrame* frame);
    static void ReleaseAVCodecContext(AVCodecContext* context);
    static void ReleaseSWRContext(SwrContext* context);
    static void ReleaseSWSContext(SwsContext* context);
    static void ReleaseAVFormatContext(AVFormatContext* context);

    using AVPacketPtr = std::unique_ptr<AVPacket, decltype(&ReleaseAVPacket)>;
    using AVFramePtr = std::unique_ptr<AVFrame, decltype(&ReleaseAVFrame)>;
    using AVCodecContextPtr = std::unique_ptr<AVCodecContext, decltype(&ReleaseAVCodecContext)>;
    using SWRContextPtr = std::unique_ptr<SwrContext, decltype(&ReleaseSWRContext)>;
    using SWSContextPtr = std::unique_ptr<SwsContext, decltype(&ReleaseSWSContext)>;
    using AVFormatContextPtr = std::unique_ptr<AVFormatContext, decltype(&ReleaseAVFormatContext)>;

    void DemuxerThread(std::stop_token stop);
    void VideoDecoderThread(std::stop_token stop);
    void AudioDecoderThread(std::stop_token stop);

    bool HasRunningThreads() const;

    AVFramePtr ConvertAudioFrame(const AVFrame& frame);
    AVFramePtr ConvertVideoFrame(const AVFrame& frame);

    Frame PrepareAudioFrame(GuestBuffer buffer, const AVFrame& frame);
    Frame PrepareVideoFrame(GuestBuffer buffer, const AVFrame& frame);

    AvPlayerStateCallback& m_state;
    bool m_use_vdec2 = false;

    AvPlayerMemAllocator m_memory_replacement{};
    u32 m_max_num_video_framebuffers{};

    std::atomic_bool m_is_looping = false;
    std::atomic_bool m_is_paused = false;
    std::atomic_bool m_is_eof = false;

    std::unique_ptr<IDataStreamer> m_up_data_streamer;

    AvPlayerQueue<GuestBuffer> m_audio_buffers;
    AvPlayerQueue<GuestBuffer> m_video_buffers;

    AvPlayerQueue<AVPacketPtr> m_audio_packets;
    AvPlayerQueue<AVPacketPtr> m_video_packets;

    AvPlayerQueue<Frame> m_audio_frames;
    AvPlayerQueue<Frame> m_video_frames;

    std::optional<Frame> m_current_video_frame;
    std::optional<Frame> m_current_audio_frame;

    std::optional<s32> m_video_stream_index{};
    std::optional<s32> m_audio_stream_index{};

    EventCV m_audio_packets_cv{};
    EventCV m_audio_frames_cv{};
    EventCV m_audio_buffers_cv{};

    EventCV m_video_packets_cv{};
    EventCV m_video_frames_cv{};
    EventCV m_video_buffers_cv{};

    std::mutex m_state_mutex{};
    Kernel::Thread m_demuxer_thread{};
    Kernel::Thread m_video_decoder_thread{};
    Kernel::Thread m_audio_decoder_thread{};

    AVFormatContextPtr m_avformat_context{nullptr, &ReleaseAVFormatContext};
    AVCodecContextPtr m_video_codec_context{nullptr, &ReleaseAVCodecContext};
    AVCodecContextPtr m_audio_codec_context{nullptr, &ReleaseAVCodecContext};
    SWRContextPtr m_swr_context{nullptr, &ReleaseSWRContext};
    SWSContextPtr m_sws_context{nullptr, &ReleaseSWSContext};

    std::optional<u64> m_last_audio_ts{};
    std::optional<std::chrono::high_resolution_clock::time_point> m_start_time{};
    std::chrono::high_resolution_clock::time_point m_pause_time{};
    std::chrono::high_resolution_clock::duration m_pause_duration{};
};

} // namespace Libraries::AvPlayer
