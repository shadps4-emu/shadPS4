// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "kernel_memory.h"
#include <core/libraries/libs.h>

namespace Libraries::Kernel {

s32 PS4_SYSV_ABI sceKernelMapNamedFlexibleMemory(void** addrInOut, std::size_t len, int prot,
                                                 int flags, const char* name) {
    return 0;
}

void RegisterKernelMemory(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("mL8NDH86iQI", "libkernel", 1, "libkernel", 1, 1, sceKernelMapNamedFlexibleMemory);
}

} // namespace Libraries::Kernel