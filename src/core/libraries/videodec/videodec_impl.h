// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>

#include "videodec.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace Libraries::Videodec {

class VdecDecoder {
public:
    VdecDecoder(const OrbisVideodecConfigInfo& pCfgInfoIn,
                const OrbisVideodecResourceInfo& pRsrcInfoIn);
    ~VdecDecoder();
    s32 Decode(const OrbisVideodecInputData& pInputDataIn,
               OrbisVideodecFrameBuffer& pFrameBufferInOut,
               OrbisVideodecPictureInfo& pPictureInfoOut);
    s32 Flush(OrbisVideodecFrameBuffer& pFrameBufferInOut,
              OrbisVideodecPictureInfo& pPictureInfoOut);
    s32 Reset();

private:
    AVFrame* ConvertNV12Frame(AVFrame& frame);

private:
    AVCodecContext* mCodecContext = nullptr;
    SwsContext* mSwsContext = nullptr;
};

} // namespace Libraries::Videodec