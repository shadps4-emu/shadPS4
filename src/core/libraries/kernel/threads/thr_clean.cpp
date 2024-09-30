// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/threads/threads.h"

namespace Libraries::Kernel {

void __pthread_cleanup_push_imp(PthreadCleanupFunc routine, void* arg, PthreadCleanup* newbuf) {
    Pthread* curthread = g_curthread;
    newbuf->routine = routine;
    newbuf->routine_arg = arg;
    newbuf->onheap = 0;
    curthread->cleanup.push_front(newbuf);
}

void PS4_SYSV_ABI posix_pthread_cleanup_push(PthreadCleanupFunc routine, void* arg) {
    Pthread* curthread = g_curthread;
    PthreadCleanup* newbuf = (PthreadCleanup*)malloc(sizeof(PthreadCleanup));
    if (newbuf == nullptr) {
        return;
    }

    newbuf->routine = routine;
    newbuf->routine_arg = arg;
    newbuf->onheap = 1;
    curthread->cleanup.push_front(newbuf);
}

void PS4_SYSV_ABI posix_pthread_cleanup_pop(int execute) {
    Pthread* curthread = g_curthread;
    if (!curthread->cleanup.empty()) {
        PthreadCleanup* old = curthread->cleanup.front();
        curthread->cleanup.pop_front();
        if (execute) {
            old->routine(old->routine_arg);
        }
        if (old->onheap) {
            free(old);
        }
    }
}

} // namespace Libraries::Kernel
