// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <set>
#include <stack>
#include "common/singleton.h"
#include "common/slab_heap.h"
#include "common/types.h"

namespace Libraries::Kernel {

struct Pthread;
struct PthreadAttr;

struct Stack {
    size_t stacksize; /* Stack size (rounded up). */
    size_t guardsize; /* Guard size. */
    void* stackaddr;  /* Stack address. */
};

struct ThreadState {
    static constexpr size_t GcThreshold = 5;
    static constexpr size_t MaxThreads = 100000;
    static constexpr size_t MaxCachedThreads = 100;

    explicit ThreadState();

    bool GcNeeded() const noexcept {
        return gc_list.size() >= GcThreshold;
    }

    void Collect(Pthread* curthread);

    void TryCollect(Pthread* thread);

    Pthread* Alloc(Pthread* curthread);

    void Free(Pthread* curthread, Pthread* thread);

    int FindThread(Pthread* thread, bool include_dead);

    int RefAdd(Pthread* thread, bool include_dead);

    void RefDelete(Pthread* thread);

    int CreateStack(PthreadAttr* attr);

    void FreeStack(PthreadAttr* attr);

    void Link(Pthread* curthread, Pthread* thread) {
        {
            std::scoped_lock lk{thread_list_lock};
            threads.insert(thread);
        }
        active_threads.fetch_add(1);
    }

    void Unlink(Pthread* curthread, Pthread* thread) {
        {
            std::scoped_lock lk{thread_list_lock};
            threads.erase(thread);
        }
        active_threads.fetch_sub(1);
    }

    Common::SlabHeap<Pthread> thread_heap;
    std::set<Pthread*> threads;
    std::list<Pthread*> free_threads;
    std::list<Pthread*> gc_list;
    std::mutex free_thread_lock;
    std::mutex tcb_lock;
    std::mutex thread_list_lock;
    std::atomic<s32> total_threads{};
    std::atomic<s32> active_threads{};
    std::stack<Stack*> dstackq;
    std::list<Stack*> mstackq;
    VAddr last_stack = 0;
};

using ThrState = Common::Singleton<ThreadState>;

} // namespace Libraries::Kernel
