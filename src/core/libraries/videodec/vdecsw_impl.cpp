// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "vdecsw_impl.h"

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "video_utils.h"
#include "videodec_error.h"

#include "common/support/avdec.h"

namespace Libraries::Vdecsw {

VdecDecoder::VdecDecoder(const OrbisVdecswDecoderConfigInfo& config_info,
                         const OrbisVdecswDecoderMemoryInfo& memory_info) {
    const AVCodec* codec = avcodec_find_decoder(
        config_info.codec_type == OrbisVdecswCodecType::Avc ? AV_CODEC_ID_H264
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
    ClearFrameQueue();
    avcodec_free_context(&m_codec_context);
    sws_freeContext(m_sws_context);
}

s32 VdecDecoder::SetDecodeInput(const OrbisVdecswInputData& input_data) {
    if (m_pending_input.has_value()) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_INPUT_QUEUE_FULL");
        return ORBIS_VDECSW_ERROR_INPUT_QUEUE_FULL;
    }
    if (!input_data.au_data) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_ACCESS_UNIT_POINTER");
        return ORBIS_VDECSW_ERROR_ACCESS_UNIT_POINTER;
    }
    if (input_data.au_size == 0) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_ACCESS_UNIT_SIZE");
        return ORBIS_VDECSW_ERROR_ACCESS_UNIT_SIZE;
    }

    m_pending_input = input_data;
    return ORBIS_OK;
}

s32 VdecDecoder::SyncDecodeInput(OrbisVdecswInputResult& input_result) {
    if (!m_pending_input.has_value()) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_INPUT_QUEUE_EMPTY");
        return ORBIS_VDECSW_ERROR_INPUT_QUEUE_EMPTY;
    }

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        LOG_ERROR(Lib_Vdecsw, "Failed to allocate packet");
        return ORBIS_VDECSW_ERROR_API_FAIL;
    }

    packet->data = (u8*)m_pending_input->au_data;
    packet->size = m_pending_input->au_size;
    packet->pts = m_pending_input->pts_data;
    packet->dts = m_pending_input->dts_data;
    packet->opaque = reinterpret_cast<void*>(m_pending_input->attached_data);

    int ret = avcodec_send_packet(m_codec_context, packet);
    if (ret == AVERROR_EOF) {
        // Attempt to flush buffers and try again.
        avcodec_flush_buffers(m_codec_context);
        ret = avcodec_send_packet(m_codec_context, packet);
    }
    if (ret < 0) {
        LOG_ERROR(Lib_Vdecsw, "Error sending packet to decoder: {}", ret);
        m_pending_input.reset();
        av_packet_free(&packet);
        return ORBIS_VDECSW_ERROR_API_FAIL;
    }

    const u32 frame_count = ReceiveFrames();

    input_result.decoded_au = m_pending_input->au_data;
    input_result.output_frame_count = frame_count;
    input_result.reserved0 = 0;

    m_pending_input.reset();
    av_packet_free(&packet);
    return ORBIS_OK;
}

s32 VdecDecoder::SetDecodeOutput(const OrbisVdecswFrameBuffer& frame_buffer) {
    if (m_pending_output.has_value()) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_OUTPUT_BUFFER_FULL");
        return ORBIS_VDECSW_ERROR_OUTPUT_BUFFER_FULL;
    }
    if (!frame_buffer.frame_buffer) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_FRAME_BUFFER_POINTER");
        return ORBIS_VDECSW_ERROR_FRAME_BUFFER_POINTER;
    }
    if (frame_buffer.frame_buffer_size == 0) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_FRAME_BUFFER_SIZE");
        return ORBIS_VDECSW_ERROR_FRAME_BUFFER_SIZE;
    }

    m_pending_output = frame_buffer;
    return ORBIS_OK;
}

s32 VdecDecoder::SyncDecodeOutput(OrbisVdecswOutputInfo& output_info) {
    output_info.is_valid = false;
    output_info.is_last_frame = false;
    output_info.is_error_frame = false;
    output_info.is_discarded_frame = false;
    output_info.picture_count = 0;

    if (!m_pending_output.has_value()) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_OUTPUT_BUFFER_EMPTY");
        return ORBIS_VDECSW_ERROR_OUTPUT_BUFFER_EMPTY;
    }
    if (m_frame_queue.empty()) {
        if (m_finalized) {
            m_pending_output.reset();
            return ORBIS_OK;
        }
        LOG_TRACE(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_DECODE_PENDING");
        return ORBIS_VDECSW_ERROR_DECODE_PENDING;
    }

    AVFrame* frame = m_frame_queue.front();
    m_frame_queue.pop_front();

    const bool is_avc = m_codec_context->codec_id == AV_CODEC_ID_H264;
    const u64 info_size =
        is_avc ? sizeof(OrbisVdecswAvcPictureInfo) : sizeof(OrbisVdecswHevcPictureInfo);
    Videodec::CopyNV12Data((u8*)m_pending_output->frame_buffer,
                           m_pending_output->frame_buffer_size - info_size, *frame);

    const auto width = Common::AlignUp<u32>(frame->width, 16);
    const auto pitch = Common::AlignUp<u32>(frame->width, 64);
    const auto height = Common::AlignUp<u32>(frame->height, 16);

    output_info.is_valid = true;
    output_info.is_last_frame = m_finalized && m_frame_queue.empty();
    output_info.picture_count = 1; // TODO: 2 pictures for interlaced video
    output_info.codec_type = is_avc ? OrbisVdecswCodecType::Avc : OrbisVdecswCodecType::Hevc;
    output_info.frame_width = width;
    output_info.frame_pitch = pitch;
    output_info.frame_height = height;
    output_info.frame_buffer = m_pending_output->frame_buffer;
    output_info.frame_buffer_size = (pitch * height * 3) / 2;

    // Only set frameFormat and framePitchInBytes if the game uses the newer struct version.
    if (output_info.this_size == sizeof(OrbisVdecswOutputInfo)) {
        output_info.frame_format = 0;
        output_info.frame_pitch_in_bytes = pitch;
    }

    if (is_avc) {
        auto& picture_info = *(OrbisVdecswAvcPictureInfo*)((u8*)output_info.frame_buffer +
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
        auto& picture_info = *(OrbisVdecswHevcPictureInfo*)((u8*)output_info.frame_buffer +
                                                            output_info.frame_buffer_size);

        picture_info = {};
        picture_info.is_valid = true;

        picture_info.pts_data = frame->pts;
        picture_info.dts_data = frame->pkt_dts;
        picture_info.attached_data = reinterpret_cast<u64>(frame->opaque);

        picture_info.pic_width_in_luma_samples = frame->width;
        picture_info.pic_height_in_luma_samples = frame->height;

        picture_info.frame_crop_top_offset = 0;
        picture_info.frame_crop_left_offset = 0;
        picture_info.frame_crop_right_offset = pitch - frame->width;
        picture_info.frame_crop_bottom_offset = height - frame->height;
    }

    m_pending_output.reset();
    av_frame_free(&frame);
    return ORBIS_OK;
}

s32 VdecDecoder::FinalizeDecodeSequence() {
    avcodec_send_packet(m_codec_context, nullptr);
    ReceiveFrames();
    m_finalized = true;
    return ORBIS_OK;
}

s32 VdecDecoder::Reset() {
    ClearFrameQueue();
    m_pending_input.reset();
    m_pending_output.reset();
    m_finalized = false;
    avcodec_flush_buffers(m_codec_context);
    return ORBIS_OK;
}

u32 VdecDecoder::ReceiveFrames() {
    u32 frame_count = 0;

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        LOG_ERROR(Lib_Vdecsw, "Failed to allocate frame");
        return 0;
    }

    for (;;) {
        int ret = avcodec_receive_frame(m_codec_context, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            LOG_ERROR(Lib_Vdecsw, "Error receiving frame from decoder: {}", ret);
            break;
        }

        if (frame->flags & AV_FRAME_FLAG_INTERLACED) {
            LOG_ERROR(Lib_Vdecsw, "Interlaced video output is not suported.");
        }

        if (frame->format != AV_PIX_FMT_NV12) {
            AVFrame* nv12_frame = ConvertNV12Frame(*frame);
            ASSERT(nv12_frame);
            if (nv12_frame) {
                m_frame_queue.push_back(nv12_frame);
                ++frame_count;
            }
        } else {
            AVFrame* clone = av_frame_clone(frame);
            if (clone) {
                m_frame_queue.push_back(clone);
                ++frame_count;
            }
        }
        av_frame_unref(frame);
    }

    av_frame_free(&frame);
    return frame_count;
}

void VdecDecoder::ClearFrameQueue() {
    while (!m_frame_queue.empty()) {
        AVFrame* frame = m_frame_queue.front();
        m_frame_queue.pop_front();
        av_frame_free(&frame);
    }
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
        LOG_ERROR(Lib_Vdecsw, "Could not convert to NV12: {}", av_err2str(res));
        av_frame_free(&nv12_frame);
        return nullptr;
    }

    return nv12_frame;
}

} // namespace Libraries::Vdecsw
