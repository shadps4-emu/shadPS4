// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>
#include <boost/asio/steady_timer.hpp>

#include <unordered_map>
#include "common/rdtsc.h"
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

class EqueueInternal;
struct EqueueEvent;

using SceKernelUseconds = u32;
using SceKernelEqueue = EqueueInternal*;

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

struct OrbisVideoOutEventHint {
    u64 event_id : 8;
    u64 video_id : 8;
    u64 flip_arg : 48;
};

struct OrbisVideoOutEventData {
    u64 time : 12;
    u64 count : 4;
    u64 flip_arg : 48;
};

struct EqueueEvent {
    SceKernelEvent event;
    void* data = nullptr;
    std::chrono::steady_clock::time_point time_added;
    std::chrono::nanoseconds timer_interval;
    std::unique_ptr<boost::asio::steady_timer> timer;

    void Clear() {
        is_triggered = false;
        event.fflags = 0;
        event.data = 0;
    }

    void Trigger(void* data) {
        is_triggered = true;
        event.data = reinterpret_cast<uintptr_t>(data);
    }

    void TriggerUser(void* data) {
        is_triggered = true;
        event.udata = data;
    }

    void TriggerTimer() {
        is_triggered = true;
        event.data++;
    }

    void TriggerDisplay(void* data) {
        is_triggered = true;
        if (data != nullptr) {
            auto event_data = std::bit_cast<OrbisVideoOutEventData>(event.data);
            auto event_hint_raw = reinterpret_cast<u64>(data);
            auto event_hint = static_cast<OrbisVideoOutEventHint>(event_hint_raw);
            if (event_hint.event_id == event.ident && event.ident != 0xfe) {
                auto time = Common::FencedRDTSC();
                auto counter = event_data.count;
                if (counter != 0xf) {
                    counter++;
                }
                event.data =
                    (time & 0xfff) | (counter << 0xc) | (event_hint_raw & 0xffffffffffff0000);
            }
        }
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
    struct SmallTimer {
        SceKernelEvent event;
        std::chrono::steady_clock::time_point added;
        std::chrono::nanoseconds interval;
    };

public:
    explicit EqueueInternal(std::string_view name) : m_name(name) {}

    std::string_view GetName() const {
        return m_name;
    }

    bool AddEvent(EqueueEvent& event);
    bool ScheduleEvent(u64 id, s16 filter,
                       void (*callback)(SceKernelEqueue, const SceKernelEvent&));
    bool RemoveEvent(u64 id, s16 filter);
    int WaitForEvents(SceKernelEvent* ev, int num, const SceKernelUseconds* timo);
    bool TriggerEvent(u64 ident, s16 filter, void* trigger_data);
    int GetTriggeredEvents(SceKernelEvent* ev, int num);

    bool AddSmallTimer(EqueueEvent& event);
    bool HasSmallTimer() {
        std::scoped_lock lock{m_mutex};
        return !m_small_timers.empty();
    }
    bool RemoveSmallTimer(u64 id) {
        if (HasSmallTimer()) {
            std::scoped_lock lock{m_mutex};
            return m_small_timers.erase(id) > 0;
        }
        return false;
    }

    int WaitForSmallTimer(SceKernelEvent* ev, int num, u32 micros);

    bool EventExists(u64 id, s16 filter);

private:
    std::string m_name;
    std::mutex m_mutex;
    std::vector<EqueueEvent> m_events;
    std::condition_variable m_cond;
    std::unordered_map<u64, SmallTimer> m_small_timers;
};

u64 PS4_SYSV_ABI sceKernelGetEventData(const SceKernelEvent* ev);

void RegisterEventQueue(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
