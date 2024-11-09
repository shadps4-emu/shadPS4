// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <forward_list>
#include <list>
#include <mutex>
#include <semaphore>
#include <shared_mutex>
#include <boost/intrusive/list.hpp>

#include "common/enum.h"
#include "core/libraries/kernel/time.h"
#include "core/thread.h"
#include "core/tls.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

struct Pthread;

using ListBaseHook =
    boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>;

enum class PthreadMutexFlags : u32 {
    TypeMask = 0xff,
    Defered = 0x200,
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

struct PthreadMutex : public ListBaseHook {
    std::timed_mutex m_lock;
    PthreadMutexFlags m_flags;
    Pthread* m_owner;
    int m_count;
    int m_spinloops;
    int m_yieldloops;
    PthreadMutexProt m_protocol;
    std::string name;

    PthreadMutexType Type() const noexcept {
        return static_cast<PthreadMutexType>(m_flags & PthreadMutexFlags::TypeMask);
    }

    void lock() {
        Lock(nullptr);
    }

    void unlock() {
        Unlock();
    }

    int SelfTryLock();
    int SelfLock(const OrbisKernelTimespec* abstime, u64 usec);

    int TryLock();
    int Lock(const OrbisKernelTimespec* abstime, u64 usec = 0);

    int Unlock();

    bool IsOwned(Pthread* curthread) const;
};
using PthreadMutexT = PthreadMutex*;

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
    std::condition_variable_any cond;
    u32 has_user_waiters;
    u32 has_kern_waiters;
    u32 flags;
    ClockId clock_id;
    std::string name;

    int Wait(PthreadMutexT* mutex, const OrbisKernelTimespec* abstime);
    int Wait(PthreadMutexT* mutex, u64 usec);
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
    Detached = 1,
    ScopeSystem = 2,
    InheritSched = 4,
    NoFloat = 8,
    StackUser = 0x100,
};
DECLARE_ENUM_FLAG_OPERATORS(PthreadAttrFlags)

enum class SchedPolicy : u32 {
    Fifo = 0,
    RoundRobin = 3,
};

struct Cpuset {
    u64 bits;
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
};
using PthreadRwlockAttrT = PthreadRwlockAttr*;

struct PthreadRwlock {
    std::shared_timed_mutex lock;
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
    int allocated;
    int seqno;
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

constexpr u32 TidTerminated = 1;

struct SleepQueue {
    std::list<Pthread*> sq_blocked;
    std::forward_list<SleepQueue*> sq_freeq;
    std::list<SleepQueue*> sq_hash;
    std::forward_list<SleepQueue*> sq_flink;
    void* sq_wchan;
    int sq_type;
};

struct SchedParam {
    int sched_priority;
};

struct Pthread {
    static constexpr u32 ThrMagic = 0xd09ba115U;

    std::atomic<long> tid;
    std::mutex lock;
    u32 cycle;
    int locklevel;
    int critical_count;
    int sigblock;
    int refcount;
    PthreadEntryFunc start_routine;
    void* arg;
    Core::Thread native_thr;
    PthreadAttr attr;
    bool cancel_enable;
    bool cancel_pending;
    bool cancel_point;
    bool no_cancel;
    bool cancel_async;
    bool cancelling;
    Cpuset sigmask;
    bool unblock_sigcancel;
    bool in_sigsuspend;
    bool force_exit;
    PthreadState state;
    int error;
    Pthread* joiner;
    ThreadFlags flags;
    ThreadListFlags tlflags;
    std::list<PthreadMutex> mutexq;
    std::list<PthreadMutex> pp_mutexq;
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
    std::binary_semaphore wake_sema{0};
    SleepQueue* sleepqueue;
    void* wchan;
    PthreadMutex* mutex_obj;
    bool will_sleep;
    bool has_user_waiters;

    bool InCritical() const noexcept {
        return locklevel > 0 || critical_count > 0;
    }

    bool ShouldCollect() const noexcept {
        return refcount == 0 && state == PthreadState::Dead && True(flags & ThreadFlags::Detached);
    }

    bool ShouldCancel() const noexcept {
        return cancel_pending && cancel_enable && no_cancel == 0;
    }

    void Enqueue(PthreadMutex* mutex) {
        mutex->m_owner = this;
        // mutexq.push_back(*mutex);
    }

    void Dequeue(PthreadMutex* mutex) {
        mutex->m_owner = nullptr;
        // mutexq.erase(decltype(mutexq)::s_iterator_to(*mutex));
    }
};
using PthreadT = Pthread*;

extern thread_local Pthread* g_curthread;

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
