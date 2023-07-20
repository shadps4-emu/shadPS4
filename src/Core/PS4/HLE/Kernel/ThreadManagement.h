#pragma once
#define _TIMESPEC_DEFINED

#include <pthread.h>
#include <sched.h>
#include "../../../../types.h"
#include <string>

namespace HLE::Libs::LibKernel::ThreadManagement {

struct PthreadAttrInternal;

using SceKernelSchedParam  = ::sched_param;
using ScePthreadAttr = PthreadAttrInternal*;

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

class PThreadCxt {};

void Pthread_Init_Self_MainThread();

    //HLE FUNCTIONS
int scePthreadAttrInit(ScePthreadAttr* attr);
int scePthreadAttrSetdetachstate(ScePthreadAttr* attr, int detachstate);
int scePthreadAttrSetinheritsched(ScePthreadAttr* attr, int inheritSched);
int scePthreadAttrSetschedparam(ScePthreadAttr* attr, const SceKernelSchedParam* param);
int scePthreadAttrSetschedpolicy(ScePthreadAttr* attr, int policy);

}  // namespace HLE::Libs::LibKernel::ThreadManagement