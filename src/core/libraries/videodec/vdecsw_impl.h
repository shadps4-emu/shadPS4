// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "vdecsw.h"

#include "core/libraries/kernel/threads.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

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
    s32 TrySyncDecodeInput(OrbisVdecswInputResult& inputResult);
    s32 SetDecodeOutput(const OrbisVdecswFrameBuffer& frameBuffer);
    s32 SyncDecodeOutput(OrbisVdecswOutputInfo& outputInfo);
    s32 TrySyncDecodeOutput(OrbisVdecswOutputInfo& outputInfo);
    s32 FinalizeDecodeSequence();
    s32 Reset();

private:
    struct VdecSwCommand {
        enum class Type {
            Input,
            Flush,
            Reset,
        };
        Type type = Type::Input;
        OrbisVdecswInputData input = {};
    };

    struct CompletedInput {
        void* au_data = nullptr;
        u32 output_frame_count = 0;
        s32 result = 0;
    };

    void WorkerLoop(std::stop_token stop_token);
    void ProcessInput(OrbisVdecswInputData& item);
    void ProcessFlush();
    void ProcessReset();
    void CompleteInput(void* au_data, u32 frame_count, s32 result);
    u32 ReceiveFrames();
    AVFrame* ConvertNV12Frame(AVFrame& frame);
    s32 WriteFrame(OrbisVdecswOutputInfo& outputInfo, AVFrame& frame,
                   const OrbisVdecswFrameBuffer& frameBuffer, bool isLastFrame);
    void ClearFrameQueue();

private:
    AVCodecContext* m_codec_context = nullptr;
    SwsContext* m_sws_context = nullptr;
    bool m_is_avc = false;

    std::condition_variable_any m_input_cv;
    std::condition_variable_any m_output_cv;
    Kernel::Thread m_worker_thread;

    std::mutex m_mutex;
    std::deque<VdecSwCommand> m_command_queue;
    std::deque<CompletedInput> m_completed_inputs;

    s32 m_inflight_inputs = 0;
    bool m_finalized = false;
    std::optional<OrbisVdecswFrameBuffer> m_pending_output;
    std::deque<AVFrame*> m_frame_queue;
};

} // namespace Libraries::Vdecsw
