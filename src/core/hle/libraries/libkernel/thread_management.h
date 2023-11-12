#pragma once
#define _TIMESPEC_DEFINED

#include <pthread.h>
#include <sched.h>

#include <string>

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibKernel {

struct PthreadAttrInternal;
struct PthreadMutexInternal;
struct PthreadMutexattrInternal;

using SceKernelSchedParam = ::sched_param;
using ScePthreadAttr = PthreadAttrInternal*;
using ScePthreadMutex = PthreadMutexInternal*;
using ScePthreadMutexattr = PthreadMutexattrInternal*;

struct PthreadInternal {
    u08 reserved[4096];
    std::string name;
    pthread_t pth;
    ScePthreadAttr attr;
};

struct PthreadAttrInternal {
    u08 reserved[64];
    u64 affinity;
    size_t guard_size;
    int policy;
    bool detached;
    pthread_attr_t pth_attr;
};

struct PthreadMutexInternal {
    u08 reserved[256];
    std::string name;
    pthread_mutex_t pth_mutex;
};

struct PthreadMutexattrInternal {
    u08 reserved[64];
    pthread_mutexattr_t pth_mutex_attr;
    int pprotocol;
};

class PThreadCxt {
  public:
    ScePthreadMutexattr* getDefaultMutexattr() { return &m_default_mutexattr; }
    void setDefaultMutexattr(ScePthreadMutexattr attr) { m_default_mutexattr = attr; }

  private:
    ScePthreadMutexattr m_default_mutexattr = nullptr;
};

void init_pthreads();
void pthreadInitSelfMainThread();

int PS4_SYSV_ABI scePthreadAttrInit(ScePthreadAttr* attr);
int PS4_SYSV_ABI scePthreadAttrSetdetachstate(ScePthreadAttr* attr, int detachstate);
int PS4_SYSV_ABI scePthreadAttrSetinheritsched(ScePthreadAttr* attr, int inheritSched);
int PS4_SYSV_ABI scePthreadAttrSetschedparam(ScePthreadAttr* attr, const SceKernelSchedParam* param);
int PS4_SYSV_ABI scePthreadAttrSetschedpolicy(ScePthreadAttr* attr, int policy);

/***
 * Mutex calls
 */
int PS4_SYSV_ABI scePthreadMutexInit(ScePthreadMutex* mutex, const ScePthreadMutexattr* attr, const char* name);
int PS4_SYSV_ABI scePthreadMutexattrInit(ScePthreadMutexattr* attr);
int PS4_SYSV_ABI scePthreadMutexattrSettype(ScePthreadMutexattr* attr, int type);
int PS4_SYSV_ABI scePthreadMutexattrSetprotocol(ScePthreadMutexattr* attr, int protocol);
int PS4_SYSV_ABI scePthreadMutexLock(ScePthreadMutex* mutex);
int PS4_SYSV_ABI scePthreadMutexUnlock(ScePthreadMutex* mutex);
/****
 * Posix calls
 */
int PS4_SYSV_ABI posix_pthread_mutex_init(ScePthreadMutex* mutex, const ScePthreadMutexattr* attr);
int PS4_SYSV_ABI posix_pthread_mutex_lock(ScePthreadMutex* mutex);
int PS4_SYSV_ABI posix_pthread_mutex_unlock(ScePthreadMutex* mutex);

void pthreadSymbolsRegister(Loader::SymbolsResolver* sym);
}  // namespace Core::Libraries::LibKernel
