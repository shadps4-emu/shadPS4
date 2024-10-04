// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <mutex>
#include <vector>
#include <queue>

#include "common/types.h"

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
    acb,
    dcb,
    ccb,
};

struct QueueDump {
    QueueType type;
    u32 submit_num;
    u32 num2; // acb: queue_num; else: buffer_in_submit
    std::vector<u32> data;
};

struct FrameDump {
    std::vector<QueueDump> queues;
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
    bool waiting_submit_pause = false;
    bool should_show_frame_dump = false;

    std::mutex frame_dump_list_mutex;
    std::vector<FrameDump> frame_dump_list{};

    std::queue<std::string> debug_message_popup;

public:
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

    bool ShouldPauseInSubmit() const {
        return waiting_submit_pause && gnm_frame_dump_request_count == 0;
    }

    void RequestFrameDump(s32 count = 1);

    FrameDump& GetFrameDump() {
        return frame_dump_list[frame_dump_list.size() - gnm_frame_dump_request_count];
    }

    void PushQueueDump(QueueDump dump) {
        std::unique_lock lock{frame_dump_list_mutex};
        GetFrameDump().queues.push_back(std::move(dump));
    }

    void ShowDebugMessage(std::string message) {
        if (message.empty()) {
            return;
        }
        debug_message_popup.push(std::move(message));
    }
};
} // namespace DebugStateType

extern DebugStateType::DebugStateImpl& DebugState;
