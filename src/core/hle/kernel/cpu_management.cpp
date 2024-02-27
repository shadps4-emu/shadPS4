// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Util/config.h"
#include "common/logging/log.h"
#include "core/hle/kernel/cpu_management.h"

namespace Core::Kernel {

int PS4_SYSV_ABI sceKernelIsNeoMode() {
    LOG_INFO(Kernel_Sce, "called");
    return Config::isNeoMode();
}

} // namespace Core::Kernel
