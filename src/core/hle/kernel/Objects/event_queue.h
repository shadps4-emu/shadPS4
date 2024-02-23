#pragma once

#include <mutex>
#include <string>
#include <vector>
#include <condition_variable>
#include "common/types.h"

namespace Core::Kernel {

constexpr s16 EVFILT_READ = -1;
constexpr s16 EVFILT_WRITE = -2;
constexpr s16 EVFILT_AIO = -3;     // attached to aio requests
constexpr s16 EVFILT_VNODE = -4;   // attached to vnodes
constexpr s16 EVFILT_PROC = -5;    // attached to struct proc
constexpr s16 EVFILT_SIGNAL = -6;  // attached to struct proc
constexpr s16 EVFILT_TIMER = -7;   // timers
constexpr s16 EVFILT_FS = -9;      // filesystem events
constexpr s16 EVFILT_LIO = -10;    // attached to lio requests
constexpr s16 EVFILT_USER = -11;   // User events
constexpr s16 EVFILT_POLLING = -12;
constexpr s16 EVFILT_VIDEO_OUT = -13;
constexpr s16 EVFILT_GRAPHICS_CORE = -14;
constexpr s16 EVFILT_HRTIMER = -15;
constexpr s16 EVFILT_UVD_TRAP = -16;
constexpr s16 EVFILT_VCE_TRAP = -17;
constexpr s16 EVFILT_SDMA_TRAP = -18;
constexpr s16 EVFILT_REG_EV = -19;
constexpr s16 EVFILT_GPU_EXCEPTION = -20;
constexpr s16 EVFILT_GPU_SYSTEM_EXCEPTION = -21;
constexpr s16 EVFILT_GPU_DBGGC_EV = -22;
constexpr s16 EVFILT_SYSCOUNT = 22;

class EqueueInternal;
struct EqueueEvent;

using TriggerFunc = void (*)(EqueueEvent* event, void* trigger_data);
using ResetFunc = void (*)(EqueueEvent* event);
using DeleteFunc = void (*)(EqueueInternal* eq, EqueueEvent* event);

struct SceKernelEvent {
    u64 ident = 0;  /* identifier for this event */
    s16 filter = 0; /* filter for event */
    u16 flags = 0;
    u32 fflags = 0;
    s64 data = 0;
    void* udata = nullptr; /* opaque user data identifier */
};

struct Filter {
    void* data = nullptr;
    TriggerFunc trigger_event_func = nullptr;
    ResetFunc reset_event_func = nullptr;
    DeleteFunc delete_event_func = nullptr;
};

struct EqueueEvent {
    bool isTriggered = false;
    SceKernelEvent event;
    Filter filter;
};

class EqueueInternal {
  public:  
    EqueueInternal() = default;
    virtual ~EqueueInternal();
    void setName(const std::string& m_name) { this->m_name = m_name; }
    int addEvent(const EqueueEvent& event);
    int waitForEvents(SceKernelEvent* ev, int num, u32 micros);
    bool triggerEvent(u64 ident, s16 filter, void* trigger_data);
    int getTriggeredEvents(SceKernelEvent* ev, int num);
  private:
    std::string m_name;
    std::mutex m_mutex; 
    std::vector<EqueueEvent> m_events;
    std::condition_variable m_cond;
};

} // namespace Core::Kernel
