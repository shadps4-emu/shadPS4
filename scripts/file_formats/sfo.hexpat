// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

import std.io;
import std.sys;

struct Header {
    u32 magic;
    u32 version;
    u32 key_table_offset;
    u32 data_table_offset;
    u32 index_table_entries;
};

struct KeyEntry {
    char name[];
} [[inline]];

struct DataEntry<auto fmt, auto size> {
    if (fmt == 0x0404) {
        u32 int_value;
    } else if(fmt == 0x0004) {
		char bin_value[size];
	} else if(fmt == 0x0204) {
        char str_value[size];
    } else {
        std::warning("unknown fmt type");
    }
} [[inline]];

struct IndexEntry {
    u16 key_offset;
    u16 param_fmt;
    u32 param_len;
    u32 param_max_len;
    u32 data_offset;
};

struct Entry<auto KeyTableOffset, auto DataTableOffset> {
    u64 begin = $;
    IndexEntry index;
    KeyEntry key @ KeyTableOffset + index.key_offset;
    DataEntry<index.param_fmt, index.param_len> data @ DataTableOffset + index.data_offset;
	u8 data_empty[index.param_max_len - index.param_len] @ DataTableOffset + index.data_offset + index.param_len;
    $ = begin + sizeof(IndexEntry);
};

Header header @ 0;
std::assert(header.magic == 0x46535000, "Miss match magic");
std::assert(header.version == 0x00000101, "Miss match version");

Entry<header.key_table_offset, header.data_table_offset> list[header.index_table_entries] @ 0x14;