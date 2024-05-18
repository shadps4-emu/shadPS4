// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/event_queues.h"

namespace Libraries::Kernel {

int PS4_SYSV_ABI sceKernelCreateEqueue(SceKernelEqueue* eq, const char* name) {
    if (eq == nullptr) {
        LOG_ERROR(Kernel_Event, "Event queue is null!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (name == nullptr) {
        LOG_ERROR(Kernel_Event, "Event queue name is null!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    // Maximum is 32 including null terminator
    static constexpr size_t MaxEventQueueNameSize = 32;
    if (std::strlen(name) > MaxEventQueueNameSize) {
        LOG_ERROR(Kernel_Event, "Event queue name exceeds 32 bytes!");
        return ORBIS_KERNEL_ERROR_ENAMETOOLONG;
    }

    LOG_INFO(Kernel_Event, "name = {}", name);

    *eq = new EqueueInternal;
    (*eq)->setName(std::string(name));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelDeleteEqueue(SceKernelEqueue eq) {
    if (eq == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    delete eq;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelWaitEqueue(SceKernelEqueue eq, SceKernelEvent* ev, int num, int* out,
                                     SceKernelUseconds* timo) {
    LOG_INFO(Kernel_Event, "num = {}", num);

    if (eq == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    if (ev == nullptr) {
        return SCE_KERNEL_ERROR_EFAULT;
    }

    if (num < 1) {
        return SCE_KERNEL_ERROR_EINVAL;
    }

    if (timo == nullptr) { // wait until an event arrives without timing out
        *out = eq->waitForEvents(ev, num, 0);
    }

    if (timo != nullptr) {
        // Only events that have already arrived at the time of this function call can be received
        if (*timo == 0) {
            UNREACHABLE();
        } else {
            // Wait until an event arrives with timing out
            UNREACHABLE();
        }
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelAddUserEvent(SceKernelEqueue eq, int id) {
    if (eq == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    Kernel::EqueueEvent event{};
    event.isTriggered = false;
    event.event.ident = id;
    event.event.filter = Kernel::EVFILT_USER;
    event.event.udata = 0;
    event.event.fflags = 0;
    event.event.data = 0;

    return eq->addEvent(event);
}

void* PS4_SYSV_ABI sceKernelGetEventUserData(const SceKernelEvent* ev) {
    if (!ev) {
        return nullptr;
    }

    return ev->udata;
}

} // namespace Libraries::Kernel
