// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <mutex>

#include "common/types.h"

namespace Libraries::Kernel {

class EventFlagInternal {
public:
    enum class ClearMode { None, All, Bits };

    enum class WaitMode { And, Or };

    enum class ThreadMode { Single, Multi };

    enum class QueueMode { Fifo, ThreadPrio };

    EventFlagInternal(const std::string& name, ThreadMode thread_mode, QueueMode queue_mode,
                      uint64_t bits)
        : m_name(name), m_thread_mode(thread_mode), m_queue_mode(queue_mode), m_bits(bits) {};

    int Wait(u64 bits, WaitMode wait_mode, ClearMode clear_mode, u64* result, u32* ptr_micros);
    int Poll(u64 bits, WaitMode wait_mode, ClearMode clear_mode, u64* result);
    void Set(u64 bits);
    void Clear(u64 bits);

private:
    enum class Status { Set, Canceled, Deleted };

    std::mutex m_mutex;
    std::condition_variable m_cond_var;
    Status m_status = Status::Set;
    int m_waiting_threads = 0;
    std::string m_name;
    ThreadMode m_thread_mode = ThreadMode::Single;
    QueueMode m_queue_mode = QueueMode::Fifo;
    u64 m_bits = 0;
};
} // namespace Libraries::Kernel
