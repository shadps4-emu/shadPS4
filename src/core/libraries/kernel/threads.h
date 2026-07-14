// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include "common/polyfill_thread.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/tls.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

int PS4_SYSV_ABI posix_pthread_attr_init(PthreadAttrT* attr);

int PS4_SYSV_ABI posix_pthread_attr_destroy(PthreadAttrT* attr);

int PS4_SYSV_ABI posix_pthread_attr_getaffinity_np(const PthreadAttrT* pattr, size_t cpusetsize,
                                                   Cpuset* cpusetp);

int PS4_SYSV_ABI posix_pthread_attr_setaffinity_np(PthreadAttrT* pattr, size_t cpusetsize,
                                                   const Cpuset* cpusetp);

int PS4_SYSV_ABI posix_pthread_create(PthreadT* thread, const PthreadAttrT* attr,
                                      PthreadEntryFunc start_routine, void* arg);

int PS4_SYSV_ABI posix_pthread_join(PthreadT pthread, void** thread_return);
int PS4_SYSV_ABI posix_pthread_detach(PthreadT pthread);

int PS4_SYSV_ABI posix_pthread_mutexattr_init(PthreadMutexAttrT* attr);
int PS4_SYSV_ABI posix_pthread_mutexattr_settype(PthreadMutexAttrT* attr, PthreadMutexType type);
int PS4_SYSV_ABI posix_pthread_mutexattr_destroy(PthreadMutexAttrT* attr);

int PS4_SYSV_ABI scePthreadMutexInit(PthreadMutexT* mutex, const PthreadMutexAttrT* mutex_attr,
                                     const char* name);
int PS4_SYSV_ABI posix_pthread_mutex_lock(PthreadMutexT* mutex);
int PS4_SYSV_ABI posix_pthread_mutex_unlock(PthreadMutexT* mutex);
int PS4_SYSV_ABI posix_pthread_mutex_destroy(PthreadMutexT* mutex);
s32 PS4_SYSV_ABI posix_pthread_mutex_trylock(PthreadMutexT* mutex);

int PS4_SYSV_ABI posix_pthread_condattr_init(PthreadCondAttrT* attr);
int PS4_SYSV_ABI posix_pthread_condattr_destroy(PthreadCondAttrT* attr);
int PS4_SYSV_ABI scePthreadCondInit(PthreadCondT* cond, const PthreadCondAttrT* cond_attr,
                                    const char* name);
int PS4_SYSV_ABI posix_pthread_cond_destroy(PthreadCondT* cond);
int PS4_SYSV_ABI posix_pthread_cond_signal(PthreadCondT* cond);
int PS4_SYSV_ABI posix_pthread_cond_wait(PthreadCondT* cond, PthreadMutexT* mutex);
int PS4_SYSV_ABI posix_pthread_cond_reltimedwait_np(PthreadCondT* cond, PthreadMutexT* mutex,
                                                    u64 usec);

int PS4_SYSV_ABI posix_pthread_attr_setstacksize(PthreadAttrT* attr, size_t stacksize);
int PS4_SYSV_ABI posix_pthread_attr_setinheritsched(PthreadAttrT* attr, int sched_inherit);
int PS4_SYSV_ABI posix_pthread_attr_setschedpolicy(PthreadAttrT* attr, SchedPolicy policy);
int PS4_SYSV_ABI posix_pthread_attr_setschedparam(PthreadAttrT* attr, SchedParam* param);
int PS4_SYSV_ABI scePthreadAttrSetaffinity(PthreadAttrT* attr, const u64 mask);
int PS4_SYSV_ABI posix_pthread_create_name_np(PthreadT* thread, const PthreadAttrT* attr,
                                              PthreadEntryFunc start_routine, void* arg,
                                              const char* name);

void RegisterThreads(Core::Loader::SymbolsResolver* sym);

class Thread {
public:
    explicit Thread() = default;
    ~Thread() {
        Stop();
    }

    void Run(std::function<void(std::stop_token)>&& func) {
        this->func = std::move(func);
        PthreadAttrT attr{};
        posix_pthread_attr_init(&attr);
        posix_pthread_create(&thread, &attr, HOST_CALL(RunWrapper), this);
        posix_pthread_attr_destroy(&attr);
    }

    void Join() {
        if (thread) {
            posix_pthread_join(thread, nullptr);
            thread = nullptr;
        }
    }

    bool Joinable() const {
        return thread != nullptr;
    }

    void Stop() {
        if (Joinable()) {
            stop.request_stop();
            Join();
        }
        thread = nullptr;
        func = nullptr;
        stop = std::stop_source{};
    }

    static void* PS4_SYSV_ABI RunWrapper(void* arg) {
        Thread* thr = (Thread*)arg;
        thr->func(thr->stop.get_token());
        return nullptr;
    }

private:
    PthreadT thread{};
    std::function<void(std::stop_token)> func;
    std::stop_source stop;
};

} // namespace Libraries::Kernel
