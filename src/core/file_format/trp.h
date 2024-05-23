// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include "common/endian.h"
#include "core/crypto/crypto.h"
#include "src/common/io_file.h"
#include "src/common/types.h"

struct trp_header {
    u32_be magic; // (0xDCA24D00)
    u32_be version;
    u64_be file_size;         // size of full trp file
    u32_be entry_num;         // num entries
    u32_be entry_size;        // size of entry
    u32_be dev_flag;          // 1: dev
    unsigned char digest[20]; // sha1 hash
    u32_be key_index;         // 3031300?
    unsigned char padding[44];
};

struct trp_entry {
    char entry_name[32];
    u64_be entry_pos;
    u64_be entry_len;
    u32_be flag; // 3 = CONFIG/ESFM , 0 = PNG
    unsigned char padding[12];
};

class TRP {
public:
    TRP();
    ~TRP();
    bool Extract(std::filesystem::path trophyPath);
    void GetNPcommID(std::filesystem::path trophyPath, int fileNbr);
    void removePadding(std::vector<u8>& vec);

private:
    Crypto crypto;
    std::vector<std::vector<CryptoPP::byte>> NPcommID;
    std::vector<CryptoPP::byte> efsmIv;
    std::filesystem::path trpFilesPath;
};