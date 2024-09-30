// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/threads/threads.h"
#include "core/libraries/kernel/threads/thread_state.h"
#include "core/libraries/kernel/libkernel.h"
#include "core/memory.h"

#include <pthread.h>

namespace Libraries::Kernel {

extern PthreadAttr PthreadAttrDefault;

using PthreadEntryFunc = void *(*)(void *);

void PS4_SYSV_ABI posix_pthread_exit(void *status);

static void RunThread(Pthread* curthread) {
    g_curthread = curthread;

    /* Run the current thread's start routine with argument: */
    posix_pthread_exit(curthread->start_routine(curthread->arg));
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
    pthread_attr_setstack(&pattr, new_thread->attr.stackaddr_attr,
                                  new_thread->attr.stacksize_attr);
    pthread_t pthr;
    int ret = pthread_create(&pthr, &pattr, (PthreadEntryFunc)RunThread, new_thread);
    if (ret) {
        *thread = nullptr;
    }
    return ret;
}

void RegisterThread(Core::Loader::SymbolsResolver* sym) {
}

} // namespace Libraries::Kernel
