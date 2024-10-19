// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/scope_exit.h"
#include "common/types.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/libkernel.h"
#include "core/libraries/kernel/threads/threads.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

static constexpr u32 MUTEX_ADAPTIVE_SPINS = 2000;
static std::mutex MutxStaticLock;

#define THR_MUTEX_INITIALIZER ((PthreadMutex*)NULL)
#define THR_ADAPTIVE_MUTEX_INITIALIZER ((PthreadMutex*)1)
#define THR_MUTEX_DESTROYED ((PthreadMutex*)2)

#define CPU_SPINWAIT __asm__ volatile("pause")

#define CHECK_AND_INIT_MUTEX                                                                       \
    if (PthreadMutex* m = *mutex; m <= THR_MUTEX_DESTROYED) [[unlikely]] {                         \
        if (m == THR_MUTEX_DESTROYED) {                                                            \
            return POSIX_EINVAL;                                                                   \
        }                                                                                          \
        if (s32 ret = InitStatic(g_curthread, mutex); ret) {                                       \
            return ret;                                                                            \
        }                                                                                          \
        m = *mutex;                                                                                \
    }

static constexpr PthreadMutexAttr PthreadMutexattrDefault = {
    .m_type = PthreadMutexType::ErrorCheck, .m_protocol = PthreadMutexProt::None, .m_ceiling = 0};

static constexpr PthreadMutexAttr PthreadMutexattrAdaptiveDefault = {
    .m_type = PthreadMutexType::AdaptiveNp, .m_protocol = PthreadMutexProt::None, .m_ceiling = 0};

using CallocFun = void* (*)(size_t, size_t);

static int MutexInit(PthreadMutexT* mutex, const PthreadMutexAttr* mutex_attr,
                     CallocFun calloc_cb) {
    const PthreadMutexAttr* attr;
    if (mutex_attr == NULL) {
        attr = &PthreadMutexattrDefault;
    } else {
        attr = mutex_attr;
        if (attr->m_type < PthreadMutexType::ErrorCheck || attr->m_type >= PthreadMutexType::Max) {
            return POSIX_EINVAL;
        }
        if (attr->m_protocol > PthreadMutexProt::Protect) {
            return POSIX_EINVAL;
        }
    }
    PthreadMutex* pmutex = (PthreadMutex*)calloc(1, sizeof(PthreadMutex));
    if (pmutex == nullptr) {
        return POSIX_ENOMEM;
    }

    std::construct_at(pmutex);
    pmutex->m_flags = PthreadMutexFlags(attr->m_type);
    pmutex->m_owner = NULL;
    pmutex->m_count = 0;
    pmutex->m_spinloops = 0;
    pmutex->m_yieldloops = 0;
    pmutex->m_protocol = attr->m_protocol;
    if (attr->m_type == PthreadMutexType::AdaptiveNp) {
        pmutex->m_spinloops = MUTEX_ADAPTIVE_SPINS;
        // pmutex->m_yieldloops = _thr_yieldloops;
    }

    *mutex = pmutex;
    return 0;
}

static int InitStatic(Pthread* thread, PthreadMutexT* mutex) {
    std::scoped_lock lk{MutxStaticLock};

    if (*mutex == THR_MUTEX_INITIALIZER) {
        return MutexInit(mutex, &PthreadMutexattrDefault, calloc);
    } else if (*mutex == THR_ADAPTIVE_MUTEX_INITIALIZER) {
        return MutexInit(mutex, &PthreadMutexattrAdaptiveDefault, calloc);
    }
    return 0;
}

int PS4_SYSV_ABI posix_pthread_mutex_init(PthreadMutexT* mutex,
                                          const PthreadMutexAttrT* mutex_attr) {
    return MutexInit(mutex, mutex_attr ? *mutex_attr : nullptr, calloc);
}

int PS4_SYSV_ABI posix_pthread_mutex_destroy(PthreadMutexT* mutex) {
    PthreadMutexT m = *mutex;
    if (m < THR_MUTEX_DESTROYED) {
        return 0;
    }
    if (m == THR_MUTEX_DESTROYED) {
        return POSIX_EINVAL;
    }
    if (m->m_owner != nullptr) {
        return POSIX_EBUSY;
    }
    *mutex = THR_MUTEX_DESTROYED;
    std::destroy_at(m);
    free(m);
    return 0;
}

int PthreadMutex::SelfTryLock() {
    switch (Type()) {
    case PthreadMutexType::ErrorCheck:
    case PthreadMutexType::Normal:
        return POSIX_EBUSY;
    case PthreadMutexType::Recursive: {
        /* Increment the lock count: */
        if (m_count + 1 > 0) {
            m_count++;
            return 0;
        }
        return POSIX_EAGAIN;
    }
    default:
        return POSIX_EINVAL;
    }
}

int PthreadMutex::SelfLock(const OrbisKernelTimespec* abstime) {
    switch (Type()) {
    case PthreadMutexType::ErrorCheck:
    case PthreadMutexType::AdaptiveNp: {
        if (abstime) {
            if (abstime->tv_sec < 0 || abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000) {
                return POSIX_EINVAL;
            } else {
                std::this_thread::sleep_until(abstime->TimePoint());
                return POSIX_ETIMEDOUT;
            }
        }
        /*
         * POSIX specifies that mutexes should return
         * EDEADLK if a recursive lock is detected.
         */
        return POSIX_EDEADLK;
    }
    case PthreadMutexType::Normal: {
        /*
         * What SS2 define as a 'normal' mutex.  Intentionally
         * deadlock on attempts to get a lock you already own.
         */
        if (abstime) {
            if (abstime->tv_sec < 0 || abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000) {
                return POSIX_EINVAL;
            } else {
                std::this_thread::sleep_until(abstime->TimePoint());
                return POSIX_ETIMEDOUT;
            }
        }
        UNREACHABLE_MSG("Mutex deadlock occured");
        return 0;
    }
    case PthreadMutexType::Recursive: {
        /* Increment the lock count: */
        if (m_count + 1 > 0) {
            m_count++;
            return 0;
        }
        return POSIX_EAGAIN;
    }
    default:
        return POSIX_EINVAL;
    }
}

int PthreadMutex::Lock(const OrbisKernelTimespec* abstime) {
    Pthread* curthread = g_curthread;
    if (m_owner == curthread) {
        return SelfLock(abstime);
    }

    int ret = 0;
    SCOPE_EXIT {
        if (ret == 0) {
            curthread->Enqueue(this);
        }
    };

    /*
     * For adaptive mutexes, spin for a bit in the expectation
     * that if the application requests this mutex type then
     * the lock is likely to be released quickly and it is
     * faster than entering the kernel
     */
    if (m_protocol == PthreadMutexProt::None) [[likely]] {
        int count = m_spinloops;
        while (count--) {
            if (m_lock.try_lock()) {
                return 0;
            }
            CPU_SPINWAIT;
        }

        count = m_yieldloops;
        while (count--) {
            std::this_thread::yield();
            if (m_lock.try_lock()) {
                return 0;
            }
        }
    }

    if (abstime == nullptr) {
        m_lock.lock();
    } else if (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000) [[unlikely]] {
        ret = POSIX_EINVAL;
    } else {
        ret = m_lock.try_lock_until(abstime->TimePoint()) ? 0 : POSIX_ETIMEDOUT;
    }
    return ret;
}

int PthreadMutex::TryLock() {
    Pthread* curthread = g_curthread;
    if (m_owner == curthread) {
        return SelfTryLock();
    }
    const int ret = m_lock.try_lock() ? 0 : POSIX_EBUSY;
    if (ret == 0) {
        curthread->Enqueue(this);
    }
    return ret;
}

int PS4_SYSV_ABI posix_pthread_mutex_trylock(PthreadMutexT* mutex) {
    CHECK_AND_INIT_MUTEX
    return (*mutex)->TryLock();
}

int PS4_SYSV_ABI posix_pthread_mutex_lock(PthreadMutexT* mutex) {
    CHECK_AND_INIT_MUTEX
    return (*mutex)->Lock(nullptr);
}

int PS4_SYSV_ABI posix_pthread_mutex_timedlock(PthreadMutexT* mutex,
                                               const OrbisKernelTimespec* abstime) {
    CHECK_AND_INIT_MUTEX
    return (*mutex)->Lock(abstime);
}

int PthreadMutex::Unlock() {
    Pthread* curthread = g_curthread;
    /*
     * Check if the running thread is not the owner of the mutex.
     */
    if (m_owner != curthread) [[unlikely]] {
        return POSIX_EPERM;
    }

    if (Type() == PthreadMutexType::Recursive && m_count > 0) [[unlikely]] {
        m_count--;
    } else {
        curthread->Dequeue(this);
        m_lock.unlock();
    }
    return 0;
}

int PS4_SYSV_ABI posix_pthread_mutex_unlock(PthreadMutexT* mutex) {
    PthreadMutex* mp = *mutex;
    if (mp <= THR_MUTEX_DESTROYED) [[unlikely]] {
        if (mp == THR_MUTEX_DESTROYED) {
            return POSIX_EINVAL;
        }
        return POSIX_EPERM;
    }
    return mp->Unlock();
}

int PS4_SYSV_ABI posix_pthread_mutex_getspinloops_np(PthreadMutexT* mutex, int* count) {
    CHECK_AND_INIT_MUTEX
    *count = (*mutex)->m_spinloops;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_mutex_setspinloops_np(PthreadMutexT* mutex, int count) {
    CHECK_AND_INIT_MUTEX(*mutex)->m_spinloops = count;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_mutex_getyieldloops_np(PthreadMutexT* mutex, int* count) {
    CHECK_AND_INIT_MUTEX
    *count = (*mutex)->m_yieldloops;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_mutex_setyieldloops_np(PthreadMutexT* mutex, int count) {
    CHECK_AND_INIT_MUTEX(*mutex)->m_yieldloops = count;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_mutex_isowned_np(PthreadMutexT* mutex) {
    PthreadMutex* m = *mutex;
    if (m <= THR_MUTEX_DESTROYED) {
        return 0;
    }
    return m->m_owner == g_curthread;
}

bool PthreadMutex::IsOwned(Pthread* curthread) const {
    if (this <= THR_MUTEX_DESTROYED) [[unlikely]] {
        if (this == THR_MUTEX_DESTROYED) {
            return POSIX_EINVAL;
        }
        return POSIX_EPERM;
    }
    if (m_owner != curthread) {
        return POSIX_EPERM;
    }
    return 0;
}

int PS4_SYSV_ABI posix_pthread_mutexattr_init(PthreadMutexAttrT* attr) {
    PthreadMutexAttrT pattr = (PthreadMutexAttrT)malloc(sizeof(PthreadMutexAttr));
    if (pattr == nullptr) {
        return POSIX_ENOMEM;
    }
    memcpy(pattr, &PthreadMutexattrDefault, sizeof(PthreadMutexAttr));
    *attr = pattr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_mutexattr_setkind_np(PthreadMutexAttrT* attr,
                                                    PthreadMutexType kind) {
    if (attr == nullptr || *attr == nullptr) {
        *__Error() = POSIX_EINVAL;
        return -1;
    }
    (*attr)->m_type = kind;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_mutexattr_getkind_np(PthreadMutexAttrT attr) {
    if (attr == nullptr) {
        *__Error() = POSIX_EINVAL;
        return -1;
    }
    return static_cast<int>(attr->m_type);
}

int PS4_SYSV_ABI posix_pthread_mutexattr_settype(PthreadMutexAttrT* attr, PthreadMutexType type) {
    if (attr == nullptr || *attr == nullptr || type >= PthreadMutexType::Max) {
        return POSIX_EINVAL;
    }
    (*attr)->m_type = type;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_mutexattr_gettype(PthreadMutexAttrT* attr, PthreadMutexType* type) {
    if (attr == nullptr || *attr == nullptr || (*attr)->m_type >= PthreadMutexType::Max) {
        return POSIX_EINVAL;
    }
    *type = (*attr)->m_type;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_mutexattr_destroy(PthreadMutexAttrT* attr) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    free(*attr);
    *attr = nullptr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_mutexattr_getprotocol(PthreadMutexAttrT* mattr,
                                                     PthreadMutexProt* protocol) {
    if (mattr == nullptr || *mattr == nullptr) {
        return POSIX_EINVAL;
    }
    *protocol = (*mattr)->m_protocol;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_mutexattr_setprotocol(PthreadMutexAttrT* mattr,
                                                     PthreadMutexProt protocol) {
    if (mattr == nullptr || *mattr == nullptr || (protocol < PthreadMutexProt::None) ||
        (protocol > PthreadMutexProt::Protect)) {
        return POSIX_EINVAL;
    }
    (*mattr)->m_protocol = protocol;
    //(*mattr)->m_ceiling = THR_MAX_RR_PRIORITY;
    return 0;
}

void RegisterMutex(Core::Loader::SymbolsResolver* sym) {
    // Posix
    LIB_FUNCTION("ttHNfU+qDBU", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_mutex_init);
    LIB_FUNCTION("7H0iTOciTLo", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_mutex_lock);
    LIB_FUNCTION("2Z+PpY6CaJg", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_mutex_unlock);
    LIB_FUNCTION("ltCfaGr2JGE", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_mutex_destroy);
    LIB_FUNCTION("dQHWEsJtoE4", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_mutexattr_init);
    LIB_FUNCTION("mDmgMOGVUqg", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_mutexattr_settype);
    LIB_FUNCTION("5txKfcMUAok", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_mutexattr_setprotocol);
    LIB_FUNCTION("HF7lK46xzjY", "libScePosix", 1, "libkernel", 1, 1,
                 posix_pthread_mutexattr_destroy);
    LIB_FUNCTION("K-jXhbt2gn4", "libScePosix", 1, "libkernel", 1, 1, posix_pthread_mutex_trylock);

    // Posix-Kernel
    LIB_FUNCTION("7H0iTOciTLo", "libkernel", 1, "libkernel", 1, 1, posix_pthread_mutex_lock);
    LIB_FUNCTION("2Z+PpY6CaJg", "libkernel", 1, "libkernel", 1, 1, posix_pthread_mutex_unlock);

    // Orbis
    LIB_FUNCTION("cmo1RIYva9o", "libkernel", 1, "libkernel", 1, 1, ORBIS(posix_pthread_mutex_init));
    LIB_FUNCTION("2Of0f+3mhhE", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_mutex_destroy));
    LIB_FUNCTION("F8bUHwAG284", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_mutexattr_init));
    LIB_FUNCTION("smWEktiyyG0", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_mutexattr_destroy));
    LIB_FUNCTION("iMp8QpE+XO4", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_mutexattr_settype));
    LIB_FUNCTION("1FGvU0i9saQ", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_mutexattr_setprotocol));
    LIB_FUNCTION("9UK1vLZQft4", "libkernel", 1, "libkernel", 1, 1, ORBIS(posix_pthread_mutex_lock));
    LIB_FUNCTION("tn3VlD0hG60", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_mutex_unlock));
    LIB_FUNCTION("upoVrzMHFeE", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_mutex_trylock));
    LIB_FUNCTION("IafI2PxcPnQ", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_mutex_timedlock));
    LIB_FUNCTION("qH1gXoq71RY", "libkernel", 1, "libkernel", 1, 1, ORBIS(posix_pthread_mutex_init));
    LIB_FUNCTION("n2MMpvU8igI", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_mutexattr_init));
}

} // namespace Libraries::Kernel
