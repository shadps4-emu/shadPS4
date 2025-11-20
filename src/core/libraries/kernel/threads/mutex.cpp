// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>
#include "common/assert.h"
#include "common/types.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/threads/pthread.h"
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

static int MutexInit(PthreadMutexT* mutex, const PthreadMutexAttr* mutex_attr, const char* name) {
    const PthreadMutexAttr* attr;
    if (mutex_attr == nullptr) {
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
    auto* pmutex = new (std::nothrow) PthreadMutex{};
    if (pmutex == nullptr) {
        return POSIX_ENOMEM;
    }

    if (name) {
        pmutex->name = name;
    } else {
        static int MutexId = 0;
        pmutex->name = fmt::format("Mutex{}", MutexId++);
    }

    pmutex->m_flags = PthreadMutexFlags(attr->m_type);
    pmutex->m_owner = nullptr;
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
        return MutexInit(mutex, &PthreadMutexattrDefault, nullptr);
    } else if (*mutex == THR_ADAPTIVE_MUTEX_INITIALIZER) {
        return MutexInit(mutex, &PthreadMutexattrAdaptiveDefault, nullptr);
    }
    return 0;
}

int PS4_SYSV_ABI posix_pthread_mutex_init(PthreadMutexT* mutex,
                                          const PthreadMutexAttrT* mutex_attr) {
    return MutexInit(mutex, mutex_attr ? *mutex_attr : nullptr, nullptr);
}

int PS4_SYSV_ABI scePthreadMutexInit(PthreadMutexT* mutex, const PthreadMutexAttrT* mutex_attr,
                                     const char* name) {
    return MutexInit(mutex, mutex_attr ? *mutex_attr : nullptr, name);
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
    delete m;
    return 0;
}

int PthreadMutex::SelfTryLock() {
    switch (Type()) {
    case PthreadMutexType::ErrorCheck:
    case PthreadMutexType::Normal:
    case PthreadMutexType::AdaptiveNp:
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

int PthreadMutex::SelfLock(const OrbisKernelTimespec* abstime, u64 usec) {
    const auto DoSleep = [&] {
        if (abstime == THR_RELTIME) {
            std::this_thread::sleep_for(std::chrono::microseconds(usec));
            return POSIX_ETIMEDOUT;
        } else {
            if (abstime->tv_sec < 0 || abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000) {
                return POSIX_EINVAL;
            } else {
                std::this_thread::sleep_until(abstime->TimePoint());
                return POSIX_ETIMEDOUT;
            }
        }
    };
    switch (Type()) {
    case PthreadMutexType::ErrorCheck:
    case PthreadMutexType::AdaptiveNp: {
        if (abstime) {
            return DoSleep();
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
            return DoSleep();
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

int PthreadMutex::Lock(const OrbisKernelTimespec* abstime, u64 usec) {
    Pthread* curthread = g_curthread;
    if (m_owner == curthread) {
        return SelfLock(abstime, usec);
    }

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
                m_owner = curthread;
                return 0;
            }
            CPU_SPINWAIT;
        }

        count = m_yieldloops;
        while (count--) {
            std::this_thread::yield();
            if (m_lock.try_lock()) {
                m_owner = curthread;
                return 0;
            }
        }
    }

    int ret = 0;
    if (abstime == nullptr) {
        m_lock.lock();
    } else if (abstime != THR_RELTIME && (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000))
        [[unlikely]] {
        ret = POSIX_EINVAL;
    } else {
        if (abstime == THR_RELTIME) {
            ret = m_lock.try_lock_for(std::chrono::microseconds(usec)) ? 0 : POSIX_ETIMEDOUT;
        } else {
            ret = m_lock.try_lock_until(abstime->TimePoint()) ? 0 : POSIX_ETIMEDOUT;
        }
    }
    if (ret == 0) {
        m_owner = curthread;
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
        m_owner = curthread;
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
    UNREACHABLE();
    return (*mutex)->Lock(abstime);
}

int PS4_SYSV_ABI posix_pthread_mutex_reltimedlock_np(PthreadMutexT* mutex, u64 usec) {
    CHECK_AND_INIT_MUTEX
    return (*mutex)->Lock(THR_RELTIME, usec);
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
        const bool deferred = True(m_flags & PthreadMutexFlags::Deferred);
        m_flags &= ~PthreadMutexFlags::Deferred;

        m_owner = nullptr;
        m_lock.unlock();

        if (curthread->will_sleep == 0 && deferred) {
            curthread->WakeAll();
        }
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

int PthreadMutex::IsOwned(Pthread* curthread) const {
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
    auto pattr = new (std::nothrow) PthreadMutexAttr{};
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
    delete *attr;
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
    LIB_FUNCTION("ttHNfU+qDBU", "libScePosix", 1, "libkernel", posix_pthread_mutex_init);
    LIB_FUNCTION("7H0iTOciTLo", "libScePosix", 1, "libkernel", posix_pthread_mutex_lock);
    LIB_FUNCTION("Io9+nTKXZtA", "libScePosix", 1, "libkernel", posix_pthread_mutex_timedlock);
    LIB_FUNCTION("2Z+PpY6CaJg", "libScePosix", 1, "libkernel", posix_pthread_mutex_unlock);
    LIB_FUNCTION("ltCfaGr2JGE", "libScePosix", 1, "libkernel", posix_pthread_mutex_destroy);
    LIB_FUNCTION("dQHWEsJtoE4", "libScePosix", 1, "libkernel", posix_pthread_mutexattr_init);
    LIB_FUNCTION("mDmgMOGVUqg", "libScePosix", 1, "libkernel", posix_pthread_mutexattr_settype);
    LIB_FUNCTION("5txKfcMUAok", "libScePosix", 1, "libkernel", posix_pthread_mutexattr_setprotocol);
    LIB_FUNCTION("HF7lK46xzjY", "libScePosix", 1, "libkernel", posix_pthread_mutexattr_destroy);
    LIB_FUNCTION("K-jXhbt2gn4", "libScePosix", 1, "libkernel", posix_pthread_mutex_trylock);

    // Posix-Kernel
    LIB_FUNCTION("ttHNfU+qDBU", "libkernel", 1, "libkernel", posix_pthread_mutex_init);
    LIB_FUNCTION("7H0iTOciTLo", "libkernel", 1, "libkernel", posix_pthread_mutex_lock);
    LIB_FUNCTION("2Z+PpY6CaJg", "libkernel", 1, "libkernel", posix_pthread_mutex_unlock);
    LIB_FUNCTION("ltCfaGr2JGE", "libkernel", 1, "libkernel", posix_pthread_mutex_destroy);
    LIB_FUNCTION("dQHWEsJtoE4", "libkernel", 1, "libkernel", posix_pthread_mutexattr_init);
    LIB_FUNCTION("mDmgMOGVUqg", "libkernel", 1, "libkernel", posix_pthread_mutexattr_settype);
    LIB_FUNCTION("HF7lK46xzjY", "libkernel", 1, "libkernel", posix_pthread_mutexattr_destroy);
    LIB_FUNCTION("K-jXhbt2gn4", "libkernel", 1, "libkernel", posix_pthread_mutex_trylock);

    // Orbis
    LIB_FUNCTION("cmo1RIYva9o", "libkernel", 1, "libkernel", ORBIS(scePthreadMutexInit));
    LIB_FUNCTION("2Of0f+3mhhE", "libkernel", 1, "libkernel", ORBIS(posix_pthread_mutex_destroy));
    LIB_FUNCTION("F8bUHwAG284", "libkernel", 1, "libkernel", ORBIS(posix_pthread_mutexattr_init));
    LIB_FUNCTION("smWEktiyyG0", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_mutexattr_destroy));
    LIB_FUNCTION("iMp8QpE+XO4", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_mutexattr_settype));
    LIB_FUNCTION("1FGvU0i9saQ", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_mutexattr_setprotocol));
    LIB_FUNCTION("9UK1vLZQft4", "libkernel", 1, "libkernel", ORBIS(posix_pthread_mutex_lock));
    LIB_FUNCTION("tn3VlD0hG60", "libkernel", 1, "libkernel", ORBIS(posix_pthread_mutex_unlock));
    LIB_FUNCTION("upoVrzMHFeE", "libkernel", 1, "libkernel", ORBIS(posix_pthread_mutex_trylock));
    LIB_FUNCTION("IafI2PxcPnQ", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_mutex_reltimedlock_np));
    LIB_FUNCTION("qH1gXoq71RY", "libkernel", 1, "libkernel", ORBIS(posix_pthread_mutex_init));
    LIB_FUNCTION("n2MMpvU8igI", "libkernel", 1, "libkernel", ORBIS(posix_pthread_mutexattr_init));
}

} // namespace Libraries::Kernel
