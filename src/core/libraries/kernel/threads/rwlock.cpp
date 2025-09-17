// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

static std::mutex RwlockStaticLock;

#define THR_RWLOCK_INITIALIZER ((PthreadRwlock*)NULL)
#define THR_RWLOCK_DESTROYED ((PthreadRwlock*)1)

#define CHECK_AND_INIT_RWLOCK                                                                      \
    if (prwlock = (*rwlock); prwlock <= THR_RWLOCK_DESTROYED) [[unlikely]] {                       \
        if (prwlock == THR_RWLOCK_INITIALIZER) {                                                   \
            int ret;                                                                               \
            ret = InitStatic(g_curthread, rwlock);                                                 \
            if (ret)                                                                               \
                return (ret);                                                                      \
        } else if (prwlock == THR_RWLOCK_DESTROYED) {                                              \
            return POSIX_EINVAL;                                                                   \
        }                                                                                          \
        prwlock = *rwlock;                                                                         \
    }

static int RwlockInit(PthreadRwlockT* rwlock, const PthreadRwlockAttrT* attr) {
    auto* prwlock = new (std::nothrow) PthreadRwlock{};
    if (prwlock == nullptr) {
        return POSIX_ENOMEM;
    }
    *rwlock = prwlock;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlock_destroy(PthreadRwlockT* rwlock) {
    PthreadRwlockT prwlock = *rwlock;
    if (prwlock == THR_RWLOCK_INITIALIZER) {
        return 0;
    }
    if (prwlock == THR_RWLOCK_DESTROYED) {
        return POSIX_EINVAL;
    }
    *rwlock = THR_RWLOCK_DESTROYED;
    delete prwlock;
    return 0;
}

static int InitStatic(Pthread* thread, PthreadRwlockT* rwlock) {
    std::scoped_lock lk{RwlockStaticLock};
    if (*rwlock == THR_RWLOCK_INITIALIZER) {
        return RwlockInit(rwlock, nullptr);
    }
    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlock_init(PthreadRwlockT* rwlock, const PthreadRwlockAttrT* attr) {
    *rwlock = nullptr;
    return RwlockInit(rwlock, attr);
}

int PthreadRwlock::Rdlock(const OrbisKernelTimespec* abstime) {
    Pthread* curthread = g_curthread;

    /*
     * POSIX said the validity of the abstimeout parameter need
     * not be checked if the lock can be immediately acquired.
     */
    if (lock.try_lock_shared()) {
        curthread->rdlock_count++;
        return 0;
    }
    if (abstime && (abstime->tv_nsec >= 1000000000 || abstime->tv_nsec < 0)) [[unlikely]] {
        return POSIX_EINVAL;
    }

    // Note: On interruption an attempt to relock the mutex is made.
    if (abstime != nullptr) {
        if (!lock.try_lock_shared_until(abstime->TimePoint())) {
            return POSIX_ETIMEDOUT;
        }
    } else {
        lock.lock_shared();
    }

    curthread->rdlock_count++;
    return 0;
}

int PthreadRwlock::Wrlock(const OrbisKernelTimespec* abstime) {
    Pthread* curthread = g_curthread;

    /*
     * POSIX said the validity of the abstimeout parameter need
     * not be checked if the lock can be immediately acquired.
     */
    if (lock.try_lock()) {
        owner = curthread;
        return 0;
    }

    if (abstime && (abstime->tv_nsec >= 1000000000 || abstime->tv_nsec < 0)) {
        return POSIX_EINVAL;
    }

    // Note: On interruption an attempt to relock the mutex is made.
    if (abstime != nullptr) {
        if (!lock.try_lock_until(abstime->TimePoint())) {
            return POSIX_ETIMEDOUT;
        }
    } else {
        lock.lock();
    }

    owner = curthread;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlock_rdlock(PthreadRwlockT* rwlock) {
    PthreadRwlockT prwlock{};
    CHECK_AND_INIT_RWLOCK
    return prwlock->Rdlock(nullptr);
}

int PS4_SYSV_ABI posix_pthread_rwlock_timedrdlock(PthreadRwlockT* rwlock,
                                                  const OrbisKernelTimespec* abstime) {
    PthreadRwlockT prwlock{};
    CHECK_AND_INIT_RWLOCK
    return prwlock->Rdlock(abstime);
}

int PS4_SYSV_ABI posix_pthread_rwlock_tryrdlock(PthreadRwlockT* rwlock) {
    Pthread* curthread = g_curthread;
    PthreadRwlockT prwlock{};
    CHECK_AND_INIT_RWLOCK

    if (!prwlock->lock.try_lock_shared()) {
        return POSIX_EBUSY;
    }

    curthread->rdlock_count++;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlock_trywrlock(PthreadRwlockT* rwlock) {
    Pthread* curthread = g_curthread;
    PthreadRwlockT prwlock{};
    CHECK_AND_INIT_RWLOCK

    if (!prwlock->lock.try_lock()) {
        return POSIX_EBUSY;
    }
    prwlock->owner = curthread;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlock_wrlock(PthreadRwlockT* rwlock) {
    PthreadRwlockT prwlock{};
    CHECK_AND_INIT_RWLOCK
    return prwlock->Wrlock(nullptr);
}

int PS4_SYSV_ABI posix_pthread_rwlock_timedwrlock(PthreadRwlockT* rwlock,
                                                  const OrbisKernelTimespec* abstime) {
    PthreadRwlockT prwlock{};
    CHECK_AND_INIT_RWLOCK
    return prwlock->Wrlock(abstime);
}

int PS4_SYSV_ABI posix_pthread_rwlock_unlock(PthreadRwlockT* rwlock) {
    Pthread* curthread = g_curthread;
    PthreadRwlockT prwlock = *rwlock;
    if (prwlock <= THR_RWLOCK_DESTROYED) [[unlikely]] {
        return POSIX_EINVAL;
    }

    if (prwlock->owner == curthread) {
        prwlock->owner = nullptr;
        prwlock->lock.unlock();
    } else {
        if (prwlock->owner == nullptr) {
            curthread->rdlock_count--;
        }
        prwlock->lock.unlock_shared();
    }

    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlockattr_destroy(PthreadRwlockAttrT* rwlockattr) {
    if (rwlockattr == nullptr) {
        return POSIX_EINVAL;
    }
    PthreadRwlockAttrT prwlockattr = *rwlockattr;
    if (prwlockattr == nullptr) {
        return POSIX_EINVAL;
    }

    delete prwlockattr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlockattr_getpshared(const PthreadRwlockAttrT* rwlockattr,
                                                     int* pshared) {
    *pshared = (*rwlockattr)->pshared;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlockattr_init(PthreadRwlockAttrT* rwlockattr) {
    if (rwlockattr == nullptr) {
        return POSIX_EINVAL;
    }

    auto prwlockattr = new (std::nothrow) PthreadRwlockAttr{};
    if (prwlockattr == nullptr) {
        return POSIX_ENOMEM;
    }

    prwlockattr->pshared = 0;
    *rwlockattr = prwlockattr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlockattr_setpshared(PthreadRwlockAttrT* rwlockattr, int pshared) {
    /* Only PTHREAD_PROCESS_PRIVATE is supported. */
    if (pshared != 0) {
        return POSIX_EINVAL;
    }

    (*rwlockattr)->pshared = pshared;
    return 0;
}

void RegisterRwlock(Core::Loader::SymbolsResolver* sym) {
    // Posix-Kernel
    LIB_FUNCTION("1471ajPzxh0", "libkernel", 1, "libkernel", posix_pthread_rwlock_destroy);
    LIB_FUNCTION("ytQULN-nhL4", "libkernel", 1, "libkernel", posix_pthread_rwlock_init);
    LIB_FUNCTION("iGjsr1WAtI0", "libkernel", 1, "libkernel", posix_pthread_rwlock_rdlock);
    LIB_FUNCTION("lb8lnYo-o7k", "libkernel", 1, "libkernel", posix_pthread_rwlock_timedrdlock);
    LIB_FUNCTION("9zklzAl9CGM", "libkernel", 1, "libkernel", posix_pthread_rwlock_timedwrlock);
    LIB_FUNCTION("SFxTMOfuCkE", "libkernel", 1, "libkernel", posix_pthread_rwlock_tryrdlock);
    LIB_FUNCTION("XhWHn6P5R7U", "libkernel", 1, "libkernel", posix_pthread_rwlock_trywrlock);
    LIB_FUNCTION("EgmLo6EWgso", "libkernel", 1, "libkernel", posix_pthread_rwlock_unlock);
    LIB_FUNCTION("sIlRvQqsN2Y", "libkernel", 1, "libkernel", posix_pthread_rwlock_wrlock);
    LIB_FUNCTION("qsdmgXjqSgk", "libkernel", 1, "libkernel", posix_pthread_rwlockattr_destroy);
    LIB_FUNCTION("VqEMuCv-qHY", "libkernel", 1, "libkernel", posix_pthread_rwlockattr_getpshared);
    LIB_FUNCTION("xFebsA4YsFI", "libkernel", 1, "libkernel", posix_pthread_rwlockattr_init);
    LIB_FUNCTION("OuKg+kRDD7U", "libkernel", 1, "libkernel", posix_pthread_rwlockattr_setpshared);

    // Posix
    LIB_FUNCTION("1471ajPzxh0", "libScePosix", 1, "libkernel", posix_pthread_rwlock_destroy);
    LIB_FUNCTION("ytQULN-nhL4", "libScePosix", 1, "libkernel", posix_pthread_rwlock_init);
    LIB_FUNCTION("iGjsr1WAtI0", "libScePosix", 1, "libkernel", posix_pthread_rwlock_rdlock);
    LIB_FUNCTION("lb8lnYo-o7k", "libScePosix", 1, "libkernel", posix_pthread_rwlock_timedrdlock);
    LIB_FUNCTION("9zklzAl9CGM", "libScePosix", 1, "libkernel", posix_pthread_rwlock_timedwrlock);
    LIB_FUNCTION("SFxTMOfuCkE", "libScePosix", 1, "libkernel", posix_pthread_rwlock_tryrdlock);
    LIB_FUNCTION("XhWHn6P5R7U", "libScePosix", 1, "libkernel", posix_pthread_rwlock_trywrlock);
    LIB_FUNCTION("EgmLo6EWgso", "libScePosix", 1, "libkernel", posix_pthread_rwlock_unlock);
    LIB_FUNCTION("sIlRvQqsN2Y", "libScePosix", 1, "libkernel", posix_pthread_rwlock_wrlock);
    LIB_FUNCTION("qsdmgXjqSgk", "libScePosix", 1, "libkernel", posix_pthread_rwlockattr_destroy);
    LIB_FUNCTION("VqEMuCv-qHY", "libScePosix", 1, "libkernel", posix_pthread_rwlockattr_getpshared);
    LIB_FUNCTION("xFebsA4YsFI", "libScePosix", 1, "libkernel", posix_pthread_rwlockattr_init);
    LIB_FUNCTION("OuKg+kRDD7U", "libScePosix", 1, "libkernel", posix_pthread_rwlockattr_setpshared);

    // Orbis
    LIB_FUNCTION("i2ifZ3fS2fo", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_rwlockattr_destroy));
    LIB_FUNCTION("LcOZBHGqbFk", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_rwlockattr_getpshared));
    LIB_FUNCTION("yOfGg-I1ZII", "libkernel", 1, "libkernel", ORBIS(posix_pthread_rwlockattr_init));
    LIB_FUNCTION("-ZvQH18j10c", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_rwlockattr_setpshared));
    LIB_FUNCTION("BB+kb08Tl9A", "libkernel", 1, "libkernel", ORBIS(posix_pthread_rwlock_destroy));
    LIB_FUNCTION("6ULAa0fq4jA", "libkernel", 1, "libkernel", ORBIS(posix_pthread_rwlock_init));
    LIB_FUNCTION("Ox9i0c7L5w0", "libkernel", 1, "libkernel", ORBIS(posix_pthread_rwlock_rdlock));
    LIB_FUNCTION("iPtZRWICjrM", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_rwlock_timedrdlock));
    LIB_FUNCTION("adh--6nIqTk", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_rwlock_timedwrlock));
    LIB_FUNCTION("XD3mDeybCnk", "libkernel", 1, "libkernel", ORBIS(posix_pthread_rwlock_tryrdlock));
    LIB_FUNCTION("bIHoZCTomsI", "libkernel", 1, "libkernel", ORBIS(posix_pthread_rwlock_trywrlock));
    LIB_FUNCTION("+L98PIbGttk", "libkernel", 1, "libkernel", ORBIS(posix_pthread_rwlock_unlock));
    LIB_FUNCTION("mqdNorrB+gI", "libkernel", 1, "libkernel", ORBIS(posix_pthread_rwlock_wrlock));
}

} // namespace Libraries::Kernel
