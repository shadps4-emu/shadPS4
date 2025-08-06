// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "videodec2_impl.h"

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/videodec/videodec_error.h"

#include "common/support/avdec.h"

namespace Libraries::Vdec2 {

std::vector<OrbisVideodec2AvcPictureInfo> gPictureInfos;
std::vector<OrbisVideodec2LegacyAvcPictureInfo> gLegacyPictureInfos;

static inline void CopyNV12Data(u8* dst, const AVFrame& src) {
    if (src.width == src.linesize[0]) {
        std::memcpy(dst, src.data[0], src.width * src.height);
        std::memcpy(dst + (src.width * src.height), src.data[1], (src.width * src.height) / 2);
        return;
    }

    for (u16 row = 0; row < src.height; row++) {
        u64 dst_offset = row * src.width;
        std::memcpy(dst + dst_offset, src.data[0] + (row * src.linesize[0]), src.width);
    }

    u64 dst_base = src.width * src.height;
    for (u16 row = 0; row < src.height / 2; row++) {
        u64 dst_offset = row * src.width;
        std::memcpy(dst + dst_base + dst_offset, src.data[1] + (row * src.linesize[1]), src.width);
    }
}

VdecDecoder::VdecDecoder(const OrbisVideodec2DecoderConfigInfo& configInfo,
                         const OrbisVideodec2DecoderMemoryInfo& memoryInfo) {
    ASSERT(configInfo.codecType == 1); /* AVC */

    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    ASSERT(codec);

    mCodecContext = avcodec_alloc_context3(codec);
    ASSERT(mCodecContext);
    mCodecContext->width = configInfo.maxFrameWidth;
    mCodecContext->height = configInfo.maxFrameHeight;

    avcodec_open2(mCodecContext, codec, nullptr);
}

VdecDecoder::~VdecDecoder() {
    avcodec_free_context(&mCodecContext);
    sws_freeContext(mSwsContext);

    gPictureInfos.clear();
}

s32 VdecDecoder::Decode(const OrbisVideodec2InputData& inputData,
                        OrbisVideodec2FrameBuffer& frameBuffer,
                        OrbisVideodec2OutputInfo& outputInfo) {
    frameBuffer.isAccepted = false;
    outputInfo.isValid = false;
    outputInfo.isErrorFrame = true;
    outputInfo.pictureCount = 0;

    // Only set frameFormat if the game uses the newer struct version.
    if (outputInfo.thisSize == sizeof(OrbisVideodec2OutputInfo)) {
        outputInfo.frameFormat = 0;
    }

    if (!inputData.auData) {
        return ORBIS_VIDEODEC2_ERROR_ACCESS_UNIT_POINTER;
    }
    if (inputData.auSize == 0) {
        return ORBIS_VIDEODEC2_ERROR_ACCESS_UNIT_SIZE;
    }

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        LOG_ERROR(Lib_Vdec2, "Failed to allocate packet");
        return ORBIS_VIDEODEC2_ERROR_API_FAIL;
    }

    packet->data = (u8*)inputData.auData;
    packet->size = inputData.auSize;
    packet->pts = inputData.ptsData;
    packet->dts = inputData.dtsData;

    int ret = avcodec_send_packet(mCodecContext, packet);
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

    while (true) {
        ret = avcodec_receive_frame(mCodecContext, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            LOG_ERROR(Lib_Vdec2, "Error receiving frame from decoder: {}", ret);
            av_packet_free(&packet);
            av_frame_free(&frame);
            return ORBIS_VIDEODEC2_ERROR_API_FAIL;
        }

        if (frame->format != AV_PIX_FMT_NV12) {
            AVFrame* nv12_frame = ConvertNV12Frame(*frame);
            ASSERT(nv12_frame);
            av_frame_free(&frame);
            frame = nv12_frame;
        }

        CopyNV12Data((u8*)frameBuffer.frameBuffer, *frame);
        frameBuffer.isAccepted = true;

        outputInfo.codecType = 1; // FIXME: Hardcoded to AVC
        outputInfo.frameWidth = frame->width;
        outputInfo.frameHeight = frame->height;
        outputInfo.framePitch = frame->width;
        outputInfo.frameBufferSize = frameBuffer.frameBufferSize;
        outputInfo.frameBuffer = frameBuffer.frameBuffer;

        outputInfo.isValid = true;
        outputInfo.isErrorFrame = false;
        outputInfo.pictureCount = 1; // TODO: 2 pictures for interlaced video

        // For proper compatibility with older games, check the inputted OutputInfo struct size.
        if (outputInfo.thisSize == sizeof(OrbisVideodec2OutputInfo)) {
            // framePitchInBytes only exists in the newer struct.
            outputInfo.framePitchInBytes = frame->width;
            if (outputInfo.isValid) {
                OrbisVideodec2AvcPictureInfo pictureInfo = {};

                pictureInfo.thisSize = sizeof(OrbisVideodec2AvcPictureInfo);
                pictureInfo.isValid = true;

                pictureInfo.ptsData = inputData.ptsData;
                pictureInfo.dtsData = inputData.dtsData;
                pictureInfo.attachedData = inputData.attachedData;

                pictureInfo.frameCropLeftOffset = frame->crop_left;
                pictureInfo.frameCropRightOffset = frame->crop_right;
                pictureInfo.frameCropTopOffset = frame->crop_top;
                pictureInfo.frameCropBottomOffset = frame->crop_bottom;

                gPictureInfos.push_back(pictureInfo);
            }
        } else {
            if (outputInfo.isValid) {
                // If the game uses the older struct versions, we need to use it too.
                OrbisVideodec2LegacyAvcPictureInfo pictureInfo = {};

                pictureInfo.thisSize = sizeof(OrbisVideodec2LegacyAvcPictureInfo);
                pictureInfo.isValid = true;

                pictureInfo.ptsData = inputData.ptsData;
                pictureInfo.dtsData = inputData.dtsData;
                pictureInfo.attachedData = inputData.attachedData;

                pictureInfo.frameCropLeftOffset = frame->crop_left;
                pictureInfo.frameCropRightOffset = frame->crop_right;
                pictureInfo.frameCropTopOffset = frame->crop_top;
                pictureInfo.frameCropBottomOffset = frame->crop_bottom;

                gLegacyPictureInfos.push_back(pictureInfo);
            }
        }
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
    return ORBIS_OK;
}

s32 VdecDecoder::Flush(OrbisVideodec2FrameBuffer& frameBuffer,
                       OrbisVideodec2OutputInfo& outputInfo) {
    frameBuffer.isAccepted = false;
    outputInfo.isValid = false;
    outputInfo.isErrorFrame = true;
    outputInfo.pictureCount = 0;

    // Only set frameFormat if the game uses the newer struct version.
    if (outputInfo.thisSize == sizeof(OrbisVideodec2OutputInfo)) {
        outputInfo.frameFormat = 0;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        LOG_ERROR(Lib_Vdec2, "Failed to allocate frame");
        return ORBIS_VIDEODEC2_ERROR_API_FAIL;
    }

    while (true) {
        int ret = avcodec_receive_frame(mCodecContext, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
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

        CopyNV12Data((u8*)frameBuffer.frameBuffer, *frame);
        frameBuffer.isAccepted = true;

        outputInfo.codecType = 1; // FIXME: Hardcoded to AVC
        outputInfo.frameWidth = frame->width;
        outputInfo.frameHeight = frame->height;
        outputInfo.framePitch = frame->linesize[0];
        outputInfo.frameBufferSize = frameBuffer.frameBufferSize;
        outputInfo.frameBuffer = frameBuffer.frameBuffer;

        outputInfo.isValid = true;
        outputInfo.isErrorFrame = false;
        outputInfo.pictureCount = 1; // TODO: 2 pictures for interlaced video

        // Only set framePitchInBytes if the game uses the newer struct version.
        if (outputInfo.thisSize == sizeof(OrbisVideodec2OutputInfo)) {
            outputInfo.framePitchInBytes = frame->linesize[0];
        }

        // FIXME: Should we add picture info here too?
    }

    av_frame_free(&frame);
    return ORBIS_OK;
}

s32 VdecDecoder::Reset() {
    avcodec_flush_buffers(mCodecContext);
    gPictureInfos.clear();
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
        LOG_ERROR(Lib_Vdec2, "Could not convert to NV12: {}", av_err2str(res));
        return nullptr;
    }

    return nv12_frame;
}

} // namespace Libraries::Vdec2
