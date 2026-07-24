// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <condition_variable>
#include <forward_list>
#include <list>
#include <mutex>
#include <shared_mutex>

#include "common/enum.h"
#include "common/shared_first_mutex.h"
#include "core/libraries/kernel/sync/mutex.h"
#include "core/libraries/kernel/sync/semaphore.h"
#include "core/libraries/kernel/time.h"
#include "core/thread.h"
#include "core/tls.h"

#define GLOBAL_PID 0xBAD1

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

constexpr int PthreadInheritSched = 4;

constexpr int ORBIS_KERNEL_PRIO_FIFO_DEFAULT = 700;
constexpr int ORBIS_KERNEL_PRIO_FIFO_LOWEST = 256;
constexpr int ORBIS_KERNEL_PRIO_FIFO_HIGHEST = 767;
constexpr int ORBIS_KERNEL_PRIO_OTHER_DEFAULT = 900;
constexpr int ORBIS_KERNEL_PRIO_OTHER_LOWEST = 768;
constexpr int ORBIS_KERNEL_PRIO_OTHER_HIGHEST = 959;
constexpr int ORBIS_KERNEL_PRIO_RR_DEFAULT = 700;
constexpr int ORBIS_KERNEL_PRIO_RR_LOWEST = 256;
constexpr int ORBIS_KERNEL_PRIO_RR_HIGHEST = 767;

struct Pthread;

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
    std::atomic<Pthread*> m_owner;
    int m_count;
    int m_spinloops;
    int m_yieldloops;
    PthreadMutexProt m_protocol;
    u64 : 64;
    PthreadMutexFlags m_flags;
    TimedMutex m_lock;
    std::string name;

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

// libc and libSceLibcInternal modify the m_flags of a mutex, make sure it's in the right spot.
static_assert(offsetof(PthreadMutex, m_flags) == 0x20, "Incorrect offset for mutex flags");

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
    PthreadCleanupFunc routine;
    void* routine_arg;
    int onheap;
};

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
    void* stackaddr_attr;
    size_t stacksize_attr;
    size_t guardsize_attr;
    size_t cpusetsize;
    Cpuset* cpuset;
};
using PthreadAttrT = PthreadAttr*;

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
    std::mutex mutex;
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

struct Pthread {
    static constexpr u32 ThrMagic = 0xd09ba115U;
    static constexpr u32 MaxDeferWaiters = 50;

    std::atomic<s32> tid;
    std::mutex lock;
    u32 cycle;
    std::atomic_int locklevel;
    std::atomic_int critical_count;
    int sigblock;
    int refcount;
    PthreadEntryFunc start_routine;
    void* arg;
    Core::NativeThread native_thr;
    PthreadAttr attr;
    std::atomic_bool cancel_enable;
    std::atomic_bool cancel_pending;
    std::atomic_bool cancel_point;
    std::atomic_bool no_cancel;
    std::atomic_bool cancel_async;
    std::atomic_bool cancelling;
    u64 sigmask;
    bool unblock_sigcancel;
    bool in_sigsuspend;
    bool force_exit;
    PthreadState state;
    int error;
    Pthread* joiner;
    ThreadFlags flags;
    ThreadListFlags tlflags;
    void* ret;
    PthreadSpecificElem* specific;
    int specific_data_count;
    int rdlock_count;
    int rtld_bits;
    Core::Tcb* tcb;
    std::forward_list<PthreadCleanup*> cleanup;
    u32 pad[27];
    u32 magic;
    int report_events;
    int event_mask;
    std::string name;
    WakeSemaphore wake_sema{0};
    SleepQueue* sleepqueue;
    void* wchan;
    PthreadMutex* mutex_obj;
    bool will_sleep;
    bool has_user_waiters;
    int nwaiter_defer;
    WakeSemaphore* defer_waiters[MaxDeferWaiters]{};
    Pthread* join_target{};
    std::mutex join_wait_mutex;
    std::condition_variable join_wait_cv;

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
            WakeSemaphore* waiter = defer_waiters[i];
            defer_waiters[i] = nullptr;
            if (waiter != nullptr) {
                waiter->release();
            }
        }
        nwaiter_defer = 0;
    }

    void ClearWake() {
        while (wake_sema.try_acquire_pending()) {
        }
    }

    bool Sleep(const OrbisKernelTimespec* abstime, u64 usec, ClockId clock_id = ClockId::Realtime) {
        will_sleep = false;
        if (nwaiter_defer > 0) {
            WakeAll();
        }
        if (abstime == THR_RELTIME) {
            constexpr u64 MaxUsec =
                static_cast<u64>(std::chrono::duration_cast<std::chrono::microseconds>(
                                     std::chrono::nanoseconds::max())
                                     .count());
            const auto timeout = std::chrono::microseconds(std::min(usec, MaxUsec));
            return wake_sema.try_acquire_for(timeout);
        } else if (abstime != nullptr) {
            OrbisKernelTimespec now{};
            if (posix_clock_gettime(static_cast<u32>(clock_id), &now) != 0) {
                return false;
            }

            if (abstime->tv_sec < now.tv_sec ||
                (abstime->tv_sec == now.tv_sec && abstime->tv_nsec <= now.tv_nsec)) {
                return wake_sema.try_acquire_pending();
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
            return wake_sema.try_acquire_for(timeout);
        } else {
            wake_sema.acquire();
            return true;
        }
    }

    int SetAffinity(const Cpuset* cpuset);
};
using PthreadT = Pthread*;

extern thread_local Pthread* g_curthread;

void PthreadTestCancel();
void PthreadCancelInterrupt() noexcept;
#ifndef _WIN32
bool IsPthreadCancelSignal(int native_signal) noexcept;
#endif
void PS4_SYSV_ABI posix_pthread_exit(void* status);

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
