// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// The kernel calls libSceNet forwards to (threads, sleep, process time), on host threads.

#include <chrono>
#include <cstdlib>
#include <thread>

#include "core/libraries/kernel/threads.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/kernel/time.h"

namespace Libraries::Kernel {

u64 PS4_SYSV_ABI sceKernelGetProcessTime() {
    static const auto start = std::chrono::steady_clock::now();
    return static_cast<u64>(std::chrono::duration_cast<std::chrono::microseconds>(
                                std::chrono::steady_clock::now() - start)
                                .count());
}

s32 PS4_SYSV_ABI sceKernelUsleep(u32 microseconds) {
    std::this_thread::sleep_for(std::chrono::microseconds{microseconds});
    return 0;
}

int PS4_SYSV_ABI posix_pthread_create_name_np(PthreadT* thread, const PthreadAttrT*,
                                              PthreadEntryFunc start_routine, void* arg,
                                              const char*) {
    auto* t = new std::thread([start_routine, arg] { start_routine(arg); });
    *thread = reinterpret_cast<PthreadT>(t);
    return 0;
}

int PS4_SYSV_ABI posix_pthread_join(PthreadT pthread, void** thread_return) {
    auto* t = reinterpret_cast<std::thread*>(pthread);
    t->join();
    delete t;
    if (thread_return != nullptr) {
        *thread_return = nullptr;
    }
    return 0;
}

void PS4_SYSV_ABI posix_pthread_exit(void*) {
    std::abort(); // no test exits a thread this way
}

} // namespace Libraries::Kernel
