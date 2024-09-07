// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/logging/log.h"
#include "core/libraries/kernel/cpu_management.h"

namespace Libraries::Kernel {

int PS4_SYSV_ABI sceKernelIsNeoMode() {
    LOG_DEBUG(Kernel_Sce, "called");
    return Config::isNeoMode();
}

} // namespace Libraries::Kernel
