// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <forward_list>
#include <mutex>
#include <shared_mutex>
#include <boost/intrusive/list.hpp>
#include <boost/thread/thread.hpp>

#include "common/enum.h"
#include "core/libraries/kernel/time_management.h"
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

    PthreadMutexType Type() const noexcept {
        return static_cast<PthreadMutexType>(m_flags & PthreadMutexFlags::TypeMask);
    }

    int SelfTryLock();
    int SelfLock(const OrbisKernelTimespec* abstime);

    int TryLock();
    int Lock(const OrbisKernelTimespec* abstime);

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

struct PthreadCond {
    std::condition_variable_any cond;
    u32 has_user_waiters;
    u32 has_kern_waiters;
    u32 flags;
    u32 clock_id;

    int Wait(PthreadMutexT* mutex, const OrbisKernelTimespec* abstime);
};
using PthreadCondT = PthreadCond*;

struct PthreadCondAttr {
    int c_pshared;
    int c_clockid;
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
};
using PthreadAttrT = PthreadAttr*;

static constexpr u32 ThrStackDefault = 2_MB;
static constexpr u32 ThrStackInitial = 2 * ThrStackDefault;
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

struct PthreadKey {
    int allocated;
    int seqno;
    void PS4_SYSV_ABI (*destructor)(const void*);
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

constexpr u32 TidTerminated = 1;

struct Pthread {
    static constexpr u32 ThrMagic = 0xd09ba115U;

    std::atomic<long> tid;
    std::unique_ptr<std::mutex> lock;
    u32 cycle;
    int locklevel;
    int critical_count;
    int sigblock;
    int refcount;
    void PS4_SYSV_ABI* (*start_routine)(void*);
    void* arg;
    PthreadAttr attr;
    bool cancel_enable;
    bool cancel_pending;
    bool cancel_point;
    bool no_cancel;
    bool cancel_async;
    bool cancelling;
    sigset_t sigmask;
    bool unblock_sigcancel;
    bool in_sigsuspend;
    bool force_exit;
    PthreadState state;
    int error;
    Pthread* joiner;
    ThreadFlags flags;
    ThreadListFlags tlflags;
    boost::intrusive::list<PthreadMutex> mutexq;
    boost::intrusive::list<PthreadMutex> pp_mutexq;
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
        mutexq.push_back(*mutex);
    }

    void Dequeue(PthreadMutex* mutex) {
        mutex->m_owner = nullptr;
        mutexq.erase(decltype(mutexq)::s_iterator_to(*mutex));
    }
};
using PthreadT = Pthread*;

extern Pthread* g_curthread;

void RegisterMutex(Core::Loader::SymbolsResolver* sym);
void RegisterCond(Core::Loader::SymbolsResolver* sym);
void RegisterRwlock(Core::Loader::SymbolsResolver* sym);
void RegisterSemaphore(Core::Loader::SymbolsResolver* sym);
void RegisterSpec(Core::Loader::SymbolsResolver* sym);
void RegisterThreadAttr(Core::Loader::SymbolsResolver* sym);
void RegisterThread(Core::Loader::SymbolsResolver* sym);

inline void RegisterThreads(Core::Loader::SymbolsResolver* sym) {
    RegisterMutex(sym);
    RegisterCond(sym);
    RegisterRwlock(sym);
    RegisterSemaphore(sym);
    RegisterSpec(sym);
    RegisterThreadAttr(sym);
    RegisterThread(sym);
}

} // namespace Libraries::Kernel
