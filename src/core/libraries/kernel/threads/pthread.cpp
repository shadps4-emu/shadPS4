// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/thread.h"
#include "core/debug_state.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/kernel/threads/thread_state.h"
#include "core/libraries/libs.h"
#include "core/memory.h"

namespace Libraries::Kernel {

constexpr int PthreadInheritSched = 4;

constexpr int ORBIS_KERNEL_PRIO_FIFO_DEFAULT = 700;
constexpr int ORBIS_KERNEL_PRIO_FIFO_HIGHEST = 256;
constexpr int ORBIS_KERNEL_PRIO_FIFO_LOWEST = 767;

extern PthreadAttr PthreadAttrDefault;

void _thread_cleanupspecific();

using ThreadDtor = void (*)();
static ThreadDtor* ThreadDtors{};

void PS4_SYSV_ABI _sceKernelSetThreadDtors(ThreadDtor* dtor) {
    ThreadDtors = dtor;
}

static void ExitThread() {
    Pthread* curthread = g_curthread;

    /* Check if there is thread specific data: */
    if (curthread->specific != nullptr) {
        /* Run the thread-specific data destructors: */
        _thread_cleanupspecific();
    }

    auto* thread_state = ThrState::Instance();
    ASSERT(thread_state->active_threads.fetch_sub(1) != 1);

    curthread->lock.lock();
    curthread->state = PthreadState::Dead;
    ASSERT(False(curthread->flags & ThreadFlags::NeedSuspend));

    /*
     * Thread was created with initial refcount 1, we drop the
     * reference count to allow it to be garbage collected.
     */
    curthread->refcount--;
    thread_state->TryCollect(curthread); /* thread lock released */

    /*
     * Kernel will do wakeup at the address, so joiner thread
     * will be resumed if it is sleeping at the address.
     */
    curthread->tid.store(TidTerminated);
    curthread->tid.notify_all();

    curthread->native_thr.Exit();
    UNREACHABLE();
    /* Never reach! */
}

void PS4_SYSV_ABI posix_pthread_exit(void* status) {
    Pthread* curthread = g_curthread;

    /* Check if this thread is already in the process of exiting: */
    ASSERT_MSG(!curthread->cancelling, "Thread {} has called pthread_exit from a destructor",
               fmt::ptr(curthread));

    /* Flag this thread as exiting. */
    curthread->cancelling = 1;
    curthread->no_cancel = 1;
    curthread->cancel_async = 0;
    curthread->cancel_point = 0;

    /* Save the return value: */
    curthread->ret = status;
    while (!curthread->cleanup.empty()) {
        PthreadCleanup* old = curthread->cleanup.front();
        curthread->cleanup.pop_front();
        old->routine(old->routine_arg);
        if (old->onheap) {
            delete old;
        }
    }
    /*if (ThreadDtors && *ThreadDtors) {
        (*ThreadDtors)();
    }*/
    ExitThread();
}

static int JoinThread(PthreadT pthread, void** thread_return, const OrbisKernelTimespec* abstime) {
    Pthread* curthread = g_curthread;

    if (pthread == nullptr) {
        return POSIX_EINVAL;
    }

    if (pthread == curthread) {
        return POSIX_EDEADLK;
    }

    auto* thread_state = ThrState::Instance();
    if (int ret = thread_state->FindThread(pthread, 1); ret != 0) {
        return POSIX_ESRCH;
    }

    int ret = 0;
    if (True(pthread->flags & ThreadFlags::Detached)) {
        ret = POSIX_EINVAL;
    } else if (pthread->joiner != nullptr) {
        /* Multiple joiners are not supported. */
        ret = POSIX_ENOTSUP;
    }
    if (ret) {
        pthread->lock.unlock();
        return ret;
    }
    /* Set the running thread to be the joiner: */
    pthread->joiner = curthread;
    pthread->lock.unlock();

    const auto backout_join = [](void* arg) PS4_SYSV_ABI {
        Pthread* pthread = (Pthread*)arg;
        std::scoped_lock lk{pthread->lock};
        pthread->joiner = nullptr;
    };

    PthreadCleanup cup{backout_join, pthread, 0};
    curthread->cleanup.push_front(&cup);

    //_thr_cancel_enter(curthread);

    const int tid = pthread->tid;
    while (pthread->tid.load() != TidTerminated) {
        //_thr_testcancel(curthread);
        ASSERT(abstime == nullptr);
        pthread->tid.wait(tid);
    }

    //_thr_cancel_leave(curthread, 0);
    curthread->cleanup.pop_front();

    if (ret == POSIX_ETIMEDOUT) {
        backout_join(pthread);
        return ret;
    }

    void* tmp = pthread->ret;
    pthread->lock.lock();
    pthread->flags |= ThreadFlags::Detached;
    pthread->joiner = nullptr;
    thread_state->TryCollect(pthread); /* thread lock released */
    if (thread_return != nullptr) {
        *thread_return = tmp;
    }

    return 0;
}

int PS4_SYSV_ABI posix_pthread_join(PthreadT pthread, void** thread_return) {
    return JoinThread(pthread, thread_return, NULL);
}

int PS4_SYSV_ABI posix_pthread_timedjoin_np(PthreadT pthread, void** thread_return,
                                            const OrbisKernelTimespec* abstime) {
    if (abstime == nullptr || abstime->tv_sec < 0 || abstime->tv_nsec < 0 ||
        abstime->tv_nsec >= 1000000000) {
        return POSIX_EINVAL;
    }

    return JoinThread(pthread, thread_return, abstime);
}

int PS4_SYSV_ABI posix_pthread_detach(PthreadT pthread) {
    if (pthread == nullptr) {
        return POSIX_EINVAL;
    }

    auto* thread_state = ThrState::Instance();
    if (int ret = thread_state->FindThread(pthread, 1); ret != 0) {
        return ret;
    }

    /* Check if the thread is already detached or has a joiner. */
    if (True(pthread->flags & ThreadFlags::Detached) || (pthread->joiner != NULL)) {
        pthread->lock.unlock();
        return POSIX_EINVAL;
    }

    /* Flag the thread as detached. */
    pthread->flags |= ThreadFlags::Detached;
    thread_state->TryCollect(pthread); /* thread lock released */
    return 0;
}

static void RunThread(void* arg) {
    Pthread* curthread = (Pthread*)arg;
    g_curthread = curthread;
    Common::SetCurrentThreadName(curthread->name.c_str());
    DebugState.AddCurrentThreadToGuestList();

    /* Run the current thread's start routine with argument: */
    curthread->native_thr.Initialize();
    void* ret = Core::ExecuteGuest(curthread->start_routine, curthread->arg);

    /* Remove thread from tracking */
    DebugState.RemoveCurrentThreadFromGuestList();
    posix_pthread_exit(ret);
}

int PS4_SYSV_ABI posix_pthread_create_name_np(PthreadT* thread, const PthreadAttrT* attr,
                                              PthreadEntryFunc start_routine, void* arg,
                                              const char* name) {
    Pthread* curthread = g_curthread;
    auto* thread_state = ThrState::Instance();
    Pthread* new_thread = thread_state->Alloc(curthread);
    if (new_thread == nullptr) {
        return POSIX_EAGAIN;
    }

    if (attr == nullptr || *attr == nullptr) {
        new_thread->attr = PthreadAttrDefault;
    } else {
        new_thread->attr = *(*attr);
        new_thread->attr.cpusetsize = 0;
    }
    if (new_thread->attr.sched_inherit == PthreadInheritSched) {
        if (True(curthread->attr.flags & PthreadAttrFlags::ScopeSystem)) {
            new_thread->attr.flags |= PthreadAttrFlags::ScopeSystem;
        } else {
            new_thread->attr.flags &= ~PthreadAttrFlags::ScopeSystem;
        }
        new_thread->attr.prio = curthread->attr.prio;
        new_thread->attr.sched_policy = curthread->attr.sched_policy;
    }

    static int TidCounter = 1;
    new_thread->tid = ++TidCounter;

    if (new_thread->attr.stackaddr_attr == 0) {
        /* Add additional stack space for HLE */
        static constexpr size_t AdditionalStack = 128_KB;
        new_thread->attr.stacksize_attr += AdditionalStack;
    }

    if (thread_state->CreateStack(&new_thread->attr) != 0) {
        /* Insufficient memory to create a stack: */
        thread_state->Free(curthread, new_thread);
        return POSIX_EAGAIN;
    }

    /*
     * Write a magic value to the thread structure
     * to help identify valid ones:
     */
    new_thread->magic = Pthread::ThrMagic;
    new_thread->start_routine = start_routine;
    new_thread->arg = arg;
    new_thread->cancel_enable = 1;
    new_thread->cancel_async = 0;

    auto* memory = Core::Memory::Instance();
    if (name && memory->IsValidAddress(name)) {
        new_thread->name = name;
    } else {
        new_thread->name = fmt::format("Thread{}", new_thread->tid.load());
    }

    ASSERT(new_thread->attr.suspend == 0);
    new_thread->state = PthreadState::Running;

    if (True(new_thread->attr.flags & PthreadAttrFlags::Detached)) {
        new_thread->flags |= ThreadFlags::Detached;
    }

    /* Add the new thread. */
    new_thread->refcount = 1;
    thread_state->Link(curthread, new_thread);

    /* Return thread pointer eariler so that new thread can use it. */
    (*thread) = new_thread;

    /* Create thread */
    new_thread->native_thr = Core::NativeThread();
    int ret = new_thread->native_thr.Create(RunThread, new_thread, &new_thread->attr);

    ASSERT_MSG(ret == 0, "Failed to create thread with error {}", ret);

    if (attr != nullptr && *attr != nullptr && (*attr)->cpuset != nullptr) {
        new_thread->SetAffinity((*attr)->cpuset);
    }
    if (ret) {
        *thread = nullptr;
    }
    return ret;
}

int PS4_SYSV_ABI posix_pthread_create(PthreadT* thread, const PthreadAttrT* attr,
                                      PthreadEntryFunc start_routine, void* arg) {
    return posix_pthread_create_name_np(thread, attr, start_routine, arg, nullptr);
}

int PS4_SYSV_ABI posix_pthread_getthreadid_np() {
    return g_curthread->tid;
}

int PS4_SYSV_ABI posix_pthread_getname_np(PthreadT thread, char* name) {
    std::memcpy(name, thread->name.data(), std::min<size_t>(thread->name.size(), 32));
    return 0;
}

int PS4_SYSV_ABI posix_pthread_equal(PthreadT thread1, PthreadT thread2) {
    return (thread1 == thread2 ? 1 : 0);
}

PthreadT PS4_SYSV_ABI posix_pthread_self() {
    return g_curthread;
}

void PS4_SYSV_ABI posix_pthread_set_name_np(PthreadT thread, const char* name) {
    Common::SetCurrentThreadName(name);
}

void PS4_SYSV_ABI posix_pthread_yield() {
    std::this_thread::yield();
}

void PS4_SYSV_ABI sched_yield() {
    std::this_thread::yield();
}

int PS4_SYSV_ABI posix_getpid() {
    return g_curthread->tid;
}

int PS4_SYSV_ABI posix_pthread_once(PthreadOnce* once_control,
                                    void PS4_SYSV_ABI (*init_routine)()) {
    for (;;) {
        auto state = once_control->state.load();
        if (state == PthreadOnceState::Done) {
            return 0;
        }
        if (state == PthreadOnceState::NeverDone) {
            if (once_control->state.compare_exchange_strong(state, PthreadOnceState::InProgress,
                                                            std::memory_order_acquire)) {
                break;
            }
        } else if (state == PthreadOnceState::InProgress) {
            if (once_control->state.compare_exchange_strong(state, PthreadOnceState::Wait,
                                                            std::memory_order_acquire)) {
                once_control->state.wait(PthreadOnceState::Wait);
            }
        } else if (state == PthreadOnceState::Wait) {
            once_control->state.wait(state);
        } else {
            return POSIX_EINVAL;
        }
    }

    const auto once_cancel_handler = [](void* arg) PS4_SYSV_ABI {
        PthreadOnce* once_control = (PthreadOnce*)arg;
        auto state = PthreadOnceState::InProgress;
        if (once_control->state.compare_exchange_strong(state, PthreadOnceState::NeverDone,
                                                        std::memory_order_release)) {
            return;
        }

        once_control->state.store(PthreadOnceState::NeverDone, std::memory_order_release);
        once_control->state.notify_all();
    };

    PthreadCleanup cup{once_cancel_handler, once_control, 0};
    g_curthread->cleanup.push_front(&cup);
    init_routine();
    g_curthread->cleanup.pop_front();

    auto state = PthreadOnceState::InProgress;
    if (once_control->state.compare_exchange_strong(state, PthreadOnceState::Done,
                                                    std::memory_order_release)) {
        return 0;
    }
    once_control->state.store(PthreadOnceState::Done);
    once_control->state.notify_all();
    return 0;
}

int PS4_SYSV_ABI posix_sched_get_priority_max() {
    return ORBIS_KERNEL_PRIO_FIFO_HIGHEST;
}

int PS4_SYSV_ABI posix_sched_get_priority_min() {
    return ORBIS_KERNEL_PRIO_FIFO_LOWEST;
}

int PS4_SYSV_ABI posix_pthread_rename_np(PthreadT thread, const char* name) {
    if (thread == nullptr) {
        return POSIX_EINVAL;
    }
    if (name == nullptr) {
        return 0;
    }
    LOG_INFO(Kernel_Pthread, "name = {}", name);
    Common::SetThreadName(reinterpret_cast<void*>(thread->native_thr.GetHandle()), name);
    thread->name = name;
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_pthread_getschedparam(PthreadT pthread, SchedPolicy* policy,
                                             SchedParam* param) {
    if (policy == nullptr || param == nullptr) {
        return POSIX_EINVAL;
    }

    if (pthread == g_curthread) {
        /*
         * Avoid searching the thread list when it is the current
         * thread.
         */
        std::scoped_lock lk{g_curthread->lock};
        *policy = g_curthread->attr.sched_policy;
        param->sched_priority = g_curthread->attr.prio;
        return 0;
    }
    auto* thread_state = ThrState::Instance();
    /* Find the thread in the list of active threads. */
    if (int ret = thread_state->RefAdd(pthread, /*include dead*/ 0); ret != 0) {
        return ret;
    }
    pthread->lock.lock();
    *policy = pthread->attr.sched_policy;
    param->sched_priority = pthread->attr.prio;
    pthread->lock.unlock();
    thread_state->RefDelete(pthread);
    return 0;
}

int PS4_SYSV_ABI posix_pthread_setschedparam(PthreadT pthread, SchedPolicy policy,
                                             const SchedParam* param) {
    if (pthread == nullptr || param == nullptr) {
        return POSIX_EINVAL;
    }

    auto* thread_state = ThrState::Instance();
    if (pthread == g_curthread) {
        g_curthread->lock.lock();
    } else if (int ret = thread_state->FindThread(pthread, /*include dead*/ 0); ret != 0) {
        return ret;
    }

    if (pthread->attr.sched_policy == policy &&
        (policy == SchedPolicy::Other || pthread->attr.prio == param->sched_priority)) {
        pthread->attr.prio = param->sched_priority;
        pthread->lock.unlock();
        return 0;
    }

    // TODO: _thr_setscheduler
    pthread->attr.sched_policy = policy;
    pthread->attr.prio = param->sched_priority;
    pthread->lock.unlock();
    return 0;
}

int PS4_SYSV_ABI scePthreadGetprio(PthreadT thread, int* priority) {
    SchedParam param;
    SchedPolicy policy;

    posix_pthread_getschedparam(thread, &policy, &param);
    *priority = param.sched_priority;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_setprio(PthreadT thread, int prio) {
    SchedParam param;

    param.sched_priority = prio;

    auto* thread_state = ThrState::Instance();
    if (thread == g_curthread) {
        g_curthread->lock.lock();
    } else if (int ret = thread_state->FindThread(thread, /*include dead*/ 0); ret != 0) {
        return ret;
    }

    if (thread->attr.sched_policy == SchedPolicy::Other || thread->attr.prio == prio) {
        thread->attr.prio = prio;
    } else {
        // TODO: _thr_setscheduler
        thread->attr.prio = prio;
    }

    thread->lock.unlock();
    return 0;
}

enum class PthreadCancelState : u32 {
    Enable = 0,
    Disable = 1,
};

#define POSIX_PTHREAD_CANCELED ((void*)1)

static inline void TestCancel(Pthread* curthread) {
    if (curthread->ShouldCancel() && !curthread->InCritical()) [[unlikely]] {
        posix_pthread_exit(POSIX_PTHREAD_CANCELED);
    }
}

int PS4_SYSV_ABI posix_pthread_setcancelstate(PthreadCancelState state,
                                              PthreadCancelState* oldstate) {
    Pthread* curthread = g_curthread;
    int oldval = curthread->cancel_enable;
    switch (state) {
    case PthreadCancelState::Disable:
        curthread->cancel_enable = 0;
        break;
    case PthreadCancelState::Enable:
        curthread->cancel_enable = 1;
        TestCancel(curthread);
        break;
    default:
        return POSIX_EINVAL;
    }

    if (oldstate) {
        *oldstate = oldval ? PthreadCancelState::Enable : PthreadCancelState::Disable;
    }
    return 0;
}

int Pthread::SetAffinity(const Cpuset* cpuset) {
    const auto processor_count = std::thread::hardware_concurrency();
    if (processor_count < 8) {
        return 0;
    }
    if (cpuset == nullptr) {
        return POSIX_EINVAL;
    }

    uintptr_t handle = native_thr.GetHandle();
    if (handle == 0) {
        return POSIX_ESRCH;
    }

    // We don't use this currently because some games gets performance problems
    // when applying affinity even on strong hardware
    /*
    u64 mask = cpuset->bits;
    #ifdef _WIN64
        DWORD_PTR affinity_mask = static_cast<DWORD_PTR>(mask);
        if (!SetThreadAffinityMask(reinterpret_cast<HANDLE>(handle), affinity_mask)) {
            return POSIX_EINVAL;
        }

    #elif defined(__linux__)
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);

        u64 mask = cpuset->bits;
        for (int cpu = 0; cpu < std::min(64, CPU_SETSIZE); ++cpu) {
            if (mask & (1ULL << cpu)) {
                CPU_SET(cpu, &cpu_set);
            }
        }

        int result =
            pthread_setaffinity_np(static_cast<pthread_t>(handle), sizeof(cpu_set_t), &cpu_set);
        if (result != 0) {
            return POSIX_EINVAL;
        }
    #endif
    */
    return 0;
}

int PS4_SYSV_ABI posix_pthread_getaffinity_np(PthreadT thread, size_t cpusetsize, Cpuset* cpusetp) {
    if (thread == nullptr || cpusetp == nullptr) {
        return POSIX_EINVAL;
    }

    auto* thread_state = ThrState::Instance();
    if (thread == g_curthread) {
        g_curthread->lock.lock();
    } else if (auto ret = thread_state->FindThread(thread, /*include dead*/ 0); ret != 0) {
        return ret;
    }

    auto* attr_ptr = &thread->attr;
    auto ret = posix_pthread_attr_getaffinity_np(&attr_ptr, cpusetsize, cpusetp);

    thread->lock.unlock();
    return ret;
}

int PS4_SYSV_ABI posix_pthread_setaffinity_np(PthreadT thread, size_t cpusetsize,
                                              const Cpuset* cpusetp) {
    if (thread == nullptr || cpusetp == nullptr) {
        return POSIX_EINVAL;
    }

    auto* thread_state = ThrState::Instance();
    if (thread == g_curthread) {
        g_curthread->lock.lock();
    } else if (auto ret = thread_state->FindThread(thread, /*include dead*/ 0); ret != 0) {
        return ret;
    }

    auto* attr_ptr = &thread->attr;
    auto ret = posix_pthread_attr_setaffinity_np(&attr_ptr, cpusetsize, cpusetp);

    if (ret == ORBIS_OK) {
        ret = thread->SetAffinity(thread->attr.cpuset);
    }

    thread->lock.unlock();
    return ret;
}

int PS4_SYSV_ABI scePthreadGetaffinity(PthreadT thread, u64* mask) {
    Cpuset cpuset;
    const int ret = posix_pthread_getaffinity_np(thread, sizeof(Cpuset), &cpuset);
    if (ret == 0) {
        *mask = cpuset.bits;
    }
    return ret;
}

int PS4_SYSV_ABI scePthreadSetaffinity(PthreadT thread, const u64 mask) {
    const Cpuset cpuset = {.bits = mask};
    return posix_pthread_setaffinity_np(thread, sizeof(Cpuset), &cpuset);
}

void RegisterThread(Core::Loader::SymbolsResolver* sym) {
    // Posix
    LIB_FUNCTION("Z4QosVuAsA0", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_once);
    LIB_FUNCTION("7Xl257M4VNI", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_equal);
    LIB_FUNCTION("CBNtXOoef-E", "libScePosix", 1, "libkernel", 1, 1, posix_sched_get_priority_max);
    LIB_FUNCTION("m0iS6jNsXds", "libScePosix", 1, "libkernel", 1, 1, posix_sched_get_priority_min);
    LIB_FUNCTION("EotR8a3ASf4", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_self);
    LIB_FUNCTION("B5GmVDKwpn0", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_yield);
    LIB_FUNCTION("+U1R4WtXvoc", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_detach);
    LIB_FUNCTION("FJrT5LuUBAU", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_exit);
    LIB_FUNCTION("h9CcP3J0oVM", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_join);
    LIB_FUNCTION("OxhIB8LB-PQ", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_create);
    LIB_FUNCTION("Jmi+9w9u0E4", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_create_name_np);
    LIB_FUNCTION("lZzFeSxPl08", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_setcancelstate);
    LIB_FUNCTION("a2P9wYGeZvc", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_setprio);
    LIB_FUNCTION("9vyP6Z7bqzc", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_rename_np);
    LIB_FUNCTION("FIs3-UQT9sg", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_getschedparam);
    LIB_FUNCTION("Xs9hdiD7sAA", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_setschedparam);
    LIB_FUNCTION("6XG4B33N09g", "libScePosix", 1, "libkernel", 1, 1, sched_yield);
    LIB_FUNCTION("HoLVWNanBBc", "libScePosix", 1, "libkernel", 1, 1, posix_getpid);

    // Posix-Kernel
    LIB_FUNCTION("Z4QosVuAsA0", "libkernel", 1, "libkernel", 1, 1, posix_pthread_once);
    LIB_FUNCTION("EotR8a3ASf4", "libkernel", 1, "libkernel", 1, 1, posix_pthread_self);
    LIB_FUNCTION("OxhIB8LB-PQ", "libkernel", 1, "libkernel", 1, 1, posix_pthread_create);
    LIB_FUNCTION("Jb2uGFMr688", "libkernel", 1, "libkernel", 1, 1, posix_pthread_getaffinity_np);
    LIB_FUNCTION("5KWrg7-ZqvE", "libkernel", 1, "libkernel", 1, 1, posix_pthread_setaffinity_np);

    // Orbis
    LIB_FUNCTION("14bOACANTBo", "libkernel", 1, "libkernel", 1, 1, ORBIS(posix_pthread_once));
    LIB_FUNCTION("GBUY7ywdULE", "libkernel", 1, "libkernel", 1, 1, ORBIS(posix_pthread_rename_np));
    LIB_FUNCTION("6UgtwV+0zb4", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_create_name_np));
    LIB_FUNCTION("4qGrR6eoP9Y", "libkernel", 1, "libkernel", 1, 1, ORBIS(posix_pthread_detach));
    LIB_FUNCTION("onNY9Byn-W8", "libkernel", 1, "libkernel", 1, 1, ORBIS(posix_pthread_join));
    LIB_FUNCTION("P41kTWUS3EI", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_getschedparam));
    LIB_FUNCTION("oIRFTjoILbg", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_setschedparam));
    LIB_FUNCTION("How7B8Oet6k", "libkernel", 1, "libkernel", 1, 1, ORBIS(posix_pthread_getname_np));
    LIB_FUNCTION("3kg7rT0NQIs", "libkernel", 1, "libkernel", 1, 1, posix_pthread_exit);
    LIB_FUNCTION("aI+OeCz8xrQ", "libkernel", 1, "libkernel", 1, 1, posix_pthread_self);
    LIB_FUNCTION("oxMp8uPqa+U", "libkernel", 1, "libkernel", 1, 1, posix_pthread_set_name_np);
    LIB_FUNCTION("3PtV6p3QNX4", "libkernel", 1, "libkernel", 1, 1, posix_pthread_equal);
    LIB_FUNCTION("T72hz6ffq08", "libkernel", 1, "libkernel", 1, 1, posix_pthread_yield);
    LIB_FUNCTION("EI-5-jlq2dE", "libkernel", 1, "libkernel", 1, 1, posix_pthread_getthreadid_np);
    LIB_FUNCTION("1tKyG7RlMJo", "libkernel", 1, "libkernel", 1, 1, scePthreadGetprio);
    LIB_FUNCTION("W0Hpm2X0uPE", "libkernel", 1, "libkernel", 1, 1, ORBIS(posix_pthread_setprio));
    LIB_FUNCTION("rNhWz+lvOMU", "libkernel", 1, "libkernel", 1, 1, _sceKernelSetThreadDtors);
    LIB_FUNCTION("6XG4B33N09g", "libkernel", 1, "libkernel", 1, 1, sched_yield);
    LIB_FUNCTION("HoLVWNanBBc", "libkernel", 1, "libkernel", 1, 1, posix_getpid);
    LIB_FUNCTION("rcrVFJsQWRY", "libkernel", 1, "libkernel", 1, 1, ORBIS(scePthreadGetaffinity));
    LIB_FUNCTION("bt3CTBKmGyI", "libkernel", 1, "libkernel", 1, 1, ORBIS(scePthreadSetaffinity));
}

} // namespace Libraries::Kernel
