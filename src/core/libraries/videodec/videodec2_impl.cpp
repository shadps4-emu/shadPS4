// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "videodec2_impl.h"

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "video_utils.h"
#include "videodec_error.h"

#include "common/support/avdec.h"

namespace Libraries::Videodec2 {

VdecDecoder::VdecDecoder(const OrbisVideodec2DecoderConfigInfo& config_info,
                         const OrbisVideodec2DecoderMemoryInfo& memory_info) {
    const AVCodec* codec = avcodec_find_decoder(
        config_info.codec_type == OrbisVideodec2CodecType::Avc ? AV_CODEC_ID_H264
                                                               : AV_CODEC_ID_HEVC);
    ASSERT(codec);

    m_codec_context = avcodec_alloc_context3(codec);
    ASSERT(m_codec_context);
    m_codec_context->width = config_info.max_frame_width;
    m_codec_context->height = config_info.max_frame_height;
    m_codec_context->flags |= AV_CODEC_FLAG_COPY_OPAQUE;

    avcodec_open2(m_codec_context, codec, nullptr);
}

VdecDecoder::~VdecDecoder() {
    avcodec_free_context(&m_codec_context);
    sws_freeContext(m_sws_context);
}

s32 VdecDecoder::Decode(const OrbisVideodec2InputData& input_data,
                        OrbisVideodec2FrameBuffer& frame_buffer,
                        OrbisVideodec2OutputInfo& output_info) {
    frame_buffer.is_accepted = false;
    output_info.is_valid = false;
    output_info.picture_count = 0;

    if (!input_data.au_data) {
        LOG_ERROR(Lib_Vdec2, "ORBIS_VIDEODEC2_ERROR_ACCESS_UNIT_POINTER");
        return ORBIS_VIDEODEC2_ERROR_ACCESS_UNIT_POINTER;
    }
    if (input_data.au_size == 0) {
        LOG_ERROR(Lib_Vdec2, "ORBIS_VIDEODEC2_ERROR_ACCESS_UNIT_SIZE");
        return ORBIS_VIDEODEC2_ERROR_ACCESS_UNIT_SIZE;
    }

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        LOG_ERROR(Lib_Vdec2, "Failed to allocate packet");
        return ORBIS_VIDEODEC2_ERROR_API_FAIL;
    }

    packet->data = (u8*)input_data.au_data;
    packet->size = input_data.au_size;
    packet->pts = input_data.pts_data;
    packet->dts = input_data.dts_data;
    packet->opaque = reinterpret_cast<void*>(input_data.attached_data);

    int ret = avcodec_send_packet(m_codec_context, packet);
    if (ret == AVERROR_EOF) {
        // Attempt to flush buffers and try again.
        avcodec_flush_buffers(m_codec_context);
        ret = avcodec_send_packet(m_codec_context, packet);
    }
    if (ret < 0) {
        LOG_ERROR(Lib_Vdec2, "Error sending packet to decoder: {}", ret);
        av_packet_free(&packet);
        return ORBIS_VIDEODEC2_ERROR_API_FAIL;
    }

    AVFrame* frame = av_frame_alloc();
    if (frame == nullptr) {
        LOG_ERROR(Lib_Vdec2, "Failed to allocate frame");
        av_packet_free(&packet);
        return ORBIS_VIDEODEC2_ERROR_API_FAIL;
    }

    ret = avcodec_receive_frame(m_codec_context, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        LOG_TRACE(Lib_Vdec2, "AVERROR_EOF or AVERROR(EAGAIN)");
        av_packet_free(&packet);
        av_frame_free(&frame);
        return ORBIS_OK;
    } else if (ret < 0) {
        LOG_ERROR(Lib_Vdec2, "Error receiving frame from decoder: {}", ret);
        av_packet_free(&packet);
        av_frame_free(&frame);
        return ORBIS_VIDEODEC2_ERROR_API_FAIL;
    }

    if (frame->flags & AV_FRAME_FLAG_INTERLACED) {
        LOG_ERROR(Lib_Vdec2, "Interlaced video output is not suported.");
    }

    if (frame->format != AV_PIX_FMT_NV12) {
        AVFrame* nv12_frame = ConvertNV12Frame(*frame);
        ASSERT(nv12_frame);
        av_frame_free(&frame);
        frame = nv12_frame;
    }

    const bool is_avc = m_codec_context->codec_id == AV_CODEC_ID_H264;
    const u64 info_size =
        is_avc ? sizeof(OrbisVideodec2AvcPictureInfo) : sizeof(OrbisVideodec2HevcPictureInfo);
    Videodec::CopyNV12Data((u8*)frame_buffer.frame_buffer,
                           frame_buffer.frame_buffer_size - info_size, *frame);
    frame_buffer.is_accepted = true;

    const auto width = Common::AlignUp<u32>(frame->width, 16);
    const auto pitch = Common::AlignUp<u32>(frame->width, 64);
    const auto height = Common::AlignUp<u32>(frame->height, 16);

    output_info.is_valid = true;
    output_info.is_error_frame = false;
    output_info.picture_count = 1; // TODO: 2 pictures for interlaced video
    output_info.codec_type = is_avc ? OrbisVideodec2CodecType::Avc : OrbisVideodec2CodecType::Hevc;
    output_info.frame_width = width;
    output_info.frame_pitch = pitch;
    output_info.frame_height = height;
    output_info.frame_buffer = frame_buffer.frame_buffer;
    output_info.frame_buffer_size = (pitch * height * 3) / 2;

    // Only set frameFormat and framePitchInBytes if the game uses the newer struct version.
    if (output_info.this_size == sizeof(OrbisVideodec2OutputInfo)) {
        output_info.frame_format = 0;
        output_info.frame_pitch_in_bytes = pitch;
    }

    if (is_avc) {
        auto& picture_info = *(OrbisVideodec2AvcPictureInfo*)((u8*)output_info.frame_buffer +
                                                              output_info.frame_buffer_size);

        picture_info = {};
        picture_info.is_valid = true;

        picture_info.pts_data = frame->pts;
        picture_info.dts_data = frame->pkt_dts;
        picture_info.attached_data = reinterpret_cast<u64>(frame->opaque);

        picture_info.frame_crop_top_offset = 0;
        picture_info.frame_crop_left_offset = 0;
        picture_info.frame_crop_right_offset = pitch - frame->width;
        picture_info.frame_crop_bottom_offset = height - frame->height;
    } else {
        auto& picture_info = *(OrbisVideodec2HevcPictureInfo*)((u8*)output_info.frame_buffer +
                                                               output_info.frame_buffer_size);
        picture_info = {};
        picture_info.is_valid = true;

        picture_info.pts_data = frame->pts;
        picture_info.dts_data = frame->pkt_dts;
        picture_info.attached_data = reinterpret_cast<u64>(frame->opaque);

        picture_info.frame_crop_top_offset = 0;
        picture_info.frame_crop_left_offset = 0;
        picture_info.frame_crop_right_offset = pitch - frame->width;
        picture_info.frame_crop_bottom_offset = height - frame->height;
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
    return ORBIS_OK;
}

s32 VdecDecoder::Flush(OrbisVideodec2FrameBuffer& frame_buffer,
                       OrbisVideodec2OutputInfo& output_info) {
    frame_buffer.is_accepted = false;
    output_info.is_valid = false;
    output_info.picture_count = 0;

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        LOG_ERROR(Lib_Vdec2, "Failed to allocate frame");
        return ORBIS_VIDEODEC2_ERROR_API_FAIL;
    }

    avcodec_send_packet(m_codec_context, nullptr);
    int ret = avcodec_receive_frame(m_codec_context, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        av_frame_free(&frame);
        return ORBIS_OK;
    } else if (ret < 0) {
        LOG_ERROR(Lib_Vdec2, "Error receiving frame from decoder: {}", ret);
        av_frame_free(&frame);
        return ORBIS_VIDEODEC2_ERROR_API_FAIL;
    }

    if (frame->format != AV_PIX_FMT_NV12) {
        AVFrame* nv12_frame = ConvertNV12Frame(*frame);
        ASSERT(nv12_frame);
        av_frame_free(&frame);
        frame = nv12_frame;
    }

    const bool is_avc = m_codec_context->codec_id == AV_CODEC_ID_H264;
    const u64 info_size =
        is_avc ? sizeof(OrbisVideodec2AvcPictureInfo) : sizeof(OrbisVideodec2HevcPictureInfo);
    Videodec::CopyNV12Data((u8*)frame_buffer.frame_buffer,
                           frame_buffer.frame_buffer_size - info_size, *frame);
    frame_buffer.is_accepted = true;

    const auto width = Common::AlignUp<u32>(frame->width, 16);
    const auto pitch = Common::AlignUp<u32>(frame->width, 64);
    const auto height = Common::AlignUp<u32>(frame->height, 16);

    output_info.is_valid = true;
    output_info.is_error_frame = false;
    output_info.picture_count = 1; // TODO: 2 pictures for interlaced video
    output_info.codec_type = is_avc ? OrbisVideodec2CodecType::Avc : OrbisVideodec2CodecType::Hevc;
    output_info.frame_width = width;
    output_info.frame_pitch = pitch;
    output_info.frame_height = height;
    output_info.frame_buffer = frame_buffer.frame_buffer;
    output_info.frame_buffer_size = (pitch * height * 3) / 2;

    // Only set frameFormat and framePitchInBytes if the game uses the newer struct version.
    if (output_info.this_size == sizeof(OrbisVideodec2OutputInfo)) {
        output_info.frame_format = 0;
        output_info.frame_pitch_in_bytes = pitch;
    }

    if (m_codec_context->codec_id == AV_CODEC_ID_H264) {
        auto& picture_info = *(OrbisVideodec2AvcPictureInfo*)((u8*)output_info.frame_buffer +
                                                              output_info.frame_buffer_size);

        picture_info = {};
        picture_info.is_valid = true;

        picture_info.pts_data = frame->pts;
        picture_info.dts_data = frame->pkt_dts;
        picture_info.attached_data = reinterpret_cast<u64>(frame->opaque);

        picture_info.frame_crop_top_offset = 0;
        picture_info.frame_crop_left_offset = 0;
        picture_info.frame_crop_right_offset = pitch - frame->width;
        picture_info.frame_crop_bottom_offset = height - frame->height;
    } else {
        auto& picture_info = *(OrbisVideodec2HevcPictureInfo*)((u8*)output_info.frame_buffer +
                                                               output_info.frame_buffer_size);
        picture_info = {};
        picture_info.is_valid = true;

        picture_info.pts_data = frame->pts;
        picture_info.dts_data = frame->pkt_dts;
        picture_info.attached_data = reinterpret_cast<u64>(frame->opaque);

        picture_info.frame_crop_top_offset = 0;
        picture_info.frame_crop_left_offset = 0;
        picture_info.frame_crop_right_offset = pitch - frame->width;
        picture_info.frame_crop_bottom_offset = height - frame->height;
    }

    av_frame_free(&frame);
    return ORBIS_OK;
}

s32 VdecDecoder::Reset() {
    avcodec_flush_buffers(m_codec_context);
    return ORBIS_OK;
}

AVFrame* VdecDecoder::ConvertNV12Frame(AVFrame& frame) {
    AVFrame* nv12_frame = av_frame_alloc();
    nv12_frame->pts = frame.pts;
    nv12_frame->pkt_dts = frame.pkt_dts < 0 ? 0 : frame.pkt_dts;
    nv12_frame->format = AV_PIX_FMT_NV12;
    nv12_frame->width = frame.width;
    nv12_frame->height = frame.height;
    nv12_frame->sample_aspect_ratio = frame.sample_aspect_ratio;
    nv12_frame->crop_top = frame.crop_top;
    nv12_frame->crop_bottom = frame.crop_bottom;
    nv12_frame->crop_left = frame.crop_left;
    nv12_frame->crop_right = frame.crop_right;
    nv12_frame->opaque = frame.opaque;

    av_frame_get_buffer(nv12_frame, 0);

    if (m_sws_context == nullptr) {
        m_sws_context = sws_getContext(frame.width, frame.height, AVPixelFormat(frame.format),
                                       nv12_frame->width, nv12_frame->height, AV_PIX_FMT_NV12,
                                       SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    }

    const auto res = sws_scale(m_sws_context, frame.data, frame.linesize, 0, frame.height,
                               nv12_frame->data, nv12_frame->linesize);
    if (res < 0) {
        LOG_ERROR(Lib_Vdec2, "Could not convert to NV12: {}", av_err2str(res));
        return nullptr;
    }

    return nv12_frame;
}

} // namespace Libraries::Videodec2
