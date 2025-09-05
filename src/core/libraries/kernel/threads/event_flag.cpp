// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <condition_variable>
#include <mutex>
#include <thread>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

constexpr int ORBIS_KERNEL_EVF_ATTR_TH_FIFO = 0x01;
constexpr int ORBIS_KERNEL_EVF_ATTR_TH_PRIO = 0x02;
constexpr int ORBIS_KERNEL_EVF_ATTR_SINGLE = 0x10;
constexpr int ORBIS_KERNEL_EVF_ATTR_MULTI = 0x20;

constexpr int ORBIS_KERNEL_EVF_WAITMODE_AND = 0x01;
constexpr int ORBIS_KERNEL_EVF_WAITMODE_OR = 0x02;
constexpr int ORBIS_KERNEL_EVF_WAITMODE_CLEAR_ALL = 0x10;
constexpr int ORBIS_KERNEL_EVF_WAITMODE_CLEAR_PAT = 0x20;

class EventFlagInternal {
public:
    enum class ClearMode { None, All, Bits };
    enum class WaitMode { And, Or };
    enum class ThreadMode { Single, Multi };
    enum class QueueMode { Fifo, ThreadPrio };

    EventFlagInternal(const std::string& name, ThreadMode thread_mode, QueueMode queue_mode,
                      uint64_t bits)
        : m_name(name), m_thread_mode(thread_mode), m_queue_mode(queue_mode), m_bits(bits) {};

    int Wait(u64 bits, WaitMode wait_mode, ClearMode clear_mode, u64* result, u32* ptr_micros) {
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

    int Poll(u64 bits, WaitMode wait_mode, ClearMode clear_mode, u64* result) {
        u32 micros = 0;
        auto ret = Wait(bits, wait_mode, clear_mode, result, &micros);
        if (ret == ORBIS_KERNEL_ERROR_ETIMEDOUT) {
            // Poll returns EBUSY instead.
            ret = ORBIS_KERNEL_ERROR_EBUSY;
        }
        return ret;
    }

    void Set(u64 bits) {
        std::unique_lock lock{m_mutex};

        while (m_status != Status::Set) {
            m_mutex.unlock();
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            m_mutex.lock();
        }

        m_bits |= bits;
        m_cond_var.notify_all();
    }

    void Clear(u64 bits) {
        std::unique_lock lock{m_mutex};
        while (m_status != Status::Set) {
            m_mutex.unlock();
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            m_mutex.lock();
        }

        m_bits &= bits;
    }

    void Cancel(u64 setPattern, int* numWaitThreads) {
        std::unique_lock lock{m_mutex};

        while (m_status != Status::Set) {
            m_mutex.unlock();
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            m_mutex.lock();
        }

        if (numWaitThreads) {
            *numWaitThreads = m_waiting_threads;
        }

        m_status = Status::Canceled;
        m_bits = setPattern;

        m_cond_var.notify_all();

        while (m_waiting_threads > 0) {
            m_mutex.unlock();
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            m_mutex.lock();
        }

        m_status = Status::Set;
    }

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

using OrbisKernelUseconds = u32;
using OrbisKernelEventFlag = EventFlagInternal*;

struct OrbisKernelEventFlagOptParam {
    size_t size;
};

int PS4_SYSV_ABI sceKernelCreateEventFlag(OrbisKernelEventFlag* ef, const char* pName, u32 attr,
                                          u64 initPattern,
                                          const OrbisKernelEventFlagOptParam* pOptParam) {
    LOG_INFO(Kernel_Event, "called name = {} attr = {:#x} initPattern = {:#x}", pName, attr,
             initPattern);
    if (ef == nullptr || pName == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (pOptParam || !pName ||
        attr > (ORBIS_KERNEL_EVF_ATTR_MULTI | ORBIS_KERNEL_EVF_ATTR_TH_PRIO)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (strlen(pName) >= 32) {
        return ORBIS_KERNEL_ERROR_ENAMETOOLONG;
    }

    auto thread_mode = EventFlagInternal::ThreadMode::Single;
    auto queue_mode = EventFlagInternal::QueueMode::Fifo;
    switch (attr & 0xfu) {
    case 0x01:
        queue_mode = EventFlagInternal::QueueMode::Fifo;
        break;
    case 0x02:
        queue_mode = EventFlagInternal::QueueMode::ThreadPrio;
        break;
    case 0x00:
        break;
    default:
        UNREACHABLE();
    }

    switch (attr & 0xf0) {
    case 0x10:
        thread_mode = EventFlagInternal::ThreadMode::Single;
        break;
    case 0x20:
        thread_mode = EventFlagInternal::ThreadMode::Multi;
        break;
    case 0x00:
        break;
    default:
        UNREACHABLE();
    }

    if (queue_mode == EventFlagInternal::QueueMode::ThreadPrio) {
        LOG_ERROR(Kernel_Event, "ThreadPriority attr is not supported!");
    }

    *ef = new EventFlagInternal(std::string(pName), thread_mode, queue_mode, initPattern);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelDeleteEventFlag(OrbisKernelEventFlag ef) {
    if (ef == nullptr) {
        return ORBIS_KERNEL_ERROR_ESRCH;
    }

    delete ef;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelOpenEventFlag() {
    LOG_ERROR(Kernel_Event, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelCloseEventFlag() {
    LOG_ERROR(Kernel_Event, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelClearEventFlag(OrbisKernelEventFlag ef, u64 bitPattern) {
    LOG_DEBUG(Kernel_Event, "called");
    if (ef == nullptr) {
        return ORBIS_KERNEL_ERROR_ESRCH;
    }
    ef->Clear(bitPattern);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelCancelEventFlag(OrbisKernelEventFlag ef, u64 setPattern,
                                          int* pNumWaitThreads) {
    LOG_DEBUG(Kernel_Event, "called");
    if (ef == nullptr) {
        return ORBIS_KERNEL_ERROR_ESRCH;
    }
    ef->Cancel(setPattern, pNumWaitThreads);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelSetEventFlag(OrbisKernelEventFlag ef, u64 bitPattern) {
    LOG_TRACE(Kernel_Event, "called");
    if (ef == nullptr) {
        return ORBIS_KERNEL_ERROR_ESRCH;
    }
    ef->Set(bitPattern);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelPollEventFlag(OrbisKernelEventFlag ef, u64 bitPattern, u32 waitMode,
                                        u64* pResultPat) {
    LOG_DEBUG(Kernel_Event, "called bitPattern = {:#x} waitMode = {:#x}", bitPattern, waitMode);

    if (ef == nullptr) {
        return ORBIS_KERNEL_ERROR_ESRCH;
    }

    if (bitPattern == 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto wait = EventFlagInternal::WaitMode::And;
    auto clear = EventFlagInternal::ClearMode::None;
    switch (waitMode & 0xf) {
    case 0x01:
        wait = EventFlagInternal::WaitMode::And;
        break;
    case 0x02:
        wait = EventFlagInternal::WaitMode::Or;
        break;
    default:
        UNREACHABLE();
    }

    switch (waitMode & 0xf0) {
    case 0x00:
        clear = EventFlagInternal::ClearMode::None;
        break;
    case 0x10:
        clear = EventFlagInternal::ClearMode::All;
        break;
    case 0x20:
        clear = EventFlagInternal::ClearMode::Bits;
        break;
    default:
        UNREACHABLE();
    }

    auto result = ef->Poll(bitPattern, wait, clear, pResultPat);

    if (result != ORBIS_OK && result != ORBIS_KERNEL_ERROR_EBUSY) {
        LOG_DEBUG(Kernel_Event, "returned {:#x}", result);
    }

    return result;
}
int PS4_SYSV_ABI sceKernelWaitEventFlag(OrbisKernelEventFlag ef, u64 bitPattern, u32 waitMode,
                                        u64* pResultPat, OrbisKernelUseconds* pTimeout) {
    LOG_DEBUG(Kernel_Event, "called bitPattern = {:#x} waitMode = {:#x}", bitPattern, waitMode);
    if (ef == nullptr) {
        return ORBIS_KERNEL_ERROR_ESRCH;
    }

    if (bitPattern == 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto wait = EventFlagInternal::WaitMode::And;
    auto clear = EventFlagInternal::ClearMode::None;
    switch (waitMode & 0xf) {
    case 0x01:
        wait = EventFlagInternal::WaitMode::And;
        break;
    case 0x02:
        wait = EventFlagInternal::WaitMode::Or;
        break;
    default:
        UNREACHABLE();
    }

    switch (waitMode & 0xf0) {
    case 0x00:
        clear = EventFlagInternal::ClearMode::None;
        break;
    case 0x10:
        clear = EventFlagInternal::ClearMode::All;
        break;
    case 0x20:
        clear = EventFlagInternal::ClearMode::Bits;
        break;
    default:
        UNREACHABLE();
    }

    u32 result = ef->Wait(bitPattern, wait, clear, pResultPat, pTimeout);

    if (result != ORBIS_OK && result != ORBIS_KERNEL_ERROR_ETIMEDOUT) {
        LOG_DEBUG(Kernel_Event, "returned {:#x}", result);
    }

    return result;
}

void RegisterKernelEventFlag(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("PZku4ZrXJqg", "libkernel", 1, "libkernel", 1, 1, sceKernelCancelEventFlag);
    LIB_FUNCTION("7uhBFWRAS60", "libkernel", 1, "libkernel", 1, 1, sceKernelClearEventFlag);
    LIB_FUNCTION("s9-RaxukuzQ", "libkernel", 1, "libkernel", 1, 1, sceKernelCloseEventFlag);
    LIB_FUNCTION("BpFoboUJoZU", "libkernel", 1, "libkernel", 1, 1, sceKernelCreateEventFlag);
    LIB_FUNCTION("8mql9OcQnd4", "libkernel", 1, "libkernel", 1, 1, sceKernelDeleteEventFlag);
    LIB_FUNCTION("1vDaenmJtyA", "libkernel", 1, "libkernel", 1, 1, sceKernelOpenEventFlag);
    LIB_FUNCTION("9lvj5DjHZiA", "libkernel", 1, "libkernel", 1, 1, sceKernelPollEventFlag);
    LIB_FUNCTION("IOnSvHzqu6A", "libkernel", 1, "libkernel", 1, 1, sceKernelSetEventFlag);
    LIB_FUNCTION("JTvBflhYazQ", "libkernel", 1, "libkernel", 1, 1, sceKernelWaitEventFlag);
}

} // namespace Libraries::Kernel
