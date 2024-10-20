// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <queue>

#include "common/types.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/renderer_vulkan/vk_pipeline_cache.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <Windows.h>
using ThreadID = DWORD;
#else
#include <pthread.h>
#include <signal.h>
using ThreadID = pthread_t;
#endif

namespace Core::Devtools {
class Layer;
namespace Widget {
class FrameGraph;
}
} // namespace Core::Devtools

namespace DebugStateType {

enum class QueueType {
    dcb = 0,
    ccb = 1,
    acb = 2,
};

struct QueueDump {
    QueueType type;
    u32 submit_num;
    u32 num2; // acb: queue_num; else: buffer_in_submit
    std::vector<u32> data;
    uintptr_t base_addr;
};

struct ShaderDump {
    Vulkan::Liverpool::ShaderProgram user_data{};
    std::vector<u32> code{};
};

struct ComputerShaderDump {
    Vulkan::Liverpool::ComputeProgram cs_program{};
    std::vector<u32> code{};
};

struct RegDump {
    bool is_compute{false};
    static constexpr size_t MaxShaderStages = 5;
    Vulkan::Liverpool::Regs regs{};
    std::array<ShaderDump, MaxShaderStages> stages{};
    ComputerShaderDump cs_data{};
};

struct FrameDump {
    u32 frame_id;
    std::vector<QueueDump> queues;
    std::unordered_map<uintptr_t, RegDump> regs; // address -> reg dump
};

class DebugStateImpl {
    friend class Core::Devtools::Layer;
    friend class Core::Devtools::Widget::FrameGraph;

    std::mutex guest_threads_mutex{};
    std::vector<ThreadID> guest_threads{};
    std::atomic_bool is_guest_threads_paused = false;
    u64 pause_time{};

    std::atomic_int32_t flip_frame_count = 0;
    std::atomic_int32_t gnm_frame_count = 0;

    s32 gnm_frame_dump_request_count = -1;
    std::unordered_map<size_t, FrameDump*> waiting_reg_dumps;
    std::unordered_map<size_t, std::string> waiting_reg_dumps_dbg;
    bool waiting_submit_pause = false;
    bool should_show_frame_dump = false;

    std::shared_mutex frame_dump_list_mutex;
    std::vector<FrameDump> frame_dump_list{};

    std::queue<std::string> debug_message_popup;

public:
    void ShowDebugMessage(std::string message) {
        if (message.empty()) {
            return;
        }
        debug_message_popup.push(std::move(message));
    }

    void AddCurrentThreadToGuestList();

    void RemoveCurrentThreadFromGuestList();

    void PauseGuestThreads();

    void ResumeGuestThreads();

    bool IsGuestThreadsPaused() const {
        return is_guest_threads_paused;
    }

    void IncFlipFrameNum() {
        ++flip_frame_count;
    }

    void IncGnmFrameNum() {
        ++gnm_frame_count;
        --gnm_frame_dump_request_count;
    }

    u32 GetFrameNum() const {
        return flip_frame_count;
    }

    bool DumpingCurrentFrame() const {
        return gnm_frame_dump_request_count > 0;
    }

    bool DumpingCurrentReg() {
        std::shared_lock lock{frame_dump_list_mutex};
        return !waiting_reg_dumps.empty();
    }

    bool ShouldPauseInSubmit() const {
        return waiting_submit_pause && gnm_frame_dump_request_count == 0;
    }

    void RequestFrameDump(s32 count = 1);

    FrameDump& GetFrameDump() {
        return frame_dump_list[frame_dump_list.size() - gnm_frame_dump_request_count];
    }

    void PushQueueDump(QueueDump dump);

    void PushRegsDump(uintptr_t base_addr, uintptr_t header_addr,
                      const AmdGpu::Liverpool::Regs& regs, bool is_compute = false);
};
} // namespace DebugStateType

extern DebugStateType::DebugStateImpl& DebugState;
