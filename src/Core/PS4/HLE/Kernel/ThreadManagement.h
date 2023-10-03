#pragma once
#define _TIMESPEC_DEFINED
#include <types.h>
#include <pthread.h>
#include <sched.h>

#include <string>

namespace HLE::Libs::LibKernel::ThreadManagement {

struct PthreadAttrInternal;
struct PthreadMutexInternal;
struct PthreadMutexAttrInternal;

using SceKernelSchedParam = ::sched_param;
using ScePthreadAttr = PthreadAttrInternal*;
using ScePthreadMutex = PthreadMutexInternal*;
using ScePthreadMutexattr = PthreadMutexAttrInternal*;

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


struct PthreadMutexAttrInternal {
    u08 reserved[64];
    pthread_mutexattr_t mutex_attr;
    int attr_protocol;
};

struct PthreadMutexInternal {
    u08 reserved[256];
    std::string name;
    pthread_mutex_t mutex;
};

class PThreadCxt {};

void Pthread_Init_Self_MainThread();

// HLE FUNCTIONS
int PS4_SYSV_ABI scePthreadAttrInit(ScePthreadAttr* attr);
int PS4_SYSV_ABI scePthreadAttrSetdetachstate(ScePthreadAttr* attr, int detachstate);
int PS4_SYSV_ABI scePthreadAttrSetinheritsched(ScePthreadAttr* attr, int inheritSched);
int PS4_SYSV_ABI scePthreadAttrSetschedparam(ScePthreadAttr* attr, const SceKernelSchedParam* param);
int PS4_SYSV_ABI scePthreadAttrSetschedpolicy(ScePthreadAttr* attr, int policy);
int PS4_SYSV_ABI scePthreadMutexLock(ScePthreadMutex* mutex);
int PS4_SYSV_ABI scePthreadMutexattrInit(ScePthreadMutexattr* attr);
int PS4_SYSV_ABI scePthreadMutexattrSettype(ScePthreadMutexattr* attr, int type);
int PS4_SYSV_ABI scePthreadMutexattrSetprotocol(ScePthreadMutexattr* attr, int protocol);

}  // namespace HLE::Libs::LibKernel::ThreadManagement