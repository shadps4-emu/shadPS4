// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/libkernel.h"
#include "core/libraries/kernel/thread_management.h"
#include "core/libraries/kernel/threads/threads.h"

namespace Libraries::Kernel {

int PS4_SYSV_ABI posix_pthread_attr_init(PthreadAttrT* attr);

int PS4_SYSV_ABI posix_pthread_create_name_np(PthreadT* thread, const PthreadAttrT* attr,
                                              PthreadEntryFunc start_routine, void* arg,
                                              const char* name);

PthreadT LaunchThread(PthreadEntryFunc start_routine, void* arg, const char* name) {
    PthreadT thread{};
    PthreadAttrT attr{};
    posix_pthread_attr_init(&attr);
    posix_pthread_create_name_np(&thread, &attr, start_routine, arg, name);
    return thread;
}

void RegisterThreads(Core::Loader::SymbolsResolver* sym) {
    RegisterMutex(sym);
    RegisterCond(sym);
    RegisterRwlock(sym);
    RegisterSemaphore(sym);
    RegisterSpec(sym);
    RegisterThreadAttr(sym);
    RegisterThread(sym);
    RegisterRtld(sym);
}

} // namespace Libraries::Kernel
