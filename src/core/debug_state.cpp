// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>

#include <chrono>

#include "common/assert.h"
#include "common/native_clock.h"
#include "common/singleton.h"
#include "core/signals.h"
#include "debug_state.h"
#include "devtools/widget/common.h"
#include "libraries/kernel/time.h"
#include "libraries/system/msgdialog.h"
#include "video_core/amdgpu/pm4_cmds.h"
#include "video_core/renderer_vulkan/vk_pipeline_cache.h"

using namespace DebugStateType;

DebugStateImpl& DebugState = *Common::Singleton<DebugStateImpl>::Instance();

bool DebugStateType::showing_debug_menu_bar = false;

static ThreadID ThisThreadID() {
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

static bool ThreadIDsEqual(const ThreadID left, const ThreadID right) {
#ifdef _WIN32
    return left == right;
#else
    return pthread_equal(left, right) != 0;
#endif
}

#ifndef _WIN32
namespace {
thread_local std::shared_ptr<Core::DebugPause::Participant> CurrentPauseParticipantOwner;
#ifdef SHADPS4_PAUSE_PROTOCOL_TEST
std::atomic<DebugStateType::PauseRegistrationTestHook> pause_registration_test_hook{};
std::atomic_bool pause_signal_delivery_failure{};
#endif

constexpr auto PauseAcknowledgementTimeout = std::chrono::seconds{5};

struct PauseTarget {
    ThreadID id;
    std::shared_ptr<Core::DebugPause::Participant> participant;
};

int NotifyPauseTarget(const ThreadID id) {
#ifdef SHADPS4_PAUSE_PROTOCOL_TEST
    if (pause_signal_delivery_failure.load(std::memory_order_acquire)) {
        return EAGAIN;
    }
#endif
    return pthread_kill(id, SIGSLEEP);
}

int WakePauseTarget(const std::shared_ptr<Core::DebugPause::Participant>& participant) {
    return participant->NotifyStateChange();
}
} // namespace

#ifdef SHADPS4_PAUSE_PROTOCOL_TEST
void DebugStateType::SetPauseRegistrationTestHook(PauseRegistrationTestHook hook) noexcept {
    pause_registration_test_hook.store(hook, std::memory_order_release);
}

void DebugStateType::SetPauseSignalDeliveryFailureForTest(const bool enabled) noexcept {
    pause_signal_delivery_failure.store(enabled, std::memory_order_release);
}
#endif
#endif

#ifdef _WIN32
static void PauseThread(ThreadID id) {
    auto handle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, id);
    SuspendThread(handle);
    CloseHandle(handle);
}

static void ResumeThread(ThreadID id) {
    auto handle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, id);
    ResumeThread(handle);
    CloseHandle(handle);
}
#endif

#ifndef _WIN32
bool DebugStateImpl::ReconcilePauseBookkeeping() {
    const auto current = pause_protocol.Load();
    if (current.state != Core::DebugPause::State::Running ||
        !is_guest_threads_paused.load(std::memory_order_acquire)) {
        return false;
    }

    // The protocol-owned publication count is independent of guest-list membership. Leave the
    // durable interval open until every participant that entered fail-open has completed.
    if (pause_protocol.RollbackPublicationPending()) {
        return false;
    }

    u64 end_time = Libraries::Kernel::Dev::GetClock()->GetUptime();
    const auto rollback_record = pause_protocol.LoadRollbackRecord();
    if (rollback_record.valid && rollback_record.running_epoch == current.epoch &&
        rollback_record.uptime < end_time) {
        end_time = rollback_record.uptime;
    }

    if (!is_guest_threads_paused.exchange(false, std::memory_order_acq_rel)) {
        return false;
    }
    if (end_time < pause_time) {
        end_time = pause_time;
    }
    Libraries::Kernel::Dev::GetInitialPtc() += end_time - pause_time;
    return true;
}

bool DebugStateImpl::FailOpenPublicationPending() const {
    return pause_protocol.RollbackPublicationPending();
}
#endif

void DebugStateImpl::AddCurrentThreadToGuestList() {
#ifndef _WIN32
    auto pause_participant = std::make_shared<Core::DebugPause::Participant>(pause_protocol);
    CurrentPauseParticipantOwner = pause_participant;
    Core::DebugPause::BindCurrentParticipant(pause_participant.get());
    Core::DebugPause::Snapshot registration_snapshot{};
#endif
    {
        std::lock_guard lock{guest_threads_mutex};
        const ThreadID id = ThisThreadID();
        GuestThreadEntry entry{.id = id};
#ifndef _WIN32
        entry.pause_participant = pause_participant;
        registration_snapshot = pause_protocol.Load();
#endif
        guest_threads.push_back(std::move(entry));
    }
#ifndef _WIN32
#ifdef SHADPS4_PAUSE_PROTOCOL_TEST
    if (const auto hook = pause_registration_test_hook.load(std::memory_order_acquire);
        hook != nullptr) {
        hook();
    }
#endif
    if (registration_snapshot.state == Core::DebugPause::State::Paused &&
        pause_participant->SynchronizeState() == Core::DebugPause::StateWaitResult::WaitError) {
        LOG_ERROR(Core, "Failed to honor guest pause during thread registration: {}",
                  pause_participant->WaitError());
    }
#endif
}

void DebugStateImpl::RemoveCurrentThreadFromGuestList() {
#ifndef _WIN32
    auto pause_participant = CurrentPauseParticipantOwner;
    if (pause_participant != nullptr) {
        pause_participant->Unregister();
    }
#endif
    std::lock_guard lock{guest_threads_mutex};
#ifndef _WIN32
    ReconcilePauseBookkeeping();
#endif
    const ThreadID id = ThisThreadID();
    std::erase_if(guest_threads,
                  [&](const GuestThreadEntry& entry) { return ThreadIDsEqual(entry.id, id); });
#ifndef _WIN32
    Core::DebugPause::ClearCurrentParticipant(pause_participant.get());
    CurrentPauseParticipantOwner.reset();
#endif
}

void DebugStateImpl::PauseGuestThreads() {
    using namespace Libraries::MsgDialog;
#ifndef _WIN32
    auto pause_request = pause_protocol.AcquirePauseRequest();
    std::vector<PauseTarget> acknowledgements;
    std::shared_ptr<Core::DebugPause::Participant> self_participant;
    bool delivery_failed = false;
    const ThreadID self_id = ThisThreadID();
#endif
    std::unique_lock lock{guest_threads_mutex};
#ifndef _WIN32
    // A participant can fail open after an earlier Pause has returned. Reconcile that durable
    // Running transition before a new Pause records a fresh pause start time.
    (void)ReconcilePauseBookkeeping();
    if (FailOpenPublicationPending()) {
        lock.unlock();
        (void)pause_protocol.WaitForRollbackPublicationFor(PauseAcknowledgementTimeout);
        lock.lock();
        (void)ReconcilePauseBookkeeping();
        if (FailOpenPublicationPending()) {
            return;
        }
    }
    const auto transition = pause_protocol.Request(Core::DebugPause::State::Paused);
    if (transition.exhausted) {
        LOG_ERROR(Core, "Guest pause epoch exhausted at {}", transition.snapshot.epoch);
        return;
    }
#else
    if (is_guest_threads_paused) {
        return;
    }
#endif
#ifndef _WIN32
    if (transition.changed && ShouldPauseInSubmit()) {
#else
    if (ShouldPauseInSubmit()) {
#endif
        waiting_submit_pause = false;
        should_show_frame_dump = true;
    }
    for (const auto& entry : guest_threads) {
#ifndef _WIN32
        if (ThreadIDsEqual(entry.id, self_id)) {
            self_participant = entry.pause_participant;
        } else {
            acknowledgements.push_back({.id = entry.id, .participant = entry.pause_participant});
            if (entry.pause_participant->IsRegistered() &&
                entry.pause_participant->AcknowledgedEpoch() < transition.snapshot.epoch) {
                const int wake_result = WakePauseTarget(entry.pause_participant);
                const int signal_result = NotifyPauseTarget(entry.id);
                if ((wake_result != 0 || signal_result != 0) &&
                    entry.pause_participant->IsRegistered()) {
                    LOG_ERROR(Core, "Failed to deliver guest pause notification at epoch {}: {}",
                              transition.snapshot.epoch,
                              wake_result != 0 ? wake_result : signal_result);
                    delivery_failed = true;
                }
            }
        }
#else
        PauseThread(entry.id);
#endif
    }
#ifndef _WIN32
    if (transition.changed) {
        pause_time = Libraries::Kernel::Dev::GetClock()->GetUptime();
        is_guest_threads_paused = true;
    }
#else
    pause_time = Libraries::Kernel::Dev::GetClock()->GetUptime();
    is_guest_threads_paused = true;
#endif
    lock.unlock();
#ifndef _WIN32

    const auto rollback_pause = [&] {
        const auto rollback = pause_protocol.RollbackPause(transition.snapshot.epoch);
        (void)pause_protocol.WaitForRollbackPublicationFor(PauseAcknowledgementTimeout);
        std::lock_guard list_lock{guest_threads_mutex};
        const auto current = pause_protocol.Load();
        if (current.state != Core::DebugPause::State::Running) {
            return;
        }
        bool reconciled = false;
        if (rollback.changed) {
            if (is_guest_threads_paused.exchange(false, std::memory_order_acq_rel)) {
                const u64 delta_time = Libraries::Kernel::Dev::GetClock()->GetUptime() - pause_time;
                Libraries::Kernel::Dev::GetInitialPtc() += delta_time;
                reconciled = true;
            }
        } else {
            reconciled = ReconcilePauseBookkeeping();
        }
        if (!rollback.changed && !reconciled) {
            return;
        }
        for (const auto& entry : guest_threads) {
            if (ThreadIDsEqual(entry.id, self_id)) {
                continue;
            }
            const int result = WakePauseTarget(entry.pause_participant);
            if (result != 0 && entry.pause_participant->IsRegistered()) {
                LOG_ERROR(Core, "Failed to deliver guest pause rollback wake at epoch {}: {}",
                          current.epoch, result);
            }
        }
    };

    if (delivery_failed) {
        rollback_pause();
        return;
    }

    const auto deadline = std::chrono::steady_clock::now() + PauseAcknowledgementTimeout;
    for (const auto& target : acknowledgements) {
        const auto result =
            target.participant->WaitForAckUntil(transition.snapshot.epoch, deadline);
        if (result == Core::DebugPause::AckWaitResult::Acknowledged ||
            result == Core::DebugPause::AckWaitResult::Unregistered) {
            continue;
        }
        LOG_ERROR(Core, "Guest pause acknowledgement failed at epoch {}: {}",
                  transition.snapshot.epoch,
                  result == Core::DebugPause::AckWaitResult::TimedOut ? "deadline expired"
                                                                      : "wait error");
        rollback_pause();
        return;
    }

    for (const auto& target : acknowledgements) {
        if (target.participant->WaitError() != 0) {
            LOG_ERROR(Core, "Guest pause target wait failed at epoch {}: {}",
                      transition.snapshot.epoch, target.participant->WaitError());
            rollback_pause();
            return;
        }
    }

    if (self_participant != nullptr &&
        self_participant->SynchronizeState() == Core::DebugPause::StateWaitResult::WaitError) {
        LOG_ERROR(Core, "Self guest pause failed at epoch {}: {}", transition.snapshot.epoch,
                  self_participant->WaitError());
        rollback_pause();
    }
#endif
}

void DebugStateImpl::ResumeGuestThreads() {
    std::lock_guard lock{guest_threads_mutex};
#ifndef _WIN32
    const auto transition = pause_protocol.Request(Core::DebugPause::State::Running);
    if (transition.exhausted) {
        LOG_ERROR(Core, "Guest resume epoch exhausted at {}", transition.snapshot.epoch);
        return;
    }
    if (FailOpenPublicationPending()) {
        (void)pause_protocol.WaitForRollbackPublicationFor(PauseAcknowledgementTimeout);
    }
    const bool reconciled = ReconcilePauseBookkeeping();
    const bool publication_pending = FailOpenPublicationPending();
    bool had_pause_bookkeeping = false;
    if (!publication_pending) {
        had_pause_bookkeeping = is_guest_threads_paused.exchange(false, std::memory_order_acq_rel);
    }
    if (!transition.changed && !had_pause_bookkeeping && !reconciled && !publication_pending) {
        return;
    }
#else
    if (!is_guest_threads_paused) {
        return;
    }
    const bool had_pause_bookkeeping = true;
#endif

    if (had_pause_bookkeeping) {
        u64 delta_time = Libraries::Kernel::Dev::GetClock()->GetUptime() - pause_time;
        Libraries::Kernel::Dev::GetInitialPtc() += delta_time;
    }
    for (const auto& entry : guest_threads) {
#ifndef _WIN32
        const int result = WakePauseTarget(entry.pause_participant);
        if (result != 0 && entry.pause_participant->IsRegistered()) {
            LOG_ERROR(Core, "Failed to deliver guest resume wake at epoch {}: {}",
                      transition.snapshot.epoch, result);
        }
#else
        ResumeThread(entry.id);
#endif
    }
#ifndef _WIN32
    if (publication_pending) {
        (void)ReconcilePauseBookkeeping();
    }
#endif
#ifdef _WIN32
    is_guest_threads_paused = false;
#endif
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

std::optional<RegDump*> DebugStateImpl::GetRegDump(uintptr_t base_addr, uintptr_t header_addr) {
    const auto it = waiting_reg_dumps.find(header_addr);
    if (it == waiting_reg_dumps.end()) {
        return std::nullopt;
    }
    auto& frame = *it->second;
    waiting_reg_dumps.erase(it);
    waiting_reg_dumps_dbg.erase(waiting_reg_dumps_dbg.find(header_addr));
    return &frame.regs[header_addr - base_addr];
}

void DebugStateImpl::PushRegsDump(uintptr_t base_addr, uintptr_t header_addr,
                                  const AmdGpu::Regs& regs) {
    std::scoped_lock lock{frame_dump_list_mutex};

    auto dump = GetRegDump(base_addr, header_addr);
    if (!dump) {
        return;
    }

    (*dump)->regs = regs;

    for (int i = 0; i < RegDump::MaxShaderStages; i++) {
        if ((*dump)->regs.stage_enable.IsStageEnabled(i)) {
            auto stage = (*dump)->regs.ProgramForStage(i);
            if (stage->address) {
                const auto params = AmdGpu::GetParams(*stage);
                (*dump)->stages[i] = PipelineShaderProgramDump{
                    .name = Vulkan::PipelineCache::GetShaderName(Shader::StageFromIndex(i),
                                                                 params.hash),
                    .hash = params.hash,
                    .user_data = *stage,
                    .code = std::vector<u32>{params.code.begin(), params.code.end()},
                };
            }
        }
    }
}

void DebugStateImpl::PushRegsDumpCompute(uintptr_t base_addr, uintptr_t header_addr,
                                         const CsState& cs_state) {
    std::scoped_lock lock{frame_dump_list_mutex};

    auto dump = GetRegDump(base_addr, header_addr);
    if (!dump) {
        return;
    }

    (*dump)->is_compute = true;
    auto& cs = (*dump)->regs.cs_program;
    cs = cs_state;

    const auto params = AmdGpu::GetParams(cs);
    (*dump)->cs_data = PipelineComputerProgramDump{
        .name = Vulkan::PipelineCache::GetShaderName(Shader::HwStage::Compute, params.hash),
        .hash = params.hash,
        .cs_program = cs,
        .code = std::vector<u32>{params.code.begin(), params.code.end()},
    };
}

void DebugStateImpl::CollectShader(const std::string& name, Shader::SwStage l_stage,
                                   vk::ShaderModule module, std::span<const u32> spv,
                                   std::span<const u32> raw_code, std::span<const u32> patch_spv,
                                   bool is_patched) {
    shader_dump_list.emplace_back(name, l_stage, module, std::vector<u32>{spv.begin(), spv.end()},
                                  std::vector<u32>{raw_code.begin(), raw_code.end()},
                                  std::vector<u32>{patch_spv.begin(), patch_spv.end()}, is_patched);
}
