// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "core/libraries/kernel/event_queue.h"

namespace Libraries::Kernel {

EqueueInternal::~EqueueInternal() = default;

bool EqueueInternal::AddEvent(EqueueEvent& event) {
    std::scoped_lock lock{m_mutex};

    event.time_added = std::chrono::high_resolution_clock::now();

    const auto& it = std::ranges::find(m_events, event);
    if (it != m_events.cend()) {
        *it = event;
    } else {
        m_events.emplace_back(event);
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
    std::unique_lock lock{m_mutex};
    int ret = 0;

    const auto predicate = [&] {
        ret = GetTriggeredEvents(ev, num);
        return ret > 0;
    };

    if (micros == 0) {
        m_cond.wait(lock, predicate);
    } else {
        m_cond.wait_for(lock, std::chrono::microseconds(micros), predicate);
    }
    return ret;
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
    int ret = 0;

    for (auto& event : m_events) {
        if (event.IsTriggered()) {
            if (ev->flags & SceKernelEvent::Flags::Clear) {
                event.Reset();
            }

            ev[ret++] = event.event;

            if (ret == num) {
                break;
            }
        }
    }

    return ret;
}

} // namespace Libraries::Kernel
