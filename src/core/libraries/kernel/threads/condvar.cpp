// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma clang optimize off
#include <cstring>
#include "common/assert.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

static std::mutex CondStaticLock;

#define THR_COND_INITIALIZER ((PthreadCond*)NULL)
#define THR_COND_DESTROYED ((PthreadCond*)1)

static constexpr PthreadCondAttr PhreadCondattrDefault = {
    .c_pshared = 0,
    .c_clockid = ClockId::Realtime,
};

static int CondInit(PthreadCondT* cond, const PthreadCondAttrT* cond_attr, const char* name) {
    auto* cvp = (PthreadCond*)malloc(sizeof(PthreadCond));
    std::memset(cvp, 0, sizeof(PthreadCond));
    std::construct_at(cvp);

    if (cvp == nullptr) {
        return POSIX_ENOMEM;
    }

    if (name) {
        cvp->name = name;
    } else {
        static int CondId = 0;
        cvp->name = fmt::format("Cond{}", CondId++);
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
    free(cvp);
    return 0;
}

static std::mutex sc_lock;
static std::unordered_map<void*, SleepQueue*> sc_table;

void _sleepq_lock(void* wchan) {
    sc_lock.lock();
}

void _sleepq_unlock(void* wchan) {
    sc_lock.unlock();
}

SleepQueue* _sleepq_lookup(void* wchan) {
    const auto it = sc_table.find(wchan);
    if (it != sc_table.end()) {
        return it->second;
    }
    return nullptr;
}

void _sleepq_add(void* wchan, Pthread* td) {
    SleepQueue* sq = _sleepq_lookup(wchan);
    if (sq != NULL) {
        sq->sq_freeq.push_front(td->sleepqueue);
    } else {
        sc_table.emplace(wchan, td->sleepqueue);
        td->sleepqueue->sq_wchan = wchan;
    }
    td->sleepqueue = nullptr;
    td->wchan = wchan;
    sq->sq_blocked.push_front(td);
}

bool _sleepq_remove(SleepQueue* sq, Pthread* td) {
    std::erase(sq->sq_blocked, td);
    if (sq->sq_blocked.empty()) {
        sc_table.erase(td->wchan);
        td->sleepqueue = sq;
        td->wchan = nullptr;
        return false;
    } else {
        td->sleepqueue = sq->sq_freeq.front();
        sq->sq_freeq.pop_front();
        td->wchan = nullptr;
        return true;
    }
}

void _sleepq_drop(SleepQueue* sq, void (*cb)(Pthread*, void* arg), void* arg) {
    if (sq->sq_blocked.empty()) {
        return;
    }
    Pthread* td = sq->sq_blocked.front();
    sc_table.erase(td->wchan);
    std::erase(sq->sq_blocked, td);

    cb(td, arg);
    td->sleepqueue = sq;
    td->wchan = nullptr;

    auto sq2 = sq->sq_freeq.begin();
    for (Pthread* td : sq->sq_blocked) {
        cb(td, arg);
        td->sleepqueue = *sq2;
        td->wchan = NULL;
        sq2++;
    }

    sq->sq_blocked.clear();
    sq->sq_freeq.clear();
}

/*static int cond_wait_user(PthreadCond *cvp, PthreadMutex* mp,
                          const OrbisKernelTimespec *abstime, int cancel) {
    Pthread* curthread = g_curthread;
    int	recurse;
    int	error;

    ASSERT_MSG(curthread->wchan == nullptr, "Thread was already on queue");
    //if (cancel)
        //_thr_testcancel(curthread);

    _sleepq_lock(cvp);
    cvp->has_user_waiters = 1;
    curthread->will_sleep = 1;
    _mutex_cv_unlock(mp, &recurse);
    curthread->mutex_obj = mp;
    _sleepq_add(cvp, curthread);
    for(;;) {
        _thr_clear_wake(curthread);
        _sleepq_unlock(cvp);

        if (cancel) {
            //_thr_cancel_enter2(curthread, 0);
            error = _thr_sleep(curthread, cvp->__clock_id, abstime);
            //_thr_cancel_leave(curthread, 0);
        } else {
            error = _thr_sleep(curthread, cvp->__clock_id, abstime);
        }

        _sleepq_lock(cvp);
        if (curthread->wchan == nullptr) {
            error = 0;
            break;
        } else if (cancel && curthread->ShouldCancel()) {
            SleepQueue* sq = _sleepq_lookup(cvp);
            cvp->has_user_waiters = _sleepq_remove(sq, curthread);
            _sleepq_unlock(cvp);
            curthread->mutex_obj = NULL;
            _mutex_cv_lock(mp, recurse);
            if (!THR_IN_CRITICAL(curthread))
                _pthread_exit(PTHREAD_CANCELED);
            else
                return (0);
        } else if (error == POSIX_ETIMEDOUT) {
            SleepQueue* sq = _sleepq_lookup(cvp);
            cvp->has_user_waiters = _sleepq_remove(sq, curthread);
            break;
        }
    }
    _sleepq_unlock(cvp);
    curthread->mutex_obj = NULL;
    _mutex_cv_lock(mp, recurse);
    return (error);
}*/

int PthreadCond::Wait(PthreadMutexT* mutex, const OrbisKernelTimespec* abstime) {
    PthreadMutex* mp = *mutex;
    if (int error = mp->IsOwned(g_curthread); error != 0) {
        return error;
    }

    //_thr_testcancel(curthread);
    //_thr_cancel_enter2(curthread, 0);
    if (abstime) {
        const auto status = cond.wait_until(*mp, abstime->TimePoint());
        return status == std::cv_status::timeout ? POSIX_ETIMEDOUT : 0;
    } else {
        cond.wait(*mp);
        return 0;
    }
    //_thr_cancel_leave(curthread, 0);
}

int PthreadCond::Wait(PthreadMutexT* mutex, u64 usec) {
    PthreadMutex* mp = *mutex;
    if (int error = mp->IsOwned(g_curthread); error != 0) {
        return error;
    }

    //_thr_testcancel(curthread);
    //_thr_cancel_enter2(curthread, 0);
    const auto status = cond.wait_for(*mp, std::chrono::microseconds(usec));
    return status == std::cv_status::timeout ? POSIX_ETIMEDOUT : 0;
    //_thr_cancel_leave(curthread, 0);
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
    return cvp->Wait(mutex, usec);
}

int PS4_SYSV_ABI posix_pthread_cond_signal(PthreadCondT* cond) {
    PthreadCond* cvp{};
    CHECK_AND_INIT_COND
    cvp->cond.notify_one();
    return 0;
}

int PS4_SYSV_ABI posix_pthread_cond_broadcast(PthreadCondT* cond) {
    PthreadCond* cvp{};
    CHECK_AND_INIT_COND
    cvp->cond.notify_all();
    return 0;
}

int PS4_SYSV_ABI posix_pthread_condattr_init(PthreadCondAttrT* attr) {
    PthreadCondAttr* pattr = new PthreadCondAttr{};
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
    delete *attr;
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
    (*attr)->c_clockid = clock_id;
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
    LIB_FUNCTION("2Tb92quprl0", "libkernel", 1, "libkernel", 1, 1, ORBIS(scePthreadCondInit));
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
                 ORBIS(posix_pthread_cond_reltimedwait_np));
    LIB_FUNCTION("g+PZd2hiacg", "libkernel", 1, "libkernel", 1, 1,
                 ORBIS(posix_pthread_cond_destroy));
}

} // namespace Libraries::Kernel
