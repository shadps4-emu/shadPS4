// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <pthread.h>
#include "common/assert.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/threads/thread_state.h"
#include "core/libraries/kernel/threads/threads.h"

namespace Libraries::Kernel {

void _thread_cleanupspecific();

static void ExitThread() {
    Pthread* curthread = g_curthread;

    /* Check if there is thread specific data: */
    if (curthread->specific != nullptr) {
        /* Run the thread-specific data destructors: */
        _thread_cleanupspecific();
    }

    auto* thread_state = ThrState::Instance();
    ASSERT(thread_state->active_threads.fetch_sub(1) != 1);

    curthread->lock->lock();
    curthread->state = PthreadState::Dead;
    ASSERT(False(curthread->flags & ThreadFlags::NeedSuspend));

    /*
     * Thread was created with initial refcount 1, we drop the
     * reference count to allow it to be garbage collected.
     */
    curthread->refcount--;
    thread_state->TryCollect(curthread, curthread); /* thread lock released */

    /*
     * Kernel will do wakeup at the address, so joiner thread
     * will be resumed if it is sleeping at the address.
     */
    curthread->tid.store(TidTerminated);
    curthread->tid.notify_all();

    pthread_exit(nullptr);
    UNREACHABLE();
    /* Never reach! */
}

void PS4_SYSV_ABI posix_pthread_exit(void* status) {
    Pthread* curthread = g_curthread;

    /* Check if this thread is already in the process of exiting: */
    ASSERT_MSG(!curthread->cancelling, "Thread {} has called pthread_exit from a destructor",
               fmt::ptr(curthread));

    /* Flag this thread as exiting. */
    curthread->cancelling = 1;
    curthread->no_cancel = 1;
    curthread->cancel_async = 0;
    curthread->cancel_point = 0;

    /* Save the return value: */
    curthread->ret = status;
    while (!curthread->cleanup.empty()) {
        PthreadCleanup* old = curthread->cleanup.front();
        curthread->cleanup.pop_front();
        old->routine(old->routine_arg);
        if (old->onheap) {
            free(old);
        }
    }

    ExitThread();
}

static int JoinThread(PthreadT pthread, void** thread_return, const OrbisKernelTimespec* abstime) {
    Pthread* curthread = g_curthread;

    if (pthread == nullptr) {
        return POSIX_EINVAL;
    }

    if (pthread == curthread) {
        return POSIX_EDEADLK;
    }

    auto* thread_state = ThrState::Instance();
    if (int ret = thread_state->FindThread(pthread, 1); ret != 0) {
        return POSIX_ESRCH;
    }

    int ret = 0;
    if (True(pthread->flags & ThreadFlags::Detached)) {
        ret = POSIX_EINVAL;
    } else if (pthread->joiner != nullptr) {
        /* Multiple joiners are not supported. */
        ret = POSIX_ENOTSUP;
    }
    if (ret) {
        pthread->lock->unlock();
        return ret;
    }
    /* Set the running thread to be the joiner: */
    pthread->joiner = curthread;
    pthread->lock->unlock();

    const auto backout_join = [](void* arg) {
        Pthread* pthread = (Pthread*)arg;
        std::scoped_lock lk{*pthread->lock};
        pthread->joiner = nullptr;
    };

    PthreadCleanup cup{backout_join, pthread, 0};
    curthread->cleanup.push_front(&cup);

    //_thr_cancel_enter(curthread);

    const int tid = pthread->tid;
    while (pthread->tid.load() != TidTerminated) {
        //_thr_testcancel(curthread);
        ASSERT(abstime == nullptr);
        pthread->tid.wait(tid);
    }

    //_thr_cancel_leave(curthread, 0);
    curthread->cleanup.pop_front();

    if (ret == POSIX_ETIMEDOUT) {
        backout_join(pthread);
        return ret;
    }

    void* tmp = pthread->ret;
    pthread->lock->lock();
    pthread->flags |= ThreadFlags::Detached;
    pthread->joiner = nullptr;
    thread_state->TryCollect(curthread, pthread); /* thread lock released */
    if (thread_return != nullptr) {
        *thread_return = tmp;
    }

    return 0;
}

int PS4_SYSV_ABI posix_pthread_join(PthreadT pthread, void** thread_return) {
    return JoinThread(pthread, thread_return, NULL);
}

int PS4_SYSV_ABI posix_pthread_timedjoin_np(PthreadT pthread, void** thread_return,
                                            const OrbisKernelTimespec* abstime) {
    if (abstime == nullptr || abstime->tv_sec < 0 || abstime->tv_nsec < 0 ||
        abstime->tv_nsec >= 1000000000) {
        return POSIX_EINVAL;
    }

    return JoinThread(pthread, thread_return, abstime);
}

int PS4_SYSV_ABI posix_pthread_detach(PthreadT pthread) {
    if (pthread == nullptr) {
        return POSIX_EINVAL;
    }

    auto* thread_state = ThrState::Instance();
    if (int ret = thread_state->FindThread(pthread, 1); ret != 0) {
        return ret;
    }

    /* Check if the thread is already detached or has a joiner. */
    if (True(pthread->flags & ThreadFlags::Detached) || (pthread->joiner != NULL)) {
        pthread->lock->unlock();
        return POSIX_EINVAL;
    }

    /* Flag the thread as detached. */
    pthread->flags |= ThreadFlags::Detached;
    thread_state->TryCollect(g_curthread, pthread); /* thread lock released */
    return 0;
}

} // namespace Libraries::Kernel
