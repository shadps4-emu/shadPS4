// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

void PS4_SYSV_ABI __pthread_cleanup_push_imp(PthreadCleanupFunc routine, void* arg,
                                             PthreadCleanup* newbuf) {
    newbuf->routine = routine;
    newbuf->routine_arg = arg;
    newbuf->onheap = 0;
    g_curthread->cleanup.push_front(newbuf);
}

void PS4_SYSV_ABI posix_pthread_cleanup_push(PthreadCleanupFunc routine, void* arg) {
    Pthread* curthread = g_curthread;
    PthreadCleanup* newbuf = new PthreadCleanup{};
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
            delete old;
        }
    }
}

void RegisterPthreadClean(Core::Loader::SymbolsResolver* sym) {
    // Posix
    LIB_FUNCTION("4ZeZWcMsAV0", "libScePosix", 1, "libkernel", posix_pthread_cleanup_push);
    LIB_FUNCTION("RVxb0Ssa5t0", "libScePosix", 1, "libkernel", posix_pthread_cleanup_pop);

    // Posix-Kernel
    LIB_FUNCTION("1xvtUVx1-Sg", "libkernel", 1, "libkernel", __pthread_cleanup_push_imp);
    LIB_FUNCTION("iWsFlYMf3Kw", "libkernel", 1, "libkernel", posix_pthread_cleanup_pop);
}

} // namespace Libraries::Kernel
