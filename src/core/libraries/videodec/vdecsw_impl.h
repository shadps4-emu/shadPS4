// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <deque>
#include <optional>

#include "vdecsw.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace Libraries::Vdecsw {

class VdecDecoder {
public:
    VdecDecoder(const OrbisVdecswDecoderConfigInfo& configInfo,
                const OrbisVdecswDecoderMemoryInfo& memoryInfo);
    ~VdecDecoder();

    s32 SetDecodeInput(const OrbisVdecswInputData& inputData);
    s32 SyncDecodeInput(OrbisVdecswInputResult& inputResult);
    s32 SetDecodeOutput(const OrbisVdecswFrameBuffer& frameBuffer);
    s32 SyncDecodeOutput(OrbisVdecswOutputInfo& outputInfo);
    s32 FinalizeDecodeSequence();
    s32 Reset();

private:
    AVFrame* ConvertNV12Frame(AVFrame& frame);
    u32 ReceiveFrames();
    void ClearFrameQueue();

private:
    AVCodecContext* m_codec_context = nullptr;
    SwsContext* m_sws_context = nullptr;
    std::optional<OrbisVdecswInputData> m_pending_input;
    std::optional<OrbisVdecswFrameBuffer> m_pending_output;
    std::deque<AVFrame*> m_frame_queue;
    bool m_finalized = false;
};

} // namespace Libraries::Vdecsw
