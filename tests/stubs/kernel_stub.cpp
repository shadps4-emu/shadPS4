// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/process.h"

namespace Libraries::Kernel {

s32 PS4_SYSV_ABI sceKernelGetCompiledSdkVersion(s32* ver) {
    if (ver) {
        *ver = 0x4500000;
    }
    return 0;
}

} // namespace Libraries::Kernel
