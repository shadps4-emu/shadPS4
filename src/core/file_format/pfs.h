// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <string>

#include "common/types.h"

#define PFS_FILE 2
#define PFS_DIR 3
#define PFS_CURRENT_DIR 4
#define PFS_PARENT_DIR 5

enum PfsMode : unsigned short {
    None = 0,
    Signed = 0x1,
    Is64Bit = 0x2,
    Encrypted = 0x4,
    UnknownFlagAlwaysSet = 0x8
};

struct PSFHeader_ {
    s64 version;
    s64 magic;
    s64 id;
    u8 fmode;
    u8 clean;
    u8 read_only;
    u8 rsv;
    PfsMode mode;
    s16 unk1;
    s32 block_size;
    s32 n_backup;
    s64 n_block;
    s64 dinode_count;
    s64 nd_block;
    s64 dinode_block_count;
    s64 superroot_ino;
};

struct PFSCHdr {
    s32 magic;
    s32 unk4;
    s32 unk8;
    s32 block_sz;
    s64 block_sz2;
    s64 block_offsets;
    u64 data_start;
    s64 data_length;
};

enum InodeMode : u16 {
    o_read = 1,
    o_write = 2,
    o_execute = 4,
    g_read = 8,
    g_write = 16,
    g_execute = 32,
    u_read = 64,
    u_write = 128,
    u_execute = 256,
    dir = 16384,
    file = 32768,
};

enum InodeFlags : u32 {
    compressed = 0x1,
    unk1 = 0x2,
    unk2 = 0x4,
    unk3 = 0x8,
    readonly = 0x10,
    unk4 = 0x20,
    unk5 = 0x40,
    unk6 = 0x80,
    unk7 = 0x100,
    unk8 = 0x200,
    unk9 = 0x400,
    unk10 = 0x800,
    unk11 = 0x1000,
    unk12 = 0x2000,
    unk13 = 0x4000,
    unk14 = 0x8000,
    unk15 = 0x10000,
    internal = 0x20000
};

struct Inode {
    u16 Mode;
    u16 Nlink;
    u32 Flags;
    s64 Size;
    s64 SizeCompressed;
    s64 Time1_sec;
    s64 Time2_sec;
    s64 Time3_sec;
    s64 Time4_sec;
    u32 Time1_nsec;
    u32 Time2_nsec;
    u32 Time3_nsec;
    u32 Time4_nsec;
    u32 Uid;
    u32 Gid;
    u64 Unk1;
    u64 Unk2;
    u32 Blocks;
    u32 loc;
};

struct pfs_fs_table {
    std::string name;
    u32 inode;
    u32 type;
};

struct Dirent {
    s32 ino;
    s32 type;
    s32 namelen;
    s32 entsize;
    char name[512];
};
