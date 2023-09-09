#include "event_queue.h"

#include <Lib/Timer.h>

#include "debug.h"

namespace HLE::Kernel::Objects {
EqueueInternal::~EqueueInternal() {}

int EqueueInternal::addEvent(const EqueueEvent& event) {
    Lib::LockMutexGuard lock(m_mutex);

    if (m_events.size() > 0) {
        BREAKPOINT();
    }
    // TODO check if event is already exists and return it. Currently we just add in m_events array
    m_events.push_back(event);

    if (event.isTriggered) {
        BREAKPOINT();  // we don't support that either yet
    }

    return 0;
}

int EqueueInternal::waitForEvents(SceKernelEvent* ev, int num, u32 micros) {
    Lib::LockMutexGuard lock(m_mutex);

    u32 timeElapsed = 0;
    Lib::Timer t;
    t.Start();

    for (;;) {
        int ret = getTriggeredEvents(ev, num);

        if (ret > 0 || (timeElapsed >= micros && micros != 0)) {
            return ret;
        }

        if (micros == 0) {
            m_cond.WaitCondVar(&m_mutex);
        } else {
            m_cond.WaitCondVarFor(&m_mutex, micros - timeElapsed);
        }

        timeElapsed = static_cast<uint32_t>(t.GetTimeSec() * 1000000.0);
    }

    return 0;
}

int EqueueInternal::getTriggeredEvents(SceKernelEvent* ev, int num) {
    Lib::LockMutexGuard lock(m_mutex);

    int ret = 0;

    if (m_events.size() > 1) {
        BREAKPOINT();  // we currently support one event
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

};  // namespace HLE::Kernel::Objects