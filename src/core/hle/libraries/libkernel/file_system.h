// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibKernel {

struct SceKernelIovec {
    void* iov_base;
    std::size_t iov_len;
};

int PS4_SYSV_ABI sceKernelOpen(const char* path, int flags, /* SceKernelMode*/ u16 mode);

int PS4_SYSV_ABI posix_open(const char* path, int flags, /* SceKernelMode*/ u16 mode);
s64 PS4_SYSV_ABI lseek(int d, s64 offset, int whence);

void fileSystemSymbolsRegister(Loader::SymbolsResolver* sym);

} // namespace Core::Libraries::LibKernel
