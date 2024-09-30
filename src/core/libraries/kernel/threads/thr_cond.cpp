// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/libkernel.h"
#include "core/libraries/kernel/threads/threads.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

static std::mutex CondStaticLock;

#define THR_COND_INITIALIZER ((PthreadCond*)NULL)
#define THR_COND_DESTROYED ((PthreadCond*)1)

enum class ClockId : u32 {
    Realtime = 0,
    Virtual = 1,
    Prof = 2,
    Monotonic = 4,
    Uptime = 5,
    UptimePrecise = 7,
    UptimeFast = 8,
    RealtimePrecise = 9,
    RealtimeFast = 10,
    MonotonicPrecise = 11,
    MonotonicFast = 12,
    Second = 13,
    ThreadCputimeID = 14,
};

static constexpr PthreadCondAttr PhreadCondattrDefault = {.c_pshared = PTHREAD_PROCESS_PRIVATE,
                                                          .c_clockid = CLOCK_REALTIME};

static int CondInit(PthreadCondT* cond, const PthreadCondAttrT* cond_attr) {
    PthreadCond* cvp = (PthreadCond*)calloc(1, sizeof(PthreadCond));
    if (cvp == nullptr) {
        return POSIX_ENOMEM;
    }
    std::construct_at(cvp);
    if (cond_attr == nullptr || *cond_attr == nullptr) {
        cvp->clock_id = CLOCK_REALTIME;
    } else {
        // if ((*cond_attr)->c_pshared) {
        //     cvp->flags |= USYNC_PROCESS_SHARED;
        // }
        cvp->clock_id = (*cond_attr)->c_clockid;
    }
    *cond = cvp;
    return 0;
}

static int InitStatic(Pthread* thread, PthreadCondT* cond) {
    std::scoped_lock lk{CondStaticLock};
    if (*cond == NULL)
        return CondInit(cond, NULL);
    return 0;
}

#define CHECK_AND_INIT_COND                                                                        \
    if (cvp = *cond; cvp <= THR_COND_DESTROYED) [[unlikely]] {                                     \
        if (cvp == THR_COND_INITIALIZER) {                                                         \
            int ret;                                                                               \
            ret = InitStatic(g_curthread, cond);                                                   \
            if (ret)                                                                               \
                return (ret);                                                                      \
        } else if (cvp == THR_COND_DESTROYED) {                                                    \
            return POSIX_EINVAL;                                                                   \
        }                                                                                          \
    }

int PS4_SYSV_ABI posix_pthread_cond_init(PthreadCondT* cond, const PthreadCondAttrT* cond_attr) {
    *cond = nullptr;
    return CondInit(cond, cond_attr);
}

int PS4_SYSV_ABI posix_pthread_cond_destroy(PthreadCondT* cond) {
    PthreadCond* cvp = *cond;
    if (cvp == THR_COND_INITIALIZER) {
        return 0;
    }
    if (cvp == THR_COND_DESTROYED) {
        return POSIX_EINVAL;
    }
    cvp = *cond;
    *cond = THR_COND_DESTROYED;
    std::destroy_at(cvp);
    free(cvp);
    return 0;
}

int PthreadCond::Wait(PthreadMutexT* mutex, const OrbisKernelTimespec* abstime) {
    Pthread* curthread = g_curthread;
    PthreadMutex* mp = *mutex;

    if (int error = mp->IsOwned(curthread); error != 0) {
        return error;
    }

    //_thr_testcancel(curthread);
    //_thr_cancel_enter2(curthread, 0);
    if (abstime) {
        const auto status = cond.wait_until(mp->m_lock, abstime->TimePoint());
        return status == std::cv_status::timeout ? POSIX_ETIMEDOUT : 0;
    } else {
        cond.wait(mp->m_lock);
        return 0;
    }
    //_thr_cancel_leave(curthread, 0);
}

int PS4_SYSV_ABI posix_pthread_cond_wait(PthreadCondT* cond, PthreadMutexT* mutex) {
    PthreadCond* cvp;
    CHECK_AND_INIT_COND
    return cvp->Wait(mutex, nullptr);
}

int PS4_SYSV_ABI posix_pthread_cond_timedwait(PthreadCondT* cond, PthreadMutexT* mutex,
                                              const OrbisKernelTimespec* abstime) {
    if (abstime == nullptr || abstime->tv_sec < 0 || abstime->tv_nsec < 0 ||
        abstime->tv_nsec >= 1000000000) {
        return POSIX_EINVAL;
    }

    PthreadCond* cvp;
    CHECK_AND_INIT_COND
    return cvp->Wait(mutex, abstime);
}

int PS4_SYSV_ABI posix_pthread_cond_signal(PthreadCondT* cond) {
    PthreadCond* cvp;
    CHECK_AND_INIT_COND
    cvp->cond.notify_one();
    return 0;
}

int PS4_SYSV_ABI posix_pthread_cond_broadcast(PthreadCondT* cond) {
    PthreadCond* cvp;
    CHECK_AND_INIT_COND
    cvp->cond.notify_all();
    return 0;
}

int PS4_SYSV_ABI posix_pthread_condattr_init(PthreadCondAttrT* attr) {
    PthreadCondAttr* pattr = (PthreadCondAttr*)malloc(sizeof(PthreadCondAttr));
    if (pattr == nullptr) {
        return POSIX_ENOMEM;
    }
    memcpy(pattr, &PhreadCondattrDefault, sizeof(PthreadCondAttr));
    *attr = pattr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_condattr_destroy(PthreadCondAttrT* attr) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    free(*attr);
    *attr = nullptr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_condattr_getclock(const PthreadCondAttrT* attr, ClockId* clock_id) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    *clock_id = static_cast<ClockId>((*attr)->c_clockid);
    return 0;
}

int PS4_SYSV_ABI posix_pthread_condattr_setclock(PthreadCondAttrT* attr, ClockId clock_id) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    if (clock_id != ClockId::Realtime && clock_id != ClockId::Virtual &&
        clock_id != ClockId::Prof && clock_id != ClockId::Monotonic) {
        return POSIX_EINVAL;
    }
    (*attr)->c_clockid = static_cast<int>(clock_id);
    return 0;
}

int PS4_SYSV_ABI posix_pthread_condattr_getpshared(const PthreadCondAttrT* attr, int* pshared) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    *pshared = PTHREAD_PROCESS_PRIVATE;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_condattr_setpshared(PthreadCondAttrT* attr, int pshared) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    if (pshared != PTHREAD_PROCESS_PRIVATE) {
        return POSIX_EINVAL;
    }
    return 0;
}

void RegisterCond(Core::Loader::SymbolsResolver* sym) {
    // Posix
    LIB_FUNCTION("0TyVk4MSLt0", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_cond_init);
    LIB_FUNCTION("2MOy+rUfuhQ", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_cond_signal);
    LIB_FUNCTION("RXXqi4CtF8w", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_cond_destroy);
    LIB_FUNCTION("Op8TBGY5KHg", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_cond_wait);
    LIB_FUNCTION("27bAgiJmOh0", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_cond_timedwait);
    LIB_FUNCTION("mkx2fVhNMsg", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_cond_broadcast);

    // Posix-Kernel
    LIB_FUNCTION("Op8TBGY5KHg", "libkernel", 1, "libkernel", 1, 1, posix_pthread_cond_wait);
    LIB_FUNCTION("mkx2fVhNMsg", "libkernel", 1, "libkernel", 1, 1, posix_pthread_cond_broadcast);

    // Orbis
    LIB_FUNCTION("2Tb92quprl0", "libkernel", 1, "libkernel", 1, 1, ORBIS(posix_pthread_cond_init));
    LIB_FUNCTION("m5-2bsNfv7s", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_condattr_init));
    LIB_FUNCTION("JGgj7Uvrl+A", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_cond_broadcast));
    LIB_FUNCTION("WKAXJ4XBPQ4", "libkernel", 1, "libkernel", 1, 1, ORBIS(posix_pthread_cond_wait));
    LIB_FUNCTION("waPcxYiR3WA", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_condattr_destroy));
    LIB_FUNCTION("kDh-NfxgMtE", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_cond_signal));
    LIB_FUNCTION("BmMjYxmew1w", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_cond_timedwait));
    LIB_FUNCTION("g+PZd2hiacg", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_cond_destroy));
}

} // namespace Libraries::Kernel
