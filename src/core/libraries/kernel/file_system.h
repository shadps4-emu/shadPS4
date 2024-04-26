// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "thread_management.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

struct SceKernelIovec {
    void* iov_base;
    std::size_t iov_len;
};

struct OrbisKernelStat {
    u32 st_dev;
    u32 st_ino;
    u16 st_mode;
    u16 st_nlink;
    u32 st_uid;
    u32 st_gid;
    u32 st_rdev;
    SceKernelTimespec st_atim;
    SceKernelTimespec st_mtim;
    SceKernelTimespec st_ctim;
    s64 st_size;
    s64 st_blocks;
    u32 st_blksize;
    u32 st_flags;
    u32 st_gen;
    int32_t st_lspare;
    SceKernelTimespec st_birthtim;
    unsigned int : (8 / 2) * (16 - static_cast<int>(sizeof(SceKernelTimespec)));
    unsigned int : (8 / 2) * (16 - static_cast<int>(sizeof(SceKernelTimespec)));
};

int PS4_SYSV_ABI sceKernelOpen(const char* path, int flags, /* SceKernelMode*/ u16 mode);

int PS4_SYSV_ABI posix_open(const char* path, int flags, /* SceKernelMode*/ u16 mode);
s64 PS4_SYSV_ABI lseek(int d, s64 offset, int whence);

void fileSystemSymbolsRegister(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
