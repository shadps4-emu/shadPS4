// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include "common/spin_lock.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/kernel/threads/sleepq.h"

namespace Libraries::Kernel {

static constexpr int HASHSHIFT = 9;
static constexpr int HASHSIZE = (1 << HASHSHIFT);
#define SC_HASH(wchan)                                                                             \
    ((u32)((((uintptr_t)(wchan) >> 3) ^ ((uintptr_t)(wchan) >> (HASHSHIFT + 3))) & (HASHSIZE - 1)))
#define SC_LOOKUP(wc) &sc_table[SC_HASH(wc)]

struct SleepQueueChain {
    Common::SpinLock sc_lock;
    SleepqList sc_queues;
    int sc_type;
};

static std::array<SleepQueueChain, HASHSIZE> sc_table{};

void SleepqLock(void* wchan) {
    SleepQueueChain* sc = SC_LOOKUP(wchan);
    sc->sc_lock.lock();
}

void SleepqUnlock(void* wchan) {
    SleepQueueChain* sc = SC_LOOKUP(wchan);
    sc->sc_lock.unlock();
}

SleepQueue* SleepqLookup(void* wchan) {
    SleepQueueChain* sc = SC_LOOKUP(wchan);
    for (auto& sq : sc->sc_queues) {
        if (sq.sq_wchan == wchan) {
            return std::addressof(sq);
        }
    }
    return nullptr;
}

void SleepqAdd(void* wchan, Pthread* td) {
    SleepQueue* sq = SleepqLookup(wchan);
    if (sq != nullptr) {
        sq->sq_freeq.push_front(*td->sleepqueue);
    } else {
        SleepQueueChain* sc = SC_LOOKUP(wchan);
        sq = td->sleepqueue;
        sc->sc_queues.push_front(*sq);
        sq->sq_wchan = wchan;
        /* sq->sq_type = type; */
    }
    td->sleepqueue = nullptr;
    td->wchan = wchan;
    sq->sq_blocked.push_front(td);
}

int SleepqRemove(SleepQueue* sq, Pthread* td) {
    std::erase(sq->sq_blocked, td);
    if (sq->sq_blocked.empty()) {
        td->sleepqueue = sq;
        sq->unlink();
        td->wchan = nullptr;
        return 0;
    } else {
        td->sleepqueue = std::addressof(sq->sq_freeq.front());
        sq->sq_freeq.pop_front();
        td->wchan = nullptr;
        return 1;
    }
}

void SleepqDrop(SleepQueue* sq, void (*callback)(Pthread*, void*), void* arg) {
    if (sq->sq_blocked.empty()) {
        return;
    }

    sq->unlink();
    Pthread* td = sq->sq_blocked.front();
    sq->sq_blocked.pop_front();

    callback(td, arg);

    td->sleepqueue = sq;
    td->wchan = nullptr;

    auto sq2 = sq->sq_freeq.begin();
    for (Pthread* td2 : sq->sq_blocked) {
        callback(td2, arg);
        td2->sleepqueue = std::addressof(*sq2);
        td2->wchan = nullptr;
        ++sq2;
    }
    sq->sq_blocked.clear();
    sq->sq_freeq.clear();
}

} // namespace Libraries::Kernel