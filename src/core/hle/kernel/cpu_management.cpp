// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Util/config.h"
#include "common/log.h"
#include "core/hle/kernel/cpu_management.h"
#include "core/hle/libraries/libs.h"

namespace Core::Kernel {

int PS4_SYSV_ABI sceKernelIsNeoMode() {
    PRINT_FUNCTION_NAME();
    return Config::isNeoMode();
}

} // namespace Core::Kernel
