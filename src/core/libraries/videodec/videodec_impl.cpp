// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "videodec_impl.h"

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "video_utils.h"
#include "videodec_error.h"

#include "common/support/avdec.h"

namespace Libraries::Videodec {

VdecDecoder::VdecDecoder(const OrbisVideodecConfigInfo& pCfgInfoIn,
                         const OrbisVideodecResourceInfo& pRsrcInfoIn) {

    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    ASSERT(codec);

    mCodecContext = avcodec_alloc_context3(codec);
    ASSERT(mCodecContext);
    mCodecContext->width = pCfgInfoIn.maxFrameWidth;
    mCodecContext->height = pCfgInfoIn.maxFrameHeight;

    avcodec_open2(mCodecContext, codec, nullptr);
}

VdecDecoder::~VdecDecoder() {
    avcodec_free_context(&mCodecContext);
    sws_freeContext(mSwsContext);
}

s32 VdecDecoder::Decode(const OrbisVideodecInputData& pInputDataIn,
                        OrbisVideodecFrameBuffer& pFrameBufferInOut,
                        OrbisVideodecPictureInfo& pPictureInfoOut) {
    pPictureInfoOut.isValid = false;

    if (!pInputDataIn.pAuData) {
        return ORBIS_VIDEODEC_ERROR_AU_POINTER;
    }
    if (pInputDataIn.auSize == 0) {
        return ORBIS_VIDEODEC_ERROR_AU_SIZE;
    }

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        LOG_ERROR(Lib_Videodec, "Failed to allocate packet");
        return ORBIS_VIDEODEC_ERROR_API_FAIL;
    }

    packet->data = (u8*)pInputDataIn.pAuData;
    packet->size = pInputDataIn.auSize;
    packet->pts = pInputDataIn.ptsData;
    packet->dts = pInputDataIn.dtsData;

    int ret = avcodec_send_packet(mCodecContext, packet);
    if (ret < 0) {
        LOG_ERROR(Lib_Videodec, "Error sending packet to decoder: {}", ret);
        av_packet_free(&packet);
        return ORBIS_VIDEODEC_ERROR_API_FAIL;
    }

    AVFrame* frame = av_frame_alloc();
    if (frame == nullptr) {
        LOG_ERROR(Lib_Videodec, "Failed to allocate frame");
        av_packet_free(&packet);
        return ORBIS_VIDEODEC_ERROR_API_FAIL;
    }

    ret = avcodec_receive_frame(mCodecContext, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        av_packet_free(&packet);
        av_frame_free(&frame);
        return ORBIS_OK;
    } else if (ret < 0) {
        LOG_ERROR(Lib_Videodec, "Error receiving frame from decoder: {}", ret);
        av_packet_free(&packet);
        av_frame_free(&frame);
        return ORBIS_VIDEODEC_ERROR_API_FAIL;
    }

    if (frame->format != AV_PIX_FMT_NV12) {
        AVFrame* nv12_frame = ConvertNV12Frame(*frame);
        ASSERT(nv12_frame);
        av_frame_free(&frame);
        frame = nv12_frame;
    }

    CopyNV12Data((u8*)pFrameBufferInOut.pFrameBuffer, *frame);

    const auto width = Common::AlignUp<u32>(frame->width, 16);
    const auto pitch = Common::AlignUp<u32>(frame->width, 64);
    const auto height = Common::AlignUp<u32>(frame->height, 16);

    pPictureInfoOut.codecType = 0;
    pPictureInfoOut.frameWidth = width;
    pPictureInfoOut.framePitch = pitch;
    pPictureInfoOut.frameHeight = height;

    pPictureInfoOut.isValid = true;
    pPictureInfoOut.isErrorPic = false;

    pPictureInfoOut.codec.avc.frameCropTopOffset = 0;
    pPictureInfoOut.codec.avc.frameCropLeftOffset = 0;
    pPictureInfoOut.codec.avc.frameCropRightOffset = pitch - frame->width;
    pPictureInfoOut.codec.avc.frameCropBottomOffset = height - frame->height;
    pPictureInfoOut.attachedData = pInputDataIn.attachedData;

    av_packet_free(&packet);
    av_frame_free(&frame);
    return ORBIS_OK;
}

s32 VdecDecoder::Flush(OrbisVideodecFrameBuffer& pFrameBufferInOut,
                       OrbisVideodecPictureInfo& pPictureInfoOut) {
    pPictureInfoOut.isValid = false;

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        LOG_ERROR(Lib_Videodec, "Failed to allocate frame");
        return ORBIS_VIDEODEC_ERROR_API_FAIL;
    }

    // avcodec_send_packet with packet set to nullptr results in codec flush.
    // This flush can produce multiple frames but we can only return one.
    // We cannot skip frames, some games hang if we do so we return only 1st.
    // Games can send multiple consecutive flushes, but ffmpeg doesn't work
    // this way and returns error. There is no way to make it flush only one
    // frame so all we can do is just ignore the result and pray.
    avcodec_send_packet(mCodecContext, nullptr);
    int ret = avcodec_receive_frame(mCodecContext, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        av_frame_free(&frame);
        return ORBIS_OK;
    } else if (ret < 0) {
        LOG_ERROR(Lib_Videodec, "Error receiving frame from decoder: {}", ret);
        av_frame_free(&frame);
        return ORBIS_VIDEODEC_ERROR_API_FAIL;
    }

    if (frame->format != AV_PIX_FMT_NV12) {
        AVFrame* nv12_frame = ConvertNV12Frame(*frame);
        ASSERT(nv12_frame);
        av_frame_free(&frame);
        frame = nv12_frame;
    }

    CopyNV12Data((u8*)pFrameBufferInOut.pFrameBuffer, *frame);

    const auto width = Common::AlignUp<u32>(frame->width, 16);
    const auto pitch = Common::AlignUp<u32>(frame->width, 64);
    const auto height = Common::AlignUp<u32>(frame->height, 16);

    pPictureInfoOut.codecType = 0;
    pPictureInfoOut.frameWidth = width;
    pPictureInfoOut.framePitch = pitch;
    pPictureInfoOut.frameHeight = height;

    pPictureInfoOut.isValid = true;
    pPictureInfoOut.isErrorPic = false;

    pPictureInfoOut.codec.avc.frameCropTopOffset = 0;
    pPictureInfoOut.codec.avc.frameCropLeftOffset = 0;
    pPictureInfoOut.codec.avc.frameCropRightOffset = pitch - frame->width;
    pPictureInfoOut.codec.avc.frameCropBottomOffset = height - frame->height;

    av_frame_free(&frame);
    return ORBIS_OK;
}

s32 VdecDecoder::Reset() {
    avcodec_flush_buffers(mCodecContext);
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

    av_frame_get_buffer(nv12_frame, 0);

    if (mSwsContext == nullptr) {
        mSwsContext = sws_getContext(frame.width, frame.height, AVPixelFormat(frame.format),
                                     nv12_frame->width, nv12_frame->height, AV_PIX_FMT_NV12,
                                     SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    }

    const auto res = sws_scale(mSwsContext, frame.data, frame.linesize, 0, frame.height,
                               nv12_frame->data, nv12_frame->linesize);
    if (res < 0) {
        LOG_ERROR(Lib_Videodec, "Could not convert to NV12: {}", av_err2str(res));
        return nullptr;
    }

    return nv12_frame;
}

} // namespace Libraries::Videodec
