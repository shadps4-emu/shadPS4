// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/windows_fault_tracker.h"

#ifdef _WIN64

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include "core/signals.h"

namespace Core::WindowsFaultTracker {
namespace {

constexpr wchar_t MappingEnvironment[] = L"SHADPS4_WINDOWS_FAULT_TRACKER";
constexpr wchar_t TargetPidEnvironment[] = L"SHADPS4_WINDOWS_FAULT_TRACKER_PID";
constexpr wchar_t ReadyEventSuffix[] = L".Ready";
constexpr u32 SharedVersion = 8;
constexpr u32 MonitorStarting = 0;
constexpr u32 MonitorReady = 1;
constexpr u32 MonitorFailed = 2;
constexpr u32 MemoryHandlerEnabled = 1U << 31;
constexpr u32 IllegalInstructionHandlerEnabled = 1U << 30;
constexpr u32 HandlerCallCountMask = IllegalInstructionHandlerEnabled - 1;
constexpr size_t GuestAddressBits = 40;
constexpr size_t PageBits = 12;
constexpr size_t PageSize = 1ULL << PageBits;
constexpr size_t NumGuestPages = 1ULL << (GuestAddressBits - PageBits);
constexpr size_t NumBitmapWords = NumGuestPages / 64;
constexpr size_t MaxFaultSlots = 256;
constexpr size_t HandlerStackSize = 256_KB;
constexpr size_t HandlerHomeSpaceSize = 32;
constexpr DWORD ContextFlags =
    CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT | CONTEXT_XSTATE;

enum class FaultKind : u8 {
    Memory,
    IllegalInstruction,
};

struct alignas(64) FaultSlot {
    u64 generation{};
    u64 completed_generation{};
    VAddr fault_address{};
    u64 context_offset{};
    u32 context_size{};
    u32 callback_handled{};
    MemoryAccess access{};
    FaultKind kind{};
    u8 reserved[6]{};
};

struct alignas(64) SharedHeader {
    u32 version{};
    u32 slot_count{};
    alignas(64) u32 handler_state{};
    u32 monitor_status{};
    u32 context_buffer_size{};
    u32 context_buffer_stride{};
    u64 context_buffers_offset{};
    u64 watched_bitmap_offset{};
    u64 read_bitmap_offset{};
    u64 candidate_bitmap_offset{};
    u64 mapping_size{};
    u64 debuggee_view_address{};
    u64 trampoline_address{};
    std::array<FaultSlot, MaxFaultSlots> fault_slots{};
};

struct MappingLayout {
    u32 context_buffer_size{};
    u32 context_buffer_stride{};
    size_t context_buffers_offset{};
    size_t watched_bitmap_offset{};
    size_t read_bitmap_offset{};
    size_t candidate_bitmap_offset{};
    size_t mapping_size{};
};

struct MonitorSlot {
    u32 thread_id{};
    u64 generation{};
    void* stack{};
    CONTEXT* context{};
    DWORD context_length{};
    bool fallback{};
    FaultKind kind{};
    u64 fault_instruction{};
    VAddr fault_address{};
    MemoryAccess fault_access{};
};

struct MonitorState {
    std::byte* view{};
    HANDLE process{};
    u64 enabled_xstate_features{};
    std::array<MonitorSlot, MaxFaultSlots> slots{};
    std::unordered_map<u32, HANDLE> threads;
};

constexpr size_t AlignCacheLine(size_t value) {
    return (value + 63) & ~size_t{63};
}

std::optional<MappingLayout> QueryMappingLayout() {
    DWORD context_size{};
    if (InitializeContext(nullptr, ContextFlags, nullptr, &context_size) != FALSE ||
        GetLastError() != ERROR_INSUFFICIENT_BUFFER || context_size < sizeof(CONTEXT)) {
        return std::nullopt;
    }

    const size_t context_stride = AlignCacheLine(context_size);
    if (context_stride > std::numeric_limits<u32>::max()) {
        return std::nullopt;
    }
    const size_t context_buffers_offset = AlignCacheLine(sizeof(SharedHeader));
    const size_t watched_bitmap_offset = context_buffers_offset + context_stride * MaxFaultSlots;
    const size_t read_bitmap_offset = watched_bitmap_offset + NumBitmapWords * sizeof(u64);
    const size_t candidate_bitmap_offset = read_bitmap_offset + NumBitmapWords * sizeof(u64);
    const size_t mapping_size = candidate_bitmap_offset + NumBitmapWords * sizeof(u64);
    return MappingLayout{
        .context_buffer_size = context_size,
        .context_buffer_stride = static_cast<u32>(context_stride),
        .context_buffers_offset = context_buffers_offset,
        .watched_bitmap_offset = watched_bitmap_offset,
        .read_bitmap_offset = read_bitmap_offset,
        .candidate_bitmap_offset = candidate_bitmap_offset,
        .mapping_size = mapping_size,
    };
}

bool HasExpectedLayout(const SharedHeader* shared, const MappingLayout& layout) {
    return shared->version == SharedVersion && shared->slot_count == MaxFaultSlots &&
           shared->context_buffer_size == layout.context_buffer_size &&
           shared->context_buffer_stride == layout.context_buffer_stride &&
           shared->context_buffers_offset == layout.context_buffers_offset &&
           shared->watched_bitmap_offset == layout.watched_bitmap_offset &&
           shared->read_bitmap_offset == layout.read_bitmap_offset &&
           shared->candidate_bitmap_offset == layout.candidate_bitmap_offset &&
           shared->mapping_size == layout.mapping_size;
}

HANDLE debuggee_mapping{};
std::byte* debuggee_view{};
std::function<bool(VAddr, u64, MemoryAccess)> fault_callback;

SharedHeader* Header(std::byte* view) {
    return reinterpret_cast<SharedHeader*>(view);
}

u64* WatchedBitmap(std::byte* view, MemoryAccess access) {
    const auto* shared = Header(view);
    const size_t offset =
        access == MemoryAccess::Write ? shared->watched_bitmap_offset : shared->read_bitmap_offset;
    return reinterpret_cast<u64*>(view + offset);
}

u64* CandidateBitmap(std::byte* view) {
    return reinterpret_cast<u64*>(view + Header(view)->candidate_bitmap_offset);
}

std::byte* ContextBuffer(std::byte* view, size_t slot_index) {
    const auto* shared = Header(view);
    return view + shared->context_buffers_offset + slot_index * shared->context_buffer_stride;
}

std::atomic_ref<u64> Atomic(u64& value) {
    return std::atomic_ref<u64>{value};
}

std::atomic_ref<u32> Atomic(u32& value) {
    return std::atomic_ref<u32>{value};
}

static_assert(std::atomic_ref<u64>::is_always_lock_free);
static_assert(std::atomic_ref<u32>::is_always_lock_free);

bool AcquireHandlerCall(SharedHeader* shared, u32 required_handler) {
    auto state = Atomic(shared->handler_state).load(std::memory_order_acquire);
    while ((state & required_handler) != 0 &&
           (state & HandlerCallCountMask) != HandlerCallCountMask) {
        if (Atomic(shared->handler_state)
                .compare_exchange_weak(state, state + 1, std::memory_order_acq_rel,
                                       std::memory_order_acquire)) {
            return true;
        }
    }
    return false;
}

void ReleaseHandlerCall(SharedHeader* shared) {
    Atomic(shared->handler_state).fetch_sub(1, std::memory_order_release);
}

bool IsSet(const u64* bitmap, VAddr address) {
    if (address >= (1ULL << GuestAddressBits)) {
        return false;
    }
    const size_t page = address >> PageBits;
    const size_t word = page >> 6;
    const u64 mask = 1ULL << (page & 63);
    return (Atomic(const_cast<u64&>(bitmap[word])).load(std::memory_order_acquire) & mask) != 0;
}

bool IsTrackedAccess(std::byte* view, VAddr address, MemoryAccess access) {
    return IsSet(WatchedBitmap(view, access), address);
}

bool IsTrackingCandidate(std::byte* view, VAddr address) {
    return IsSet(CandidateBitmap(view), address);
}

[[noreturn]] void FaultTrampoline(FaultSlot* slot) noexcept {
    const u64 generation = Atomic(slot->generation).load(std::memory_order_acquire);
    auto* context = reinterpret_cast<CONTEXT*>(debuggee_view + slot->context_offset);
    bool handled = false;
    if (slot->kind == FaultKind::Memory) {
        handled = fault_callback && fault_callback(slot->fault_address, 8, slot->access);
    } else {
        EXCEPTION_RECORD record{};
        record.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        record.ExceptionAddress = reinterpret_cast<void*>(context->Rip);
        EXCEPTION_POINTERS pointers{
            .ExceptionRecord = &record,
            .ContextRecord = context,
        };
        handled = Signals::Instance()->DispatchIllegalInstruction(&pointers);
    }
    Atomic(slot->callback_handled).store(handled, std::memory_order_relaxed);
    Atomic(slot->completed_generation).store(generation, std::memory_order_release);
    ReleaseHandlerCall(Header(debuggee_view));

    RtlRestoreContext(context, nullptr);
    __assume(false);
}

HANDLE RegisterMonitorThread(MonitorState& state, u32 thread_id, HANDLE thread) {
    if (!thread) {
        return nullptr;
    }
    const auto [it, inserted] = state.threads.emplace(thread_id, thread);
    if (!inserted) {
        CloseHandle(thread);
    }
    return it->second;
}

HANDLE GetMonitorThread(MonitorState& state, u32 thread_id) {
    if (const auto it = state.threads.find(thread_id); it != state.threads.end()) {
        return it->second;
    }

    HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, thread_id);
    if (thread) {
        thread = RegisterMonitorThread(state, thread_id, thread);
    }
    return thread;
}

void ReleaseMonitorThread(MonitorState& state, u32 thread_id) {
    if (const auto it = state.threads.find(thread_id); it != state.threads.end()) {
        CloseHandle(it->second);
        state.threads.erase(it);
    }
}

void ReleaseThreadSlots(MonitorState& state, u32 thread_id) {
    for (auto& slot : state.slots) {
        if (slot.thread_id == thread_id) {
            slot.thread_id = 0;
            slot.generation = 0;
            slot.fallback = false;
            slot.kind = FaultKind::Memory;
        }
    }
}

size_t AcquireFaultSlot(MonitorState& state, u32 thread_id) {
    auto* shared = Header(state.view);

    // A new exception on the same thread proves that its previous trampoline restored the
    // context. Reclaim all completed nested slots and preferentially reuse one of them.
    size_t reusable = MaxFaultSlots;
    for (size_t index = 0; index < MaxFaultSlots; ++index) {
        auto& monitor_slot = state.slots[index];
        if (monitor_slot.thread_id != thread_id) {
            continue;
        }
        const u64 completed =
            Atomic(shared->fault_slots[index].completed_generation).load(std::memory_order_acquire);
        if (completed >= monitor_slot.generation) {
            if (reusable == MaxFaultSlots) {
                reusable = index;
            } else {
                monitor_slot.thread_id = 0;
                monitor_slot.generation = 0;
                monitor_slot.fallback = false;
                monitor_slot.kind = FaultKind::Memory;
            }
        }
    }
    if (reusable != MaxFaultSlots) {
        return reusable;
    }

    for (size_t index = 0; index < MaxFaultSlots; ++index) {
        if (state.slots[index].thread_id == 0) {
            return index;
        }
    }
    return MaxFaultSlots;
}

bool ShouldBypassRetry(MonitorState& state, const DEBUG_EVENT& event, VAddr fault_address,
                       MemoryAccess access, bool tracked) {
    // A callback can decline an access, and a candidate-page fallback can turn out to be a genuine
    // guest exception. In either case the restored instruction faults again. Recognize that exact
    // retry and let Windows dispatch it normally instead of repeatedly entering the trampoline.
    auto* shared = Header(state.view);
    const u64 instruction =
        reinterpret_cast<u64>(event.u.Exception.ExceptionRecord.ExceptionAddress);
    bool bypass_retry = false;
    for (size_t index = 0; index < MaxFaultSlots; ++index) {
        auto& monitor_slot = state.slots[index];
        if (monitor_slot.thread_id != event.dwThreadId) {
            continue;
        }
        const u64 completed =
            Atomic(shared->fault_slots[index].completed_generation).load(std::memory_order_acquire);
        if (completed < monitor_slot.generation) {
            continue;
        }

        const bool same_fault = monitor_slot.kind == FaultKind::Memory &&
                                monitor_slot.fault_instruction == instruction &&
                                monitor_slot.fault_address == fault_address &&
                                monitor_slot.fault_access == access;
        const bool handled =
            Atomic(shared->fault_slots[index].callback_handled).load(std::memory_order_relaxed) !=
            0;
        bypass_retry |= same_fault && (!handled || (monitor_slot.fallback && !tracked));
        monitor_slot.thread_id = 0;
        monitor_slot.generation = 0;
        monitor_slot.fallback = false;
        monitor_slot.kind = FaultKind::Memory;
    }
    return bypass_retry;
}

bool ShouldBypassIllegalInstructionRetry(MonitorState& state, const DEBUG_EVENT& event) {
    // An unhandled illegal instruction resumes at the same RIP. Forward that retry to the normal
    // Windows exception path instead of repeatedly entering the alternate-stack trampoline.
    auto* shared = Header(state.view);
    const u64 instruction =
        reinterpret_cast<u64>(event.u.Exception.ExceptionRecord.ExceptionAddress);
    bool bypass_retry = false;
    for (size_t index = 0; index < MaxFaultSlots; ++index) {
        auto& monitor_slot = state.slots[index];
        if (monitor_slot.thread_id != event.dwThreadId) {
            continue;
        }
        const u64 completed =
            Atomic(shared->fault_slots[index].completed_generation).load(std::memory_order_acquire);
        if (completed < monitor_slot.generation) {
            continue;
        }

        const bool same_fault = monitor_slot.kind == FaultKind::IllegalInstruction &&
                                monitor_slot.fault_instruction == instruction;
        const bool handled =
            Atomic(shared->fault_slots[index].callback_handled).load(std::memory_order_relaxed) !=
            0;
        bypass_retry |= same_fault && !handled;
        monitor_slot.thread_id = 0;
        monitor_slot.generation = 0;
        monitor_slot.fallback = false;
        monitor_slot.kind = FaultKind::Memory;
    }
    return bypass_retry;
}

uintptr_t HandlerStackPointer(void* stack) {
    const uintptr_t stack_top =
        (reinterpret_cast<uintptr_t>(stack) + HandlerStackSize) & ~uintptr_t{15};
    return stack_top - sizeof(uintptr_t) - HandlerHomeSpaceSize;
}

bool PrepareHandlerStack(MonitorState& state, MonitorSlot& slot) {
    if (slot.stack) {
        return true;
    }

    slot.stack = VirtualAllocEx(state.process, nullptr, HandlerStackSize, MEM_RESERVE | MEM_COMMIT,
                                PAGE_READWRITE);
    if (!slot.stack) {
        return false;
    }

    // A guard page catches accidental handler-stack exhaustion without involving the guest stack.
    DWORD old_protection{};
    VirtualProtectEx(state.process, slot.stack, PageSize, PAGE_READWRITE | PAGE_GUARD,
                     &old_protection);

    // At function entry, Windows x64 requires a return address followed by 32 bytes of caller-owned
    // home space. Keep all of it inside the allocation even if an optimized trampoline spills.
    const uintptr_t stack_pointer = HandlerStackPointer(slot.stack);
    const uintptr_t fake_return{};
    if (WriteProcessMemory(state.process, reinterpret_cast<void*>(stack_pointer), &fake_return,
                           sizeof(fake_return), nullptr) != FALSE) {
        return true;
    }
    VirtualFreeEx(state.process, slot.stack, 0, MEM_RELEASE);
    slot.stack = nullptr;
    return false;
}

bool PrepareContextBuffer(MonitorState& state, size_t slot_index) {
    auto& slot = state.slots[slot_index];
    if (slot.context) {
        return true;
    }

    auto* context_buffer = ContextBuffer(state.view, slot_index);
    DWORD context_length = Header(state.view)->context_buffer_size;
    CONTEXT* context{};
    if (InitializeContext(context_buffer, ContextFlags, &context, &context_length) == FALSE) {
        return false;
    }

    slot.context = context;
    slot.context_length = context_length;
    return true;
}

bool RedirectToTrampoline(MonitorState& state, const DEBUG_EVENT& event, FaultKind kind,
                          VAddr fault_address, MemoryAccess access, bool fallback) {
    const auto& exception = event.u.Exception.ExceptionRecord;
    auto* shared = Header(state.view);
    const u32 required_handler =
        kind == FaultKind::Memory ? MemoryHandlerEnabled : IllegalInstructionHandlerEnabled;
    if (!AcquireHandlerCall(shared, required_handler)) {
        return false;
    }

    const size_t slot_index = AcquireFaultSlot(state, event.dwThreadId);
    if (slot_index == MaxFaultSlots) {
        ReleaseHandlerCall(shared);
        return false;
    }
    auto& monitor_slot = state.slots[slot_index];
    monitor_slot.fallback = false;
    monitor_slot.kind = kind;
    if (!PrepareHandlerStack(state, monitor_slot)) {
        ReleaseHandlerCall(shared);
        return false;
    }

    if (!PrepareContextBuffer(state, slot_index)) {
        ReleaseHandlerCall(shared);
        return false;
    }

    HANDLE thread = GetMonitorThread(state, event.dwThreadId);
    if (!thread) {
        ReleaseHandlerCall(shared);
        return false;
    }

    auto& fault_slot = shared->fault_slots[slot_index];
    auto* original_context = monitor_slot.context;
    original_context->ContextFlags = ContextFlags;
    const bool captured =
        SetXStateFeaturesMask(original_context, state.enabled_xstate_features) != FALSE &&
        GetThreadContext(thread, original_context) != FALSE;
    if (!captured) {
        ReleaseHandlerCall(shared);
        return false;
    }

    monitor_slot.thread_id = event.dwThreadId;
    monitor_slot.generation++;
    if (monitor_slot.generation == 0) {
        monitor_slot.generation++;
    }
    monitor_slot.fallback = fallback;
    monitor_slot.kind = kind;
    monitor_slot.fault_instruction = reinterpret_cast<u64>(exception.ExceptionAddress);
    monitor_slot.fault_address = fault_address;
    monitor_slot.fault_access = access;
    fault_slot.fault_address = fault_address;
    fault_slot.access = access;
    fault_slot.kind = kind;
    fault_slot.context_offset =
        static_cast<u64>(reinterpret_cast<std::byte*>(original_context) - state.view);
    fault_slot.context_size = monitor_slot.context_length;
    Atomic(fault_slot.callback_handled).store(0, std::memory_order_relaxed);
    Atomic(fault_slot.completed_generation)
        .store(monitor_slot.generation - 1, std::memory_order_relaxed);
    Atomic(fault_slot.generation).store(monitor_slot.generation, std::memory_order_release);

    const uintptr_t child_slot_address = shared->debuggee_view_address +
                                         offsetof(SharedHeader, fault_slots) +
                                         slot_index * sizeof(FaultSlot);
    CONTEXT trampoline_context = *original_context;
    trampoline_context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
    trampoline_context.Rip = shared->trampoline_address;
    trampoline_context.Rsp = HandlerStackPointer(monitor_slot.stack);
    trampoline_context.Rcx = child_slot_address;

    const bool redirected = SetThreadContext(thread, &trampoline_context) != FALSE;
    if (!redirected) {
        monitor_slot.thread_id = 0;
        monitor_slot.generation = 0;
        monitor_slot.fallback = false;
        monitor_slot.kind = FaultKind::Memory;
        ReleaseHandlerCall(shared);
        return false;
    }

    return true;
}

bool RedirectTrackedAccess(MonitorState& state, const DEBUG_EVENT& event) {
    const auto& exception = event.u.Exception.ExceptionRecord;
    if (event.u.Exception.dwFirstChance == 0 ||
        exception.ExceptionCode != EXCEPTION_ACCESS_VIOLATION || exception.NumberParameters < 2 ||
        exception.ExceptionInformation[0] > 1) {
        return false;
    }

    const VAddr fault_address = exception.ExceptionInformation[1];
    const MemoryAccess access =
        exception.ExceptionInformation[0] == 1 ? MemoryAccess::Write : MemoryAccess::Read;
    const bool tracked = IsTrackedAccess(state.view, fault_address, access);
    if (ShouldBypassRetry(state, event, fault_address, access, tracked)) {
        return false;
    }
    // Protection and bitmap updates occur on another guest thread. An access violation can already
    // be in flight when that thread clears the watch, so accept one provisional callback for any
    // page that has previously participated in tracking. ShouldBypassRetry forwards an unresolved
    // retry as a genuine exception.
    if (!tracked && !IsTrackingCandidate(state.view, fault_address)) {
        return false;
    }
    return RedirectToTrampoline(state, event, FaultKind::Memory, fault_address, access, !tracked);
}

bool RedirectIllegalInstruction(MonitorState& state, const DEBUG_EVENT& event) {
    const auto& exception = event.u.Exception.ExceptionRecord;
    if (event.u.Exception.dwFirstChance == 0 ||
        exception.ExceptionCode != EXCEPTION_ILLEGAL_INSTRUCTION ||
        ShouldBypassIllegalInstructionRetry(state, event)) {
        return false;
    }
    return RedirectToTrampoline(state, event, FaultKind::IllegalInstruction, 0, MemoryAccess::Read,
                                false);
}

std::wstring MakeMappingName() {
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    return L"Local\\shadPS4.WindowsFaultTracker." + std::to_wstring(GetCurrentProcessId()) + L"." +
           std::to_wstring(counter.QuadPart);
}

void SignalMonitorStatus(std::byte* view, HANDLE ready_event, u32 status) {
    Atomic(Header(view)->monitor_status).store(status, std::memory_order_release);
    SetEvent(ready_event);
}

int MonitorAttachedProcess(std::byte* view, DWORD target_pid, HANDLE ready_event) {
    if (!DebugActiveProcess(target_pid)) {
        std::fprintf(stderr, "Windows fault monitor could not attach: %lu\n", GetLastError());
        SignalMonitorStatus(view, ready_event, MonitorFailed);
        return 1;
    }
    DebugSetProcessKillOnExit(TRUE);

    MonitorState state{
        .view = view,
        .enabled_xstate_features = GetEnabledXStateFeatures(),
    };
    bool running = true;
    bool initial_breakpoint = true;
    bool monitor_failed = false;
    bool ready_signaled = false;
    while (running) {
        DEBUG_EVENT event{};
        if (!WaitForDebugEvent(&event, INFINITE)) {
            std::fprintf(stderr, "Windows fault monitor failed: %lu\n", GetLastError());
            monitor_failed = true;
            break;
        }

        DWORD continue_status = DBG_CONTINUE;
        bool signal_ready_after_continue = false;
        switch (event.dwDebugEventCode) {
        case CREATE_PROCESS_DEBUG_EVENT:
            state.process = event.u.CreateProcessInfo.hProcess;
            RegisterMonitorThread(state, event.dwThreadId, event.u.CreateProcessInfo.hThread);
            if (event.u.CreateProcessInfo.hFile) {
                CloseHandle(event.u.CreateProcessInfo.hFile);
            }
            break;
        case CREATE_THREAD_DEBUG_EVENT:
            RegisterMonitorThread(state, event.dwThreadId, event.u.CreateThread.hThread);
            break;
        case EXIT_THREAD_DEBUG_EVENT:
            ReleaseThreadSlots(state, event.dwThreadId);
            ReleaseMonitorThread(state, event.dwThreadId);
            break;
        case LOAD_DLL_DEBUG_EVENT:
            if (event.u.LoadDll.hFile) {
                CloseHandle(event.u.LoadDll.hFile);
            }
            break;
        case EXCEPTION_DEBUG_EVENT: {
            const auto& exception = event.u.Exception.ExceptionRecord;
            const bool startup_breakpoint =
                exception.ExceptionCode == EXCEPTION_BREAKPOINT && initial_breakpoint;
            initial_breakpoint &= !startup_breakpoint;
            signal_ready_after_continue = startup_breakpoint;
            const bool redirected =
                RedirectTrackedAccess(state, event) || RedirectIllegalInstruction(state, event);
            if (!redirected && !startup_breakpoint &&
                exception.ExceptionCode != DBG_PRINTEXCEPTION_C &&
                exception.ExceptionCode != DBG_PRINTEXCEPTION_WIDE_C &&
                exception.ExceptionCode != 0x406D1388) {
                continue_status = DBG_EXCEPTION_NOT_HANDLED;
            }
            break;
        }
        case EXIT_PROCESS_DEBUG_EVENT:
            running = false;
            break;
        default:
            break;
        }

        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, continue_status)) {
            std::fprintf(stderr, "Windows fault monitor could not continue the debuggee: %lu\n",
                         GetLastError());
            monitor_failed = true;
            break;
        }
        if (signal_ready_after_continue) {
            SignalMonitorStatus(view, ready_event, MonitorReady);
            ready_signaled = true;
        }
    }

    if (monitor_failed) {
        if (!ready_signaled) {
            SignalMonitorStatus(view, ready_event, MonitorFailed);
        }
        if (state.process) {
            TerminateProcess(state.process, 1);
        }
    }
    if (!ready_signaled && !monitor_failed) {
        SignalMonitorStatus(view, ready_event, MonitorFailed);
    }
    for (const auto& entry : state.threads) {
        CloseHandle(entry.second);
    }
    state.threads.clear();
    if (state.process) {
        CloseHandle(state.process);
    }
    return monitor_failed ? 1 : 0;
}

} // namespace

std::optional<int> Bootstrap(int argc, char* argv[]) {
    std::array<wchar_t, 512> mapping_name{};
    std::array<wchar_t, 32> target_pid_text{};
    const DWORD mapping_name_length = GetEnvironmentVariableW(
        MappingEnvironment, mapping_name.data(), static_cast<DWORD>(mapping_name.size()));
    const DWORD target_pid_length = GetEnvironmentVariableW(
        TargetPidEnvironment, target_pid_text.data(), static_cast<DWORD>(target_pid_text.size()));
    if (mapping_name_length != 0 && mapping_name_length < mapping_name.size() &&
        target_pid_length != 0 && target_pid_length < target_pid_text.size()) {
        SetEnvironmentVariableW(MappingEnvironment, nullptr);
        SetEnvironmentVariableW(TargetPidEnvironment, nullptr);

        const auto layout = QueryMappingLayout();
        HANDLE mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapping_name.data());
        auto* view =
            mapping ? static_cast<std::byte*>(MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0))
                    : nullptr;
        const std::wstring ready_event_name = std::wstring{mapping_name.data()} + ReadyEventSuffix;
        HANDLE ready_event =
            OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, ready_event_name.c_str());
        if (!view || !ready_event || !layout || !HasExpectedLayout(Header(view), *layout)) {
            if (ready_event) {
                SetEvent(ready_event);
                CloseHandle(ready_event);
            }
            if (view) {
                UnmapViewOfFile(view);
            }
            if (mapping) {
                CloseHandle(mapping);
            }
            return 1;
        }

        wchar_t* parse_end{};
        const unsigned long parsed_pid = std::wcstoul(target_pid_text.data(), &parse_end, 10);
        const bool valid_pid =
            parsed_pid != 0 && parse_end && parse_end != target_pid_text.data() && *parse_end == 0;
        const int result =
            valid_pid ? MonitorAttachedProcess(view, static_cast<DWORD>(parsed_pid), ready_event)
                      : 1;
        if (!valid_pid) {
            SignalMonitorStatus(view, ready_event, MonitorFailed);
        }
        CloseHandle(ready_event);
        UnmapViewOfFile(view);
        CloseHandle(mapping);
        return result;
    }

    bool wait_for_external_debugger = false;
    for (int index = 1; index < argc; ++index) {
        wait_for_external_debugger |= std::string_view{argv[index]} == "--wait-for-debugger";
    }
    if (IsDebuggerPresent() || wait_for_external_debugger) {
        return std::nullopt;
    }

    const auto layout = QueryMappingLayout();
    if (!layout) {
        std::fprintf(stderr, "Could not determine the required Windows XSTATE context size.\n");
        return 1;
    }

    const std::wstring new_mapping_name = MakeMappingName();
    const std::wstring ready_event_name = new_mapping_name + ReadyEventSuffix;
    HANDLE ready_event = CreateEventW(nullptr, TRUE, FALSE, ready_event_name.c_str());
    if (!ready_event) {
        return 1;
    }
    HANDLE mapping =
        CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                           static_cast<DWORD>(layout->mapping_size >> 32),
                           static_cast<DWORD>(layout->mapping_size), new_mapping_name.c_str());
    if (!mapping) {
        CloseHandle(ready_event);
        return 1;
    }
    auto* view = static_cast<std::byte*>(
        MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, layout->mapping_size));
    if (!view) {
        CloseHandle(mapping);
        CloseHandle(ready_event);
        return 1;
    }

    // A new paging-file-backed mapping is demand-zeroed. Do not eagerly touch its context slots
    // and sparse page bitmaps.
    Header(view)->version = SharedVersion;
    Header(view)->slot_count = MaxFaultSlots;
    Header(view)->context_buffer_size = layout->context_buffer_size;
    Header(view)->context_buffer_stride = layout->context_buffer_stride;
    Header(view)->context_buffers_offset = layout->context_buffers_offset;
    Header(view)->watched_bitmap_offset = layout->watched_bitmap_offset;
    Header(view)->read_bitmap_offset = layout->read_bitmap_offset;
    Header(view)->candidate_bitmap_offset = layout->candidate_bitmap_offset;
    Header(view)->mapping_size = layout->mapping_size;
    Atomic(Header(view)->monitor_status).store(MonitorStarting, std::memory_order_relaxed);

    const std::wstring target_pid = std::to_wstring(GetCurrentProcessId());
    if (!SetEnvironmentVariableW(MappingEnvironment, new_mapping_name.c_str()) ||
        !SetEnvironmentVariableW(TargetPidEnvironment, target_pid.c_str())) {
        SetEnvironmentVariableW(MappingEnvironment, nullptr);
        SetEnvironmentVariableW(TargetPidEnvironment, nullptr);
        UnmapViewOfFile(view);
        CloseHandle(mapping);
        CloseHandle(ready_event);
        return 1;
    }

    std::array<wchar_t, 32768> executable{};
    const DWORD executable_length =
        GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
    STARTUPINFOW startup_info{};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info{};
    std::wstring command_line = L"\"" + std::wstring{executable.data()} + L"\"";
    const bool monitor_started =
        executable_length != 0 && executable_length < executable.size() &&
        CreateProcessW(executable.data(), command_line.data(), nullptr, nullptr, FALSE, 0, nullptr,
                       nullptr, &startup_info, &process_info) != FALSE;
    SetEnvironmentVariableW(MappingEnvironment, nullptr);
    SetEnvironmentVariableW(TargetPidEnvironment, nullptr);
    if (!monitor_started) {
        std::fprintf(stderr, "Failed to start Windows fault monitor: %lu\n", GetLastError());
        UnmapViewOfFile(view);
        CloseHandle(mapping);
        CloseHandle(ready_event);
        return 1;
    }
    CloseHandle(process_info.hThread);

    constexpr DWORD AttachTimeoutMs = 30'000;
    const DWORD wait_result = WaitForSingleObject(ready_event, AttachTimeoutMs);
    const u32 monitor_status = Atomic(Header(view)->monitor_status).load(std::memory_order_acquire);
    CloseHandle(ready_event);
    if (wait_result != WAIT_OBJECT_0 || monitor_status != MonitorReady) {
        std::fprintf(stderr, "Windows fault monitor did not become ready.\n");
        TerminateProcess(process_info.hProcess, 1);
        WaitForSingleObject(process_info.hProcess, 5'000);
        CloseHandle(process_info.hProcess);
        UnmapViewOfFile(view);
        CloseHandle(mapping);
        return 1;
    }
    CloseHandle(process_info.hProcess);

    debuggee_mapping = mapping;
    debuggee_view = view;
    return std::nullopt;
}

bool IsEnabled() noexcept {
    return debuggee_view != nullptr &&
           (Atomic(Header(debuggee_view)->handler_state).load(std::memory_order_acquire) &
            MemoryHandlerEnabled) != 0;
}

void InstallFaultHandler(std::function<bool(VAddr, u64, MemoryAccess)> callback) {
    if (!debuggee_view || !callback) {
        return;
    }

    auto* header = Header(debuggee_view);
    if ((Atomic(header->handler_state).load(std::memory_order_acquire) & MemoryHandlerEnabled) !=
        0) {
        return;
    }
    fault_callback = std::move(callback);
    header->debuggee_view_address = reinterpret_cast<uintptr_t>(debuggee_view);
    header->trampoline_address = reinterpret_cast<uintptr_t>(&FaultTrampoline);
    Atomic(header->handler_state).fetch_or(MemoryHandlerEnabled, std::memory_order_release);
}

void RemoveFaultHandler() {
    if (!debuggee_view) {
        return;
    }
    auto& handler_state = Header(debuggee_view)->handler_state;
    Atomic(handler_state).fetch_and(~MemoryHandlerEnabled, std::memory_order_acq_rel);
    while ((Atomic(handler_state).load(std::memory_order_acquire) & HandlerCallCountMask) != 0) {
        Sleep(0);
    }
    fault_callback = {};
}

void InstallIllegalInstructionHandler() {
    if (!debuggee_view) {
        return;
    }

    auto* header = Header(debuggee_view);
    header->debuggee_view_address = reinterpret_cast<uintptr_t>(debuggee_view);
    header->trampoline_address = reinterpret_cast<uintptr_t>(&FaultTrampoline);
    Atomic(header->handler_state)
        .fetch_or(IllegalInstructionHandlerEnabled, std::memory_order_release);
}

static void UpdateBitmap(u64* bitmap, u64* sticky_bitmap, VAddr address, u64 size,
                         bool enable) noexcept {
    if (!bitmap || size == 0 || address >= (1ULL << GuestAddressBits)) {
        return;
    }
    const size_t start_page = address >> PageBits;
    const VAddr last_address = std::min<VAddr>(
        address + std::min<u64>(size - 1, std::numeric_limits<VAddr>::max() - address),
        (1ULL << GuestAddressBits) - 1);
    const size_t end_page = (last_address >> PageBits) + 1;
    const size_t first_word = start_page >> 6;
    const size_t last_word = (end_page - 1) >> 6;
    for (size_t word = first_word; word <= last_word; ++word) {
        const size_t first_bit = word == first_word ? start_page & 63 : 0;
        const size_t end_bit = word == last_word ? end_page & 63 : 0;
        u64 mask = std::numeric_limits<u64>::max() << first_bit;
        if (end_bit != 0) {
            mask &= (1ULL << end_bit) - 1;
        }
        if (enable) {
            if (sticky_bitmap) {
                Atomic(sticky_bitmap[word]).fetch_or(mask, std::memory_order_release);
            }
            Atomic(bitmap[word]).fetch_or(mask, std::memory_order_release);
        } else {
            Atomic(bitmap[word]).fetch_and(~mask, std::memory_order_release);
        }
    }
}

void WatchMemory(VAddr address, u64 size, MemoryAccess access, bool enable) noexcept {
    if (!IsEnabled()) {
        return;
    }
    UpdateBitmap(WatchedBitmap(debuggee_view, access), CandidateBitmap(debuggee_view), address,
                 size, enable);
}

} // namespace Core::WindowsFaultTracker

#else

namespace Core::WindowsFaultTracker {

std::optional<int> Bootstrap(int, char*[]) {
    return std::nullopt;
}

bool IsEnabled() noexcept {
    return false;
}

void InstallFaultHandler(std::function<bool(VAddr, u64, MemoryAccess)>) {}

void RemoveFaultHandler() {}

void InstallIllegalInstructionHandler() {}

void WatchMemory(VAddr, u64, MemoryAccess, bool) noexcept {}

} // namespace Core::WindowsFaultTracker

#endif
