// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <vector>
#include <queue>

#include "videodec2.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace Libraries::Videodec2 {

class VdecDecoder {
public:
    VdecDecoder(const OrbisVideodec2DecoderConfigInfo& configInfo,
                const OrbisVideodec2DecoderMemoryInfo& memoryInfo);
    ~VdecDecoder();

    s32 Decode(const OrbisVideodec2InputData& inputData, OrbisVideodec2FrameBuffer& frameBuffer,
               OrbisVideodec2OutputInfo& outputInfo);
    s32 Flush(OrbisVideodec2FrameBuffer& frameBuffer, OrbisVideodec2OutputInfo& outputInfo);
    s32 Reset();

private:
    static void ReleaseAVPacket(AVPacket* packet);
    static void ReleaseAVFrame(AVFrame* frame);
    static void ReleaseAVCodecContext(AVCodecContext* context);
    static void ReleaseSwsContext(SwsContext* context);

    using AVPacketPtr = std::unique_ptr<AVPacket, decltype(&ReleaseAVPacket)>;
    using AVFramePtr = std::unique_ptr<AVFrame, decltype(&ReleaseAVFrame)>;
    using AVCodecContextPtr = std::unique_ptr<AVCodecContext, decltype(&ReleaseAVCodecContext)>;
    using SwsContextPtr = std::unique_ptr<SwsContext, decltype(&ReleaseSwsContext)>;

    void DrainFrames();
    AVFramePtr ConvertNV12Frame(AVFrame& frame);

private:
    AVCodecContextPtr m_codec_context{nullptr, &ReleaseAVCodecContext};
    SwsContextPtr m_sws_context{nullptr, &ReleaseSwsContext};
    std::queue<AVFramePtr> m_frame_queue;
};

} // namespace Libraries::Videodec2
