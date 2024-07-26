// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "avplayer_source.h"

#include "avplayer_file_streamer.h"

#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/time_management.h"

#include <magic_enum.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace Libraries::AvPlayer {

using namespace Kernel;

AvPlayerSource::AvPlayerSource(AvPlayerStateCallback& state) : m_state(state) {}

AvPlayerSource::~AvPlayerSource() {
    if (!m_video_frame_storage.empty()) {
        m_memory_replacement.deallocate(m_memory_replacement.object_ptr,
                                        m_video_frame_storage.data());
    }
    if (!m_audio_frame_storage.empty()) {
        m_memory_replacement.deallocate(m_memory_replacement.object_ptr,
                                        m_audio_frame_storage.data());
    }
    Stop();
}

s32 AvPlayerSource::Init(std::string_view path, SceAvPlayerMemAllocator& memory_replacement,
                         SceAvPlayerFileReplacement& file_replacement, ThreadPriorities& priorities,
                         SceAvPlayerSourceType source_type) {
    if (m_avformat_context != nullptr) {
        return -1;
    }

    m_priorities = priorities;
    m_memory_replacement = memory_replacement;

    m_avformat_context = avformat_alloc_context();
    if (m_avformat_context == nullptr) {
        return -1;
    }
    if (file_replacement.open != nullptr) {
        m_up_data_streamer = std::make_unique<AvPlayerFileStreamer>(file_replacement, path);
        m_avformat_context->pb = m_up_data_streamer->GetContext();
        if (avformat_open_input(&m_avformat_context, nullptr, nullptr, nullptr) < 0) {
            return -1;
        }
    } else {
        const auto mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
        const auto filepath = mnt->GetHostPath(path);
        if (AVPLAYER_IS_ERROR(avformat_open_input(&m_avformat_context, filepath.string().c_str(),
                                                  nullptr, nullptr))) {
            return -1;
        }
    }

    return 0;
}

bool AvPlayerSource::FindStreamInfo() {
    if (m_avformat_context == nullptr) {
        return false;
    }
    if (m_avformat_context->nb_streams > 0) {
        return true;
    }
    return avformat_find_stream_info(m_avformat_context, nullptr) == 0;
}

s32 AvPlayerSource::GetStreamCount() {
    if (m_avformat_context == nullptr) {
        return -1;
    }
    LOG_DEBUG(Lib_AvPlayer, "num streams: {}", m_avformat_context->nb_streams);
    return m_avformat_context->nb_streams;
}

static s32 CodecTypeToStreamType(AVMediaType codec_type) {
    switch (codec_type) {
    case AVMediaType::AVMEDIA_TYPE_VIDEO:
        return SCE_AVPLAYER_VIDEO;
    case AVMediaType::AVMEDIA_TYPE_AUDIO:
        return SCE_AVPLAYER_AUDIO;
    case AVMediaType::AVMEDIA_TYPE_SUBTITLE:
        return SCE_AVPLAYER_TIMEDTEXT;
    default:
        LOG_ERROR(Lib_AvPlayer, "Unexpected AVMediaType {}", magic_enum::enum_name(codec_type));
        return -1;
    }
}

static f32 AVRationalToF32(AVRational rational) {
    return f32(rational.num) / rational.den;
}

s32 AvPlayerSource::GetStreamInfo(u32 stream_index, SceAvPlayerStreamInfo& info) {
    info = {};
    if (m_avformat_context == nullptr || stream_index >= m_avformat_context->nb_streams) {
        return -1;
    }
    const auto p_stream = m_avformat_context->streams[stream_index];
    if (p_stream == nullptr || p_stream->codecpar == nullptr) {
        return -1;
    }
    info.type = CodecTypeToStreamType(p_stream->codecpar->codec_type);
    info.start_time = p_stream->start_time;
    info.duration = p_stream->duration;
    const auto p_lang_node = av_dict_get(p_stream->metadata, "language", nullptr, 0);
    if (p_lang_node == nullptr) {
        return -1;
    }
    LOG_DEBUG(Lib_AvPlayer, "Stream {} language = {}", stream_index, p_lang_node->value);
    switch (info.type) {
    case SCE_AVPLAYER_VIDEO:
        LOG_DEBUG(Lib_AvPlayer, "Stream {} is a video stream.", stream_index);
        info.details.video.aspect_ratio = AVRationalToF32(p_stream->codecpar->sample_aspect_ratio);
        info.details.video.width = p_stream->codecpar->width;
        info.details.video.height = p_stream->codecpar->height;
        std::memcpy(info.details.video.language_code, p_lang_node->value,
                    std::min(strlen(p_lang_node->value), 3ull));
        break;
    case SCE_AVPLAYER_AUDIO:
        LOG_DEBUG(Lib_AvPlayer, "Stream {} is an audio stream.", stream_index);
        info.details.audio.channel_count = p_stream->codecpar->ch_layout.nb_channels;
        info.details.audio.sample_rate = p_stream->codecpar->sample_rate;
        info.details.audio.size = 0; // sceAvPlayerGetStreamInfo() is expected to set this to 0
        std::memcpy(info.details.audio.language_code, p_lang_node->value,
                    std::min(strlen(p_lang_node->value), 3ull));
        break;
    case SCE_AVPLAYER_TIMEDTEXT:
        LOG_WARNING(Lib_AvPlayer, "Stream {} is a timedtext stream.", stream_index);
        info.details.subs.font_size = 12;
        info.details.subs.text_size = 12;
        std::memcpy(info.details.subs.language_code, p_lang_node->value,
                    std::min(strlen(p_lang_node->value), 3ull));
        break;
    default:
        return -1;
    }
    return 0;
}

static AVPixelFormat GetPreferredVideoFormat(AVCodecContext* s, const AVPixelFormat* fmt) {
    auto curr = fmt;
    while (*curr != AV_PIX_FMT_NONE) {
        LOG_TRACE(Lib_AvPlayer, "Supported format: {}", magic_enum::enum_name(*fmt));
        if (*curr == AV_PIX_FMT_NV12) {
            return AV_PIX_FMT_NV12;
        }
        ++curr;
    }
    return AV_PIX_FMT_NONE;
}

s32 AvPlayerSource::EnableStream(u32 stream_index) {
    if (m_avformat_context == nullptr || stream_index >= m_avformat_context->nb_streams) {
        return -1;
    }
    const auto stream = m_avformat_context->streams[stream_index];
    const auto decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (decoder == nullptr) {
        return -1;
    }
    switch (stream->codecpar->codec_type) {
    case AVMediaType::AVMEDIA_TYPE_VIDEO: {
        m_video_stream_index = stream_index;
        m_video_codec_context =
            AVCodecContextPtr(avcodec_alloc_context3(decoder), &ReleaseAVCodecContext);
        if (avcodec_parameters_to_context(m_video_codec_context.get(), stream->codecpar) < 0) {
            return -1;
        }
        if (avcodec_open2(m_video_codec_context.get(), decoder, nullptr) < 0) {
            return -1;
        }
        LOG_INFO(Lib_AvPlayer, "Video stream {} enabled", stream_index);
        break;
    }
    case AVMediaType::AVMEDIA_TYPE_AUDIO: {
        m_audio_stream_index = stream_index;
        m_audio_codec_context =
            AVCodecContextPtr(avcodec_alloc_context3(decoder), &ReleaseAVCodecContext);
        if (avcodec_parameters_to_context(m_audio_codec_context.get(), stream->codecpar) < 0) {
            return -1;
        }
        if (avcodec_open2(m_audio_codec_context.get(), decoder, nullptr) < 0) {
            return -1;
        }
        LOG_INFO(Lib_AvPlayer, "Audio stream {} enabled", stream_index);
        break;
    }
    default:
        LOG_WARNING(Lib_AvPlayer, "Unknown stream type {} for stream {}",
                    magic_enum::enum_name(stream->codecpar->codec_type), stream_index);
        break;
    }
    return 0;
}

u8* AvPlayerSource::GetVideoBuffer(AVFrame* frame) {
    const auto size = (frame->width * frame->height * 3) / 2;
    if (m_video_frame_storage.size() < size) {
        if (!m_video_frame_storage.empty()) {
            m_memory_replacement.deallocate(m_memory_replacement.object_ptr,
                                            m_video_frame_storage.data());
        }
        const auto ptr = reinterpret_cast<u8*>(
            m_memory_replacement.allocate(m_memory_replacement.object_ptr, frame->width, size));
        m_video_frame_storage = std::span<u8>(ptr, size);
    }
    return m_video_frame_storage.data();
}

u8* AvPlayerSource::GetAudioBuffer(AVFrame* frame) {
    const auto size = frame->ch_layout.nb_channels * frame->nb_samples * sizeof(u16);
    if (m_audio_frame_storage.size() < size) {
        if (!m_audio_frame_storage.empty()) {
            m_memory_replacement.deallocate(m_memory_replacement.object_ptr,
                                            m_audio_frame_storage.data());
        }
        const auto ptr = reinterpret_cast<u8*>(
            m_memory_replacement.allocate(m_memory_replacement.object_ptr, 0x40, size));
        m_audio_frame_storage = std::span<u8>(ptr, size);
    }
    return m_audio_frame_storage.data();
}

void AvPlayerSource::SetLooping(bool is_looping) {
    m_is_looping = is_looping;
}

std::optional<bool> AvPlayerSource::HasFrames(u32 num_frames) {
    return m_video_frames.Size() > num_frames;
}

s32 AvPlayerSource::Start() {
    if (m_audio_codec_context == nullptr && m_video_codec_context == nullptr) {
        return -1;
    }
    {
        ThreadParameters demuxer_params{
            .p_user_data = this,
            .thread_name = "AvPlayer_Demuxer",
            .stack_size = 0x4000,
            .priority = m_priorities.demuxer_priority,
            .affinity = m_priorities.demuxer_affinity,
        };
        m_demuxer_thread = CreateThread(&DemuxerThread, demuxer_params);
        if (m_demuxer_thread == nullptr) {
            return -1;
        }
    }
    if (m_video_codec_context != nullptr) {
        ThreadParameters video_decoder_params{
            .p_user_data = this,
            .thread_name = "AvPlayer_VideoDecoder",
            .stack_size = 0x4000,
            .priority = m_priorities.video_decoder_priority,
            .affinity = m_priorities.video_decoder_affinity,
        };
        m_video_decoder_thread = CreateThread(&VideoDecoderThread, video_decoder_params);
        if (m_video_decoder_thread == nullptr) {
            return -1;
        }
    }
    if (m_audio_codec_context != nullptr) {
        ThreadParameters audio_decoder_params{
            .p_user_data = this,
            .thread_name = "AvPlayer_AudioDecoder",
            .stack_size = 0x4000,
            .priority = m_priorities.audio_decoder_priority,
            .affinity = m_priorities.audio_decoder_affinity,
        };
        m_audio_decoder_thread = CreateThread(&AudioDecoderThread, audio_decoder_params);
        if (m_audio_decoder_thread == nullptr) {
            return -1;
        }
    }
    m_start_time = std::chrono::high_resolution_clock::now();
    return 0;
}

bool AvPlayerSource::Stop() {
    m_is_stop = true;

    void* res = nullptr;
    if (m_video_decoder_thread != nullptr) {
        scePthreadJoin(m_video_decoder_thread, &res);
    }
    if (m_audio_decoder_thread != nullptr) {
        scePthreadJoin(m_audio_decoder_thread, &res);
    }
    if (m_demuxer_thread != nullptr) {
        scePthreadJoin(m_demuxer_thread, &res);
    }
    return true;
}

bool AvPlayerSource::GetVideoData(SceAvPlayerFrameInfo& video_info) {
    SceAvPlayerFrameInfoEx info{};
    if (!GetVideoData(info)) {
        return false;
    }
    video_info = {};
    video_info.timestamp = u64(info.timestamp);
    video_info.pData = reinterpret_cast<u8*>(info.pData);
    video_info.details.video.aspect_ratio = info.details.video.aspect_ratio;
    video_info.details.video.width = info.details.video.width;
    video_info.details.video.height = info.details.video.height;
    return true;
}

static void CopyNV12Data(u8* dst, const AVFrame& src) {
    std::memcpy(dst, src.data[0], src.width * src.height);
    std::memcpy(dst + src.width * src.height, src.data[1], (src.width * src.height) / 2);
}

bool AvPlayerSource::GetVideoData(SceAvPlayerFrameInfoEx& video_info) {
    while (m_video_frames.Size() == 0 && !m_is_eof) {
        sceKernelUsleep(5000);
    }

    auto frame = m_video_frames.Pop();
    if (!frame.has_value()) {
        return false;
    }

    auto& up_frame = *frame;

    const auto pkt_dts = u64(up_frame->pkt_dts) * 1000;
    const auto stream = m_avformat_context->streams[m_video_stream_index.value()];
    const auto time_base = stream->time_base;
    const auto den = time_base.den;
    const auto num = time_base.num;
    const auto timestamp = (num != 0 && den > 1) ? (pkt_dts * num) / den : pkt_dts;

    {
        using namespace std::chrono;
        auto elapsed_time =
            duration_cast<milliseconds>(high_resolution_clock::now() - m_start_time).count();
        while (elapsed_time < timestamp) {
            sceKernelUsleep((timestamp - elapsed_time) * 1000);
            elapsed_time =
                duration_cast<milliseconds>(high_resolution_clock::now() - m_start_time).count();
        }
    }

    auto buffer = GetVideoBuffer(up_frame.get());

    CopyNV12Data(buffer, *up_frame);

    video_info = {};
    video_info.timestamp = timestamp;
    video_info.pData = buffer;
    video_info.details.video.width = up_frame->width;
    video_info.details.video.height = up_frame->height;
    video_info.details.video.aspect_ratio = AVRationalToF32(up_frame->sample_aspect_ratio);
    video_info.details.video.pitch = up_frame->linesize[0];
    video_info.details.video.luma_bit_depth = 8;
    video_info.details.video.chroma_bit_depth = 8;

    m_last_video_timestamp = timestamp;
    return true;
}

bool AvPlayerSource::GetAudioData(SceAvPlayerFrameInfo& audio_info) {
    while (m_audio_frames.Size() == 0 && !m_is_eof) {
        sceKernelUsleep(5000);
    }

    auto frame = m_audio_frames.Pop();
    if (!frame.has_value()) {
        return false;
    }

    const auto& up_frame = *frame;

    const auto pkt_dts = u64(up_frame->pkt_dts) * 1000;
    const auto stream = m_avformat_context->streams[m_audio_stream_index.value()];
    const auto time_base = stream->time_base;
    const auto den = time_base.den;
    const auto num = time_base.num;
    const auto timestamp = (num != 0 && den > 1) ? (pkt_dts * num) / den : pkt_dts;

    {
        using namespace std::chrono;
        auto elapsed_time =
            duration_cast<milliseconds>(high_resolution_clock::now() - m_start_time).count();
        while (elapsed_time < timestamp) {
            sceKernelUsleep((timestamp - elapsed_time) * 1000);
            elapsed_time =
                duration_cast<milliseconds>(high_resolution_clock::now() - m_start_time).count();
        }
    }

    auto buffer = GetAudioBuffer(up_frame.get());
    const auto size = up_frame->ch_layout.nb_channels * up_frame->nb_samples * sizeof(u16);
    std::memcpy(buffer, up_frame->data[0], size);

    audio_info = {};
    audio_info.timestamp = timestamp;
    audio_info.pData = m_audio_frame_storage.data();
    audio_info.details.audio.size = u32(m_audio_frame_storage.size());
    audio_info.details.audio.channel_count = up_frame->ch_layout.nb_channels;
    return true;
}

u64 AvPlayerSource::CurrentTime() {
    // using namespace std::chrono;
    //  return duration_cast<milliseconds>(high_resolution_clock::now() - m_start_time).count();
    return m_last_video_timestamp;
}

bool AvPlayerSource::IsActive() {
    return !m_is_stop && (!m_is_eof || m_audio_packets.Size() != 0 || m_video_packets.Size() != 0 ||
                          m_video_frames.Size() != 0 || m_audio_frames.Size() != 0);
}

void AvPlayerSource::ReleaseAVPacket(AVPacket* packet) {
    if (packet != nullptr) {
        av_packet_free(&packet);
    }
}

void AvPlayerSource::ReleaseAVFrame(AVFrame* frame) {
    if (frame != nullptr) {
        av_frame_free(&frame);
    }
}

void AvPlayerSource::ReleaseAVCodecContext(AVCodecContext* context) {
    if (context != nullptr) {
        avcodec_free_context(&context);
    }
}

void AvPlayerSource::ReleaseSWRContext(SwrContext* context) {
    if (context != nullptr) {
        swr_free(&context);
    }
}

void AvPlayerSource::ReleaseSWSContext(SwsContext* context) {
    if (context != nullptr) {
        sws_freeContext(context);
    }
}

void* AvPlayerSource::DemuxerThread(void* opaque) {
    LOG_TRACE(Lib_AvPlayer, "Demuxer Thread started");
    const auto self = reinterpret_cast<AvPlayerSource*>(opaque);
    if (!self->m_audio_stream_index.has_value() && !self->m_video_stream_index.has_value()) {
        return nullptr;
    }

    while (!self->m_is_stop) {
        if (self->m_video_packets.Size() > 60) {
            sceKernelUsleep(5000);
            continue;
        }
        AVPacketPtr up_packet(av_packet_alloc(), &ReleaseAVPacket);
        const auto res = av_read_frame(self->m_avformat_context, up_packet.get());
        if (res < 0) {
            if (res == AVERROR_EOF) {
                LOG_TRACE(Lib_AvPlayer, "EOF reached in demuxer");
                break;
            } else {
                LOG_ERROR(Lib_AvPlayer, "Could not read AV frame: error = {}", res);
                self->m_state.OnError();
                scePthreadExit(0);
            }
            break;
        }
        if (up_packet->stream_index == self->m_video_stream_index) {
            self->m_video_packets.Push(std::move(up_packet));
        } else if (up_packet->stream_index == self->m_audio_stream_index) {
            self->m_audio_packets.Push(std::move(up_packet));
        }
    }

    self->m_is_eof = true;

    void* res;
    if (self->m_video_decoder_thread) {
        scePthreadJoin(self->m_video_decoder_thread, &res);
    }
    if (self->m_audio_decoder_thread) {
        scePthreadJoin(self->m_audio_decoder_thread, &res);
    }
    self->m_state.OnEOF();

    LOG_TRACE(Lib_AvPlayer, "Demuxer Thread exited normaly");
    scePthreadExit(0);
}

AvPlayerSource::AVFramePtr AvPlayerSource::ConvertVideoFrame(const AVFrame& frame) {
    auto nv12_frame = AVFramePtr{av_frame_alloc(), &ReleaseAVFrame};
    nv12_frame->pts = frame.pts;
    nv12_frame->pkt_dts = frame.pkt_dts;
    nv12_frame->format = AV_PIX_FMT_NV12;
    nv12_frame->width = frame.width;
    nv12_frame->height = frame.height;
    nv12_frame->sample_aspect_ratio = frame.sample_aspect_ratio;

    av_frame_get_buffer(nv12_frame.get(), 0);

    if (m_sws_context == nullptr) {
        m_sws_context =
            SWSContextPtr(sws_getContext(frame.width, frame.height, AVPixelFormat(frame.format),
                                         frame.width, frame.height, AV_PIX_FMT_NV12,
                                         SWS_FAST_BILINEAR, nullptr, nullptr, nullptr),
                          &ReleaseSWSContext);
    }
    const auto res = sws_scale(m_sws_context.get(), frame.data, frame.linesize, 0, frame.height,
                               nv12_frame->data, nv12_frame->linesize);
    if (res < 0) {
        LOG_ERROR(Lib_AvPlayer, "Could not convert to NV12: {}", av_err2str(res));
        return AVFramePtr{nullptr, &ReleaseAVFrame};
    }
    return nv12_frame;
}

void* AvPlayerSource::VideoDecoderThread(void* opaque) {
    LOG_TRACE(Lib_AvPlayer, "Video Decoder Thread started");
    const auto self = reinterpret_cast<AvPlayerSource*>(opaque);

    while ((!self->m_is_eof || self->m_video_packets.Size() != 0) && !self->m_is_stop) {
        if (self->m_video_frames.Size() > 60 || self->m_video_packets.Size() == 0) {
            sceKernelUsleep(5000);
            continue;
        }
        const auto packet = self->m_video_packets.Pop();
        if (!packet.has_value()) {
            continue;
        }
        auto res = avcodec_send_packet(self->m_video_codec_context.get(), packet->get());
        if (res < 0 && res != AVERROR(EAGAIN)) {
            self->m_state.OnError();
            LOG_ERROR(Lib_AvPlayer, "Could not send packet to the video codec. Error = {}",
                      av_err2str(res));
            scePthreadExit(nullptr);
        }
        while (res >= 0) {
            auto up_frame = AVFramePtr(av_frame_alloc(), &ReleaseAVFrame);
            res = avcodec_receive_frame(self->m_video_codec_context.get(), up_frame.get());
            if (res < 0) {
                if (res == AVERROR_EOF) {
                    LOG_TRACE(Lib_AvPlayer, "EOF reached in video decoder");
                    scePthreadExit(nullptr);
                } else if (res != AVERROR(EAGAIN)) {
                    LOG_ERROR(Lib_AvPlayer,
                              "Could not receive frame from the video codec. Error = {}",
                              av_err2str(res));
                    self->m_state.OnError();
                    scePthreadExit(nullptr);
                }
            } else {
                LOG_TRACE(Lib_AvPlayer, "Producing Video Frame. Num Frames: {}",
                          self->m_video_frames.Size());
                if (up_frame->format != AV_PIX_FMT_NV12) {
                    self->m_video_frames.Push(self->ConvertVideoFrame(*up_frame));
                } else {
                    self->m_video_frames.Push(std::move(up_frame));
                }
            }
        }
    }

    LOG_TRACE(Lib_AvPlayer, "Video Decoder Thread exited normaly");
    scePthreadExit(nullptr);
}

AvPlayerSource::AVFramePtr AvPlayerSource::ConvertAudioFrame(const AVFrame& frame) {
    auto pcm16_frame = AVFramePtr{av_frame_alloc(), &ReleaseAVFrame};
    pcm16_frame->pts = frame.pts;
    pcm16_frame->pkt_dts = frame.pkt_dts;
    pcm16_frame->format = AV_SAMPLE_FMT_S16;
    pcm16_frame->ch_layout = frame.ch_layout;
    pcm16_frame->sample_rate = frame.sample_rate;

    if (m_swr_context == nullptr) {
        SwrContext* swr_context = nullptr;
        AVChannelLayout in_ch_layout = frame.ch_layout;
        AVChannelLayout out_ch_layout = frame.ch_layout;
        swr_alloc_set_opts2(&swr_context, &out_ch_layout, AV_SAMPLE_FMT_S16, frame.sample_rate,
                            &in_ch_layout, AVSampleFormat(frame.format), frame.sample_rate, 0,
                            nullptr);
        m_swr_context = SWRContextPtr(swr_context, &ReleaseSWRContext);
        swr_init(m_swr_context.get());
    }
    const auto res = swr_convert_frame(m_swr_context.get(), pcm16_frame.get(), &frame);
    if (res < 0) {
        LOG_ERROR(Lib_AvPlayer, "Could not convert to NV12: {}", av_err2str(res));
        return AVFramePtr{nullptr, &ReleaseAVFrame};
    }
    return pcm16_frame;
}

void* AvPlayerSource::AudioDecoderThread(void* opaque) {
    LOG_TRACE(Lib_AvPlayer, "Audio Decoder Thread started");
    const auto self = reinterpret_cast<AvPlayerSource*>(opaque);

    while ((!self->m_is_eof || self->m_audio_packets.Size() != 0) && !self->m_is_stop) {
        if (self->m_audio_frames.Size() > 60 || self->m_audio_packets.Size() == 0) {
            sceKernelUsleep(5000);
            continue;
        }
        const auto packet = self->m_audio_packets.Pop();
        if (!packet.has_value()) {
            continue;
        }
        auto res = avcodec_send_packet(self->m_audio_codec_context.get(), packet->get());
        if (res < 0 && res != AVERROR(EAGAIN)) {
            self->m_state.OnError();
            LOG_ERROR(Lib_AvPlayer, "Could not send packet to the audio codec. Error = {}",
                      av_err2str(res));
            scePthreadExit(nullptr);
        }
        while (res >= 0) {
            auto up_frame = AVFramePtr(av_frame_alloc(), &ReleaseAVFrame);
            res = avcodec_receive_frame(self->m_audio_codec_context.get(), up_frame.get());
            if (res < 0) {
                if (res == AVERROR_EOF) {
                    LOG_TRACE(Lib_AvPlayer, "EOF reached in audio decoder");
                    scePthreadExit(nullptr);
                } else if (res != AVERROR(EAGAIN)) {
                    self->m_state.OnError();
                    LOG_ERROR(Lib_AvPlayer,
                              "Could not receive frame from the audio codec. Error = {}",
                              av_err2str(res));
                    scePthreadExit(nullptr);
                }
            } else {
                if (up_frame->format != AV_SAMPLE_FMT_S16) {
                    self->m_audio_frames.Push(self->ConvertAudioFrame(*up_frame));
                } else {
                    self->m_audio_frames.Push(std::move(up_frame));
                }
            }
        }
    }

    LOG_TRACE(Lib_AvPlayer, "Audio Decoder Thread exited normaly");
    scePthreadExit(nullptr);
}

} // namespace Libraries::AvPlayer
