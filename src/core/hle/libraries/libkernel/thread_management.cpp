// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/error_codes.h"
#include "core/hle/libraries/libkernel/thread_management.h"
#include "core/hle/libraries/libs.h"

namespace Core::Libraries::LibKernel {

thread_local ScePthread g_pthread_self{};
PThreadCxt* g_pthread_cxt = nullptr;

void init_pthreads() {
    g_pthread_cxt = new PThreadCxt{};
    // default mutex init
    ScePthreadMutexattr default_mutexattr = nullptr;
    scePthreadMutexattrInit(&default_mutexattr);
    g_pthread_cxt->setDefaultMutexattr(default_mutexattr);
    // default cond init
    ScePthreadCondattr default_condattr = nullptr;
    scePthreadCondattrInit(&default_condattr);
    g_pthread_cxt->setDefaultCondattr(default_condattr);
}

void pthreadInitSelfMainThread() {
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
    case 0:
        return SCE_OK;
    case ENOMEM:
        return SCE_KERNEL_ERROR_ENOMEM;
    default:
        return SCE_KERNEL_ERROR_EINVAL;
    }
}

int PS4_SYSV_ABI scePthreadAttrDestroy(ScePthreadAttr* attr) {

    int result = pthread_attr_destroy(&(*attr)->pth_attr);

    delete *attr;
    *attr = nullptr;

    if (result == 0) {
        return SCE_OK;
    }
    return SCE_KERNEL_ERROR_EINVAL;
}

int PS4_SYSV_ABI scePthreadAttrSetdetachstate(ScePthreadAttr* attr, int detachstate) {
    if (attr == nullptr || *attr == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    int pstate = PTHREAD_CREATE_JOINABLE;
    switch (detachstate) {
    case 0:
        pstate = PTHREAD_CREATE_JOINABLE;
        break;
    case 1:
        pstate = PTHREAD_CREATE_DETACHED;
        break;
    default:
        UNREACHABLE_MSG("Invalid detachstate: {}", detachstate);
    }

    // int result = pthread_attr_setdetachstate(&(*attr)->pth_attr, pstate); doesn't seem to work
    // correctly
    int result = 0;

    (*attr)->detached = (pstate == PTHREAD_CREATE_DETACHED);

    return result == 0 ? SCE_OK : SCE_KERNEL_ERROR_EINVAL;
}

int PS4_SYSV_ABI scePthreadAttrSetinheritsched(ScePthreadAttr* attr, int inheritSched) {
    if (attr == nullptr || *attr == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    int pinherit_sched = PTHREAD_INHERIT_SCHED;
    switch (inheritSched) {
    case 0:
        pinherit_sched = PTHREAD_EXPLICIT_SCHED;
        break;
    case 4:
        pinherit_sched = PTHREAD_INHERIT_SCHED;
        break;
    default:
        UNREACHABLE_MSG("Invalid inheritSched: {}", inheritSched);
    }

    int result = pthread_attr_setinheritsched(&(*attr)->pth_attr, pinherit_sched);

    return result == 0 ? SCE_OK : SCE_KERNEL_ERROR_EINVAL;
}

int PS4_SYSV_ABI scePthreadAttrSetschedparam(ScePthreadAttr* attr,
                                             const SceKernelSchedParam* param) {
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

    return result == 0 ? SCE_OK : SCE_KERNEL_ERROR_EINVAL;
}

int PS4_SYSV_ABI scePthreadAttrSetschedpolicy(ScePthreadAttr* attr, int policy) {
    if (attr == nullptr || *attr == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    int ppolicy = SCHED_OTHER; // winpthreads only supports SCHED_OTHER
    if (policy != SCHED_OTHER) {
        LOG_ERROR(Kernel_Pthread, "policy={} not supported by winpthreads\n", policy);
    }
    (*attr)->policy = policy;

    int result = pthread_attr_setschedpolicy(&(*attr)->pth_attr, ppolicy);

    return result == 0 ? SCE_OK : SCE_KERNEL_ERROR_EINVAL;
}
ScePthread PS4_SYSV_ABI scePthreadSelf() {
    return g_pthread_self;
}

int PS4_SYSV_ABI scePthreadAttrSetaffinity(ScePthreadAttr* pattr,
                                           const /*SceKernelCpumask*/ u64 mask) {
    LOG_INFO(Kernel_Pthread, "called");

    if (pattr == nullptr || *pattr == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    (*pattr)->affinity = mask;

    return SCE_OK;
}

int PS4_SYSV_ABI scePthreadAttrGetaffinity(const ScePthreadAttr* pattr,
                                           /* SceKernelCpumask*/ u64* mask) {
    if (pattr == nullptr || *pattr == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    *mask = (*pattr)->affinity;

    return SCE_OK;
}
int PS4_SYSV_ABI scePthreadSetaffinity(ScePthread thread, const /*SceKernelCpumask*/ u64 mask) {
    LOG_INFO(Kernel_Pthread, "called");

    if (thread == nullptr) {
        return SCE_KERNEL_ERROR_ESRCH;
    }

    auto result = scePthreadAttrSetaffinity(&thread->attr, mask);

    return result;
}
int PS4_SYSV_ABI scePthreadCreate(ScePthread* thread, const ScePthreadAttr* attr,
                                  pthreadEntryFunc start_routine, void* arg, const char* name) {
    LOG_INFO(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

void* createMutex(void* addr) {
    if (addr == nullptr || *static_cast<ScePthreadMutex*>(addr) != nullptr) {
        return addr;
    }
    auto vaddr = reinterpret_cast<u64>(addr);

    std::string name = fmt::format("mutex{:#x}", vaddr);
    scePthreadMutexInit(static_cast<ScePthreadMutex*>(addr), nullptr, name.c_str());
    return addr;
}

int PS4_SYSV_ABI scePthreadMutexInit(ScePthreadMutex* mutex, const ScePthreadMutexattr* attr,
                                     const char* name) {
    if (mutex == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (attr == nullptr) {
        attr = g_pthread_cxt->getDefaultMutexattr();
    }

    *mutex = new PthreadMutexInternal{};
    if (name != nullptr) {
        (*mutex)->name = name;
    } else {
        (*mutex)->name = "nonameMutex";
    }

    int result = pthread_mutex_init(&(*mutex)->pth_mutex, &(*attr)->pth_mutex_attr);

    if (name != nullptr) {
        LOG_INFO(Kernel_Pthread, "name={}, result={}", name, result);
    }

    switch (result) {
    case 0:
        return SCE_OK;
    case EAGAIN:
        return SCE_KERNEL_ERROR_EAGAIN;
    case EINVAL:
        return SCE_KERNEL_ERROR_EINVAL;
    case ENOMEM:
        return SCE_KERNEL_ERROR_ENOMEM;
    default:
        return SCE_KERNEL_ERROR_EINVAL;
    }
}

int PS4_SYSV_ABI scePthreadMutexDestroy(ScePthreadMutex* mutex) {

    if (mutex == nullptr || *mutex == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    int result = pthread_mutex_destroy(&(*mutex)->pth_mutex);

    LOG_INFO(Kernel_Pthread, "name={}, result={}", (*mutex)->name, result);

    delete *mutex;
    *mutex = nullptr;

    switch (result) {
    case 0:
        return SCE_OK;
    case EBUSY:
        return SCE_KERNEL_ERROR_EBUSY;
    case EINVAL:
    default:
        return SCE_KERNEL_ERROR_EINVAL;
    }
}
int PS4_SYSV_ABI scePthreadMutexattrInit(ScePthreadMutexattr* attr) {
    *attr = new PthreadMutexattrInternal{};

    int result = pthread_mutexattr_init(&(*attr)->pth_mutex_attr);

    result = (result == 0 ? scePthreadMutexattrSettype(attr, 1) : result);
    result = (result == 0 ? scePthreadMutexattrSetprotocol(attr, 0) : result);

    switch (result) {
    case 0:
        return SCE_OK;
    case ENOMEM:
        return SCE_KERNEL_ERROR_ENOMEM;
    default:
        return SCE_KERNEL_ERROR_EINVAL;
    }
}

int PS4_SYSV_ABI scePthreadMutexattrSettype(ScePthreadMutexattr* attr, int type) {
    int ptype = PTHREAD_MUTEX_DEFAULT;
    switch (type) {
    case 1:
        ptype = PTHREAD_MUTEX_ERRORCHECK;
        break;
    case 2:
        ptype = PTHREAD_MUTEX_RECURSIVE;
        break;
    case 3:
    case 4:
        ptype = PTHREAD_MUTEX_NORMAL;
        break;
    default:
        UNREACHABLE_MSG("Invalid type: {}", type);
    }

    int result = pthread_mutexattr_settype(&(*attr)->pth_mutex_attr, ptype);

    return result == 0 ? SCE_OK : SCE_KERNEL_ERROR_EINVAL;
}

int PS4_SYSV_ABI scePthreadMutexattrSetprotocol(ScePthreadMutexattr* attr, int protocol) {
    int pprotocol = PTHREAD_PRIO_NONE;
    switch (protocol) {
    case 0:
        pprotocol = PTHREAD_PRIO_NONE;
        break;
    case 1:
        pprotocol = PTHREAD_PRIO_INHERIT;
        break;
    case 2:
        pprotocol = PTHREAD_PRIO_PROTECT;
        break;
    default:
        UNREACHABLE_MSG("Invalid protocol: {}", protocol);
    }

    int result = 0; // pthread_mutexattr_setprotocol(&(*attr)->p, pprotocol); //it appears that
                    // pprotocol has issues in winpthreads
    (*attr)->pprotocol = pprotocol;

    return result == 0 ? SCE_OK : SCE_KERNEL_ERROR_EINVAL;
}
int PS4_SYSV_ABI scePthreadMutexLock(ScePthreadMutex* mutex) {
    LOG_INFO(Kernel_Pthread, "called");
    mutex = static_cast<ScePthreadMutex*>(createMutex(mutex));

    if (mutex == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    int result = pthread_mutex_lock(&(*mutex)->pth_mutex);
    LOG_INFO(Kernel_Pthread, "name={}, result={}", (*mutex)->name, result);
    switch (result) {
    case 0:
        return SCE_OK;
    case EAGAIN:
        return SCE_KERNEL_ERROR_EAGAIN;
    case EINVAL:
        return SCE_KERNEL_ERROR_EINVAL;
    case EDEADLK:
        return SCE_KERNEL_ERROR_EDEADLK;
    default:
        return SCE_KERNEL_ERROR_EINVAL;
    }
}
int PS4_SYSV_ABI scePthreadMutexUnlock(ScePthreadMutex* mutex) {
    LOG_INFO(Kernel_Pthread, "called");
    mutex = static_cast<ScePthreadMutex*>(createMutex(mutex));
    if (mutex == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    int result = pthread_mutex_unlock(&(*mutex)->pth_mutex);
    LOG_INFO(Kernel_Pthread, "name={}, result={}", (*mutex)->name, result);
    switch (result) {
    case 0:
        return SCE_OK;

    case EINVAL:
        return SCE_KERNEL_ERROR_EINVAL;
    case EPERM:
        return SCE_KERNEL_ERROR_EPERM;
    default:
        return SCE_KERNEL_ERROR_EINVAL;
    }
}

int PS4_SYSV_ABI scePthreadMutexattrDestroy(ScePthreadMutexattr* attr) {

    int result = pthread_mutexattr_destroy(&(*attr)->pth_mutex_attr);

    delete *attr;
    *attr = nullptr;

    switch (result) {
    case 0:
        return SCE_OK;
    case ENOMEM:
        return SCE_KERNEL_ERROR_ENOMEM;
    default:
        return SCE_KERNEL_ERROR_EINVAL;
    }
}

void* createCond(void* addr) {
    if (addr == nullptr || *static_cast<ScePthreadCond*>(addr) != nullptr) {
        return addr;
    }
    auto vaddr = reinterpret_cast<u64>(addr);

    std::string name = fmt::format("cond{:#x}", vaddr);
    scePthreadCondInit(static_cast<ScePthreadCond*>(addr), nullptr, name.c_str());
    return addr;
}

int PS4_SYSV_ABI scePthreadCondInit(ScePthreadCond* cond, const ScePthreadCondattr* attr,
                                    const char* name) {
    if (cond == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    if (attr == nullptr) {
        attr = g_pthread_cxt->getDefaultCondattr();
    }

    *cond = new PthreadCondInternal{};

    if (name != nullptr) {
        (*cond)->name = name;
    } else {
        (*cond)->name = "nonameCond";
    }

    int result = pthread_cond_init(&(*cond)->cond, &(*attr)->cond_attr);

    if (name != nullptr) {
        LOG_INFO(Kernel_Pthread, "name={}, result={}", (*cond)->name, result);
    }

    switch (result) {
    case 0:
        return SCE_OK;
    case EAGAIN:
        return SCE_KERNEL_ERROR_EAGAIN;
    case EINVAL:
        return SCE_KERNEL_ERROR_EINVAL;
    case ENOMEM:
        return SCE_KERNEL_ERROR_ENOMEM;
    default:
        return SCE_KERNEL_ERROR_EINVAL;
    }
}

int PS4_SYSV_ABI scePthreadCondattrInit(ScePthreadCondattr* attr) {
    *attr = new PthreadCondAttrInternal{};

    int result = pthread_condattr_init(&(*attr)->cond_attr);

    switch (result) {
    case 0:
        return SCE_OK;
    case ENOMEM:
        return SCE_KERNEL_ERROR_ENOMEM;
    default:
        return SCE_KERNEL_ERROR_EINVAL;
    }
}

int PS4_SYSV_ABI scePthreadCondBroadcast(ScePthreadCond* cond) {
    LOG_INFO(Kernel_Pthread, "called");
    cond = static_cast<ScePthreadCond*>(createCond(cond));

    if (cond == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    int result = pthread_cond_broadcast(&(*cond)->cond);

    LOG_INFO(Kernel_Pthread, "name={}, result={}", (*cond)->name, result);

    return (result == 0 ? SCE_OK : SCE_KERNEL_ERROR_EINVAL);
}

int PS4_SYSV_ABI posix_pthread_mutex_init(ScePthreadMutex* mutex, const ScePthreadMutexattr* attr) {
    LOG_INFO(Kernel_Pthread, "posix pthread_mutex_init redirect to scePthreadMutexInit");
    int result = scePthreadMutexInit(mutex, attr, nullptr);
    if (result < 0) {
        int rt = result > SCE_KERNEL_ERROR_UNKNOWN && result <= SCE_KERNEL_ERROR_ESTOP
                     ? result + -SCE_KERNEL_ERROR_UNKNOWN
                     : POSIX_EOTHER;
        return rt;
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_mutex_lock(ScePthreadMutex* mutex) {
    LOG_INFO(Kernel_Pthread, "posix pthread_mutex_lock redirect to scePthreadMutexLock");
    int result = scePthreadMutexLock(mutex);
    if (result < 0) {
        int rt = result > SCE_KERNEL_ERROR_UNKNOWN && result <= SCE_KERNEL_ERROR_ESTOP
                     ? result + -SCE_KERNEL_ERROR_UNKNOWN
                     : POSIX_EOTHER;
        return rt;
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_mutex_unlock(ScePthreadMutex* mutex) {
    LOG_INFO(Kernel_Pthread, "posix pthread_mutex_unlock redirect to scePthreadMutexUnlock");
    int result = scePthreadMutexUnlock(mutex);
    if (result < 0) {
        int rt = result > SCE_KERNEL_ERROR_UNKNOWN && result <= SCE_KERNEL_ERROR_ESTOP
                     ? result + -SCE_KERNEL_ERROR_UNKNOWN
                     : POSIX_EOTHER;
        return rt;
    }
    return result;
}

int PS4_SYSV_ABI posix_pthread_cond_broadcast(ScePthreadCond* cond) {
    LOG_INFO(Kernel_Pthread,
             "posix posix_pthread_cond_broadcast redirect to scePthreadCondBroadcast");
    int result = scePthreadCondBroadcast(cond);
    if (result != 0) {
        int rt = result > SCE_KERNEL_ERROR_UNKNOWN && result <= SCE_KERNEL_ERROR_ESTOP
                     ? result + -SCE_KERNEL_ERROR_UNKNOWN
                     : POSIX_EOTHER;
        return rt;
    }
    return result;
}

int PS4_SYSV_ABI sceKernelClockGettime(s32 clock_id, SceKernelTimespec* tp) {
    if (tp == nullptr) {
        return SCE_KERNEL_ERROR_EFAULT;
    }
    clockid_t pclock_id = CLOCK_REALTIME;
    switch (clock_id) {
    case 0:
        pclock_id = CLOCK_REALTIME;
        break;
    case 13:
    case 4:
        pclock_id = CLOCK_MONOTONIC;
        break;
    default:
        UNREACHABLE();
    }

    timespec t{};
    int result = clock_gettime(pclock_id, &t);
    tp->tv_sec = t.tv_sec;
    tp->tv_nsec = t.tv_nsec;
    if (result == 0) {
        return SCE_OK;
    }
    return SCE_KERNEL_ERROR_EINVAL;
}

void pthreadSymbolsRegister(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("4+h9EzwKF4I", "libkernel", 1, "libkernel", 1, 1, scePthreadAttrSetschedpolicy);
    LIB_FUNCTION("-Wreprtu0Qs", "libkernel", 1, "libkernel", 1, 1, scePthreadAttrSetdetachstate);
    LIB_FUNCTION("eXbUSpEaTsA", "libkernel", 1, "libkernel", 1, 1, scePthreadAttrSetinheritsched);
    LIB_FUNCTION("DzES9hQF4f4", "libkernel", 1, "libkernel", 1, 1, scePthreadAttrSetschedparam);
    LIB_FUNCTION("nsYoNRywwNg", "libkernel", 1, "libkernel", 1, 1, scePthreadAttrInit);
    LIB_FUNCTION("62KCwEMmzcM", "libkernel", 1, "libkernel", 1, 1, scePthreadAttrDestroy);

    LIB_FUNCTION("aI+OeCz8xrQ", "libkernel", 1, "libkernel", 1, 1, scePthreadSelf);
    LIB_FUNCTION("3qxgM4ezETA", "libkernel", 1, "libkernel", 1, 1, scePthreadAttrSetaffinity);
    LIB_FUNCTION("8+s5BzZjxSg", "libkernel", 1, "libkernel", 1, 1, scePthreadAttrGetaffinity);

    LIB_FUNCTION("bt3CTBKmGyI", "libkernel", 1, "libkernel", 1, 1, scePthreadSetaffinity);
    LIB_FUNCTION("6UgtwV+0zb4", "libkernel", 1, "libkernel", 1, 1, scePthreadCreate);
    // mutex calls
    LIB_FUNCTION("cmo1RIYva9o", "libkernel", 1, "libkernel", 1, 1, scePthreadMutexInit);
    LIB_FUNCTION("2Of0f+3mhhE", "libkernel", 1, "libkernel", 1, 1, scePthreadMutexDestroy);
    LIB_FUNCTION("F8bUHwAG284", "libkernel", 1, "libkernel", 1, 1, scePthreadMutexattrInit);
    LIB_FUNCTION("smWEktiyyG0", "libkernel", 1, "libkernel", 1, 1, scePthreadMutexattrDestroy);
    LIB_FUNCTION("iMp8QpE+XO4", "libkernel", 1, "libkernel", 1, 1, scePthreadMutexattrSettype);
    LIB_FUNCTION("1FGvU0i9saQ", "libkernel", 1, "libkernel", 1, 1, scePthreadMutexattrSetprotocol);
    LIB_FUNCTION("9UK1vLZQft4", "libkernel", 1, "libkernel", 1, 1, scePthreadMutexLock);
    LIB_FUNCTION("tn3VlD0hG60", "libkernel", 1, "libkernel", 1, 1, scePthreadMutexUnlock);
    // cond calls
    LIB_FUNCTION("2Tb92quprl0", "libkernel", 1, "libkernel", 1, 1, scePthreadCondInit);
    LIB_FUNCTION("m5-2bsNfv7s", "libkernel", 1, "libkernel", 1, 1, scePthreadCondattrInit);
    LIB_FUNCTION("JGgj7Uvrl+A", "libkernel", 1, "libkernel", 1, 1, scePthreadCondBroadcast);
    // posix calls
    LIB_FUNCTION("ttHNfU+qDBU", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_mutex_init);
    LIB_FUNCTION("7H0iTOciTLo", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_mutex_lock);
    LIB_FUNCTION("2Z+PpY6CaJg", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_mutex_unlock);
    LIB_FUNCTION("mkx2fVhNMsg", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_cond_broadcast);

    LIB_FUNCTION("QBi7HCK03hw", "libkernel", 1, "libkernel", 1, 1, sceKernelClockGettime);

    // openorbis weird functions
    LIB_FUNCTION("7H0iTOciTLo", "libkernel", 1, "libkernel", 1, 1, posix_pthread_mutex_lock);
    LIB_FUNCTION("2Z+PpY6CaJg", "libkernel", 1, "libkernel", 1, 1, posix_pthread_mutex_unlock);
    LIB_FUNCTION("mkx2fVhNMsg", "libkernel", 1, "libkernel", 1, 1, posix_pthread_cond_broadcast);
}

} // namespace Core::Libraries::LibKernel
