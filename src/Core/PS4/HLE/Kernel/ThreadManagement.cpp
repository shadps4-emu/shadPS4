#include "ThreadManagement.h"

#include <Core/PS4/HLE/ErrorCodes.h>
#include <debug.h>

namespace HLE::Libs::LibKernel::ThreadManagement {

thread_local PthreadInternal* g_pthread_self = nullptr;
PThreadCxt* g_pthread_cxt = nullptr;

void Pthread_Init_Self_MainThread() {
    g_pthread_self = new PthreadInternal{};
    scePthreadAttrInit(&g_pthread_self->attr);
    g_pthread_self->pth = pthread_self();
    g_pthread_self->name = "Main_Thread";
}

int PS4_SYSV_ABI scePthreadAttrInit(ScePthreadAttr* attr) {
    *attr = new PthreadAttrInternal{};

    int result = pthread_attr_init(&(*attr)->pth_attr);

    (*attr)->affinity = 0x7f;
    (*attr)->guard_size = 0x1000;

    SceKernelSchedParam param{};
    param.sched_priority = 700;

    result = (result == 0 ? scePthreadAttrSetinheritsched(attr, 4) : result);
    result = (result == 0 ? scePthreadAttrSetschedparam(attr, &param) : result);
    result = (result == 0 ? scePthreadAttrSetschedpolicy(attr, SCHED_OTHER) : result);
    result = (result == 0 ? scePthreadAttrSetdetachstate(attr, PTHREAD_CREATE_JOINABLE) : result);

    switch (result) {
        case 0: return SCE_OK;
        case ENOMEM: return SCE_KERNEL_ERROR_ENOMEM;
        default: return SCE_KERNEL_ERROR_EINVAL;
    }
}

int PS4_SYSV_ABI scePthreadAttrSetdetachstate(ScePthreadAttr* attr, int detachstate) {
    if (attr == nullptr || *attr == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    int pstate = PTHREAD_CREATE_JOINABLE;
    switch (detachstate) {
        case 0: pstate = PTHREAD_CREATE_JOINABLE; break;
        case 1: pstate = PTHREAD_CREATE_DETACHED; break;
        default: BREAKPOINT();  // unknown state
    }

    int result = pthread_attr_setdetachstate(&(*attr)->pth_attr, pstate);

    (*attr)->detached = (pstate == PTHREAD_CREATE_DETACHED);

    if (result == 0) {
        return SCE_OK;
    }
    return SCE_KERNEL_ERROR_EINVAL;
}

int PS4_SYSV_ABI scePthreadAttrSetinheritsched(ScePthreadAttr* attr, int inheritSched) {
    if (attr == nullptr || *attr == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    int pinherit_sched = PTHREAD_INHERIT_SCHED;
    switch (inheritSched) {
        case 0: pinherit_sched = PTHREAD_EXPLICIT_SCHED; break;
        case 4: pinherit_sched = PTHREAD_INHERIT_SCHED; break;
        default: BREAKPOINT();  // unknown inheritSched
    }

    int result = pthread_attr_setinheritsched(&(*attr)->pth_attr, pinherit_sched);

    if (result == 0) {
        return SCE_OK;
    }
    return SCE_KERNEL_ERROR_EINVAL;
}

int PS4_SYSV_ABI scePthreadAttrSetschedparam(ScePthreadAttr* attr, const SceKernelSchedParam* param) {
    if (param == nullptr || attr == nullptr || *attr == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    SceKernelSchedParam pparam{};
    if (param->sched_priority <= 478) {
        pparam.sched_priority = +2;
    } else if (param->sched_priority >= 733) {
        pparam.sched_priority = -2;
    } else {
        pparam.sched_priority = 0;
    }

    int result = pthread_attr_setschedparam(&(*attr)->pth_attr, &pparam);

    if (result == 0) {
        return SCE_OK;
    }
    return SCE_KERNEL_ERROR_EINVAL;
}

int PS4_SYSV_ABI scePthreadAttrSetschedpolicy(ScePthreadAttr* attr, int policy) {
    if (attr == nullptr || *attr == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    if (policy != SCHED_OTHER) {
        BREAKPOINT();  // invest if policy is other and if winpthreadlibrary support it
    }

    (*attr)->policy = policy;

    int result = pthread_attr_setschedpolicy(&(*attr)->pth_attr, policy);

    if (result == 0) {
        return SCE_OK;
    }
    return SCE_KERNEL_ERROR_EINVAL;
}

int PS4_SYSV_ABI scePthreadMutexattrInit(ScePthreadMutexattr* attr) {
    *attr = new PthreadMutexAttrInternal{};

    int result = pthread_mutexattr_init(&(*attr)->mutex_attr);

    result = (result == 0 ? scePthreadMutexattrSettype(attr, 1) : result);
    result = (result == 0 ? scePthreadMutexattrSetprotocol(attr, 0) : result);

    switch (result) {
        case 0: return SCE_OK;
        case ENOMEM: return SCE_KERNEL_ERROR_ENOMEM;
        default: return SCE_KERNEL_ERROR_EINVAL;
    }
}

int PS4_SYSV_ABI scePthreadMutexattrSettype(ScePthreadMutexattr* attr, int type) {
    int ptype = PTHREAD_MUTEX_DEFAULT;
    switch (type) {
        case 1: ptype = PTHREAD_MUTEX_ERRORCHECK; break;
        case 2: ptype = PTHREAD_MUTEX_RECURSIVE; break;
        case 3:
        case 4: ptype = PTHREAD_MUTEX_NORMAL; break;
        default: BREAKPOINT();  // invalid
    }

    int result = pthread_mutexattr_settype(&(*attr)->mutex_attr, ptype);
    return (result == 0 ? SCE_OK : SCE_KERNEL_ERROR_EINVAL);
}
int PS4_SYSV_ABI scePthreadMutexattrSetprotocol(ScePthreadMutexattr* attr, int protocol) {
    int pprotocol = PTHREAD_PRIO_NONE;
    switch (protocol) {
        case 0: pprotocol = PTHREAD_PRIO_NONE; break;
        case 1: pprotocol = PTHREAD_PRIO_INHERIT; break;
        case 2: pprotocol = PTHREAD_PRIO_PROTECT; break;
        default: BREAKPOINT();  // invalid
    }

    int result = 0;  // pthread_mutexattr_setprotocol(&(*attr)->p, pprotocol); //TODO setprotocol seems to have issue with winpthreads (to check it)
    (*attr)->attr_protocol = pprotocol;
    return (result == 0 ? SCE_OK : SCE_KERNEL_ERROR_EINVAL);
}
int PS4_SYSV_ABI scePthreadMutexInit(ScePthreadMutex* mutex, const ScePthreadMutexattr* attr, const char* name) {
    if (mutex == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    if (attr == nullptr) {
        ScePthreadMutexattr mutexattr = nullptr;
        scePthreadMutexattrInit(&mutexattr);
        attr = &mutexattr;
    }

    *mutex = new PthreadMutexInternal{};

    (*mutex)->name = name;

    int result = pthread_mutex_init(&(*mutex)->mutex, &(*attr)->mutex_attr);

    if (name != nullptr) {
        printf("mutex init: %s, %d\n", (*mutex)->name.c_str(), result);
    }

    switch (result) {
        case 0: return SCE_OK;
        case EAGAIN: return SCE_KERNEL_ERROR_EAGAIN;
        case EINVAL: return SCE_KERNEL_ERROR_EINVAL;
        case ENOMEM: return SCE_KERNEL_ERROR_ENOMEM;
        default: return SCE_KERNEL_ERROR_EINVAL;
    }
}

int PS4_SYSV_ABI scePthreadMutexLock(ScePthreadMutex* mutex) {
    static int count = 0;
    std::string name = "internal mutex ";
    name += std::to_string(count);
    count++;
    // we assume we need one mutex so init one (we don't even check if it exists TODO)
    scePthreadMutexInit(mutex, nullptr, name.c_str());

    int result = pthread_mutex_lock(&(*mutex)->mutex);
    printf("mutex lock: %s, %d\n", (*mutex)->name.c_str(), result);
    switch (result) {
        case 0: return SCE_OK;
        case EAGAIN: return SCE_KERNEL_ERROR_EAGAIN;
        case EINVAL: return SCE_KERNEL_ERROR_EINVAL;
        case EDEADLK: return SCE_KERNEL_ERROR_EDEADLK;
        default: return SCE_KERNEL_ERROR_EINVAL;
    }
}
int PS4_SYSV_ABI scePthreadMutexUnlock(ScePthreadMutex* mutex) {
    if (mutex == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    int result = pthread_mutex_unlock(&(*mutex)->mutex);
    printf("mutex unlock: %s, %d\n", (*mutex)->name.c_str(), result);
    switch (result) {
        case 0: return SCE_OK;

        case EINVAL: return SCE_KERNEL_ERROR_EINVAL;
        case EPERM: return SCE_KERNEL_ERROR_EPERM;
        default: return SCE_KERNEL_ERROR_EINVAL;
    }
}

int PS4_SYSV_ABI scePthreadCondattrInit(ScePthreadCondattr* attr) {

    *attr = new PthreadCondAttrInternal{};

    int result = pthread_condattr_init(&(*attr)->cond_attr);

    switch (result) {
        case 0: return SCE_OK;
        case ENOMEM: return SCE_KERNEL_ERROR_ENOMEM;
        default: return SCE_KERNEL_ERROR_EINVAL;
    }
}

int PS4_SYSV_ABI scePthreadCondInit(ScePthreadCond* cond, const ScePthreadCondattr* attr, const char* name) {
    if (cond == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    if (attr == nullptr) {
        ScePthreadCondattr condattr = nullptr;
        scePthreadCondattrInit(&condattr);
        attr = &condattr;
    }

    *cond = new PthreadCondInternal{};

    (*cond)->name = name;

    int result = pthread_cond_init(&(*cond)->cond, &(*attr)->cond_attr);

    printf("cond init: %s, %d\n", (*cond)->name.c_str(), result);

    switch (result) {
        case 0: return SCE_OK;
        case EAGAIN: return SCE_KERNEL_ERROR_EAGAIN;
        case EINVAL: return SCE_KERNEL_ERROR_EINVAL;
        case ENOMEM: return SCE_KERNEL_ERROR_ENOMEM;
        default: return SCE_KERNEL_ERROR_EINVAL;
    }
}
int PS4_SYSV_ABI scePthreadCondBroadcast(ScePthreadCond* cond) {

    static int count = 0;
    std::string name = "internal cond ";
    name += std::to_string(count);
    count++;
    // we assume we need one cond so init one (we don't even check if it exists TODO)
    scePthreadCondInit(cond, nullptr, name.c_str());

    if (cond == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    int result = pthread_cond_broadcast(&(*cond)->cond);

    printf("cond broadcast: %s, %d\n", (*cond)->name.c_str(), result);

    return (result == 0 ? SCE_OK : SCE_KERNEL_ERROR_EINVAL);
}

};  // namespace HLE::Libs::LibKernel::ThreadManagement