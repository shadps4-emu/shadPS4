// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "videodec2_impl.h"

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"

namespace Libraries::Vdec2 {

static inline void CopyNV12Data(u8* dst, const AVFrame& src) {
    const auto width = Common::AlignUp(u32(src.width), 16);
    const auto height = Common::AlignUp(u32(src.height), 16);

    if (src.width == width) {
        std::memcpy(dst, src.data[0], src.width * src.height);
        std::memcpy(dst + src.width * height, src.data[1], (src.width * src.height) / 2);
    } else {
        const auto luma_dst = dst;
        for (u32 y = 0; y < src.height; ++y) {
            std::memcpy(luma_dst + y * width, src.data[0] + y * src.width, src.width);
        }
        const auto chroma_dst = dst + width * height;
        for (u32 y = 0; y < src.height / 2; ++y) {
            std::memcpy(chroma_dst + y * (width / 2), src.data[0] + y * (src.width / 2),
                        src.width / 2);
        }
    }
}

Videodec2::Videodec2(const SceVideodec2DecoderConfigInfo& configInfo,
                     const SceVideodec2DecoderMemoryInfo& memoryInfo) {
    ASSERT(configInfo.codecType == 1); /* AVC */

    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);

    mCodecContext = avcodec_alloc_context3(codec);
    mCodecContext->width = configInfo.maxFrameWidth;
    mCodecContext->height = configInfo.maxFrameHeight;

    avcodec_open2(mCodecContext, codec, nullptr);
}

Videodec2::~Videodec2() {
    avcodec_free_context(&mCodecContext);
    sws_freeContext(mSwsContext);
}

s32 Videodec2::Decode(const SceVideodec2InputData& inputData, SceVideodec2FrameBuffer& frameBuffer,
                      SceVideodec2OutputInfo& outputInfo) {
    AVPacket* packet = av_packet_alloc();

    frameBuffer.isAccepted = false;
    outputInfo.isValid = false;
    outputInfo.isErrorFrame = true;
    outputInfo.pictureCount = 0;

    packet->data = (u8*)inputData.pAuData;
    packet->size = inputData.auSize;

    int ret = avcodec_send_packet(mCodecContext, packet);
    if (ret < 0) {
        av_packet_free(&packet);
        return 0;
    }

    AVFrame* frame = av_frame_alloc();
    ret = avcodec_receive_frame(mCodecContext, frame);
    if (ret == AVERROR(EAGAIN)) {
        LOG_WARNING(Lib_Vdec2, "ffmpeg returned EAGAIN");
        av_packet_free(&packet);
        av_frame_free(&frame);
        return 0;
    } else if (ret < 0) {
        av_packet_free(&packet);
        av_frame_free(&frame);
        return 0;
    }

    if (frame->format != AV_PIX_FMT_NV12) {
        AVFrame* nv12_frame = ConvertNV12Frame(*frame);
        av_frame_free(&frame);
        frame = nv12_frame;
    }

    CopyNV12Data((u8*)frameBuffer.pFrameBuffer, *frame);
    frameBuffer.isAccepted = true;

    outputInfo.frameWidth = frame->width;
    outputInfo.frameHeight = frame->height;
    outputInfo.framePitch = frame->linesize[0];
    outputInfo.frameBufferSize = frameBuffer.frameBufferSize;
    outputInfo.pFrameBuffer = frameBuffer.pFrameBuffer;

    outputInfo.isValid = true;
    outputInfo.isErrorFrame = false;
    outputInfo.pictureCount = 1;

    av_packet_free(&packet);
    av_frame_free(&frame);
    return 0;
}

void Videodec2::Flush() {
    avcodec_flush_buffers(mCodecContext);
}

AVFrame* Videodec2::ConvertNV12Frame(AVFrame& frame) {
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
        LOG_ERROR(Lib_Vdec2, "Could not convert to NV12: {}", av_err2str(res));
        return nullptr;
    }

    return nv12_frame;
}

} // namespace Libraries::Vdec2