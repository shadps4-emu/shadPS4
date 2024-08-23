// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "common/endian.h"

struct PSFHeader {
    u32_be magic;
    u32_le version;
    u32_le key_table_offset;
    u32_le data_table_offset;
    u32_le index_table_entries;
};

struct PSFEntry {
    enum Fmt : u16 {
        TextRaw = 0x0400,    // String in UTF-8 format and not NULL terminated
        TextNormal = 0x0402, // String in UTF-8 format and NULL terminated
        Integer = 0x0404,    // Unsigned 32-bit integer
    };

    u16_le key_offset;
    u16_be param_fmt;
    u32_le param_len;
    u32_le param_max_len;
    u32_le data_offset;
};

class PSF {
public:
    PSF();
    ~PSF();

    bool open(const std::string& filepath, const std::vector<u8>& psfBuffer);

    std::string GetString(const std::string& key);
    u32 GetInteger(const std::string& key);

    std::unordered_map<std::string, std::string> map_strings;
    std::unordered_map<std::string, u32> map_integers;

private:
    std::vector<u8> psf;
};
