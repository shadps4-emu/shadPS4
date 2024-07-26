// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "avplayer.h"
#include "avplayer_common.h"
#include "avplayer_data_streamer.h"

#include "common/types.h"
#include "core/libraries/kernel/thread_management.h"

#include <atomic>
#include <chrono>
#include <optional>
#include <string>

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

    virtual void OnWarning(u32 id) = 0;
    virtual void OnError() = 0;
    virtual void OnEOF() = 0;
};

class FrameBuffer {
public:
    FrameBuffer(const SceAvPlayerMemAllocator& memory_replacement, u32 align, u32 size) noexcept
        : m_memory_replacement(memory_replacement),
          m_data(Allocate(memory_replacement, align, size), size) {}

    ~FrameBuffer() {
        if (!m_data.empty()) {
            Deallocate(m_memory_replacement, m_data.data());
            m_data = {};
        }
    }

    FrameBuffer(const FrameBuffer&) noexcept = delete;
    FrameBuffer& operator=(const FrameBuffer&) noexcept = delete;

    FrameBuffer(FrameBuffer&& r) noexcept
        : m_memory_replacement(r.m_memory_replacement), m_data(r.m_data) {
        r.m_data = {};
    };

    FrameBuffer& operator=(FrameBuffer&& r) noexcept {
        m_memory_replacement = r.m_memory_replacement;
        std::swap(m_data, r.m_data);
        return *this;
    }

    u8* GetBuffer() const noexcept {
        return m_data.data();
    }

private:
    static u8* Allocate(const SceAvPlayerMemAllocator& memory_replacement, u32 align, u32 size) {
        return reinterpret_cast<u8*>(
            memory_replacement.allocate(memory_replacement.object_ptr, align, size));
    }

    static void Deallocate(const SceAvPlayerMemAllocator& memory_replacement, void* ptr) {
        memory_replacement.deallocate(memory_replacement.object_ptr, ptr);
    }

    SceAvPlayerMemAllocator m_memory_replacement;
    std::span<u8> m_data;
};

class AvPlayerSource {
public:
    AvPlayerSource(AvPlayerStateCallback& state);
    ~AvPlayerSource();

    s32 Init(std::string_view path, SceAvPlayerMemAllocator& memory_replacement,
             SceAvPlayerFileReplacement& file_replacement, ThreadPriorities& priorities,
             SceAvPlayerSourceType source_type);

    bool FindStreamInfo();
    s32 GetStreamCount();
    s32 GetStreamInfo(u32 stream_index, SceAvPlayerStreamInfo& info);
    s32 EnableStream(u32 stream_index);
    void SetLooping(bool is_looping);
    std::optional<bool> HasFrames(u32 num_frames);
    s32 Start();
    bool Stop();
    bool GetAudioData(SceAvPlayerFrameInfo& audio_info);
    bool GetVideoData(SceAvPlayerFrameInfo& video_info);
    bool GetVideoData(SceAvPlayerFrameInfoEx& video_info);
    u64 CurrentTime();
    bool IsActive();

private:
    using ScePthread = Kernel::ScePthread;

    static void* PS4_SYSV_ABI DemuxerThread(void* opaque);
    static void* PS4_SYSV_ABI VideoDecoderThread(void* opaque);
    static void* PS4_SYSV_ABI AudioDecoderThread(void* opaque);

    static void ReleaseAVPacket(AVPacket* packet);
    static void ReleaseAVFrame(AVFrame* frame);
    static void ReleaseAVCodecContext(AVCodecContext* context);
    static void ReleaseSWRContext(SwrContext* context);
    static void ReleaseSWSContext(SwsContext* context);

    using AVPacketPtr = std::unique_ptr<AVPacket, decltype(&ReleaseAVPacket)>;
    using AVFramePtr = std::unique_ptr<AVFrame, decltype(&ReleaseAVFrame)>;
    using AVCodecContextPtr = std::unique_ptr<AVCodecContext, decltype(&ReleaseAVCodecContext)>;
    using SWRContextPtr = std::unique_ptr<SwrContext, decltype(&ReleaseSWRContext)>;
    using SWSContextPtr = std::unique_ptr<SwsContext, decltype(&ReleaseSWSContext)>;

    u8* GetVideoBuffer(AVFrame* frame);
    u8* GetAudioBuffer(AVFrame* frame);

    AVFramePtr ConvertAudioFrame(const AVFrame& frame);
    AVFramePtr ConvertVideoFrame(const AVFrame& frame);

    u64 m_last_video_timestamp{};

    AvPlayerStateCallback& m_state;

    ThreadPriorities m_priorities;
    SceAvPlayerMemAllocator m_memory_replacement;

    std::atomic_bool m_is_looping = false;
    std::atomic_bool m_is_eof = false;
    std::atomic_bool m_is_stop = false;
    std::unique_ptr<IDataStreamer> m_up_data_streamer;

    AVFormatContext* m_avformat_context{};

    AvPlayerQueue<AVPacketPtr> m_audio_packets;
    AvPlayerQueue<AVPacketPtr> m_video_packets;

    AvPlayerQueue<AVFramePtr> m_audio_frames;
    AvPlayerQueue<AVFramePtr> m_video_frames;

    std::span<u8> m_video_frame_storage;
    std::span<u8> m_audio_frame_storage;

    AVCodecContextPtr m_video_codec_context{nullptr, &ReleaseAVCodecContext};
    AVCodecContextPtr m_audio_codec_context{nullptr, &ReleaseAVCodecContext};

    std::optional<int> m_video_stream_index{};
    std::optional<int> m_audio_stream_index{};

    ScePthread m_demuxer_thread{};
    ScePthread m_video_decoder_thread{};
    ScePthread m_audio_decoder_thread{};

    SWRContextPtr m_swr_context{nullptr, &ReleaseSWRContext};
    SWSContextPtr m_sws_context{nullptr, &ReleaseSWSContext};

    std::chrono::high_resolution_clock::time_point m_start_time{};
};

} // namespace Libraries::AvPlayer
