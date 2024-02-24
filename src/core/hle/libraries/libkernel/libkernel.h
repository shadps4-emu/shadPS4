// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <sys/types.h>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibKernel {

int32_t PS4_SYSV_ABI sceKernelReleaseDirectMemory(off_t start, size_t len);
int* PS4_SYSV_ABI __Error();

void LibKernel_Register(Loader::SymbolsResolver* sym);

} // namespace Core::Libraries::LibKernel
