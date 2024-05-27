// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <pthread.h>
#include <sched.h>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

struct PthreadInternal;
struct PthreadAttrInternal;
struct PthreadMutexInternal;
struct PthreadMutexattrInternal;
struct PthreadCondInternal;
struct PthreadCondAttrInternal;
struct PthreadRwInternal;
struct PthreadRwLockAttrInernal;

using SceKernelSchedParam = ::sched_param;
using ScePthread = PthreadInternal*;
using ScePthreadAttr = PthreadAttrInternal*;
using ScePthreadMutex = PthreadMutexInternal*;
using ScePthreadMutexattr = PthreadMutexattrInternal*;
using ScePthreadCond = PthreadCondInternal*;
using ScePthreadCondattr = PthreadCondAttrInternal*;
using ScePthreadRw = PthreadRwInternal*;
using ScePthreadRwAttr = PthreadRwLockAttrInernal*;

using pthreadEntryFunc = PS4_SYSV_ABI void* (*)(void*);

typedef struct timespec SceKernelTimespec;

struct PthreadInternal {
    u8 reserved[4096];
    std::string name;
    pthread_t pth;
    ScePthreadAttr attr;
    pthreadEntryFunc entry;
    void* arg;
    std::atomic_bool is_started;
    std::atomic_bool is_detached;
    std::atomic_bool is_almost_done;
    std::atomic_bool is_free;
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
};

struct PthreadRwLockAttrInernal {
    u64 initialized = 1;
    u8 reserved[64];
    pthread_rwlockattr_t attr_rwlock;
    int type;
};

struct PthreadRwInternal {
    u64 initialized = 1;
    pthread_rwlock_t pth_rwlock;
    std::string name;
};

class PThreadPool {
public:
    ScePthread Create();

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
    ScePthreadRwAttr* getDefaultRwattr() {
        return &m_default_Rwattr;
    }
    void setDefaultRwattr(ScePthreadRwAttr attr) {
        m_default_Rwattr = attr;
    }
    PThreadPool* GetPthreadPool() {
        return m_pthread_pool;
    }
    void SetPthreadPool(PThreadPool* pool) {
        m_pthread_pool = pool;
    }

private:
    ScePthreadMutexattr m_default_mutexattr = nullptr;
    ScePthreadCondattr m_default_condattr = nullptr;
    ScePthreadRwAttr m_default_Rwattr = nullptr;
    ScePthreadAttr m_default_attr = nullptr;
    PThreadPool* m_pthread_pool = nullptr;
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
int PS4_SYSV_ABI scePthreadCreate(ScePthread* thread, const ScePthreadAttr* attr,
                                  pthreadEntryFunc start_routine, void* arg, const char* name);
int PS4_SYSV_ABI scePthreadJoin(ScePthread thread, void** res);
int PS4_SYSV_ABI scePthreadRename(ScePthread thread, const char* name);
int PS4_SYSV_ABI scePthreadKeyCreate(pthread_key_t* key, void (*dest)(void*));
int PS4_SYSV_ABI posix_pthread_create(ScePthread* thread, const ScePthreadAttr* attr,
                                      pthreadEntryFunc start_routine, void* arg);
int PS4_SYSV_ABI posix_pthread_key_create(pthread_key_t* key, void (*dest)(void*));
int PS4_SYSV_ABI posix_pthread_setspecific(pthread_key_t key, void* ptr);

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
int PS4_SYSV_ABI scePthreadCondTimedwait(ScePthreadCond* cond, ScePthreadMutex* mutex, u64 usec);
int PS4_SYSV_ABI scePthreadCondattrDestroy(ScePthreadCondattr* attr);
int PS4_SYSV_ABI scePthreadCondDestroy(ScePthreadCond* cond);
/****
 * rwlock calls
 */
int PS4_SYSV_ABI scePthreadRwlockInit(ScePthreadRw* thread, ScePthreadRwAttr* attr,
                                      const char* name);
int PS4_SYSV_ABI scePthreadRwlockRdlock(ScePthreadRw* thread);
int PS4_SYSV_ABI scePthreadRwlockWrlock(ScePthreadRw* thread);
int PS4_SYSV_ABI scePthreadRwlockUnlock(ScePthreadRw* thread);
int PS4_SYSV_ABI scePthreadRwlockDestroy(ScePthreadRw* thread);
int PS4_SYSV_ABI scePthreadRwlockattrInit(ScePthreadRwAttr* attr);
int PS4_SYSV_ABI posix_pthread_rwlock_rdlock(ScePthreadRw* thread);
int PS4_SYSV_ABI posix_pthread_rwlock_unlock(ScePthreadRw* thread);

/****
 * Posix calls
 */
int PS4_SYSV_ABI posix_pthread_mutex_init(ScePthreadMutex* mutex, const ScePthreadMutexattr* attr);
int PS4_SYSV_ABI posix_pthread_mutex_lock(ScePthreadMutex* mutex);
int PS4_SYSV_ABI posix_pthread_mutex_unlock(ScePthreadMutex* mutex);
int PS4_SYSV_ABI posix_pthread_cond_broadcast(ScePthreadCond* cond);

void pthreadSymbolsRegister(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Kernel
