// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <condition_variable>
#include <mutex>
#include "common/types.h"
#include "event_flag_codes.h"

namespace Libraries::Kernel {

class EventFlagInternal {
public:
    EventFlagInternal(const std::string& name, bool single, bool fifo, uint64_t bits)
        : m_name(name), m_single_thread(single), m_fifo(fifo), m_bits(bits){};

    int Wait(u64 bits, int wait_mode, int clear_mode, u64* result, u32* ptr_micros);

private:
    enum class Status { Set, Canceled, Deleted };

    std::mutex m_mutex;
    std::condition_variable m_cond_var;
    Status m_status = Status::Set;
    int m_waiting_threads = 0;
    std::string m_name;
    bool m_single_thread = false;
    bool m_fifo = false;
    u64 m_bits = 0;
};
} // namespace Libraries::Kernel