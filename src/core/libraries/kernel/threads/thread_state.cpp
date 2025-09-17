// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/container/small_vector.hpp>
#include "common/alignment.h"
#include "common/scope_exit.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/kernel/threads/sleepq.h"
#include "core/libraries/kernel/threads/thread_state.h"
#include "core/memory.h"
#include "core/tls.h"

namespace Libraries::Kernel {

thread_local Pthread* g_curthread{};

Core::Tcb* TcbCtor(Pthread* thread, int initial);
void TcbDtor(Core::Tcb* oldtls);

ThreadState::ThreadState() {
    // Reserve memory for maximum amount of threads allowed.
    auto* memory = Core::Memory::Instance();
    static constexpr u32 ThrHeapSize = Common::AlignUp(sizeof(Pthread) * MaxThreads, 16_KB);
    void* heap_addr{};
    const int ret = memory->MapMemory(&heap_addr, Core::SYSTEM_RESERVED_MIN, ThrHeapSize,
                                      Core::MemoryProt::CpuReadWrite, Core::MemoryMapFlags::NoFlags,
                                      Core::VMAType::File, "ThrHeap");
    ASSERT_MSG(ret == 0, "Unable to allocate thread heap memory {}", ret);
    thread_heap.Initialize(heap_addr, ThrHeapSize);
}

void ThreadState::Collect(Pthread* curthread) {
    boost::container::small_vector<Pthread*, 8> work_list;
    {
        std::scoped_lock lk{thread_list_lock};
        for (auto it = gc_list.begin(); it != gc_list.end();) {
            Pthread* td = *it;
            if (td->tid != TidTerminated) {
                ++it;
                continue;
            }
            FreeStack(&td->attr);
            work_list.push_back(td);
            it = gc_list.erase(it);
        }
    }
    for (Pthread* td : work_list) {
        Free(curthread, td);
    }
}

void ThreadState::TryCollect(Pthread* thread) {
    SCOPE_EXIT {
        thread->lock.unlock();
    };
    if (!thread->ShouldCollect()) {
        return;
    }

    thread->refcount++;
    thread->lock.unlock();
    std::scoped_lock lk{thread_list_lock};
    thread->lock.lock();
    thread->refcount--;
    if (thread->ShouldCollect()) {
        threads.erase(thread);
        gc_list.push_back(thread);
    }
}

Pthread* ThreadState::Alloc(Pthread* curthread) {
    Pthread* thread = nullptr;
    if (curthread != nullptr) {
        if (GcNeeded()) {
            Collect(curthread);
        }
        if (!free_threads.empty()) {
            std::scoped_lock lk{free_thread_lock};
            thread = free_threads.back();
            free_threads.pop_back();
        }
    }
    if (thread == nullptr) {
        if (total_threads > MaxThreads) {
            return nullptr;
        }
        total_threads.fetch_add(1);
        thread = thread_heap.Allocate();
        if (thread == nullptr) {
            total_threads.fetch_sub(1);
            return nullptr;
        }
    }
    Core::Tcb* tcb = nullptr;
    if (curthread != nullptr) {
        std::scoped_lock lk{tcb_lock};
        tcb = TcbCtor(thread, 0 /* not initial tls */);
    } else {
        tcb = TcbCtor(thread, 1 /* initial tls */);
    }
    if (tcb != nullptr) {
        memset(thread, 0, sizeof(Pthread));
        std::construct_at(thread);
        thread->tcb = tcb;
        thread->sleepqueue = new SleepQueue{};
    } else {
        thread_heap.Free(thread);
        total_threads.fetch_sub(1);
        thread = nullptr;
    }
    return thread;
}

void ThreadState::Free(Pthread* curthread, Pthread* thread) {
    if (curthread != nullptr) {
        std::scoped_lock lk{tcb_lock};
        TcbDtor(thread->tcb);
    } else {
        TcbDtor(thread->tcb);
    }
    thread->tcb = nullptr;
    std::destroy_at(thread);
    if (free_threads.size() >= MaxCachedThreads) {
        delete thread->sleepqueue;
        thread_heap.Free(thread);
        total_threads.fetch_sub(1);
    } else {
        std::scoped_lock lk{free_thread_lock};
        free_threads.push_back(thread);
    }
}

int ThreadState::FindThread(Pthread* thread, const bool include_dead) {
    if (thread == nullptr) {
        return POSIX_EINVAL;
    }
    std::scoped_lock lk{thread_list_lock};
    const auto it = threads.find(thread);
    if (it == threads.end()) {
        return POSIX_ESRCH;
    }
    thread->lock.lock();
    if (!include_dead && thread->state == PthreadState::Dead) {
        thread->lock.unlock();
        return POSIX_ESRCH;
    }
    return 0;
}

int ThreadState::RefAdd(Pthread* thread, bool include_dead) {
    if (thread == nullptr) {
        /* Invalid thread: */
        return POSIX_EINVAL;
    }

    if (int ret = FindThread(thread, include_dead); ret != 0) {
        return ret;
    }

    thread->refcount++;
    thread->lock.unlock();
    return 0;
}

void ThreadState::RefDelete(Pthread* thread) {
    thread->lock.lock();
    thread->refcount--;
    TryCollect(thread);
}

} // namespace Libraries::Kernel
