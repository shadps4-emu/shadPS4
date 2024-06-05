// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <sys/types.h>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

struct OrbisTimesec {
    time_t t;
    u32 west_sec;
    u32 dst_sec;
};

int32_t PS4_SYSV_ABI sceKernelReleaseDirectMemory(off_t start, size_t len);
int* PS4_SYSV_ABI __Error();

void LibKernel_Register(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
