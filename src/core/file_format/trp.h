// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include "common/endian.h"
#include "common/io_file.h"
#include "common/types.h"

static constexpr u32 TRP_MAGIC = 0xDCA24D00;
static constexpr u8 ENTRY_FLAG_PNG = 0;
static constexpr u8 ENTRY_FLAG_ENCRYPTED_XML = 3;

struct TrpHeader {
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

struct TrpEntry {
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
    bool Extract(const std::filesystem::path& trophyPath, const std::string titleId);

private:
    bool ProcessPngEntry(Common::FS::IOFile& file, const TrpEntry& entry,
                         const std::filesystem::path& outputPath, std::string_view name);
    bool ProcessEncryptedXmlEntry(Common::FS::IOFile& file, const TrpEntry& entry,
                                  const std::filesystem::path& outputPath, std::string_view name,
                                  const std::array<u8, 16>& user_key, const std::string& npCommId);

    std::vector<u8> NPcommID = std::vector<u8>(12);
    std::array<u8, 16> np_comm_id{};
    std::array<u8, 16> esfmIv{};
    std::filesystem::path trpFilesPath;
    static constexpr int iv_len = 16;
};
