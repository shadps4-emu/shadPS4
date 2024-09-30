// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/thread.h"
#include "core/debug_state.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/libkernel.h"
#include "core/libraries/kernel/threads/thread_state.h"
#include "core/libraries/kernel/threads/threads.h"
#include "core/libraries/libs.h"
#include "core/linker.h"
#include "core/memory.h"

#include <pthread.h>

namespace Libraries::Kernel {

constexpr int ORBIS_KERNEL_PRIO_FIFO_DEFAULT = 700;
constexpr int ORBIS_KERNEL_PRIO_FIFO_HIGHEST = 256;
constexpr int ORBIS_KERNEL_PRIO_FIFO_LOWEST = 767;

extern PthreadAttr PthreadAttrDefault;

using PthreadEntryFunc = void* (*)(void*);

void PS4_SYSV_ABI posix_pthread_exit(void* status);

static void RunThread(Pthread* curthread) {
    g_curthread = curthread;
    Common::SetCurrentThreadName(curthread->name.c_str());
    DebugState.AddCurrentThreadToGuestList();

    /* Run the current thread's start routine with argument: */
    const auto* linker = Common::Singleton<Core::Linker>::Instance();
    void* ret = linker->ExecuteGuest(curthread->start_routine, curthread->arg);

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
    if (new_thread->attr.sched_inherit == PTHREAD_INHERIT_SCHED) {
        if (True(curthread->attr.flags & PthreadAttrFlags::ScopeSystem)) {
            new_thread->attr.flags |= PthreadAttrFlags::ScopeSystem;
        } else {
            new_thread->attr.flags &= ~PthreadAttrFlags::ScopeSystem;
        }
        new_thread->attr.prio = curthread->attr.prio;
        new_thread->attr.sched_policy = curthread->attr.sched_policy;
    }

    new_thread->tid = TidTerminated;

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
    if (memory->IsValidAddress(name)) {
        new_thread->name = name;
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
    pthread_attr_t pattr;
    pthread_attr_init(&pattr);
    pthread_attr_setstack(&pattr, new_thread->attr.stackaddr_attr, new_thread->attr.stacksize_attr);
    pthread_t pthr;
    int ret = pthread_create(&pthr, &pattr, (PthreadEntryFunc)RunThread, new_thread);
    if (ret) {
        *thread = nullptr;
    }
    return ret;
}

int PS4_SYSV_ABI posix_pthread_getthreadid_np() {
    return g_curthread->tid;
}

int PS4_SYSV_ABI posix_pthread_equal(PthreadT thread1, PthreadT thread2) {
    return (thread1 == thread2 ? 1 : 0);
}

PthreadT PS4_SYSV_ABI posix_pthread_self() {
    return g_curthread;
}

void PS4_SYSV_ABI posix_pthread_yield() {
    std::this_thread::yield();
}

int PS4_SYSV_ABI posix_pthread_once(PthreadOnce* once_control, void (*init_routine)()) {
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

    const auto once_cancel_handler = [](void* arg) {
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
    LOG_INFO(Kernel_Pthread, "name = {}", name);
    thread->name = name;
    return SCE_OK;
}

void RegisterThread(Core::Loader::SymbolsResolver* sym) {
    // Posix
    LIB_FUNCTION("Z4QosVuAsA0", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_once);
    LIB_FUNCTION("7Xl257M4VNI", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_equal);
    LIB_FUNCTION("CBNtXOoef-E", "libScePosix", 1, "libkernel", 1, 1, posix_sched_get_priority_max);
    LIB_FUNCTION("m0iS6jNsXds", "libScePosix", 1, "libkernel", 1, 1, posix_sched_get_priority_min);
    LIB_FUNCTION("EotR8a3ASf4", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_self);
    LIB_FUNCTION("B5GmVDKwpn0", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_yield);

    // Posix-Kernel
    LIB_FUNCTION("EotR8a3ASf4", "libkernel", 1, "libkernel", 1, 1, posix_pthread_self);

    // Orbis
    LIB_FUNCTION("14bOACANTBo", "libkernel", 1, "libkernel", 1, 1, ORBIS(posix_pthread_once));
    LIB_FUNCTION("GBUY7ywdULE", "libkernel", 1, "libkernel", 1, 1, ORBIS(posix_pthread_rename_np));
    LIB_FUNCTION("6UgtwV+0zb4", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_create_name_np));
    LIB_FUNCTION("aI+OeCz8xrQ", "libkernel", 1, "libkernel", 1, 1, posix_pthread_self);
    LIB_FUNCTION("3PtV6p3QNX4", "libkernel", 1, "libkernel", 1, 1, posix_pthread_equal);
    LIB_FUNCTION("T72hz6ffq08", "libkernel", 1, "libkernel", 1, 1, posix_pthread_yield);
    LIB_FUNCTION("EI-5-jlq2dE", "libkernel", 1, "libkernel", 1, 1, posix_pthread_getthreadid_np);
}

} // namespace Libraries::Kernel
