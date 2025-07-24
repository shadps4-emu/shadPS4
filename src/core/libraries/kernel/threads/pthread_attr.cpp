// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/kernel/threads/thread_state.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

static constexpr u32 PthreadStackMin = 16_KB;

struct PthreadPrio {
    s32 pri_min;
    s32 pri_max;
    s32 pri_default;
};

static constexpr std::array<PthreadPrio, 3> ThrPriorities = {{
    {0x100, 0x2FF, 0x2BC}, // Fifo
    {0x300, 0x3BF, 0x384}, // Other
    {0x100, 0x2FF, 0x2BC}, // Round-Robin
}};

PthreadAttr PthreadAttrDefault = {
    .sched_policy = SchedPolicy::Fifo,
    .sched_inherit = 0,
    .prio = 0,
    .suspend = false,
    .flags = PthreadAttrFlags::ScopeSystem,
    .stackaddr_attr = NULL,
    .stacksize_attr = ThrStackDefault,
    .guardsize_attr = 0,
    .cpusetsize = 0,
    .cpuset = nullptr,
};

int PS4_SYSV_ABI posix_pthread_attr_destroy(PthreadAttrT* attr) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    delete *attr;
    *attr = nullptr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_getdetachstate(const PthreadAttrT* attr, int* detachstate) {
    if (attr == nullptr || *attr == nullptr || detachstate == nullptr) {
        return POSIX_EINVAL;
    }
    *detachstate = True((*attr)->flags & PthreadAttrFlags::Detached);
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_getguardsize(const PthreadAttrT* attr, size_t* guardsize) {
    if (attr == nullptr || *attr == nullptr || guardsize == nullptr) {
        return POSIX_EINVAL;
    }
    *guardsize = (*attr)->guardsize_attr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_getinheritsched(const PthreadAttrT* attr, int* sched_inherit) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    *sched_inherit = (*attr)->sched_inherit;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_getschedparam(const PthreadAttrT* attr, SchedParam* param) {
    if (attr == nullptr || *attr == nullptr || param == nullptr) {
        return POSIX_EINVAL;
    }
    param->sched_priority = (*attr)->prio;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_getschedpolicy(const PthreadAttrT* attr, SchedPolicy* policy) {
    if (attr == nullptr || *attr == nullptr || policy == nullptr) {
        return POSIX_EINVAL;
    }
    *policy = (*attr)->sched_policy;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_getstack(const PthreadAttrT* attr, void** stackaddr,
                                             size_t* stacksize) {
    if (attr == nullptr || *attr == nullptr || stackaddr == nullptr || stacksize == nullptr) {
        return POSIX_EINVAL;
    }
    *stackaddr = (*attr)->stackaddr_attr;
    *stacksize = (*attr)->stacksize_attr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_getstackaddr(const PthreadAttrT* attr, void** stackaddr) {
    if (attr == nullptr || *attr == nullptr || stackaddr == nullptr) {
        return POSIX_EINVAL;
    }
    *stackaddr = (*attr)->stackaddr_attr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_getstacksize(const PthreadAttrT* attr, size_t* stacksize) {
    if (attr == nullptr || *attr == nullptr || stacksize == nullptr) {
        return POSIX_EINVAL;
    }
    *stacksize = (*attr)->stacksize_attr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_init(PthreadAttrT* attr) {
    PthreadAttrT pattr = new PthreadAttr{};
    if (pattr == nullptr) {
        return POSIX_ENOMEM;
    }
    memcpy(pattr, &PthreadAttrDefault, sizeof(PthreadAttr));
    *attr = pattr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_setschedpolicy(PthreadAttrT* attr, SchedPolicy policy) {
    if (attr == NULL || *attr == NULL) {
        return POSIX_EINVAL;
    } else if ((policy < SchedPolicy::Fifo) || (policy > SchedPolicy::RoundRobin)) {
        return POSIX_ENOTSUP;
    }
    (*attr)->sched_policy = policy;
    (*attr)->prio = ThrPriorities[u32(policy) - 1].pri_default;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_setstack(PthreadAttrT* attr, void* stackaddr,
                                             size_t stacksize) {
    if (attr == nullptr || *attr == nullptr || stackaddr == nullptr ||
        stacksize < PthreadStackMin) {
        return POSIX_EINVAL;
    }
    (*attr)->stackaddr_attr = stackaddr;
    (*attr)->stacksize_attr = stacksize;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_setstackaddr(PthreadAttrT* attr, void* stackaddr) {
    if (attr == nullptr || *attr == nullptr || stackaddr == nullptr) {
        return POSIX_EINVAL;
    }
    (*attr)->stackaddr_attr = stackaddr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_setstacksize(PthreadAttrT* attr, size_t stacksize) {
    if (attr == nullptr || *attr == nullptr || stacksize < PthreadStackMin) {
        return POSIX_EINVAL;
    }
    (*attr)->stacksize_attr = stacksize;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_setdetachstate(PthreadAttrT* attr, int detachstate) {
    if (attr == nullptr || *attr == nullptr || (detachstate != 1 && detachstate != 0)) {
        return POSIX_EINVAL;
    }
    if (detachstate) {
        (*attr)->flags |= PthreadAttrFlags::Detached;
    } else {
        (*attr)->flags &= ~PthreadAttrFlags::Detached;
    }
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_setschedparam(PthreadAttrT* attr, SchedParam* param) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    if (param == nullptr) {
        return POSIX_ENOTSUP;
    }

    const auto policy = (*attr)->sched_policy;
    if (policy == SchedPolicy::RoundRobin) {
        if (param->sched_priority < ThrPriorities[u32(policy) - 1].pri_min ||
            param->sched_priority > ThrPriorities[u32(policy) - 1].pri_max) {
            return POSIX_ENOTSUP;
        }
    }
    (*attr)->prio = param->sched_priority;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_setinheritsched(PthreadAttrT* attr, int sched_inherit) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    if (sched_inherit != 4 && sched_inherit != 0) {
        return POSIX_ENOTSUP;
    }

    (*attr)->sched_inherit = sched_inherit;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_setguardsize(PthreadAttrT* attr, size_t guardsize) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    (*attr)->guardsize_attr = guardsize;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_get_np(PthreadT pthread, PthreadAttrT* dstattr) {
    PthreadAttr* dst;
    if (pthread == nullptr || dstattr == nullptr || (dst = *dstattr) == nullptr) {
        return POSIX_EINVAL;
    }
    auto* thread_state = ThrState::Instance();
    int ret = thread_state->FindThread(pthread, /*include dead*/ 0);
    if (ret != 0) {
        return ret;
    }
    PthreadAttr attr = pthread->attr;
    if (True(pthread->flags & ThreadFlags::Detached)) {
        attr.flags |= PthreadAttrFlags::Detached;
    }
    pthread->lock.unlock();
    if (ret == 0) {
        memcpy(dst, &attr, sizeof(PthreadAttr));
    }
    return ret;
}

int PS4_SYSV_ABI posix_pthread_attr_getaffinity_np(const PthreadAttrT* pattr, size_t cpusetsize,
                                                   Cpuset* cpusetp) {
    if (pattr == nullptr) {
        return POSIX_EINVAL;
    }
    PthreadAttrT attr = *pattr;
    if (attr == nullptr) {
        return POSIX_EINVAL;
    }
    if (attr->cpuset != nullptr)
        memcpy(cpusetp, attr->cpuset, std::min(cpusetsize, attr->cpusetsize));
    else
        memset(cpusetp, -1, cpusetsize);
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_setaffinity_np(PthreadAttrT* pattr, size_t cpusetsize,
                                                   const Cpuset* cpusetp) {
    if (pattr == nullptr) {
        return POSIX_EINVAL;
    }
    PthreadAttrT attr = *pattr;
    if (attr == nullptr) {
        return POSIX_EINVAL;
    }
    if (cpusetsize == 0 || cpusetp == nullptr) {
        if (attr->cpuset != nullptr) {
            free(attr->cpuset);
            attr->cpuset = nullptr;
            attr->cpusetsize = 0;
        }
        return 0;
    }
    if (attr->cpuset == nullptr) {
        attr->cpuset = static_cast<Cpuset*>(calloc(1, sizeof(Cpuset)));
        attr->cpusetsize = sizeof(Cpuset);
    }
    memcpy(attr->cpuset, cpusetp, std::min(cpusetsize, sizeof(Cpuset)));
    return 0;
}

int PS4_SYSV_ABI scePthreadAttrGetaffinity(PthreadAttrT* attr, u64* mask) {
    Cpuset cpuset;
    const int ret = posix_pthread_attr_getaffinity_np(attr, sizeof(Cpuset), &cpuset);
    if (ret == 0) {
        *mask = cpuset.bits;
    }
    return ret;
}

int PS4_SYSV_ABI scePthreadAttrSetaffinity(PthreadAttrT* attr, const u64 mask) {
    const Cpuset cpuset = {.bits = mask};
    return posix_pthread_attr_setaffinity_np(attr, sizeof(Cpuset), &cpuset);
}

void RegisterThreadAttr(Core::Loader::SymbolsResolver* sym) {
    // Posix
    LIB_FUNCTION("wtkt-teR1so", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_attr_init);
    LIB_FUNCTION("vQm4fDEsWi8", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_attr_getstack);
    LIB_FUNCTION("2Q0z6rnBrTE", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_attr_setstacksize);
    LIB_FUNCTION("Ucsu-OK+els", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_attr_get_np);
    LIB_FUNCTION("RtLRV-pBTTY", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_attr_getschedpolicy);
    LIB_FUNCTION("JarMIy8kKEY", "libkernel", 1, "libkernel", 1, 1,
                 posix_pthread_attr_setschedpolicy);
    LIB_FUNCTION("E+tyo3lp5Lw", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_attr_setdetachstate);
    LIB_FUNCTION("zHchY8ft5pk", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_attr_destroy);
    LIB_FUNCTION("euKRgm0Vn2M", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_attr_setschedparam);
    LIB_FUNCTION("7ZlAakEf0Qg", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_attr_setinheritsched);
    LIB_FUNCTION("0qOtCR-ZHck", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_attr_getstacksize);
    LIB_FUNCTION("VUT1ZSrHT0I", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_attr_getdetachstate);
    LIB_FUNCTION("JKyG3SWyA10", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_attr_setguardsize);
    LIB_FUNCTION("qlk9pSLsUmM", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_attr_getschedparam);

    // Orbis
    LIB_FUNCTION("4+h9EzwKF4I", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_attr_setschedpolicy));
    LIB_FUNCTION("-Wreprtu0Qs", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_attr_setdetachstate));
    LIB_FUNCTION("JaRMy+QcpeU", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_attr_getdetachstate));
    LIB_FUNCTION("eXbUSpEaTsA", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_attr_setinheritsched));
    LIB_FUNCTION("DzES9hQF4f4", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_attr_setschedparam));
    LIB_FUNCTION("nsYoNRywwNg", "libkernel", 1, "libkernel", 1, 1, ORBIS(posix_pthread_attr_init));
    LIB_FUNCTION("62KCwEMmzcM", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_attr_destroy));
    LIB_FUNCTION("-quPa4SEJUw", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_attr_getstack));
    LIB_FUNCTION("Bvn74vj6oLo", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_attr_setstack));
    LIB_FUNCTION("Ru36fiTtJzA", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_attr_getstackaddr));
    LIB_FUNCTION("-fA+7ZlGDQs", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_attr_getstacksize));
    LIB_FUNCTION("x1X76arYMxU", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_attr_get_np));
    LIB_FUNCTION("FXPWHNk8Of0", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_attr_getschedparam));
    LIB_FUNCTION("UTXzJbWhhTE", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_attr_setstacksize));
    LIB_FUNCTION("F+yfmduIBB8", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_attr_setstackaddr));
    LIB_FUNCTION("El+cQ20DynU", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_attr_setguardsize));
    LIB_FUNCTION("8+s5BzZjxSg", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(scePthreadAttrGetaffinity));
    LIB_FUNCTION("3qxgM4ezETA", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(scePthreadAttrSetaffinity));
}

} // namespace Libraries::Kernel
