// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <condition_variable>
#include <mutex>
#include <boost/intrusive/list.hpp>
#include <pthread.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

using ListBaseHook =
    boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>;

class Semaphore {
public:
    Semaphore(s32 init_count, s32 max_count, const char* name, bool is_fifo)
        : name{name}, token_count{init_count}, max_count{max_count}, is_fifo{is_fifo} {}

    bool Wait(bool can_block, s32 need_count, u64* timeout) {
        if (HasAvailableTokens(need_count)) {
            return true;
        }
        if (!can_block) {
            return false;
        }

        // Create waiting thread object and add it into the list of waiters.
        WaitingThread waiter{need_count, is_fifo};
        AddWaiter(waiter);
        SCOPE_EXIT {
            PopWaiter(waiter);
        };

        // Perform the wait.
        return waiter.Wait(timeout);
    }

    bool Signal(s32 signal_count) {
        std::scoped_lock lk{mutex};
        if (token_count + signal_count > max_count) {
            return false;
        }
        token_count += signal_count;

        // Wake up threads in order of priority.
        for (auto& waiter : wait_list) {
            if (waiter.need_count > token_count) {
                continue;
            }
            token_count -= waiter.need_count;
            waiter.cv.notify_one();
        }

        return true;
    }

private:
    struct WaitingThread : public ListBaseHook {
        std::mutex mutex;
        std::condition_variable cv;
        u32 priority;
        s32 need_count;

        explicit WaitingThread(s32 need_count, bool is_fifo) : need_count{need_count} {
            if (is_fifo) {
                return;
            }
            // Retrieve calling thread priority for sorting into waiting threads list.
            s32 policy;
            sched_param param;
            pthread_getschedparam(pthread_self(), &policy, &param);
            priority = param.sched_priority;
        }

        bool Wait(u64* timeout) {
            std::unique_lock lk{mutex};
            if (!timeout) {
                // Wait indefinitely until we are woken up.
                cv.wait(lk);
                return true;
            }
            // Wait until timeout runs out, recording how much remaining time there was.
            const auto start = std::chrono::high_resolution_clock::now();
            const auto status = cv.wait_for(lk, std::chrono::microseconds(*timeout));
            const auto end = std::chrono::high_resolution_clock::now();
            const auto time =
                std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            *timeout -= time;
            return status != std::cv_status::timeout;
        }

        bool operator<(const WaitingThread& other) const {
            return priority < other.priority;
        }
    };

    void AddWaiter(WaitingThread& waiter) {
        std::scoped_lock lk{mutex};
        // Insert at the end of the list for FIFO order.
        if (is_fifo) {
            wait_list.push_back(waiter);
            return;
        }
        // Find the first with priority less then us and insert right before it.
        auto it = wait_list.begin();
        while (it != wait_list.end() && it->priority > waiter.priority) {
            it++;
        }
        wait_list.insert(it, waiter);
    }

    void PopWaiter(WaitingThread& waiter) {
        std::scoped_lock lk{mutex};
        wait_list.erase(WaitingThreads::s_iterator_to(waiter));
    }

    bool HasAvailableTokens(s32 need_count) {
        std::scoped_lock lk{mutex};
        if (token_count >= need_count) {
            token_count -= need_count;
            return true;
        }
        return false;
    }

    using WaitingThreads =
        boost::intrusive::list<WaitingThread, boost::intrusive::base_hook<ListBaseHook>,
                               boost::intrusive::constant_time_size<false>>;
    WaitingThreads wait_list;
    std::string name;
    std::atomic<s32> token_count;
    std::mutex mutex;
    s32 max_count;
    bool is_fifo;
};

using OrbisKernelSema = Semaphore*;

s32 PS4_SYSV_ABI sceKernelCreateSema(OrbisKernelSema* sem, const char* pName, u32 attr,
                                     s32 initCount, s32 maxCount, const void* pOptParam) {
    if (!pName || attr > 2 || initCount < 0 || maxCount <= 0 || initCount > maxCount) {
        LOG_ERROR(Lib_Kernel, "Semaphore creation parameters are invalid!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    *sem = new Semaphore(initCount, maxCount, pName, attr == 1);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelWaitSema(OrbisKernelSema sem, s32 needCount, u64* pTimeout) {
    ASSERT(sem->Wait(true, needCount, pTimeout));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelSignalSema(OrbisKernelSema sem, s32 signalCount) {
    if (!sem->Signal(signalCount)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelPollSema(OrbisKernelSema sem, s32 needCount) {
    if (!sem->Wait(false, needCount, nullptr)) {
        return ORBIS_KERNEL_ERROR_EBUSY;
    }
    return ORBIS_OK;
}

void SemaphoreSymbolsRegister(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("188x57JYp0g", "libkernel", 1, "libkernel", 1, 1, sceKernelCreateSema);
    LIB_FUNCTION("Zxa0VhQVTsk", "libkernel", 1, "libkernel", 1, 1, sceKernelWaitSema);
    LIB_FUNCTION("4czppHBiriw", "libkernel", 1, "libkernel", 1, 1, sceKernelSignalSema);
    LIB_FUNCTION("12wOHk8ywb0", "libkernel", 1, "libkernel", 1, 1, sceKernelPollSema);
}

} // namespace Libraries::Kernel
