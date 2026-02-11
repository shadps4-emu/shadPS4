// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/kernel/time.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

constexpr int ORBIS_MAX_PATH = 255;

struct OrbisKernelIovec {
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
    OrbisKernelTimespec st_atim;
    OrbisKernelTimespec st_mtim;
    OrbisKernelTimespec st_ctim;
    s64 st_size;
    s64 st_blocks;
    u32 st_blksize;
    u32 st_flags;
    u32 st_gen;
    s32 st_lspare;
    OrbisKernelTimespec st_birthtim;
    u32 : (8 / 2) * (16 - static_cast<int>(sizeof(OrbisKernelTimespec)));
    u32 : (8 / 2) * (16 - static_cast<int>(sizeof(OrbisKernelTimespec)));
};

struct OrbisKernelDirent {
    u32 d_fileno;                    /* file number of entry */
    u16 d_reclen;                    /* length of this record */
    u8 d_type;                       /* file type, see below */
    u8 d_namlen;                     /* length of string in d_name */
    char d_name[ORBIS_MAX_PATH + 1]; /* name must be no longer than this */
};

// flags for Open
constexpr int ORBIS_KERNEL_O_RDONLY = 0x0000;
constexpr int ORBIS_KERNEL_O_WRONLY = 0x0001;
constexpr int ORBIS_KERNEL_O_RDWR = 0x0002;

constexpr int ORBIS_KERNEL_O_NONBLOCK = 0x0004;
constexpr int ORBIS_KERNEL_O_APPEND = 0x0008;
constexpr int ORBIS_KERNEL_O_FSYNC = 0x0080;
constexpr int ORBIS_KERNEL_O_SYNC = 0x0080;
constexpr int ORBIS_KERNEL_O_CREAT = 0x0200;
constexpr int ORBIS_KERNEL_O_TRUNC = 0x0400;
constexpr int ORBIS_KERNEL_O_EXCL = 0x0800;
constexpr int ORBIS_KERNEL_O_DSYNC = 0x1000;
constexpr int ORBIS_KERNEL_O_DIRECT = 0x00010000;
constexpr int ORBIS_KERNEL_O_DIRECTORY = 0x00020000;

s32 PS4_SYSV_ABI posix_open(const char* path, s32 flags, u16 mode);
s32 PS4_SYSV_ABI posix_close(s32 fd);
s64 PS4_SYSV_ABI posix_lseek(s32 fd, s64 offset, s32 whence);
s64 PS4_SYSV_ABI sceKernelWrite(s32 fd, const void* buf, u64 nbytes);
s64 PS4_SYSV_ABI sceKernelRead(s32 fd, void* buf, u64 nbytes);
s64 PS4_SYSV_ABI sceKernelPread(s32 fd, void* buf, u64 nbytes, s64 offset);
s64 PS4_SYSV_ABI sceKernelPwrite(s32 fd, void* buf, u64 nbytes, s64 offset);
void RegisterFileSystem(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
