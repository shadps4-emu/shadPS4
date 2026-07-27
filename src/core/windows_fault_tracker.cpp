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
#include <bit>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include "common/alignment.h"
#include "common/enum.h"
#include "core/signals.h"

namespace Core::WindowsFaultTracker {
namespace {

constexpr wchar_t MappingEnvironment[] = L"SHADPS4_WINDOWS_FAULT_TRACKER";
constexpr wchar_t TargetPidEnvironment[] = L"SHADPS4_WINDOWS_FAULT_TRACKER_PID";
constexpr wchar_t ReadyEventSuffix[] = L".Ready";
constexpr size_t CacheLineSize = 64;
constexpr size_t GuestAddressBits = 40;
constexpr VAddr GuestAddressLimit = 1ULL << GuestAddressBits;
constexpr size_t PageBits = 12;
constexpr size_t PageSize = 1ULL << PageBits;
constexpr size_t NumGuestPages = 1ULL << (GuestAddressBits - PageBits);
constexpr size_t BitmapWordBits = std::numeric_limits<u64>::digits;
constexpr size_t NumBitmapWords = NumGuestPages / BitmapWordBits;
constexpr size_t MaxFaultSlots = 256;
constexpr size_t HandlerStackSize = 256_KB;
constexpr size_t WindowsX64StackAlignment = 16;
constexpr size_t WindowsX64HomeSpaceSize = 4 * sizeof(uintptr_t);
constexpr DWORD AttachTimeout = 30'000;
constexpr DWORD TerminationTimeout = 5'000;
constexpr int DecimalRadix = 10;
constexpr size_t PathGrowthFactor = 2;
constexpr size_t DwordBits = std::numeric_limits<DWORD>::digits;
constexpr DWORD ContextFlags =
    CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT | CONTEXT_XSTATE;

static_assert(std::has_single_bit(CacheLineSize));
static_assert(std::has_single_bit(WindowsX64StackAlignment));

enum class MonitorStatus : u32 {
    Starting,
    Ready,
    Failed,
};

enum class HandlerFlag : u32 {
    Memory = 1U << 0,
    IllegalInstruction = 1U << 1,
};
DECLARE_ENUM_FLAG_OPERATORS(HandlerFlag)

enum class FaultKind : u8 {
    Memory,
    IllegalInstruction,
};

enum class CallbackResult : u32 {
    Unhandled,
    Handled,
};

enum class TrackingState : u8 {
    Watched,
    Provisional,
};

enum class AccessViolationOperation : ULONG_PTR {
    Read,
    Write,
};

constexpr size_t AccessViolationOperationIndex = 0;
constexpr size_t AccessViolationAddressIndex = 1;

struct alignas(CacheLineSize) FaultSlot {
    u64 generation{};
    u64 completed_generation{};
    VAddr fault_address{};
    u64 context_offset{};
    CallbackResult callback_result{};
    MemoryAccess access{};
    FaultKind kind{};
};

struct alignas(CacheLineSize) SharedHeader {
    u32 slot_count{};
    alignas(CacheLineSize) HandlerFlag enabled_handlers{};
    u32 active_handler_calls{};
    MonitorStatus monitor_status{};
    u32 context_buffer_size{};
    u32 context_buffer_stride{};
    u64 context_buffers_offset{};
    u64 watched_bitmap_offset{};
    u64 read_bitmap_offset{};
    u64 tracking_history_bitmap_offset{};
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
    size_t tracking_history_bitmap_offset{};
    size_t mapping_size{};
};

struct MonitorSlot {
    u32 thread_id{};
    u64 generation{};
    void* stack{};
    CONTEXT* context{};
    TrackingState tracking_state{};
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
    return Common::AlignUp(value, CacheLineSize);
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
    const size_t tracking_history_bitmap_offset = read_bitmap_offset + NumBitmapWords * sizeof(u64);
    const size_t mapping_size = tracking_history_bitmap_offset + NumBitmapWords * sizeof(u64);
    return MappingLayout{
        .context_buffer_size = context_size,
        .context_buffer_stride = static_cast<u32>(context_stride),
        .context_buffers_offset = context_buffers_offset,
        .watched_bitmap_offset = watched_bitmap_offset,
        .read_bitmap_offset = read_bitmap_offset,
        .tracking_history_bitmap_offset = tracking_history_bitmap_offset,
        .mapping_size = mapping_size,
    };
}

bool HasExpectedLayout(const SharedHeader* shared, const MappingLayout& layout) {
    return shared->slot_count == MaxFaultSlots &&
           shared->context_buffer_size == layout.context_buffer_size &&
           shared->context_buffer_stride == layout.context_buffer_stride &&
           shared->context_buffers_offset == layout.context_buffers_offset &&
           shared->watched_bitmap_offset == layout.watched_bitmap_offset &&
           shared->read_bitmap_offset == layout.read_bitmap_offset &&
           shared->tracking_history_bitmap_offset == layout.tracking_history_bitmap_offset &&
           shared->mapping_size == layout.mapping_size;
}

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

u64* TrackingHistoryBitmap(std::byte* view) {
    return reinterpret_cast<u64*>(view + Header(view)->tracking_history_bitmap_offset);
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

std::atomic_ref<HandlerFlag> Atomic(HandlerFlag& value) {
    return std::atomic_ref<HandlerFlag>{value};
}

std::atomic_ref<MonitorStatus> Atomic(MonitorStatus& value) {
    return std::atomic_ref<MonitorStatus>{value};
}

std::atomic_ref<CallbackResult> Atomic(CallbackResult& value) {
    return std::atomic_ref<CallbackResult>{value};
}

static_assert(std::atomic_ref<u64>::is_always_lock_free);
static_assert(std::atomic_ref<u32>::is_always_lock_free);
static_assert(std::atomic_ref<HandlerFlag>::is_always_lock_free);
static_assert(std::atomic_ref<MonitorStatus>::is_always_lock_free);
static_assert(std::atomic_ref<CallbackResult>::is_always_lock_free);

bool AcquireHandlerCall(SharedHeader* shared, HandlerFlag required_handler) {
    Atomic(shared->active_handler_calls).fetch_add(1, std::memory_order_acq_rel);
    const HandlerFlag enabled = Atomic(shared->enabled_handlers).load(std::memory_order_acquire);
    if (True(enabled & required_handler)) {
        return true;
    }
    Atomic(shared->active_handler_calls).fetch_sub(1, std::memory_order_release);
    return false;
}

void ReleaseHandlerCall(SharedHeader* shared) {
    Atomic(shared->active_handler_calls).fetch_sub(1, std::memory_order_release);
}

void EnableHandler(SharedHeader* shared, HandlerFlag handler) {
    auto enabled = Atomic(shared->enabled_handlers).load(std::memory_order_relaxed);
    while (!Atomic(shared->enabled_handlers)
                .compare_exchange_weak(enabled, enabled | handler, std::memory_order_release,
                                       std::memory_order_relaxed)) {
    }
}

void DisableHandler(SharedHeader* shared, HandlerFlag handler) {
    auto enabled = Atomic(shared->enabled_handlers).load(std::memory_order_relaxed);
    while (!Atomic(shared->enabled_handlers)
                .compare_exchange_weak(enabled, enabled & ~handler, std::memory_order_acq_rel,
                                       std::memory_order_relaxed)) {
    }
}

bool IsSet(const u64* bitmap, VAddr address) {
    if (address >= GuestAddressLimit) {
        return false;
    }
    const size_t page = address >> PageBits;
    const size_t word = page / BitmapWordBits;
    const u64 mask = 1ULL << (page % BitmapWordBits);
    return (Atomic(const_cast<u64&>(bitmap[word])).load(std::memory_order_acquire) & mask) != 0;
}

bool IsTrackedAccess(std::byte* view, VAddr address, MemoryAccess access) {
    return IsSet(WatchedBitmap(view, access), address);
}

bool WasPreviouslyTracked(std::byte* view, VAddr address) {
    return IsSet(TrackingHistoryBitmap(view), address);
}

[[noreturn]] void FaultTrampoline(FaultSlot* slot) noexcept {
    const u64 generation = Atomic(slot->generation).load(std::memory_order_acquire);
    auto* context = reinterpret_cast<CONTEXT*>(debuggee_view + slot->context_offset);
    bool handled = false;
    if (slot->kind == FaultKind::Memory) {
        handled = fault_callback && fault_callback(slot->fault_address, sizeof(u64), slot->access);
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
    Atomic(slot->callback_result)
        .store(handled ? CallbackResult::Handled : CallbackResult::Unhandled,
               std::memory_order_relaxed);
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

void ReleaseMonitorSlot(MonitorSlot& slot) {
    slot.thread_id = {};
    slot.generation = {};
}

void ReleaseThreadSlots(MonitorState& state, u32 thread_id) {
    for (auto& slot : state.slots) {
        if (slot.thread_id == thread_id) {
            ReleaseMonitorSlot(slot);
        }
    }
}

std::optional<size_t> AcquireFaultSlot(MonitorState& state, u32 thread_id) {
    auto* shared = Header(state.view);

    // A new exception on the same thread proves that its previous trampoline restored the
    // context. Reclaim all completed nested slots and preferentially reuse one of them.
    std::optional<size_t> reusable;
    for (size_t index = 0; index < MaxFaultSlots; ++index) {
        auto& monitor_slot = state.slots[index];
        if (monitor_slot.thread_id != thread_id) {
            continue;
        }
        const u64 completed =
            Atomic(shared->fault_slots[index].completed_generation).load(std::memory_order_acquire);
        if (completed >= monitor_slot.generation) {
            if (!reusable) {
                reusable = index;
            } else {
                ReleaseMonitorSlot(monitor_slot);
            }
        }
    }
    if (reusable) {
        return reusable;
    }

    for (size_t index = 0; index < MaxFaultSlots; ++index) {
        if (state.slots[index].thread_id == DWORD{}) {
            return index;
        }
    }
    return std::nullopt;
}

bool ShouldBypassRetry(MonitorState& state, const DEBUG_EVENT& event, VAddr fault_address,
                       MemoryAccess access, bool tracked) {
    // A callback can decline an access, and provisional handling of a previously tracked page can
    // turn out to be a genuine guest exception. In either case the restored instruction faults
    // again. Recognize that exact retry and let Windows dispatch it normally instead of repeatedly
    // entering the trampoline.
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
            Atomic(shared->fault_slots[index].callback_result).load(std::memory_order_relaxed) ==
            CallbackResult::Handled;
        bypass_retry |=
            same_fault &&
            (!handled || (monitor_slot.tracking_state == TrackingState::Provisional && !tracked));
        ReleaseMonitorSlot(monitor_slot);
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
            Atomic(shared->fault_slots[index].callback_result).load(std::memory_order_relaxed) ==
            CallbackResult::Handled;
        bypass_retry |= same_fault && !handled;
        ReleaseMonitorSlot(monitor_slot);
    }
    return bypass_retry;
}

uintptr_t HandlerStackPointer(void* stack) {
    const uintptr_t stack_top = Common::AlignDown(
        reinterpret_cast<uintptr_t>(stack) + HandlerStackSize, WindowsX64StackAlignment);
    return stack_top - sizeof(uintptr_t) - WindowsX64HomeSpaceSize;
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
    if (VirtualProtectEx(state.process, slot.stack, PageSize, PAGE_READWRITE | PAGE_GUARD,
                         &old_protection) == FALSE) {
        VirtualFreeEx(state.process, slot.stack, 0, MEM_RELEASE);
        slot.stack = nullptr;
        return false;
    }

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
    return true;
}

bool RedirectToTrampoline(MonitorState& state, const DEBUG_EVENT& event, FaultKind kind,
                          VAddr fault_address, MemoryAccess access, TrackingState tracking_state) {
    const auto& exception = event.u.Exception.ExceptionRecord;
    auto* shared = Header(state.view);
    const HandlerFlag required_handler =
        kind == FaultKind::Memory ? HandlerFlag::Memory : HandlerFlag::IllegalInstruction;
    if (!AcquireHandlerCall(shared, required_handler)) {
        return false;
    }

    const auto slot_index = AcquireFaultSlot(state, event.dwThreadId);
    if (!slot_index) {
        ReleaseHandlerCall(shared);
        return false;
    }
    auto& monitor_slot = state.slots[*slot_index];
    if (!PrepareHandlerStack(state, monitor_slot)) {
        ReleaseHandlerCall(shared);
        return false;
    }

    if (!PrepareContextBuffer(state, *slot_index)) {
        ReleaseHandlerCall(shared);
        return false;
    }

    HANDLE thread = GetMonitorThread(state, event.dwThreadId);
    if (!thread) {
        ReleaseHandlerCall(shared);
        return false;
    }

    auto& fault_slot = shared->fault_slots[*slot_index];
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
    monitor_slot.tracking_state = tracking_state;
    monitor_slot.kind = kind;
    monitor_slot.fault_instruction = reinterpret_cast<u64>(exception.ExceptionAddress);
    monitor_slot.fault_address = fault_address;
    monitor_slot.fault_access = access;
    fault_slot.fault_address = fault_address;
    fault_slot.access = access;
    fault_slot.kind = kind;
    fault_slot.context_offset =
        static_cast<u64>(reinterpret_cast<std::byte*>(original_context) - state.view);
    Atomic(fault_slot.callback_result).store(CallbackResult::Unhandled, std::memory_order_relaxed);
    Atomic(fault_slot.completed_generation)
        .store(monitor_slot.generation - 1, std::memory_order_relaxed);
    Atomic(fault_slot.generation).store(monitor_slot.generation, std::memory_order_release);

    const uintptr_t child_slot_address = shared->debuggee_view_address +
                                         offsetof(SharedHeader, fault_slots) +
                                         *slot_index * sizeof(FaultSlot);
    CONTEXT trampoline_context = *original_context;
    trampoline_context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
    trampoline_context.Rip = shared->trampoline_address;
    trampoline_context.Rsp = HandlerStackPointer(monitor_slot.stack);
    trampoline_context.Rcx = child_slot_address;

    const bool redirected = SetThreadContext(thread, &trampoline_context) != FALSE;
    if (!redirected) {
        ReleaseMonitorSlot(monitor_slot);
        ReleaseHandlerCall(shared);
        return false;
    }

    return true;
}

bool RedirectTrackedAccess(MonitorState& state, const DEBUG_EVENT& event) {
    const auto& exception = event.u.Exception.ExceptionRecord;
    if (event.u.Exception.dwFirstChance == FALSE ||
        exception.ExceptionCode != EXCEPTION_ACCESS_VIOLATION ||
        exception.NumberParameters <= AccessViolationAddressIndex) {
        return false;
    }

    const auto operation = static_cast<AccessViolationOperation>(
        exception.ExceptionInformation[AccessViolationOperationIndex]);
    if (operation != AccessViolationOperation::Read &&
        operation != AccessViolationOperation::Write) {
        return false;
    }

    const VAddr fault_address = exception.ExceptionInformation[AccessViolationAddressIndex];
    const MemoryAccess access =
        operation == AccessViolationOperation::Write ? MemoryAccess::Write : MemoryAccess::Read;
    const bool tracked = IsTrackedAccess(state.view, fault_address, access);
    if (ShouldBypassRetry(state, event, fault_address, access, tracked)) {
        return false;
    }
    // Protection and bitmap updates occur on another guest thread. An access violation can already
    // be in flight when that thread clears the watch, so accept one provisional callback for any
    // page that has previously participated in tracking. ShouldBypassRetry forwards an unresolved
    // retry as a genuine exception.
    if (!tracked && !WasPreviouslyTracked(state.view, fault_address)) {
        return false;
    }
    const TrackingState tracking_state =
        tracked ? TrackingState::Watched : TrackingState::Provisional;
    return RedirectToTrampoline(state, event, FaultKind::Memory, fault_address, access,
                                tracking_state);
}

bool RedirectIllegalInstruction(MonitorState& state, const DEBUG_EVENT& event) {
    const auto& exception = event.u.Exception.ExceptionRecord;
    if (event.u.Exception.dwFirstChance == FALSE ||
        exception.ExceptionCode != EXCEPTION_ILLEGAL_INSTRUCTION ||
        ShouldBypassIllegalInstructionRetry(state, event)) {
        return false;
    }
    return RedirectToTrampoline(state, event, FaultKind::IllegalInstruction, {}, MemoryAccess::Read,
                                TrackingState::Watched);
}

std::wstring MakeMappingName() {
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    return L"Local\\shadPS4.WindowsFaultTracker." + std::to_wstring(GetCurrentProcessId()) + L"." +
           std::to_wstring(counter.QuadPart);
}

void SignalMonitorStatus(std::byte* view, HANDLE ready_event, MonitorStatus status) {
    Atomic(Header(view)->monitor_status).store(status, std::memory_order_release);
    SetEvent(ready_event);
}

int MonitorAttachedProcess(std::byte* view, DWORD target_pid, HANDLE ready_event) {
    if (!DebugActiveProcess(target_pid)) {
        std::fprintf(stderr, "Windows fault monitor could not attach: %lu\n", GetLastError());
        SignalMonitorStatus(view, ready_event, MonitorStatus::Failed);
        return EXIT_FAILURE;
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
                exception.ExceptionCode != DBG_PRINTEXCEPTION_WIDE_C) {
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
            SignalMonitorStatus(view, ready_event, MonitorStatus::Ready);
            ready_signaled = true;
        }
    }

    if (monitor_failed) {
        if (!ready_signaled) {
            SignalMonitorStatus(view, ready_event, MonitorStatus::Failed);
        }
        if (state.process) {
            TerminateProcess(state.process, EXIT_FAILURE);
        }
    }
    if (!ready_signaled && !monitor_failed) {
        SignalMonitorStatus(view, ready_event, MonitorStatus::Failed);
    }
    for (const auto& entry : state.threads) {
        CloseHandle(entry.second);
    }
    state.threads.clear();
    if (state.process) {
        CloseHandle(state.process);
    }
    return monitor_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

void ClearBootstrapEnvironment() {
    SetEnvironmentVariableW(MappingEnvironment, nullptr);
    SetEnvironmentVariableW(TargetPidEnvironment, nullptr);
}

std::optional<std::wstring> ReadEnvironmentVariable(const wchar_t* name) {
    const DWORD required_size = GetEnvironmentVariableW(name, nullptr, 0);
    if (required_size == 0) {
        return std::nullopt;
    }

    std::wstring value(required_size, L'\0');
    const DWORD length =
        GetEnvironmentVariableW(name, value.data(), static_cast<DWORD>(value.size()));
    if (length == 0 || length >= value.size()) {
        return std::nullopt;
    }
    value.resize(length);
    return value;
}

std::optional<std::wstring> GetExecutablePath() {
    std::wstring path(MAX_PATH, L'\0');
    for (;;) {
        const DWORD length =
            GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0) {
            return std::nullopt;
        }
        if (length < path.size()) {
            path.resize(length);
            return path;
        }
        if (path.size() > std::numeric_limits<DWORD>::max() / PathGrowthFactor) {
            return std::nullopt;
        }
        path.resize(path.size() * PathGrowthFactor);
    }
}

} // namespace

std::optional<int> Bootstrap(int argc, char* argv[]) {
    const auto mapping_name = ReadEnvironmentVariable(MappingEnvironment);
    const auto target_pid_text = ReadEnvironmentVariable(TargetPidEnvironment);
    if (mapping_name && target_pid_text) {
        ClearBootstrapEnvironment();

        const auto layout = QueryMappingLayout();
        HANDLE mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapping_name->c_str());
        auto* view =
            mapping ? static_cast<std::byte*>(MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0))
                    : nullptr;
        const std::wstring ready_event_name = *mapping_name + ReadyEventSuffix;
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
            return EXIT_FAILURE;
        }

        wchar_t* parse_end{};
        const unsigned long parsed_pid =
            std::wcstoul(target_pid_text->c_str(), &parse_end, DecimalRadix);
        const bool valid_pid = parsed_pid != 0 && parse_end &&
                               parse_end != target_pid_text->c_str() && *parse_end == L'\0';
        const int result =
            valid_pid ? MonitorAttachedProcess(view, static_cast<DWORD>(parsed_pid), ready_event)
                      : EXIT_FAILURE;
        if (!valid_pid) {
            SignalMonitorStatus(view, ready_event, MonitorStatus::Failed);
        }
        CloseHandle(ready_event);
        UnmapViewOfFile(view);
        CloseHandle(mapping);
        return result;
    }
    if (mapping_name || target_pid_text) {
        ClearBootstrapEnvironment();
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
        return EXIT_FAILURE;
    }

    const std::wstring new_mapping_name = MakeMappingName();
    const std::wstring ready_event_name = new_mapping_name + ReadyEventSuffix;
    HANDLE ready_event = CreateEventW(nullptr, TRUE, FALSE, ready_event_name.c_str());
    if (!ready_event) {
        return EXIT_FAILURE;
    }
    HANDLE mapping =
        CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                           static_cast<DWORD>(layout->mapping_size >> DwordBits),
                           static_cast<DWORD>(layout->mapping_size), new_mapping_name.c_str());
    if (!mapping) {
        CloseHandle(ready_event);
        return EXIT_FAILURE;
    }
    auto* view = static_cast<std::byte*>(
        MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, layout->mapping_size));
    if (!view) {
        CloseHandle(mapping);
        CloseHandle(ready_event);
        return EXIT_FAILURE;
    }

    // A new paging-file-backed mapping is demand-zeroed. Do not eagerly touch its context slots
    // and sparse page bitmaps.
    Header(view)->slot_count = MaxFaultSlots;
    Header(view)->context_buffer_size = layout->context_buffer_size;
    Header(view)->context_buffer_stride = layout->context_buffer_stride;
    Header(view)->context_buffers_offset = layout->context_buffers_offset;
    Header(view)->watched_bitmap_offset = layout->watched_bitmap_offset;
    Header(view)->read_bitmap_offset = layout->read_bitmap_offset;
    Header(view)->tracking_history_bitmap_offset = layout->tracking_history_bitmap_offset;
    Header(view)->mapping_size = layout->mapping_size;
    Atomic(Header(view)->monitor_status).store(MonitorStatus::Starting, std::memory_order_relaxed);

    const std::wstring target_pid = std::to_wstring(GetCurrentProcessId());
    if (!SetEnvironmentVariableW(MappingEnvironment, new_mapping_name.c_str()) ||
        !SetEnvironmentVariableW(TargetPidEnvironment, target_pid.c_str())) {
        ClearBootstrapEnvironment();
        UnmapViewOfFile(view);
        CloseHandle(mapping);
        CloseHandle(ready_event);
        return EXIT_FAILURE;
    }

    const auto executable = GetExecutablePath();
    STARTUPINFOW startup_info{};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info{};
    std::wstring command_line = executable ? L"\"" + *executable + L"\"" : std::wstring{};
    const bool monitor_started =
        executable &&
        CreateProcessW(executable->c_str(), command_line.data(), nullptr, nullptr, FALSE,
                       CREATE_NO_WINDOW, nullptr, nullptr, &startup_info, &process_info) != FALSE;
    ClearBootstrapEnvironment();
    if (!monitor_started) {
        std::fprintf(stderr, "Failed to start Windows fault monitor: %lu\n", GetLastError());
        UnmapViewOfFile(view);
        CloseHandle(mapping);
        CloseHandle(ready_event);
        return EXIT_FAILURE;
    }
    CloseHandle(process_info.hThread);

    const DWORD wait_result = WaitForSingleObject(ready_event, AttachTimeout);
    const MonitorStatus monitor_status =
        Atomic(Header(view)->monitor_status).load(std::memory_order_acquire);
    CloseHandle(ready_event);
    if (wait_result != WAIT_OBJECT_0 || monitor_status != MonitorStatus::Ready) {
        std::fprintf(stderr, "Windows fault monitor did not become ready.\n");
        TerminateProcess(process_info.hProcess, EXIT_FAILURE);
        WaitForSingleObject(process_info.hProcess, TerminationTimeout);
        CloseHandle(process_info.hProcess);
        UnmapViewOfFile(view);
        CloseHandle(mapping);
        return EXIT_FAILURE;
    }
    CloseHandle(process_info.hProcess);

    debuggee_view = view;
    CloseHandle(mapping);
    return std::nullopt;
}

bool IsEnabled() noexcept {
    if (!debuggee_view) {
        return false;
    }
    const HandlerFlag enabled =
        Atomic(Header(debuggee_view)->enabled_handlers).load(std::memory_order_acquire);
    return True(enabled & HandlerFlag::Memory);
}

void InstallFaultHandler(std::function<bool(VAddr, u64, MemoryAccess)> callback) {
    if (!debuggee_view || !callback) {
        return;
    }

    auto* header = Header(debuggee_view);
    const HandlerFlag enabled = Atomic(header->enabled_handlers).load(std::memory_order_acquire);
    if (True(enabled & HandlerFlag::Memory)) {
        return;
    }
    fault_callback = std::move(callback);
    header->debuggee_view_address = reinterpret_cast<uintptr_t>(debuggee_view);
    header->trampoline_address = reinterpret_cast<uintptr_t>(&FaultTrampoline);
    EnableHandler(header, HandlerFlag::Memory);
}

void RemoveFaultHandler() {
    if (!debuggee_view) {
        return;
    }
    auto* header = Header(debuggee_view);
    DisableHandler(header, HandlerFlag::Memory);
    while (Atomic(header->active_handler_calls).load(std::memory_order_acquire) != 0) {
        std::this_thread::yield();
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
    EnableHandler(header, HandlerFlag::IllegalInstruction);
}

static void UpdateBitmap(u64* bitmap, u64* sticky_bitmap, VAddr address, u64 size,
                         WatchAction action) noexcept {
    if (!bitmap || size == 0 || address >= GuestAddressLimit) {
        return;
    }
    const size_t start_page = address >> PageBits;
    const VAddr last_address = std::min<VAddr>(
        address + std::min<u64>(size - 1, std::numeric_limits<VAddr>::max() - address),
        GuestAddressLimit - 1);
    const size_t end_page = (last_address >> PageBits) + 1;
    const size_t first_word = start_page / BitmapWordBits;
    const size_t last_word = (end_page - 1) / BitmapWordBits;
    for (size_t word = first_word; word <= last_word; ++word) {
        const size_t first_bit = word == first_word ? start_page % BitmapWordBits : 0;
        const size_t end_bit = word == last_word ? end_page % BitmapWordBits : 0;
        u64 mask = std::numeric_limits<u64>::max() << first_bit;
        if (end_bit != 0) {
            mask &= (1ULL << end_bit) - 1;
        }
        if (action == WatchAction::Add) {
            if (sticky_bitmap) {
                Atomic(sticky_bitmap[word]).fetch_or(mask, std::memory_order_release);
            }
            Atomic(bitmap[word]).fetch_or(mask, std::memory_order_release);
        } else {
            Atomic(bitmap[word]).fetch_and(~mask, std::memory_order_release);
        }
    }
}

void WatchMemory(VAddr address, u64 size, MemoryAccess access, WatchAction action) noexcept {
    if (!IsEnabled()) {
        return;
    }
    UpdateBitmap(WatchedBitmap(debuggee_view, access), TrackingHistoryBitmap(debuggee_view),
                 address, size, action);
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

void WatchMemory(VAddr, u64, MemoryAccess, WatchAction) noexcept {}

} // namespace Core::WindowsFaultTracker

#endif
