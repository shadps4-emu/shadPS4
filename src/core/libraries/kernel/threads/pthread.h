// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <condition_variable>
#include <forward_list>
#include <list>
#include <memory>
#include <mutex>
#include <shared_mutex>

#include "common/enum.h"
#include "common/shared_first_mutex.h"
#include "core/libraries/kernel/sync/mutex.h"
#include "core/libraries/kernel/sync/semaphore.h"
#include "core/libraries/kernel/threads/exception.h"
#include "core/libraries/kernel/time.h"
#include "core/thread.h"
#include "core/tls.h"

#define GLOBAL_PID 0xBAD1

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

constexpr int PthreadInheritSched = 4;
constexpr s32 PthreadBarrierSerialThread = -1;

constexpr int ORBIS_KERNEL_PRIO_FIFO_DEFAULT = 700;
constexpr int ORBIS_KERNEL_PRIO_FIFO_LOWEST = 256;
constexpr int ORBIS_KERNEL_PRIO_FIFO_HIGHEST = 767;
constexpr int ORBIS_KERNEL_PRIO_OTHER_DEFAULT = 900;
constexpr int ORBIS_KERNEL_PRIO_OTHER_LOWEST = 768;
constexpr int ORBIS_KERNEL_PRIO_OTHER_HIGHEST = 959;
constexpr int ORBIS_KERNEL_PRIO_RR_DEFAULT = 700;
constexpr int ORBIS_KERNEL_PRIO_RR_LOWEST = 256;
constexpr int ORBIS_KERNEL_PRIO_RR_HIGHEST = 767;

struct Umutex {
    std::atomic<u32> m_owner{0};
    u32 m_flags{0};
    u32 m_ceilings[2]{0, 0};
    u32 m_spare[4]{0, 0, 0, 0};
};
static_assert(sizeof(Umutex) == 0x20);

struct PthreadQueueEntry {
    void* next{nullptr};
    void** prev{nullptr};
};
static_assert(sizeof(PthreadQueueEntry) == 0x10);

struct PthreadListEntry {
    void* next{nullptr};
    void** prev{nullptr};
};
static_assert(sizeof(PthreadListEntry) == 0x10);

struct MutexQueue {
    void* head{nullptr};
    void** tail{nullptr};
};
static_assert(sizeof(MutexQueue) == 0x10);

struct Pthread;

struct PthreadBarrier {
    struct State {
        std::mutex mutex;
        std::condition_variable cv;
        u32 count;
        u32 arrived{};
        u64 generation{};
    };
    std::shared_ptr<State> state;
};
using PthreadBarrierT = PthreadBarrier*;

struct PthreadBarrierAttr {
    s32 pshared;
};
using PthreadBarrierAttrT = PthreadBarrierAttr*;

enum class PthreadMutexFlags : u32 {
    TypeMask = 0xff,
    Private = 0x100,
    Deferred = 0x200,
};
DECLARE_ENUM_FLAG_OPERATORS(PthreadMutexFlags)

enum class PthreadMutexType : u32 {
    ErrorCheck = 1,
    Recursive = 2,
    Normal = 3,
    AdaptiveNp = 4,
    Max
};

enum class PthreadMutexProt : u32 {
    None = 0,
    Inherit = 1,
    Protect = 2,
};

struct PthreadMutex {
    Umutex m_umutex{};                                   // 0x00 - 0x20
    PthreadMutexFlags m_flags{};                         // 0x20 - 0x24
    s32 m_pad_24{0};                                     // 0x24 - 0x28
    std::atomic<Pthread*> m_owner{nullptr};              // 0x28 - 0x30
    int m_count{0};                                      // 0x30 - 0x34
    int m_spinloops{0};                                  // 0x34 - 0x38
    int m_yieldloops{0};                                 // 0x38 - 0x3c
    PthreadMutexProt m_protocol{PthreadMutexProt::None}; // 0x3c - 0x40
    PthreadQueueEntry m_qe{};                            // 0x40 - 0x50

    // Host emulation fields (past 0x50 guest boundary):
    TimedMutex m_lock{};
    std::string name{};

    [[nodiscard]] PthreadMutexType Type() const noexcept {
        return static_cast<PthreadMutexType>(m_flags & PthreadMutexFlags::TypeMask);
    }

    int SelfTryLock();
    int SelfLock(const OrbisKernelTimespec* abstime, u64 usec);

    int TryLock();
    int Lock(const OrbisKernelTimespec* abstime, u64 usec = 0);

    int CvLock(int recurse) {
        const int error = Lock(nullptr);
        if (error == 0) {
            m_count = recurse;
        }
        return error;
    }

    int Unlock();

    int CvUnlock(int* recurse) {
        *recurse = m_count;
        m_count = 0;
        return Unlock();
    }

    int IsOwned(Pthread* curthread) const;
};
using PthreadMutexT = PthreadMutex*;

static_assert(offsetof(PthreadMutex, m_umutex) == 0x00);
static_assert(offsetof(PthreadMutex, m_flags) == 0x20);
static_assert(offsetof(PthreadMutex, m_owner) == 0x28);
static_assert(offsetof(PthreadMutex, m_count) == 0x30);
static_assert(offsetof(PthreadMutex, m_spinloops) == 0x34);
static_assert(offsetof(PthreadMutex, m_yieldloops) == 0x38);
static_assert(offsetof(PthreadMutex, m_protocol) == 0x3c);
static_assert(offsetof(PthreadMutex, m_qe) == 0x40);
static_assert(offsetof(PthreadMutex, m_lock) == 0x50);

struct PthreadMutexAttr {
    PthreadMutexType m_type;
    PthreadMutexProt m_protocol;
    int m_ceiling;
};
using PthreadMutexAttrT = PthreadMutexAttr*;

enum class PthreadCondFlags : u32 {
    Private = 1,
    Inited = 2,
    Busy = 4,
};

enum class ClockId : u32 {
    Realtime = 0,
    Virtual = 1,
    Prof = 2,
    Monotonic = 4,
    Uptime = 5,
    UptimePrecise = 7,
    UptimeFast = 8,
    RealtimePrecise = 9,
    RealtimeFast = 10,
    MonotonicPrecise = 11,
    MonotonicFast = 12,
    Second = 13,
    ThreadCputimeID = 14,
};

struct PthreadCond {
    bool has_user_waiters;
    bool has_kern_waiters;
    u32 flags;
    ClockId clock_id;
    std::string name;

    int Wait(PthreadMutexT* mutex, const OrbisKernelTimespec* abstime, u64 usec = 0);

    int Signal(Pthread* thread);
    int Broadcast();
};
using PthreadCondT = PthreadCond*;

struct PthreadCondAttr {
    int c_pshared;
    ClockId c_clockid;
};
using PthreadCondAttrT = PthreadCondAttr*;

using PthreadCleanupFunc = void PS4_SYSV_ABI (*)(void*);

struct PthreadCleanup {
    PthreadCleanup* prev{nullptr};
    PthreadCleanupFunc routine{nullptr};
    void* routine_arg{nullptr};
    int onheap{0};
};
static_assert(sizeof(PthreadCleanup) == 0x20);

enum class PthreadAttrFlags : u32 {
    ScopeProcess = 0,
    Detached = 1,
    ScopeSystem = 2,
    InheritSched = 4,
    NoFloat = 8,
    StackUser = 0x100,
};
DECLARE_ENUM_FLAG_OPERATORS(PthreadAttrFlags)

enum class SchedPolicy : u32 {
    Fifo = 1,
    Other = 2,
    RoundRobin = 3,
};

struct Cpuset {
    u64 bits;
    u64 _reserved;
};

struct PthreadAttr {
    SchedPolicy sched_policy;
    int sched_inherit;
    int prio;
    int suspend;
    PthreadAttrFlags flags;
    u32 pad;
    void* stackaddr_attr;
    size_t stacksize_attr;
    size_t guardsize_attr;
    Cpuset* cpuset;
    size_t cpusetsize;
};
using PthreadAttrT = PthreadAttr*;
static_assert(sizeof(PthreadAttr) == 0x40);

static constexpr u32 ThrStackDefault = 1_MB;
static constexpr u32 ThrStackInitial = 2_MB;
static constexpr u32 ThrPageSize = 16_KB;
static constexpr u32 ThrGuardDefault = ThrPageSize;

struct PthreadRwlockAttr {
    int pshared;
    int type;
};
using PthreadRwlockAttrT = PthreadRwlockAttr*;

struct PthreadRwlock {
    Common::SharedFirstMutex lock;
    Pthread* owner;
    std::string name;

    int Wrlock(const OrbisKernelTimespec* abstime);
    int Rdlock(const OrbisKernelTimespec* abstime);
};
using PthreadRwlockT = PthreadRwlock*;

enum class PthreadState : u32 { Running, Dead };

struct PthreadSpecificElem {
    const void* data;
    int seqno;
};

using PthreadKeyDestructor = void PS4_SYSV_ABI (*)(const void*);

struct PthreadKey {
    std::atomic<int> allocated;
    std::atomic<int> seqno;
    PthreadKeyDestructor destructor;
};
using PthreadKeyT = s32;

enum class PthreadOnceState : u32 {
    NeverDone = 0,
    Done = 1,
    InProgress = 2,
    Wait = 3,
};

struct PthreadOnce {
    std::atomic<PthreadOnceState> state;
    PthreadMutexT mutex;
};

enum class ThreadFlags : u32 {
    Private = 1,
    NeedSuspend = 2,
    Suspended = 4,
    Detached = 8,
};
DECLARE_ENUM_FLAG_OPERATORS(ThreadFlags)

enum class ThreadListFlags : u32 {
    GcSafe = 1,
    InTdList = 2,
    InGcList = 4,
};

using PthreadEntryFunc = void* PS4_SYSV_ABI (*)(void*);

constexpr s32 TidTerminated = 1;

struct SleepQueue;

struct SchedParam {
    int sched_priority;
};

#define THR_RELTIME (const OrbisKernelTimespec*)-1

struct HostThreadContext {
    std::mutex lock{};
    std::string name{};
    std::unique_ptr<Core::NativeThread> native_thr{};
    WakeSemaphore wake_sema{0};
    std::mutex join_wait_mutex{};
    std::condition_variable join_wait_cv{};
    Pthread* join_target{nullptr};
    std::atomic_bool cancel_point{false};

    SleepQueue* sleepqueue{nullptr};
    void* wchan{nullptr};

    std::array<std::atomic<u32>, 128> pending_signal_counts{};
    std::array<std::atomic<u32>, 4> guest_sigmask{};
    WakeSemaphore signal_sema{0};
    std::atomic_bool in_sigwait{false};
    std::atomic_bool sigsuspend_interrupted{false};
    Sigset sigwait_set{};
    OrbisKernelExceptionHandlerStack sigaltstack{};
    Mcontext suspended_context{};
    std::atomic_bool is_suspended_in_signal{false};
};

extern thread_local HostThreadContext* g_curhostthread;
extern thread_local Pthread* g_curthread;

HostThreadContext* GetHostThreadContext(Pthread* thread);

struct Pthread {
    static constexpr u32 ThrMagic = 0xd09ba115U;
    static constexpr u32 MaxDeferWaiters = 50;

    // 0x00
    std::atomic<s64> tid{0};

    // 0x08
    Umutex lock{};

    // 0x28
    u32 cycle{0};

    // 0x2c
    std::atomic_int locklevel{0};

    // 0x30
    std::atomic_int critical_count{0};

    // 0x34
    int sigblock{0};

    // 0x38
    PthreadQueueEntry tle{};

    // 0x48
    PthreadQueueEntry gcle{};

    // 0x58
    PthreadListEntry hle{};

    // 0x68
    PthreadQueueEntry wle{};

    // 0x78
    std::atomic_int refcount{0};

    // 0x7c
    u32 pad_7c{0};

    // 0x80
    PthreadEntryFunc start_routine{nullptr};

    // 0x88
    void* arg{nullptr};

    // 0x90
    PthreadAttr attr{};

    // 0xd0
    std::atomic_int cancel_enable{1};
    std::atomic_int cancel_pending{0};
    std::atomic_int cancelling{0};
    std::atomic_int no_cancel{0};
    std::atomic_int cancel_async{0};
    int unblock_sigcancel{0};

    // 0xe8
    u64 sigmask{0};
    u64 sigmask_high{0};

    // 0xf8
    std::atomic_int in_sigsuspend{0};
    u32 pad_fc{0};

    // 0x100..0x183 (deferred signals: 0x84 = 132 bytes)
    u8 deferred_siginfo_and_act[0x84]{};

    // 0x184
    PthreadState state{PthreadState::Running};
    int error{0};
    u32 pad_18c{0};
    Pthread* joiner{nullptr};
    ThreadFlags flags{};
    ThreadListFlags tlflags{};

    // 0x1a0
    MutexQueue mutexq{};
    // 0x1b0
    MutexQueue pp_mutexq{};

    // 0x1c0
    void* ret{nullptr};

    // 0x1c8
    PthreadSpecificElem* specific{nullptr};
    int specific_data_count{0};
    int rdlock_count{0};
    int rtld_bits{0};
    u32 pad_1dc{0};

    // 0x1e0
    Core::Tcb* tcb{nullptr};

    // 0x1e8
    PthreadCleanup* cleanup{nullptr};

    // 0x1f0
    u8 unwind_ex[0x20]{};
    void* unwind_stackend{nullptr};
    int unwind_disabled{0};
    u32 magic{ThrMagic};

    // 0x220
    int report_events{0};
    int event_mask{0};
    u8 event_buf[24]{};

    // 0x240
    void* wake_addr{nullptr};
    PthreadMutex* mutex_obj{nullptr};
    int will_sleep{0};
    int nwaiter_defer{0};
    void* defer_waiters[MaxDeferWaiters]{};
    void* spare_3e8{nullptr};
    void* spare_3f0{nullptr};
    void* spare_3f8{nullptr};

    HostThreadContext* GetHostContext() const {
        return GetHostThreadContext(const_cast<Pthread*>(this));
    }

    std::mutex& GetLock() const {
        return GetHostContext()->lock;
    }

    const std::string& GetName() const {
        static const std::string empty;
        auto* ctx = GetHostContext();
        return ctx ? ctx->name : empty;
    }

    void SetName(std::string_view n) {
        if (auto* ctx = GetHostContext()) {
            ctx->name = n;
        }
    }

    void Wake() {
        if (auto* ctx = GetHostContext()) {
            ctx->wake_sema.release();
        }
    }

    std::atomic_bool& CancelPoint() {
        return GetHostContext()->cancel_point;
    }

    SleepQueue*& Sleepqueue() {
        return GetHostContext()->sleepqueue;
    }

    void*& Wchan() {
        return GetHostContext()->wchan;
    }

    WakeSemaphore& SignalSema() {
        return GetHostContext()->signal_sema;
    }

    std::atomic_bool& InSigwait() {
        return GetHostContext()->in_sigwait;
    }

    std::atomic_bool& SigsuspendInterrupted() {
        return GetHostContext()->sigsuspend_interrupted;
    }

    Sigset& SigwaitSet() {
        return GetHostContext()->sigwait_set;
    }

    OrbisKernelExceptionHandlerStack& SigAltStack() {
        return GetHostContext()->sigaltstack;
    }

    Mcontext& SuspendedContext() {
        return GetHostContext()->suspended_context;
    }

    std::atomic_bool& IsSuspendedInSignal() {
        return GetHostContext()->is_suspended_in_signal;
    }

    bool IsSignalBlocked(s32 sig) const;
    void QueueSignal(s32 sig);
    bool ConsumeSignal(s32 sig);
    s32 FindPendingSignal(const Sigset& set) const;
    s32 FindPendingUnblockedSignal() const;
    bool HasPendingSignal() const;
    bool HasDeliverableSignal() const;
    void WakeForSignal();
    bool DispatchSignal(s32 sig, Siginfo* info, Ucontext* context);
    bool DispatchPendingSignals(Siginfo* info, Ucontext* context);
    void GetGuestSigmask(Sigset& mask);
    void SetGuestSigmask(Sigset const& mask);

    bool InCritical() const noexcept {
        return locklevel.load(std::memory_order_acquire) > 0 ||
               critical_count.load(std::memory_order_acquire) > 0;
    }

    bool ShouldCollect() const noexcept {
        return refcount == 0 && state == PthreadState::Dead && True(flags & ThreadFlags::Detached);
    }

    bool ShouldCancel() const noexcept {
        return cancel_pending && cancel_enable && no_cancel == 0;
    }

    void WakeAll() {
        for (int i = 0; i < nwaiter_defer; i++) {
            auto* waiter = reinterpret_cast<WakeSemaphore*>(defer_waiters[i]);
            defer_waiters[i] = nullptr;
            if (waiter != nullptr) {
                waiter->release();
            }
        }
        nwaiter_defer = 0;
    }

    void ClearWake() {
        if (auto* ctx = GetHostContext()) {
            while (ctx->wake_sema.try_acquire_pending()) {
            }
        }
    }

    bool Sleep(const OrbisKernelTimespec* abstime, u64 usec, ClockId clock_id = ClockId::Realtime) {
        will_sleep = 0;
        if (nwaiter_defer > 0) {
            WakeAll();
        }
        auto* ctx = GetHostContext();
        if (!ctx) {
            return false;
        }
        if (abstime == THR_RELTIME) {
            constexpr u64 MaxUsec =
                static_cast<u64>(std::chrono::duration_cast<std::chrono::microseconds>(
                                     std::chrono::nanoseconds::max())
                                     .count());
            const auto timeout = std::chrono::microseconds(std::min(usec, MaxUsec));
            return ctx->wake_sema.try_acquire_for(timeout);
        } else if (abstime != nullptr) {
            OrbisKernelTimespec now{};
            if (posix_clock_gettime(static_cast<u32>(clock_id), &now) != 0) {
                return false;
            }

            if (abstime->tv_sec < now.tv_sec ||
                (abstime->tv_sec == now.tv_sec && abstime->tv_nsec <= now.tv_nsec)) {
                return ctx->wake_sema.try_acquire_pending();
            }

            s64 seconds = abstime->tv_sec - now.tv_sec;
            s64 nanoseconds = abstime->tv_nsec - now.tv_nsec;
            if (nanoseconds < 0) {
                --seconds;
                nanoseconds += 1'000'000'000;
            }

            constexpr auto MaxTimeout = std::chrono::nanoseconds::max();
            constexpr s64 MaxSeconds =
                std::chrono::duration_cast<std::chrono::seconds>(MaxTimeout).count();
            const auto seconds_part = std::chrono::seconds(std::min(seconds, MaxSeconds));
            const auto max_nanoseconds = MaxTimeout - seconds_part;
            const auto timeout =
                seconds > MaxSeconds || std::chrono::nanoseconds(nanoseconds) > max_nanoseconds
                    ? MaxTimeout
                    : seconds_part + std::chrono::nanoseconds(nanoseconds);
            return ctx->wake_sema.try_acquire_for(timeout);
        } else {
            ctx->wake_sema.acquire();
            return true;
        }
    }

    int SetAffinity(const Cpuset* cpuset);
};

static_assert(sizeof(Pthread) == 0x400, "Pthread size must be 0x400");
static_assert(std::is_standard_layout_v<Pthread>, "Pthread must be standard layout");
static_assert(offsetof(Pthread, tid) == 0x00);
static_assert(offsetof(Pthread, lock) == 0x08);
static_assert(offsetof(Pthread, cycle) == 0x28);
static_assert(offsetof(Pthread, locklevel) == 0x2c);
static_assert(offsetof(Pthread, critical_count) == 0x30);
static_assert(offsetof(Pthread, sigblock) == 0x34);
static_assert(offsetof(Pthread, tle) == 0x38);
static_assert(offsetof(Pthread, gcle) == 0x48);
static_assert(offsetof(Pthread, hle) == 0x58);
static_assert(offsetof(Pthread, wle) == 0x68);
static_assert(offsetof(Pthread, refcount) == 0x78);
static_assert(offsetof(Pthread, start_routine) == 0x80);
static_assert(offsetof(Pthread, arg) == 0x88);
static_assert(offsetof(Pthread, attr) == 0x90);
static_assert(offsetof(Pthread, cancel_enable) == 0xd0);
static_assert(offsetof(Pthread, cancel_pending) == 0xd4);
static_assert(offsetof(Pthread, cancelling) == 0xd8);
static_assert(offsetof(Pthread, no_cancel) == 0xdc);
static_assert(offsetof(Pthread, cancel_async) == 0xe0);
static_assert(offsetof(Pthread, unblock_sigcancel) == 0xe4);
static_assert(offsetof(Pthread, sigmask) == 0xe8);
static_assert(offsetof(Pthread, sigmask_high) == 0xf0);
static_assert(offsetof(Pthread, in_sigsuspend) == 0xf8);
static_assert(offsetof(Pthread, state) == 0x184);
static_assert(offsetof(Pthread, error) == 0x188);
static_assert(offsetof(Pthread, joiner) == 0x190);
static_assert(offsetof(Pthread, flags) == 0x198);
static_assert(offsetof(Pthread, tlflags) == 0x19c);
static_assert(offsetof(Pthread, mutexq) == 0x1a0);
static_assert(offsetof(Pthread, pp_mutexq) == 0x1b0);
static_assert(offsetof(Pthread, ret) == 0x1c0);
static_assert(offsetof(Pthread, specific) == 0x1c8);
static_assert(offsetof(Pthread, specific_data_count) == 0x1d0);
static_assert(offsetof(Pthread, rdlock_count) == 0x1d4);
static_assert(offsetof(Pthread, rtld_bits) == 0x1d8);
static_assert(offsetof(Pthread, tcb) == 0x1e0);
static_assert(offsetof(Pthread, cleanup) == 0x1e8);
static_assert(offsetof(Pthread, unwind_stackend) == 0x210);
static_assert(offsetof(Pthread, unwind_disabled) == 0x218);
static_assert(offsetof(Pthread, magic) == 0x21c);
static_assert(offsetof(Pthread, report_events) == 0x220);
static_assert(offsetof(Pthread, event_mask) == 0x224);
static_assert(offsetof(Pthread, event_buf) == 0x228);
static_assert(offsetof(Pthread, wake_addr) == 0x240);
static_assert(offsetof(Pthread, mutex_obj) == 0x248);
static_assert(offsetof(Pthread, will_sleep) == 0x250);
static_assert(offsetof(Pthread, nwaiter_defer) == 0x254);
static_assert(offsetof(Pthread, defer_waiters) == 0x258);
static_assert(offsetof(Pthread, spare_3e8) == 0x3e8);
static_assert(offsetof(Pthread, spare_3f0) == 0x3f0);
static_assert(offsetof(Pthread, spare_3f8) == 0x3f8);

using PthreadT = Pthread*;

extern thread_local Pthread* g_curthread;

void PthreadTestCancel();
void PthreadCancelInterrupt() noexcept;
#ifndef _WIN32
bool IsPthreadCancelSignal(int native_signal) noexcept;
#endif
void PS4_SYSV_ABI posix_pthread_exit(void* status);

void RegisterBarrier(Core::Loader::SymbolsResolver* sym);
void RegisterMutex(Core::Loader::SymbolsResolver* sym);
void RegisterCond(Core::Loader::SymbolsResolver* sym);
void RegisterRwlock(Core::Loader::SymbolsResolver* sym);
void RegisterSemaphore(Core::Loader::SymbolsResolver* sym);
void RegisterSpec(Core::Loader::SymbolsResolver* sym);
void RegisterThreadAttr(Core::Loader::SymbolsResolver* sym);
void RegisterThread(Core::Loader::SymbolsResolver* sym);
void RegisterRtld(Core::Loader::SymbolsResolver* sym);
void RegisterKernelEventFlag(Core::Loader::SymbolsResolver* sym);
void RegisterPthreadClean(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
