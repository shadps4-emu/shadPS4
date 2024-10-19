// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>

#include "common/assert.h"
#include "common/native_clock.h"
#include "common/singleton.h"
#include "debug_state.h"
#include "devtools/widget/common.h"
#include "libraries/kernel/time_management.h"
#include "libraries/system/msgdialog.h"
#include "video_core/amdgpu/pm4_cmds.h"

using namespace DebugStateType;

DebugStateImpl& DebugState = *Common::Singleton<DebugStateImpl>::Instance();

static ThreadID ThisThreadID() {
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

static void PauseThread(ThreadID id) {
#ifdef _WIN32
    auto handle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, id);
    SuspendThread(handle);
    CloseHandle(handle);
#else
    pthread_kill(id, SIGUSR1);
#endif
}

static void ResumeThread(ThreadID id) {
#ifdef _WIN32
    auto handle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, id);
    ResumeThread(handle);
    CloseHandle(handle);
#else
    pthread_kill(id, SIGUSR1);
#endif
}

void DebugStateImpl::AddCurrentThreadToGuestList() {
    std::lock_guard lock{guest_threads_mutex};
    const ThreadID id = ThisThreadID();
    guest_threads.push_back(id);
}

void DebugStateImpl::RemoveCurrentThreadFromGuestList() {
    std::lock_guard lock{guest_threads_mutex};
    const ThreadID id = ThisThreadID();
    std::erase_if(guest_threads, [&](const ThreadID& v) { return v == id; });
}

void DebugStateImpl::PauseGuestThreads() {
    using namespace Libraries::MsgDialog;
    std::unique_lock lock{guest_threads_mutex};
    if (is_guest_threads_paused) {
        return;
    }
    if (ShouldPauseInSubmit()) {
        waiting_submit_pause = false;
        should_show_frame_dump = true;
    }
    bool self_guest = false;
    ThreadID self_id = ThisThreadID();
    for (const auto& id : guest_threads) {
        if (id == self_id) {
            self_guest = true;
        } else {
            PauseThread(id);
        }
    }
    pause_time = Libraries::Kernel::Dev::GetClock()->GetUptime();
    is_guest_threads_paused = true;
    lock.unlock();
    if (self_guest) {
        PauseThread(self_id);
    }
}

void DebugStateImpl::ResumeGuestThreads() {
    std::lock_guard lock{guest_threads_mutex};
    if (!is_guest_threads_paused) {
        return;
    }

    u64 delta_time = Libraries::Kernel::Dev::GetClock()->GetUptime() - pause_time;
    Libraries::Kernel::Dev::GetInitialPtc() += delta_time;
    for (const auto& id : guest_threads) {
        ResumeThread(id);
    }
    is_guest_threads_paused = false;
}

void DebugStateImpl::RequestFrameDump(s32 count) {
    ASSERT(!DumpingCurrentFrame());
    gnm_frame_dump_request_count = count;
    frame_dump_list.clear();
    frame_dump_list.resize(count);
    const auto f = gnm_frame_count.load() + 1;
    for (size_t i = 0; i < count; ++i) {
        frame_dump_list[i].frame_id = f + i;
    }
    waiting_submit_pause = true;
}

void DebugStateImpl::PushQueueDump(QueueDump dump) {
    ASSERT(DumpingCurrentFrame());
    std::unique_lock lock{frame_dump_list_mutex};
    auto& frame = GetFrameDump();
    { // Find draw calls
        auto data = std::span{dump.data};
        auto initial_data = data.data();
        while (!data.empty()) {
            const auto* header = reinterpret_cast<const AmdGpu::PM4Type3Header*>(data.data());
            const auto type = header->type;
            if (type == 2) {
                data = data.subspan(1);
            } else if (type != 3) {
                UNREACHABLE();
            }
            const AmdGpu::PM4ItOpcode opcode = header->opcode;
            if (Core::Devtools::Widget::IsDrawCall(opcode)) {
                const auto offset =
                    reinterpret_cast<uintptr_t>(header) - reinterpret_cast<uintptr_t>(initial_data);
                const auto addr = dump.base_addr + offset;
                waiting_reg_dumps.emplace(addr, &frame);
                waiting_reg_dumps_dbg.emplace(
                    addr,
                    fmt::format("#{} h({}) queue {} {} {}",
                                frame_dump_list.size() - gnm_frame_dump_request_count, addr,
                                magic_enum::enum_name(dump.type), dump.submit_num, dump.num2));
            }
            data = data.subspan(header->NumWords() + 1);
        }
    }
    frame.queues.push_back(std::move(dump));
}

void DebugStateImpl::PushRegsDump(uintptr_t base_addr, uintptr_t header_addr,
                                  const AmdGpu::Liverpool::Regs& regs, bool is_compute) {
    std::scoped_lock lock{frame_dump_list_mutex};
    const auto it = waiting_reg_dumps.find(header_addr);
    if (it == waiting_reg_dumps.end()) {
        return;
    }
    auto& frame = *it->second;
    waiting_reg_dumps.erase(it);
    waiting_reg_dumps_dbg.erase(waiting_reg_dumps_dbg.find(header_addr));
    auto& dump = frame.regs[header_addr - base_addr];
    dump.regs = regs;
    if (is_compute) {
        dump.is_compute = true;
        const auto& cs = dump.regs.cs_program;
        dump.cs_data = ComputerShaderDump{
            .cs_program = cs,
            .code = std::vector<u32>{cs.Code().begin(), cs.Code().end()},
        };
    } else {
        for (int i = 0; i < RegDump::MaxShaderStages; i++) {
            if (regs.stage_enable.IsStageEnabled(i)) {
                auto stage = regs.ProgramForStage(i);
                if (stage->address_lo != 0) {
                    auto code = stage->Code();
                    dump.stages[i] = ShaderDump{
                        .user_data = *stage,
                        .code = std::vector<u32>{code.begin(), code.end()},
                    };
                }
            }
        }
    }
}
