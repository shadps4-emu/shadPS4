// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <mutex>
#include <semaphore>
#include <string>
#include <vector>
#include <pthread.h>
#include <sched.h>

#include "common/types.h"

#define ORBIS_PTHREAD_MUTEX_ADAPTIVE_INITIALIZER (reinterpret_cast<ScePthreadMutex>(1))

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {
constexpr int ORBIS_KERNEL_PRIO_FIFO_DEFAULT = 700;
constexpr int ORBIS_KERNEL_PRIO_FIFO_HIGHEST = 256;
constexpr int ORBIS_KERNEL_PRIO_FIFO_LOWEST = 767;
constexpr int ORBIS_KERNEL_SEM_VALUE_MAX = 0x7FFFFFFF;

constexpr int ORBIS_PTHREAD_MUTEX_ERRORCHECK = 1;
constexpr int ORBIS_PTHREAD_MUTEX_RECURSIVE = 2;
constexpr int ORBIS_PTHREAD_MUTEX_NORMAL = 3;
constexpr int ORBIS_PTHREAD_MUTEX_ADAPTIVE = 4;

struct PthreadInternal;
struct PthreadAttrInternal;
struct PthreadMutexInternal;
struct PthreadMutexattrInternal;
struct PthreadCondInternal;
struct PthreadCondAttrInternal;
struct PthreadRwInternal;
struct PthreadRwLockAttrInternal;
class PthreadKeys;

using SceKernelSchedParam = ::sched_param;
using ScePthread = PthreadInternal*;
using ScePthreadAttr = PthreadAttrInternal*;
using ScePthreadMutex = PthreadMutexInternal*;
using ScePthreadMutexattr = PthreadMutexattrInternal*;
using ScePthreadCond = PthreadCondInternal*;
using ScePthreadCondattr = PthreadCondAttrInternal*;
using OrbisPthreadRwlock = PthreadRwInternal*;
using OrbisPthreadRwlockattr = PthreadRwLockAttrInternal*;
using OrbisPthreadKey = u32;

using PthreadKeyDestructor = PS4_SYSV_ABI void (*)(void*);
using PthreadEntryFunc = PS4_SYSV_ABI void* (*)(void*);

struct PthreadInternal {
    u8 reserved[4096];
    std::string name;
    pthread_t pth;
    ScePthreadAttr attr;
    PthreadEntryFunc entry;
    void* arg;
    std::atomic_bool is_started;
    std::atomic_bool is_detached;
    std::atomic_bool is_almost_done;
    std::atomic_bool is_free;
    using Destructor = std::pair<OrbisPthreadKey, PthreadKeyDestructor>;
    std::vector<Destructor> key_destructors;
    int prio;
};

struct PthreadAttrInternal {
    u8 reserved[64];
    u64 affinity;
    size_t guard_size;
    int policy;
    bool detached;
    pthread_attr_t pth_attr;
};

struct PthreadMutexInternal {
    u8 reserved[256];
    std::string name;
    pthread_mutex_t pth_mutex;
};

struct PthreadMutexattrInternal {
    u8 reserved[64];
    pthread_mutexattr_t pth_mutex_attr;
    int pprotocol;
};

struct PthreadCondInternal {
    u8 reserved[256];
    std::string name;
    pthread_cond_t cond;
};

struct PthreadCondAttrInternal {
    u8 reserved[64];
    pthread_condattr_t cond_attr;
    clockid_t clock;
};

struct PthreadRwLockAttrInternal {
    u8 reserved[64];
    pthread_rwlockattr_t attr_rwlock;
    int type;
};

struct PthreadRwInternal {
    pthread_rwlock_t pth_rwlock;
    std::string name;
};

struct PthreadSemInternal {
    std::counting_semaphore<ORBIS_KERNEL_SEM_VALUE_MAX> semaphore;
    std::atomic<s32> value;
};

class PThreadPool {
public:
    ScePthread Create(const char* name);

private:
    std::vector<ScePthread> m_threads;
    std::mutex m_mutex;
};

class PThreadCxt {
public:
    ScePthreadMutexattr* getDefaultMutexattr() {
        return &m_default_mutexattr;
    }
    void setDefaultMutexattr(ScePthreadMutexattr attr) {
        m_default_mutexattr = attr;
    }
    ScePthreadMutexattr* getAdaptiveMutexattr() {
        return &m_adaptive_mutexattr;
    }
    void setAdaptiveMutexattr(ScePthreadMutexattr attr) {
        m_adaptive_mutexattr = attr;
    }
    ScePthreadCondattr* getDefaultCondattr() {
        return &m_default_condattr;
    }
    void setDefaultCondattr(ScePthreadCondattr attr) {
        m_default_condattr = attr;
    }
    ScePthreadAttr* GetDefaultAttr() {
        return &m_default_attr;
    }
    void SetDefaultAttr(ScePthreadAttr attr) {
        m_default_attr = attr;
    }
    PThreadPool* GetPthreadPool() {
        return m_pthread_pool;
    }
    void SetPthreadPool(PThreadPool* pool) {
        m_pthread_pool = pool;
    }
    OrbisPthreadRwlockattr* getDefaultRwattr() {
        return &m_default_Rwattr;
    }
    void setDefaultRwattr(OrbisPthreadRwlockattr attr) {
        m_default_Rwattr = attr;
    }

private:
    ScePthreadMutexattr m_default_mutexattr = nullptr;
    ScePthreadMutexattr m_adaptive_mutexattr = nullptr;
    ScePthreadCondattr m_default_condattr = nullptr;
    ScePthreadAttr m_default_attr = nullptr;
    PThreadPool* m_pthread_pool = nullptr;
    OrbisPthreadRwlockattr m_default_Rwattr = nullptr;
};

void init_pthreads();
void pthreadInitSelfMainThread();

int PS4_SYSV_ABI scePthreadAttrInit(ScePthreadAttr* attr);
int PS4_SYSV_ABI scePthreadAttrSetdetachstate(ScePthreadAttr* attr, int detachstate);
int PS4_SYSV_ABI scePthreadAttrSetinheritsched(ScePthreadAttr* attr, int inheritSched);
int PS4_SYSV_ABI scePthreadAttrSetschedparam(ScePthreadAttr* attr,
                                             const SceKernelSchedParam* param);
int PS4_SYSV_ABI scePthreadAttrSetschedpolicy(ScePthreadAttr* attr, int policy);
ScePthread PS4_SYSV_ABI scePthreadSelf();
int PS4_SYSV_ABI scePthreadAttrSetaffinity(ScePthreadAttr* pattr,
                                           const /*SceKernelCpumask*/ u64 mask);
int PS4_SYSV_ABI scePthreadSetaffinity(ScePthread thread, const /*SceKernelCpumask*/ u64 mask);
int PS4_SYSV_ABI scePthreadGetaffinity(ScePthread thread, /*SceKernelCpumask*/ u64* mask);
int PS4_SYSV_ABI scePthreadCreate(ScePthread* thread, const ScePthreadAttr* attr,
                                  PthreadEntryFunc start_routine, void* arg, const char* name);

int PS4_SYSV_ABI scePthreadSetprio(ScePthread thread, int prio);

/***
 * Mutex calls
 */
int PS4_SYSV_ABI scePthreadMutexInit(ScePthreadMutex* mutex, const ScePthreadMutexattr* attr,
                                     const char* name);
int PS4_SYSV_ABI scePthreadMutexattrInit(ScePthreadMutexattr* attr);
int PS4_SYSV_ABI scePthreadMutexattrSettype(ScePthreadMutexattr* attr, int type);
int PS4_SYSV_ABI scePthreadMutexattrSetprotocol(ScePthreadMutexattr* attr, int protocol);
int PS4_SYSV_ABI scePthreadMutexLock(ScePthreadMutex* mutex);
int PS4_SYSV_ABI scePthreadMutexUnlock(ScePthreadMutex* mutex);
/****
 * Cond calls
 */
int PS4_SYSV_ABI scePthreadCondInit(ScePthreadCond* cond, const ScePthreadCondattr* attr,
                                    const char* name);
int PS4_SYSV_ABI scePthreadCondattrInit(ScePthreadCondattr* attr);
int PS4_SYSV_ABI scePthreadCondBroadcast(ScePthreadCond* cond);
int PS4_SYSV_ABI scePthreadCondWait(ScePthreadCond* cond, ScePthreadMutex* mutex);
/****
 * Posix calls
 */
int PS4_SYSV_ABI posix_pthread_mutex_init(ScePthreadMutex* mutex, const ScePthreadMutexattr* attr);
int PS4_SYSV_ABI posix_pthread_mutex_lock(ScePthreadMutex* mutex);
int PS4_SYSV_ABI posix_pthread_mutex_unlock(ScePthreadMutex* mutex);
int PS4_SYSV_ABI posix_pthread_cond_broadcast(ScePthreadCond* cond);

void pthreadSymbolsRegister(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Kernel
