// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <forward_list>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>

namespace Libraries::Kernel {

struct Pthread;
struct SleepQueue;

using ListBaseHook =
    boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>>;

using SleepqList = boost::intrusive::list<SleepQueue, boost::intrusive::constant_time_size<false>>;

struct SleepQueue : public ListBaseHook {
    std::forward_list<Pthread*> sq_blocked;
    SleepqList sq_freeq;
    void* sq_wchan;
    int sq_type;
};

void SleepqLock(void* wchan);

void SleepqUnlock(void* wchan);

SleepQueue* SleepqLookup(void* wchan);

void SleepqAdd(void* wchan, Pthread* td);

bool SleepqRemove(SleepQueue* sq, Pthread* td);

void SleepqDrop(SleepQueue* sq, void (*callback)(Pthread*, void*), void* arg);

} // namespace Libraries::Kernel
