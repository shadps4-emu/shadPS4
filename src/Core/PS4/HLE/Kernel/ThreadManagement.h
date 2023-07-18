#pragma once
#include <pthread.h>
#include <sched.h>
#include "../../../../types.h"

extern "C" {
struct sched_param;
}

namespace HLE::Libs::LibKernel::ThreadManagement {

struct PthreadAttrInternal;

using SceKernelSchedParam  = struct sched_param;
using ScePthreadAttr = PthreadAttrInternal*;

struct PthreadInternal {
    pthread_t p;
};
struct PthreadAttrInternal {
    u64 affinity;
    size_t guard_size;
    int policy;
    bool detached;
    pthread_attr_t p;
};

class PThreadCxt {};

//HLE FUNCTIONS
int scePthreadAttrInit(ScePthreadAttr* attr);
int scePthreadAttrSetdetachstate(ScePthreadAttr* attr, int detachstate);
int scePthreadAttrSetinheritsched(ScePthreadAttr* attr, int inheritSched);
int scePthreadAttrSetschedparam(ScePthreadAttr* attr, const SceKernelSchedParam* param);
int scePthreadAttrSetschedpolicy(ScePthreadAttr* attr, int policy);

}  // namespace HLE::Libs::LibKernel::ThreadManagement