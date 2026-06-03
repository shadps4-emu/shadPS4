// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_map>

#include "common/elf_info.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/threads/pthread.h"

namespace Libraries::Kernel {

static thread_local std::unordered_map<const PthreadRwlock*, u32> g_rdlock_depth;

// Returns true if the calling thread already holds a read lock on lock
static bool ReenterReadLock(const PthreadRwlock* lock) {
    const auto it = g_rdlock_depth.find(lock);
    if (it != g_rdlock_depth.end() && it->second > 0) {
        ++it->second;
        return true;
    }
    return false;
}

// Records the first (outermost) read-lock acquisition by the calling thread.
static void EnterReadLock(const PthreadRwlock* lock) {
    g_rdlock_depth[lock] = 1;
}

// Drops one level of read recursion.  Returns true when the underlying shared
// mutex should actually be released
static bool LeaveReadLock(const PthreadRwlock* lock) {
    const auto it = g_rdlock_depth.find(lock);
    if (it == g_rdlock_depth.end() || it->second == 0) {
        return true;
    }
    if (--it->second == 0) {
        g_rdlock_depth.erase(it);
        return true;
    }
    return false;
}

static std::mutex RwlockStaticLock;
static s32 sdk_version;

#define CHECK_AND_INIT_RWLOCK                                                                      \
    if (prwlock == nullptr || *rwlock == nullptr) {                                                \
        if (prwlock == nullptr) {                                                                  \
            return POSIX_EINVAL;                                                                   \
        }                                                                                          \
        std::lock_guard lk{RwlockStaticLock};                                                      \
        if (*rwlock == nullptr) {                                                                  \
            posix_pthread_rwlock_init(rwlock, nullptr);                                            \
        }                                                                                          \
        prwlock = *rwlock;                                                                         \
    }

int PS4_SYSV_ABI posix_pthread_rwlock_destroy(PthreadRwlockT* rwlock) {
    PthreadRwlockT prwlock = *rwlock;
    if (prwlock == nullptr) {
        return POSIX_EINVAL;
    }
    *rwlock = nullptr;
    delete prwlock;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlock_init(PthreadRwlockT* rwlock, const PthreadRwlockAttrT* attr) {
    *rwlock = new PthreadRwlock{};
    return 0;
}

int PthreadRwlock::Rdlock(const OrbisKernelTimespec* abstime) {
    Pthread* curthread = g_curthread;
    if (ReenterReadLock(this)) {
        curthread->rdlock_count++;
        return 0;
    }

    /*
     * POSIX said the validity of the abstimeout parameter need
     * not be checked if the lock can be immediately acquired.
     */
    if (lock.try_lock_shared()) {
        EnterReadLock(this);
        curthread->rdlock_count++;
        return 0;
    }

    if (abstime == nullptr) {
        lock.lock_shared();
    } else {
        const auto timeout = std::chrono::system_clock::time_point{
            std::chrono::seconds{abstime->tv_sec} + std::chrono::nanoseconds{abstime->tv_nsec}};
        if (!lock.try_lock_shared_until(timeout)) {
            return POSIX_ETIMEDOUT;
        }
    }

    EnterReadLock(this);
    curthread->rdlock_count++;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlock_rdlock(PthreadRwlockT* rwlock) {
    PthreadRwlockT prwlock = *rwlock;
    CHECK_AND_INIT_RWLOCK
    return prwlock->Rdlock(nullptr);
}

int PS4_SYSV_ABI posix_pthread_rwlock_timedrdlock(PthreadRwlockT* rwlock,
                                                  const OrbisKernelTimespec* abstime) {
    PthreadRwlockT prwlock = *rwlock;
    CHECK_AND_INIT_RWLOCK
    return prwlock->Rdlock(abstime);
}

int PS4_SYSV_ABI posix_pthread_rwlock_tryrdlock(PthreadRwlockT* rwlock) {
    Pthread* curthread = g_curthread;
    PthreadRwlockT prwlock = *rwlock;
    CHECK_AND_INIT_RWLOCK

    // Recursive read-lock: already held for read by this thread, succeed locally.
    if (ReenterReadLock(prwlock)) {
        curthread->rdlock_count++;
        return 0;
    }

    if (!prwlock->lock.try_lock_shared()) {
        return POSIX_EBUSY;
    }

    EnterReadLock(prwlock);
    curthread->rdlock_count++;
    return 0;
}

int PthreadRwlock::Wrlock(const OrbisKernelTimespec* abstime) {
    if (abstime == nullptr) {
        lock.lock();
    } else {
        const auto timeout = std::chrono::system_clock::time_point{
            std::chrono::seconds{abstime->tv_sec} + std::chrono::nanoseconds{abstime->tv_nsec}};
        if (!lock.try_lock_until(timeout)) {
            return POSIX_ETIMEDOUT;
        }
    }
    owner = g_curthread;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlock_wrlock(PthreadRwlockT* rwlock) {
    PthreadRwlockT prwlock = *rwlock;
    CHECK_AND_INIT_RWLOCK
    return prwlock->Wrlock(nullptr);
}

int PS4_SYSV_ABI posix_pthread_rwlock_timedwrlock(PthreadRwlockT* rwlock,
                                                  const OrbisKernelTimespec* abstime) {
    PthreadRwlockT prwlock = *rwlock;
    CHECK_AND_INIT_RWLOCK
    return prwlock->Wrlock(abstime);
}

int PS4_SYSV_ABI posix_pthread_rwlock_trywrlock(PthreadRwlockT* rwlock) {
    PthreadRwlockT prwlock = *rwlock;
    CHECK_AND_INIT_RWLOCK
    if (!prwlock->lock.try_lock()) {
        return POSIX_EBUSY;
    }
    prwlock->owner = g_curthread;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlock_unlock(PthreadRwlockT* rwlock) {
    Pthread* curthread = g_curthread;
    PthreadRwlockT prwlock = *rwlock;
    if (prwlock == nullptr) {
        return POSIX_EINVAL;
    }

    if (prwlock->owner != nullptr) {
        // Write lock held by this thread.
        ASSERT(prwlock->owner == curthread);
        prwlock->owner = nullptr;
        prwlock->lock.unlock();
    } else {
        // Read lock held by this thread.
        curthread->rdlock_count--;
        if (LeaveReadLock(prwlock)) {
            prwlock->lock.unlock_shared();
        }
    }

    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlockattr_init(PthreadRwlockAttrT* rwlockattr) {
    *rwlockattr = new PthreadRwlockAttr{};
    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlockattr_destroy(PthreadRwlockAttrT* rwlockattr) {
    delete *rwlockattr;
    *rwlockattr = nullptr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlockattr_getpshared(const PthreadRwlockAttrT* rwlockattr,
                                                     int* pshared) {
    *pshared = (*rwlockattr)->pshared;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_rwlockattr_setpshared(PthreadRwlockAttrT* rwlockattr, int pshared) {
    (*rwlockattr)->pshared = pshared;
    return 0;
}

} // namespace Libraries::Kernel
