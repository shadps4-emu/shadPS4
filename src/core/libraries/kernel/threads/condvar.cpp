// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <chrono>
#include <cstring>
#include "common/assert.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/kernel/threads/sleepq.h"
#include "core/libraries/kernel/threads/thread_state.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

static std::mutex CondStaticLock;

#define THR_COND_INITIALIZER ((PthreadCond*)nullptr)
#define THR_COND_DESTROYED ((PthreadCond*)1)

static constexpr PthreadCondAttr PthreadCondattrDefault = {
    .c_pshared = 0,
    .c_clockid = ClockId::Realtime,
};

static int CondInit(PthreadCondT* cond, const PthreadCondAttrT* cond_attr, const char* name) {
    auto* cvp = new (std::nothrow) PthreadCond{};
    if (cvp == nullptr) {
        return POSIX_ENOMEM;
    }

    if (name) {
        cvp->name = name;
    } else {
        static std::atomic<int> CondId{0};
        cvp->name = fmt::format("Cond{}", CondId.fetch_add(1));
    }

    if (cond_attr == nullptr || *cond_attr == nullptr) {
        cvp->clock_id = ClockId::Realtime;
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
    if (*cond == nullptr) {
        return CondInit(cond, nullptr, nullptr);
    }
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
        cvp = *cond;                                                                               \
    }

int PS4_SYSV_ABI posix_pthread_cond_init(PthreadCondT* cond, const PthreadCondAttrT* cond_attr) {
    *cond = nullptr;
    return CondInit(cond, cond_attr, nullptr);
}

int PS4_SYSV_ABI scePthreadCondInit(PthreadCondT* cond, const PthreadCondAttrT* cond_attr,
                                    const char* name) {
    *cond = nullptr;
    return CondInit(cond, cond_attr, name);
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
    delete cvp;
    return 0;
}

int PthreadCond::Wait(PthreadMutexT* mutex, const OrbisKernelTimespec* abstime, u64 usec) {
    PthreadMutex* mp = *mutex;
    if (const int error = mp->IsOwned(g_curthread); error != 0) {
        return error;
    }

    Pthread* curthread = g_curthread;
    ASSERT_MSG(curthread->wchan == nullptr, "Thread was already on queue.");
    PthreadTestCancel();
    SleepqLock(this);

    /*
     * set __has_user_waiters before unlocking mutex, this allows
     * us to check it without locking in pthread_cond_signal().
     */
    has_user_waiters = true;
    curthread->will_sleep = true;

    int recurse;
    mp->CvUnlock(&recurse);

    curthread->mutex_obj = mp;
    SleepqAdd(this, curthread);

    const bool is_reltime = abstime == THR_RELTIME;
    auto reltime_deadline = std::chrono::steady_clock::time_point{};
    if (is_reltime) {
        const auto now = std::chrono::steady_clock::now();
        const auto max_relative = std::chrono::duration_cast<std::chrono::microseconds>(
                                      std::chrono::steady_clock::time_point::max() - now)
                                      .count();
        reltime_deadline = usec > static_cast<u64>(max_relative)
                               ? std::chrono::steady_clock::time_point::max()
                               : now + std::chrono::microseconds(usec);
    }

    int error = 0;
    bool first_sleep = true;
    for (;;) {
        curthread->cancel_point = true;
        curthread->ClearWake();
        const bool cancel_pending = curthread->ShouldCancel();
        SleepqUnlock(this);

        if (cancel_pending) {
            error = 0;
        } else if (!is_reltime) {
            error = curthread->Sleep(abstime, usec, clock_id) ? 0 : POSIX_ETIMEDOUT;
        } else if (first_sleep) {
            first_sleep = false;
            error = curthread->Sleep(abstime, usec) ? 0 : POSIX_ETIMEDOUT;
        } else {
            const auto now = std::chrono::steady_clock::now();
            if (now >= reltime_deadline) {
                error = POSIX_ETIMEDOUT;
            } else {
                const auto remaining_us =
                    std::chrono::ceil<std::chrono::microseconds>(reltime_deadline - now);
                error = curthread->Sleep(abstime, static_cast<u64>(remaining_us.count()))
                            ? 0
                            : POSIX_ETIMEDOUT;
            }
        }
        SleepqLock(this);
        if (curthread->wchan == nullptr) {
            error = 0;
            break;
        } else if (curthread->ShouldCancel()) {
            if (SleepQueue* sq = SleepqLookup(this); sq != nullptr) {
                has_user_waiters = SleepqRemove(sq, curthread);
            } else {
                ASSERT_MSG(false, "Cancelled condition-variable waiter has no sleep queue");
                has_user_waiters = false;
            }
            SleepqUnlock(this);
            curthread->mutex_obj = nullptr;
            mp->CvLock(recurse);
            curthread->cancel_point = false;
            PthreadTestCancel();
            return 0;
        } else if (error == POSIX_ETIMEDOUT) {
            if (SleepQueue* sq = SleepqLookup(this); sq != nullptr) {
                has_user_waiters = SleepqRemove(sq, curthread);
            } else {
                ASSERT_MSG(false, "Timed-out condition-variable waiter has no sleep queue");
                has_user_waiters = false;
            }
            break;
        }
    }
    SleepqUnlock(this);
    curthread->mutex_obj = nullptr;
    const int error2 = mp->CvLock(recurse);
    curthread->cancel_point = false;
    PthreadCancelInterrupt();
    if (error == 0) {
        error = error2;
    }
    return error;
}

int PS4_SYSV_ABI posix_pthread_cond_wait(PthreadCondT* cond, PthreadMutexT* mutex) {
    PthreadCond* cvp{};
    CHECK_AND_INIT_COND
    return cvp->Wait(mutex, nullptr);
}

int PS4_SYSV_ABI posix_pthread_cond_timedwait(PthreadCondT* cond, PthreadMutexT* mutex,
                                              const OrbisKernelTimespec* abstime) {
    if (abstime == nullptr || abstime->tv_sec < 0 || abstime->tv_nsec < 0 ||
        abstime->tv_nsec >= 1000000000) {
        return POSIX_EINVAL;
    }

    PthreadCond* cvp{};
    CHECK_AND_INIT_COND
    return cvp->Wait(mutex, abstime);
}

int PS4_SYSV_ABI posix_pthread_cond_reltimedwait_np(PthreadCondT* cond, PthreadMutexT* mutex,
                                                    u64 usec) {
    PthreadCond* cvp{};
    CHECK_AND_INIT_COND
    return cvp->Wait(mutex, THR_RELTIME, usec);
}

int PthreadCond::Signal(Pthread* thread) {
    Pthread* curthread = g_curthread;
    if (thread) {
        auto* thread_state = ThrState::Instance();
        int ret = thread_state->FindThread(thread, false);
        if (ret != ORBIS_OK) {
            return ret;
        }
        thread->lock.unlock();
    }

    SleepqLock(this);
    SleepQueue* sq = SleepqLookup(this);
    if (sq == nullptr) {
        SleepqUnlock(this);
        return thread != nullptr ? 1 : 0;
    }

    ASSERT_MSG(!sq->sq_blocked.empty(),
               "Condition variable has a linked sleep queue with no blocked threads");
    if (sq->sq_blocked.empty()) [[unlikely]] {
        has_user_waiters = false;
        SleepqUnlock(this);
        return thread != nullptr ? 1 : 0;
    }

    Pthread* td{};
    if (thread != nullptr) {
        const auto it = std::find(sq->sq_blocked.begin(), sq->sq_blocked.end(), thread);
        if (it == sq->sq_blocked.end()) {
            SleepqUnlock(this);
            return 1;
        }
        td = *it;
    } else {
        td = sq->sq_blocked.front();
    }

    PthreadMutex* mp = td->mutex_obj;
    has_user_waiters = SleepqRemove(sq, td);

    WakeSemaphore* wake_address = &td->wake_sema;
    if (mp != nullptr && curthread != nullptr && mp->m_owner.load() == curthread) {
        if (curthread->nwaiter_defer >= Pthread::MaxDeferWaiters) {
            curthread->WakeAll();
        }
        curthread->defer_waiters[curthread->nwaiter_defer++] = &td->wake_sema;
        mp->m_flags |= PthreadMutexFlags::Deferred;
        wake_address = nullptr;
    }

    SleepqUnlock(this);
    if (wake_address != nullptr) {
        wake_address->release();
    }
    return 0;
}

struct BroadcastArg {
    Pthread* curthread;
    WakeSemaphore* immediate_wakes[Pthread::MaxDeferWaiters];
    int count;
};

int PthreadCond::Broadcast() {
    BroadcastArg ba;
    ba.curthread = g_curthread;
    ba.count = 0;

    const auto drop_cb = [](Pthread* td, void* arg) {
        auto* ba2 = static_cast<BroadcastArg*>(arg);
        Pthread* curthread = ba2->curthread;
        PthreadMutex* mp = td->mutex_obj;

        if (mp != nullptr && curthread != nullptr && mp->m_owner.load() == curthread) {
            if (curthread->nwaiter_defer >= Pthread::MaxDeferWaiters) {
                curthread->WakeAll();
            }
            curthread->defer_waiters[curthread->nwaiter_defer++] = &td->wake_sema;
            mp->m_flags |= PthreadMutexFlags::Deferred;
        } else {
            if (ba2->count >= Pthread::MaxDeferWaiters) {
                for (int i = 0; i < ba2->count; i++) {
                    ba2->immediate_wakes[i]->release();
                }
                ba2->count = 0;
            }
            ba2->immediate_wakes[ba2->count++] = &td->wake_sema;
        }
    };

    SleepqLock(this);
    SleepQueue* sq = SleepqLookup(this);
    if (sq == nullptr) {
        SleepqUnlock(this);
        return 0;
    }

    SleepqDrop(sq, drop_cb, &ba);
    has_user_waiters = false;
    SleepqUnlock(this);

    for (int i = 0; i < ba.count; i++) {
        ba.immediate_wakes[i]->release();
    }
    return 0;
}

int PS4_SYSV_ABI posix_pthread_cond_signal(PthreadCondT* cond) {
    PthreadCond* cvp{};
    CHECK_AND_INIT_COND
    return cvp->Signal(nullptr);
}

int PS4_SYSV_ABI posix_pthread_cond_signalto_np(PthreadCondT* cond, Pthread* thread) {
    PthreadCond* cvp{};
    CHECK_AND_INIT_COND
    return cvp->Signal(thread);
}

int PS4_SYSV_ABI posix_pthread_cond_broadcast(PthreadCondT* cond) {
    PthreadCond* cvp{};
    CHECK_AND_INIT_COND
    cvp->Broadcast();
    return 0;
}

int PS4_SYSV_ABI posix_pthread_condattr_init(PthreadCondAttrT* attr) {
    auto* pattr = new (std::nothrow) PthreadCondAttr{};
    if (pattr == nullptr) {
        return POSIX_ENOMEM;
    }
    memcpy(pattr, &PthreadCondattrDefault, sizeof(PthreadCondAttr));
    *attr = pattr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_condattr_destroy(PthreadCondAttrT* attr) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    delete *attr;
    *attr = nullptr;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_condattr_getclock(const PthreadCondAttrT* attr, ClockId* clock_id) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    *clock_id = (*attr)->c_clockid;
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
    (*attr)->c_clockid = clock_id;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_condattr_getpshared(const PthreadCondAttrT* attr, int* pshared) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    *pshared = 0;
    return 0;
}

int PS4_SYSV_ABI posix_pthread_condattr_setpshared(PthreadCondAttrT* attr, int pshared) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    if (pshared != 0) {
        return POSIX_EINVAL;
    }
    return 0;
}

void RegisterCond(Core::Loader::SymbolsResolver* sym) {
    // Posix
    LIB_FUNCTION("mKoTx03HRWA", "libScePosix", 1, "libkernel", posix_pthread_condattr_init);
    LIB_FUNCTION("3BpP850hBT4", "libScePosix", 1, "libkernel", posix_pthread_condattr_setpshared);
    LIB_FUNCTION("EjllaAqAPZo", "libScePosix", 1, "libkernel", posix_pthread_condattr_setclock);
    LIB_FUNCTION("h0qUqSuOmC8", "libScePosix", 1, "libkernel", posix_pthread_condattr_getpshared);
    LIB_FUNCTION("cTDYxTUNPhM", "libScePosix", 1, "libkernel", posix_pthread_condattr_getclock);
    LIB_FUNCTION("dJcuQVn6-Iw", "libScePosix", 1, "libkernel", posix_pthread_condattr_destroy);
    LIB_FUNCTION("0TyVk4MSLt0", "libScePosix", 1, "libkernel", posix_pthread_cond_init);
    LIB_FUNCTION("K953PF5u6Pc", "libScePosix", 1, "libkernel", posix_pthread_cond_reltimedwait_np);
    LIB_FUNCTION("27bAgiJmOh0", "libScePosix", 1, "libkernel", posix_pthread_cond_timedwait);
    LIB_FUNCTION("Op8TBGY5KHg", "libScePosix", 1, "libkernel", posix_pthread_cond_wait);
    LIB_FUNCTION("2MOy+rUfuhQ", "libScePosix", 1, "libkernel", posix_pthread_cond_signal);
    LIB_FUNCTION("CI6Qy73ae10", "libScePosix", 1, "libkernel", posix_pthread_cond_signalto_np);
    LIB_FUNCTION("mkx2fVhNMsg", "libScePosix", 1, "libkernel", posix_pthread_cond_broadcast);
    LIB_FUNCTION("RXXqi4CtF8w", "libScePosix", 1, "libkernel", posix_pthread_cond_destroy);

    // Posix-Kernel
    LIB_FUNCTION("mKoTx03HRWA", "libkernel", 1, "libkernel", posix_pthread_condattr_init);
    LIB_FUNCTION("3BpP850hBT4", "libkernel", 1, "libkernel", posix_pthread_condattr_setpshared);
    LIB_FUNCTION("EjllaAqAPZo", "libkernel", 1, "libkernel", posix_pthread_condattr_setclock);
    LIB_FUNCTION("h0qUqSuOmC8", "libkernel", 1, "libkernel", posix_pthread_condattr_getpshared);
    LIB_FUNCTION("cTDYxTUNPhM", "libkernel", 1, "libkernel", posix_pthread_condattr_getclock);
    LIB_FUNCTION("dJcuQVn6-Iw", "libkernel", 1, "libkernel", posix_pthread_condattr_destroy);
    LIB_FUNCTION("0TyVk4MSLt0", "libkernel", 1, "libkernel", posix_pthread_cond_init);
    LIB_FUNCTION("K953PF5u6Pc", "libkernel", 1, "libkernel", posix_pthread_cond_reltimedwait_np);
    LIB_FUNCTION("27bAgiJmOh0", "libkernel", 1, "libkernel", posix_pthread_cond_timedwait);
    LIB_FUNCTION("Op8TBGY5KHg", "libkernel", 1, "libkernel", posix_pthread_cond_wait);
    LIB_FUNCTION("2MOy+rUfuhQ", "libkernel", 1, "libkernel", posix_pthread_cond_signal);
    LIB_FUNCTION("CI6Qy73ae10", "libkernel", 1, "libkernel", posix_pthread_cond_signalto_np);
    LIB_FUNCTION("mkx2fVhNMsg", "libkernel", 1, "libkernel", posix_pthread_cond_broadcast);
    LIB_FUNCTION("RXXqi4CtF8w", "libkernel", 1, "libkernel", posix_pthread_cond_destroy);

    // Orbis
    LIB_FUNCTION("m5-2bsNfv7s", "libkernel", 1, "libkernel", ORBIS(posix_pthread_condattr_init));
    LIB_FUNCTION("6xMew9+rZwI", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_condattr_setpshared));
    LIB_FUNCTION("c-bxj027czs", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_condattr_setclock));
    LIB_FUNCTION("Dn-DRWi9t54", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_condattr_getpshared));
    LIB_FUNCTION("6qM3kO5S3Oo", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_condattr_getclock));
    LIB_FUNCTION("waPcxYiR3WA", "libkernel", 1, "libkernel", ORBIS(posix_pthread_condattr_destroy));
    LIB_FUNCTION("2Tb92quprl0", "libkernel", 1, "libkernel", ORBIS(scePthreadCondInit));
    LIB_FUNCTION("BmMjYxmew1w", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_cond_reltimedwait_np));
    LIB_FUNCTION("WKAXJ4XBPQ4", "libkernel", 1, "libkernel", ORBIS(posix_pthread_cond_wait));
    LIB_FUNCTION("kDh-NfxgMtE", "libkernel", 1, "libkernel", ORBIS(posix_pthread_cond_signal));
    LIB_FUNCTION("o69RpYO-Mu0", "libkernel", 1, "libkernel", ORBIS(posix_pthread_cond_signalto_np));
    LIB_FUNCTION("JGgj7Uvrl+A", "libkernel", 1, "libkernel", ORBIS(posix_pthread_cond_broadcast));
    LIB_FUNCTION("g+PZd2hiacg", "libkernel", 1, "libkernel", ORBIS(posix_pthread_cond_destroy));
}

} // namespace Libraries::Kernel
