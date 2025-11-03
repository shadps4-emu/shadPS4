// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <ctime>

#include "common/logging/log.h"
#include "common/va_ctx.h"
#include "core/file_sys/devices/rng_device.h"

namespace Core::Devices {

RngDevice::RngDevice() = default;
RngDevice::~RngDevice() = default;

s32 RngDevice::ioctl(u64 cmd, Common::VaCtx* args) {
    LOG_INFO(Kernel_Pthread, "called, cmd = {:#x}", cmd);
    // Both commands are for generating a random number
    if (cmd == 0x40445301 || cmd == 0x40445302) {
        auto& data = *vaArgPtr<GetRandomArgs>(&args->va_list);
        data.result = 0;
        for (u64 i = 0; i < 64; i++) {
            data.buf[i] = std::rand();
        }
    } else {
        // ENOIOCTL
        return -3;
    }
    return 0;
}

} // namespace Core::Devices