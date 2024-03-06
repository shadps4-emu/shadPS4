// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdio>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include "common/endian.h"
#include "core/crypto/crypto.h"
#include "pfs.h"

struct PKGHeader {
    u32_be magic; // Magic
    u32_be pkg_type;
    u32_be pkg_0x8; // unknown field
    u32_be pkg_file_count;
    u32_be pkg_table_entry_count;
    u16_be pkg_sc_entry_count;
    u16_be pkg_table_entry_count_2; // same as pkg_entry_count
    u32_be pkg_table_entry_offset;  // file table offset
    u32_be pkg_sc_entry_data_size;
    u64_be pkg_body_offset; // offset of PKG entries
    u64_be pkg_body_size;   // length of all PKG entries
    u64_be pkg_content_offset;
    u64_be pkg_content_size;
    u8 pkg_content_id[0x24];  // packages' content ID as a 36-byte string
    u8 pkg_padding[0xC];      // padding
    u32_be pkg_drm_type;      // DRM type
    u32_be pkg_content_type;  // Content type
    u32_be pkg_content_flags; // Content flags
    u32_be pkg_promote_size;
    u32_be pkg_version_date;
    u32_be pkg_version_hash;
    u32_be pkg_0x088;
    u32_be pkg_0x08C;
    u32_be pkg_0x090;
    u32_be pkg_0x094;
    u32_be pkg_iro_tag;
    u32_be pkg_drm_type_version;

    u8 pkg_zeroes_1[0x60];

    /* Digest table */
    u8 digest_entries1[0x20];     // sha256 digest for main entry 1
    u8 digest_entries2[0x20];     // sha256 digest for main entry 2
    u8 digest_table_digest[0x20]; // sha256 digest for digest table
    u8 digest_body_digest[0x20];  // sha256 digest for main table

    u8 pkg_zeroes_2[0x280];

    u32_be pkg_0x400;

    u32_be pfs_image_count;  // count of PFS images
    u64_be pfs_image_flags;  // PFS flags
    u64_be pfs_image_offset; // offset to start of external PFS image
    u64_be pfs_image_size;   // size of external PFS image
    u64_be mount_image_offset;
    u64_be mount_image_size;
    u64_be pkg_size;
    u32_be pfs_signed_size;
    u32_be pfs_cache_size;
    u8 pfs_image_digest[0x20];
    u8 pfs_signed_digest[0x20];
    u64_be pfs_split_size_nth_0;
    u64_be pfs_split_size_nth_1;

    u8 pkg_zeroes_3[0xB50];

    u8 pkg_digest[0x20];
};

struct PKGEntry {
    u32_be id;              // File ID, useful for files without a filename entry
    u32_be filename_offset; // Offset into the filenames table (ID 0x200) where this file's name is
                            // located
    u32_be flags1;          // Flags including encrypted flag, etc
    u32_be flags2;          // Flags including encryption key index, etc
    u32_be offset;          // Offset into PKG to find the file
    u32_be size;            // Size of the file
    u64_be padding;         // blank padding
};
static_assert(sizeof(PKGEntry) == 32);

class PKG {
public:
    PKG();
    ~PKG();

    bool Open(const std::string& filepath);
    void ExtractFiles(const int& index);
    bool Extract(const std::string& filepath, const std::filesystem::path& extract,
                 std::string& failreason);

    u32 GetNumberOfFiles() {
        return fsTable.size();
    }

    u64 GetPkgSize() {
        return pkgSize;
    }

    std::string_view GetTitleID() {
        return std::string_view(pkgTitleID, 9);
    }

private:
    Crypto crypto;
    std::vector<u8> pkg;
    u64 pkgSize = 0;
    char pkgTitleID[9];
    PKGHeader pkgheader;

    std::unordered_map<int, std::filesystem::path> extractPaths;
    std::vector<pfs_fs_table> fsTable;
    std::vector<Inode> iNodeBuf;
    std::vector<u64> sectorMap;
    u64 pfsc_offset;

    std::array<CryptoPP::byte, 32> dk3_;
    std::array<CryptoPP::byte, 32> ivKey;
    std::array<CryptoPP::byte, 256> imgKey;
    std::array<CryptoPP::byte, 32> ekpfsKey;
    std::array<CryptoPP::byte, 16> dataKey;
    std::array<CryptoPP::byte, 16> tweakKey;

    std::filesystem::path pkgpath;
    std::filesystem::path current_dir;
    std::filesystem::path extract_path;
};
