#include "event_queue.h"

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

};  // namespace HLE::Kernel::Objects