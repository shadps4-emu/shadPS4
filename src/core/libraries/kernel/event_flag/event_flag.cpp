// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "event_flag.h"

namespace Libraries::Kernel {
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

    EventFlagInternal::ThreadMode thread_mode = EventFlagInternal::ThreadMode::Single;
    EventFlagInternal::QueueMode queue_mode = EventFlagInternal::QueueMode::Fifo;

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
    ef->Clear(bitPattern);
    return ORBIS_OK;
}
int PS4_SYSV_ABI sceKernelCancelEventFlag(OrbisKernelEventFlag ef, u64 setPattern,
                                          int* pNumWaitThreads) {
    LOG_ERROR(Kernel_Event, "(STUBBED) called");
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

    EventFlagInternal::WaitMode wait = EventFlagInternal::WaitMode::And;
    EventFlagInternal::ClearMode clear = EventFlagInternal::ClearMode::None;

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
        LOG_ERROR(Kernel_Event, "returned {}", result);
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

    EventFlagInternal::WaitMode wait = EventFlagInternal::WaitMode::And;
    EventFlagInternal::ClearMode clear = EventFlagInternal::ClearMode::None;

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
        LOG_ERROR(Kernel_Event, "returned {:#x}", result);
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
