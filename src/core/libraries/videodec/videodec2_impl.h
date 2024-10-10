// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "videodec2.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace Libraries::Vdec2 {

class Videodec2 {
public:
    Videodec2(const SceVideodec2DecoderConfigInfo& configInfo,
              const SceVideodec2DecoderMemoryInfo& memoryInfo);
    ~Videodec2();

    s32 Decode(const SceVideodec2InputData& inputData, SceVideodec2FrameBuffer& frameBuffer,
               SceVideodec2OutputInfo& outputInfo);
    void Flush();

private:
    AVFrame* ConvertNV12Frame(AVFrame& frame);

private:
    AVCodecContext* mCodecContext = nullptr;
    SwsContext* mSwsContext = nullptr;
};

} // namespace Libraries::Vdec2