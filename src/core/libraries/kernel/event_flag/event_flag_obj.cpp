// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>
#include "core/libraries/error_codes.h"
#include "event_flag_obj.h"

namespace Libraries::Kernel {
int EventFlagInternal::Wait(u64 bits, WaitMode wait_mode, ClearMode clear_mode, u64* result,
                            u32* ptr_micros) {
    std::unique_lock lock{m_mutex};

    uint32_t micros = 0;
    bool infinitely = true;
    if (ptr_micros != nullptr) {
        micros = *ptr_micros;
        infinitely = false;
    }

    if (m_thread_mode == ThreadMode::Single && m_waiting_threads > 0) {
        return ORBIS_KERNEL_ERROR_EPERM;
    }

    auto const start = std::chrono::system_clock::now();
    m_waiting_threads++;
    auto waitFunc = [this, wait_mode, bits] {
        return (m_status == Status::Canceled || m_status == Status::Deleted ||
                (wait_mode == WaitMode::And && (m_bits & bits) == bits) ||
                (wait_mode == WaitMode::Or && (m_bits & bits) != 0));
    };

    if (infinitely) {
        m_cond_var.wait(lock, waitFunc);
    } else {
        if (!m_cond_var.wait_for(lock, std::chrono::microseconds(micros), waitFunc)) {
            if (result != nullptr) {
                *result = m_bits;
            }
            *ptr_micros = 0;
            --m_waiting_threads;
            return ORBIS_KERNEL_ERROR_ETIMEDOUT;
        }
    }
    --m_waiting_threads;
    if (result != nullptr) {
        *result = m_bits;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                       std::chrono::system_clock::now() - start)
                       .count();
    if (result != nullptr) {
        *result = m_bits;
    }

    if (ptr_micros != nullptr) {
        *ptr_micros = (elapsed >= micros ? 0 : micros - elapsed);
    }

    if (m_status == Status::Canceled) {
        return ORBIS_KERNEL_ERROR_ECANCELED;
    } else if (m_status == Status::Deleted) {
        return ORBIS_KERNEL_ERROR_EACCES;
    }

    if (clear_mode == ClearMode::All) {
        m_bits = 0;
    } else if (clear_mode == ClearMode::Bits) {
        m_bits &= ~bits;
    }

    return ORBIS_OK;
}

int EventFlagInternal::Poll(u64 bits, WaitMode wait_mode, ClearMode clear_mode, u64* result) {
    u32 micros = 0;
    auto ret = Wait(bits, wait_mode, clear_mode, result, &micros);
    if (ret == ORBIS_KERNEL_ERROR_ETIMEDOUT) {
        // Poll returns EBUSY instead.
        ret = ORBIS_KERNEL_ERROR_EBUSY;
    }
    return ret;
}

void EventFlagInternal::Set(u64 bits) {
    std::unique_lock lock{m_mutex};

    while (m_status != Status::Set) {
        m_mutex.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        m_mutex.lock();
    }

    m_bits |= bits;

    m_cond_var.notify_all();
}

void EventFlagInternal::Clear(u64 bits) {
    std::unique_lock lock{m_mutex};
    while (m_status != Status::Set) {
        m_mutex.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        m_mutex.lock();
    }

    m_bits &= bits;
}

} // namespace Libraries::Kernel
