// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/debug.h"
#include "core/libraries/kernel/event_queue.h"

namespace Libraries::Kernel {

EqueueInternal::~EqueueInternal() = default;

int EqueueInternal::addEvent(const EqueueEvent& event) {
    std::scoped_lock lock{m_mutex};

    if (m_events.size() > 0) {
        BREAKPOINT();
    }
    // TODO check if event is already exists and return it. Currently we just add in m_events array
    m_events.push_back(event);

    if (event.isTriggered) {
        BREAKPOINT(); // we don't support that either yet
    }

    return 0;
}

int EqueueInternal::waitForEvents(SceKernelEvent* ev, int num, u32 micros) {
    std::unique_lock lock{m_mutex};
    int ret = 0;

    const auto predicate = [&] {
        ret = getTriggeredEvents(ev, num);
        return ret > 0;
    };

    if (micros == 0) {
        m_cond.wait(lock, predicate);
    } else {
        m_cond.wait_for(lock, std::chrono::microseconds(micros), predicate);
    }
    return ret;
}

bool EqueueInternal::triggerEvent(u64 ident, s16 filter, void* trigger_data) {
    std::scoped_lock lock{m_mutex};

    if (m_events.size() > 1) {
        BREAKPOINT(); // we currently support one event
    }
    auto& event = m_events[0];

    if (event.filter.trigger_event_func != nullptr) {
        event.filter.trigger_event_func(&event, trigger_data);
    } else {
        event.isTriggered = true;
    }

    m_cond.notify_one();

    return true;
}

int EqueueInternal::getTriggeredEvents(SceKernelEvent* ev, int num) {
    int ret = 0;

    if (m_events.size() > 1) {
        BREAKPOINT(); // we currently support one event
    }
    auto& event = m_events[0];

    if (event.isTriggered) {
        ev[ret++] = event.event;

        if (event.filter.reset_event_func != nullptr) {
            event.filter.reset_event_func(&event);
        }
    }

    return ret;
}

} // namespace Libraries::Kernel
