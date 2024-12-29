// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>
#include <boost/asio/steady_timer.hpp>

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

class EqueueInternal;
struct EqueueEvent;

struct SceKernelEvent {
    enum Filter : s16 {
        None = 0,
        Read = -1,
        Write = -2,
        Aio = -3,
        Vnode = -4,
        Proc = -5,
        Signal = -6,
        Timer = -7,
        Fs = -9,
        Lio = -10,
        User = -11,
        Polling = -12,
        VideoOut = -13,
        GraphicsCore = -14,
        HrTimer = -15,
    };

    enum Flags : u16 {
        Add = 1u,
        Delete = 2u,
        Enable = 4u,
        Disable = 8u,
        OneShot = 0x10u,
        Clear = 0x20u,
        Receipt = 0x40u,
        Dispatch = 0x80u,
        Flag1 = 0x2000u,
        System = 0xf000u,
    };

    u64 ident = 0;                /* identifier for this event */
    Filter filter = Filter::None; /* filter for event */
    u16 flags = 0;
    u32 fflags = 0;
    u64 data = 0;
    void* udata = nullptr; /* opaque user data identifier */
};

struct EqueueEvent {
    SceKernelEvent event;
    void* data = nullptr;
    std::chrono::steady_clock::time_point time_added;
    std::unique_ptr<boost::asio::steady_timer> timer;

    void ResetTriggerState() {
        is_triggered = false;
    }

    void Clear() {
        event.fflags = 0;
        event.data = 0;
    }

    void Trigger(void* data) {
        is_triggered = true;
        event.fflags++;
        event.data = reinterpret_cast<uintptr_t>(data);
    }

    bool IsTriggered() const {
        return is_triggered;
    }

    bool operator==(const EqueueEvent& ev) const {
        return ev.event.ident == event.ident && ev.event.filter == event.filter;
    }

private:
    bool is_triggered = false;
};

class EqueueInternal {
public:
    explicit EqueueInternal(std::string_view name) : m_name(name) {}

    std::string_view GetName() const {
        return m_name;
    }

    bool AddEvent(EqueueEvent& event);
    bool RemoveEvent(u64 id, s16 filter);
    int WaitForEvents(SceKernelEvent* ev, int num, u32 micros);
    bool TriggerEvent(u64 ident, s16 filter, void* trigger_data);
    int GetTriggeredEvents(SceKernelEvent* ev, int num);

    bool AddSmallTimer(EqueueEvent& event);
    bool HasSmallTimer() const {
        return small_timer_event.event.data != 0;
    }

    int WaitForSmallTimer(SceKernelEvent* ev, int num, u32 micros);

private:
    std::string m_name;
    std::mutex m_mutex;
    std::vector<EqueueEvent> m_events;
    EqueueEvent small_timer_event{};
    std::condition_variable m_cond;
};

using SceKernelUseconds = u32;
using SceKernelEqueue = EqueueInternal*;

u64 PS4_SYSV_ABI sceKernelGetEventData(const SceKernelEvent* ev);

void RegisterEventQueue(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
