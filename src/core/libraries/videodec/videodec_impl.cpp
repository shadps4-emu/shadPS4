// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "videodec_impl.h"

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"

// The av_err2str macro in libavutil/error.h does not play nice with C++
#ifdef av_err2str
#undef av_err2str
#include <string>
av_always_inline std::string av_err2string(int errnum) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    return av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, errnum);
}
#define av_err2str(err) av_err2string(err).c_str()
#endif // av_err2str

namespace Libraries::Videodec {

static inline void CopyNV12Data(u8* dst, const AVFrame& src) {
    u32 width = Common::AlignUp((u32)src.width, 16);
    u32 height = Common::AlignUp((u32)src.height, 16);
    std::memcpy(dst, src.data[0], src.width * src.height);
    std::memcpy(dst + src.width * height, src.data[1], (src.width * src.height) / 2);
}

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
    pPictureInfoOut.thisSize = sizeof(OrbisVideodecPictureInfo);
    pPictureInfoOut.isValid = false;
    pPictureInfoOut.isErrorPic = true;

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
    int frameCount = 0;
    while (true) {
        ret = avcodec_receive_frame(mCodecContext, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
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

        pPictureInfoOut.codecType = 0;
        pPictureInfoOut.frameWidth = Common::AlignUp((u32)frame->width, 16);
        pPictureInfoOut.frameHeight = Common::AlignUp((u32)frame->height, 16);
        pPictureInfoOut.framePitch = frame->linesize[0];

        pPictureInfoOut.isValid = true;
        pPictureInfoOut.isErrorPic = false;
        frameCount++;
        if (frameCount > 1) {
            LOG_WARNING(Lib_Videodec, "We have more than 1 frame");
        }
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
    return ORBIS_OK;
}

s32 VdecDecoder::Flush(OrbisVideodecFrameBuffer& pFrameBufferInOut,
                       OrbisVideodecPictureInfo& pPictureInfoOut) {
    pPictureInfoOut.thisSize = sizeof(pPictureInfoOut);
    pPictureInfoOut.isValid = false;
    pPictureInfoOut.isErrorPic = true;

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        LOG_ERROR(Lib_Videodec, "Failed to allocate frame");
        return ORBIS_VIDEODEC_ERROR_API_FAIL;
    }

    int frameCount = 0;
    while (true) {
        int ret = avcodec_receive_frame(mCodecContext, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
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

        pPictureInfoOut.codecType = 0;
        pPictureInfoOut.frameWidth = Common::AlignUp((u32)frame->width, 16);
        pPictureInfoOut.frameHeight = Common::AlignUp((u32)frame->height, 16);
        pPictureInfoOut.framePitch = frame->linesize[0];

        pPictureInfoOut.isValid = true;
        pPictureInfoOut.isErrorPic = false;

        u32 width = Common::AlignUp((u32)frame->width, 16);
        u32 height = Common::AlignUp((u32)frame->height, 16);
        pPictureInfoOut.codec.avc.frameCropLeftOffset = u32(frame->crop_left);
        pPictureInfoOut.codec.avc.frameCropRightOffset =
            u32(frame->crop_right + (width - frame->width));
        pPictureInfoOut.codec.avc.frameCropTopOffset = u32(frame->crop_top);
        pPictureInfoOut.codec.avc.frameCropBottomOffset =
            u32(frame->crop_bottom + (height - frame->height));
        // TODO maybe more avc?

        if (frameCount > 1) {
            LOG_WARNING(Lib_Videodec, "We have more than 1 frame");
        }
    }

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
