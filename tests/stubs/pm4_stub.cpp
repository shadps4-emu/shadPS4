// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/gnmdriver/gnmdriver.h"
#include "core/libraries/kernel/time.h"

namespace Libraries::Kernel {

u64 PS4_SYSV_ABI sceKernelGetTscFrequency() {
    return 1;
}

u64 PS4_SYSV_ABI sceKernelReadTsc() {
    return 0;
}

} // namespace Libraries::Kernel

namespace Libraries::GnmDriver {

u32 PS4_SYSV_ABI sceGnmGetGpuCoreClockFrequency() {
    return 1;
}

} // namespace Libraries::GnmDriver
