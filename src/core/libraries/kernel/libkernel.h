// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <sys/types.h>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

void ErrSceToPosix(int result);
int ErrnoToSceKernelError(int e);
void SetPosixErrno(int e);

struct OrbisTimesec {
    time_t t;
    u32 west_sec;
    u32 dst_sec;
};

typedef struct {
    uint32_t timeLow;
    uint16_t timeMid;
    uint16_t timeHiAndVersion;
    uint8_t clockSeqHiAndReserved;
    uint8_t clockSeqLow;
    uint8_t node[6];
} OrbisKernelUuid;

int* PS4_SYSV_ABI __Error();
int PS4_SYSV_ABI sceKernelGetCompiledSdkVersion(int* ver);

void LibKernel_Register(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
