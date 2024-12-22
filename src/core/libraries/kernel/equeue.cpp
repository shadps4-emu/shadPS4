// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>

#include "common/assert.h"
#include "common/debug.h"
#include "common/logging/log.h"
#include "core/libraries/kernel/equeue.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

bool EqueueInternal::AddEvent(EqueueEvent& event) {
    std::scoped_lock lock{m_mutex};

    event.time_added = std::chrono::steady_clock::now();

    const auto& it = std::ranges::find(m_events, event);
    if (it != m_events.cend()) {
        *it = std::move(event);
    } else {
        m_events.emplace_back(std::move(event));
    }

    return true;
}

bool EqueueInternal::RemoveEvent(u64 id) {
    bool has_found = false;
    std::scoped_lock lock{m_mutex};

    const auto& it =
        std::ranges::find_if(m_events, [id](auto& ev) { return ev.event.ident == id; });
    if (it != m_events.cend()) {
        m_events.erase(it);
        has_found = true;
    }
    return has_found;
}

int EqueueInternal::WaitForEvents(SceKernelEvent* ev, int num, u32 micros) {
    int count = 0;

    const auto predicate = [&] {
        count = GetTriggeredEvents(ev, num);
        return count > 0;
    };

    if (micros == 0) {
        std::unique_lock lock{m_mutex};
        m_cond.wait(lock, predicate);
    } else {
        std::unique_lock lock{m_mutex};
        m_cond.wait_for(lock, std::chrono::microseconds(micros), predicate);
    }

    if (HasSmallTimer()) {
        if (count > 0) {
            const auto time_waited = std::chrono::duration_cast<std::chrono::microseconds>(
                                         std::chrono::steady_clock::now() - m_events[0].time_added)
                                         .count();
            count = WaitForSmallTimer(ev, num, std::max(0l, long(micros - time_waited)));
        }
        small_timer_event.event.data = 0;
    }

    if (ev->flags & SceKernelEvent::Flags::OneShot) {
        for (auto ev_id = 0u; ev_id < count; ++ev_id) {
            RemoveEvent(ev->ident);
        }
    }

    return count;
}

bool EqueueInternal::TriggerEvent(u64 ident, s16 filter, void* trigger_data) {
    bool has_found = false;
    {
        std::scoped_lock lock{m_mutex};
        for (auto& event : m_events) {
            if (event.event.ident == ident && event.event.filter == filter) {
                event.Trigger(trigger_data);
                has_found = true;
            }
        }
    }
    m_cond.notify_one();
    return has_found;
}

int EqueueInternal::GetTriggeredEvents(SceKernelEvent* ev, int num) {
    int count = 0;
    for (auto& event : m_events) {
        if (event.IsTriggered()) {
            if (event.event.flags & SceKernelEvent::Flags::Clear) {
                event.Reset();
            }
            ev[count++] = event.event;
            if (count == num) {
                break;
            }
        }
    }

    return count;
}

bool EqueueInternal::AddSmallTimer(EqueueEvent& ev) {
    // We assume that only one timer event (with the same ident across calls)
    // can be posted to the queue, based on observations so far. In the opposite case,
    // the small timer storage and wait logic should be reworked.
    ASSERT(!HasSmallTimer() || small_timer_event.event.ident == ev.event.ident);
    ev.time_added = std::chrono::steady_clock::now();
    small_timer_event = std::move(ev);
    return true;
}

int EqueueInternal::WaitForSmallTimer(SceKernelEvent* ev, int num, u32 micros) {
    int count{};

    ASSERT(num == 1);

    auto curr_clock = std::chrono::steady_clock::now();
    const auto wait_end_us = curr_clock + std::chrono::microseconds{micros};

    do {
        curr_clock = std::chrono::steady_clock::now();
        {
            std::scoped_lock lock{m_mutex};
            if ((curr_clock - small_timer_event.time_added) >
                std::chrono::microseconds{small_timer_event.event.data}) {
                ev[count++] = small_timer_event.event;
                small_timer_event.event.data = 0;
                break;
            }
        }
        std::this_thread::yield();
    } while (curr_clock < wait_end_us);

    return count;
}

extern boost::asio::io_context io_context;
extern void KernelSignalRequest();

static constexpr auto HrTimerSpinlockThresholdUs = 1200u;

static void SmallTimerCallback(const boost::system::error_code& error, SceKernelEqueue eq,
                               SceKernelEvent kevent) {
    static EqueueEvent event;
    event.event = kevent;
    event.event.data = HrTimerSpinlockThresholdUs;
    eq->AddSmallTimer(event);
    eq->TriggerEvent(kevent.ident, SceKernelEvent::Filter::HrTimer, kevent.udata);
}

int PS4_SYSV_ABI sceKernelCreateEqueue(SceKernelEqueue* eq, const char* name) {
    if (eq == nullptr) {
        LOG_ERROR(Kernel_Event, "Event queue is null!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (name == nullptr) {
        LOG_ERROR(Kernel_Event, "Event queue name is null!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    // Maximum is 32 including null terminator
    static constexpr size_t MaxEventQueueNameSize = 32;
    if (std::strlen(name) > MaxEventQueueNameSize) {
        LOG_ERROR(Kernel_Event, "Event queue name exceeds 32 bytes!");
        return ORBIS_KERNEL_ERROR_ENAMETOOLONG;
    }

    LOG_INFO(Kernel_Event, "name = {}", name);

    *eq = new EqueueInternal(name);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelDeleteEqueue(SceKernelEqueue eq) {
    if (eq == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    delete eq;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelWaitEqueue(SceKernelEqueue eq, SceKernelEvent* ev, int num, int* out,
                                     SceKernelUseconds* timo) {
    HLE_TRACE;
    TRACE_HINT(eq->GetName());
    LOG_TRACE(Kernel_Event, "equeue = {} num = {}", eq->GetName(), num);

    if (eq == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    if (ev == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }

    if (num < 1) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (eq->HasSmallTimer()) {
        ASSERT(timo && *timo);
        *out = eq->WaitForSmallTimer(ev, num, *timo);
    } else {
        if (timo == nullptr) { // wait until an event arrives without timing out
            *out = eq->WaitForEvents(ev, num, 0);
        }

        if (timo != nullptr) {
            // Only events that have already arrived at the time of this function call can be
            // received
            if (*timo == 0) {
                *out = eq->GetTriggeredEvents(ev, num);
            } else {
                // Wait until an event arrives with timing out
                *out = eq->WaitForEvents(ev, num, *timo);
            }
        }
    }

    if (*out == 0) {
        return ORBIS_KERNEL_ERROR_ETIMEDOUT;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAddHRTimerEvent(SceKernelEqueue eq, int id, timespec* ts, void* udata) {
    if (eq == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    if (ts->tv_sec > 100 || ts->tv_nsec < 100'000) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    ASSERT(ts->tv_nsec > 1000); // assume 1us resolution
    const auto total_us = ts->tv_sec * 1000'000 + ts->tv_nsec / 1000;

    EqueueEvent event{};
    event.event.ident = id;
    event.event.filter = SceKernelEvent::Filter::HrTimer;
    event.event.flags = SceKernelEvent::Flags::Add | SceKernelEvent::Flags::OneShot;
    event.event.fflags = 0;
    event.event.data = total_us;
    event.event.udata = udata;

    // HR timers cannot be implemented within the existing event queue architecture due to the
    // slowness of the notification mechanism. For instance, a 100us timer will lose its precision
    // as the trigger time drifts by +50-700%, depending on the host PC and workload. To address
    // this issue, we use a spinlock for small waits (which can be adjusted using
    // `HrTimerSpinlockThresholdUs`) and fall back to boost asio timers if the time to tick is
    // large. Even for large delays, we truncate a small portion to complete the wait
    // using the spinlock, prioritizing precision.
    if (total_us < HrTimerSpinlockThresholdUs) {
        return eq->AddSmallTimer(event) ? ORBIS_OK : ORBIS_KERNEL_ERROR_ENOMEM;
    }

    event.timer = std::make_unique<boost::asio::steady_timer>(
        io_context, std::chrono::microseconds(total_us - HrTimerSpinlockThresholdUs));

    event.timer->async_wait(std::bind(SmallTimerCallback, std::placeholders::_1, eq, event.event));

    if (!eq->AddEvent(event)) {
        return ORBIS_KERNEL_ERROR_ENOMEM;
    }

    KernelSignalRequest();

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelAddUserEvent(SceKernelEqueue eq, int id) {
    if (eq == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    EqueueEvent event{};
    event.event.ident = id;
    event.event.filter = SceKernelEvent::Filter::User;
    event.event.udata = 0;
    event.event.flags = SceKernelEvent::Flags::Add;
    event.event.fflags = 0;
    event.event.data = 0;

    return eq->AddEvent(event) ? ORBIS_OK : ORBIS_KERNEL_ERROR_ENOMEM;
}

int PS4_SYSV_ABI sceKernelAddUserEventEdge(SceKernelEqueue eq, int id) {
    if (eq == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    EqueueEvent event{};
    event.event.ident = id;
    event.event.filter = SceKernelEvent::Filter::User;
    event.event.udata = 0;
    event.event.flags = SceKernelEvent::Flags::Add | SceKernelEvent::Flags::Clear;
    event.event.fflags = 0;
    event.event.data = 0;

    return eq->AddEvent(event) ? ORBIS_OK : ORBIS_KERNEL_ERROR_ENOMEM;
}

void* PS4_SYSV_ABI sceKernelGetEventUserData(const SceKernelEvent* ev) {
    ASSERT(ev);
    return ev->udata;
}

u64 PS4_SYSV_ABI sceKernelGetEventId(const SceKernelEvent* ev) {
    return ev->ident;
}

int PS4_SYSV_ABI sceKernelTriggerUserEvent(SceKernelEqueue eq, int id, void* udata) {
    if (eq == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    if (!eq->TriggerEvent(id, SceKernelEvent::Filter::User, udata)) {
        return ORBIS_KERNEL_ERROR_ENOENT;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelDeleteUserEvent(SceKernelEqueue eq, int id) {
    if (eq == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    if (!eq->RemoveEvent(id)) {
        return ORBIS_KERNEL_ERROR_ENOENT;
    }
    return ORBIS_OK;
}

s16 PS4_SYSV_ABI sceKernelGetEventFilter(const SceKernelEvent* ev) {
    return ev->filter;
}

void RegisterEventQueue(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("D0OdFMjp46I", "libkernel", 1, "libkernel", 1, 1, sceKernelCreateEqueue);
    LIB_FUNCTION("jpFjmgAC5AE", "libkernel", 1, "libkernel", 1, 1, sceKernelDeleteEqueue);
    LIB_FUNCTION("fzyMKs9kim0", "libkernel", 1, "libkernel", 1, 1, sceKernelWaitEqueue);
    LIB_FUNCTION("vz+pg2zdopI", "libkernel", 1, "libkernel", 1, 1, sceKernelGetEventUserData);
    LIB_FUNCTION("4R6-OvI2cEA", "libkernel", 1, "libkernel", 1, 1, sceKernelAddUserEvent);
    LIB_FUNCTION("WDszmSbWuDk", "libkernel", 1, "libkernel", 1, 1, sceKernelAddUserEventEdge);
    LIB_FUNCTION("R74tt43xP6k", "libkernel", 1, "libkernel", 1, 1, sceKernelAddHRTimerEvent);
    LIB_FUNCTION("F6e0kwo4cnk", "libkernel", 1, "libkernel", 1, 1, sceKernelTriggerUserEvent);
    LIB_FUNCTION("LJDwdSNTnDg", "libkernel", 1, "libkernel", 1, 1, sceKernelDeleteUserEvent);
    LIB_FUNCTION("mJ7aghmgvfc", "libkernel", 1, "libkernel", 1, 1, sceKernelGetEventId);
    LIB_FUNCTION("23CPPI1tyBY", "libkernel", 1, "libkernel", 1, 1, sceKernelGetEventFilter);
}

} // namespace Libraries::Kernel
