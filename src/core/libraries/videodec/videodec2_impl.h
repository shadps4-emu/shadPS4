// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>

#include "videodec2.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace Libraries::Videodec2 {

extern std::vector<OrbisVideodec2AvcPictureInfo> gPictureInfos;
extern std::vector<OrbisVideodec2LegacyAvcPictureInfo> gLegacyPictureInfos;

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
    AVFrame* ConvertNV12Frame(AVFrame& frame);

private:
    AVCodecContext* mCodecContext = nullptr;
    SwsContext* mSwsContext = nullptr;
};

} // namespace Libraries::Videodec2