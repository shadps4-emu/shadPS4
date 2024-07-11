// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "core/libraries/kernel/event_queue.h"

namespace Libraries::Kernel {

EqueueInternal::~EqueueInternal() = default;

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
            ASSERT_MSG(event.event.filter == filter,
                       "Event to trigger doesn't match to queue events");
            if (event.event.ident == ident) {
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
            if (ev->flags & SceKernelEvent::Flags::Clear) {
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
            std::unique_lock lock{m_mutex};
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

} // namespace Libraries::Kernel
