// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "common/endian.h"

constexpr u32 PSF_MAGIC = 0x00505346;
constexpr u32 PSF_VERSION_1_1 = 0x00000101;
constexpr u32 PSF_VERSION_1_0 = 0x00000100;

struct PSFHeader {
    u32_be magic;
    u32_le version;
    u32_le key_table_offset;
    u32_le data_table_offset;
    u32_le index_table_entries;
};
static_assert(sizeof(PSFHeader) == 0x14);

struct PSFRawEntry {
    u16_le key_offset;
    u16_be param_fmt;
    u32_le param_len;
    u32_le param_max_len;
    u32_le data_offset;
};
static_assert(sizeof(PSFRawEntry) == 0x10);

enum class PSFEntryFmt : u16 {
    Binary = 0x0004,  // Binary data
    Text = 0x0204,    // String in UTF-8 format and NULL terminated
    Integer = 0x0404, // Signed 32-bit integer
};

class PSF {
    struct Entry {
        std::string key;
        PSFEntryFmt param_fmt;
        u32 max_len;
    };

public:
    PSF() = default;
    ~PSF() = default;

    PSF(const PSF& other) = default;
    PSF(PSF&& other) noexcept = default;
    PSF& operator=(const PSF& other) = default;
    PSF& operator=(PSF&& other) noexcept = default;

    bool Open(const std::filesystem::path& filepath);
    bool Open(const std::vector<u8>& psf_buffer);

    [[nodiscard]] std::vector<u8> Encode() const;
    void Encode(std::vector<u8>& buf) const;
    bool Encode(const std::filesystem::path& filepath) const;

    std::optional<std::span<const u8>> GetBinary(std::string_view key) const;
    std::optional<std::string_view> GetString(std::string_view key) const;
    std::optional<s32> GetInteger(std::string_view key) const;

    void AddBinary(std::string key, std::vector<u8> value, bool update = false);
    void AddBinary(std::string key, uint64_t value, bool update = false); // rsv4 format
    void AddString(std::string key, std::string value, bool update = false);
    void AddInteger(std::string key, s32 value, bool update = false);

    [[nodiscard]] std::filesystem::file_time_type GetLastWrite() const {
        return last_write;
    }

    [[nodiscard]] const std::vector<Entry>& GetEntries() const {
        return entry_list;
    }

private:
    mutable std::filesystem::file_time_type last_write;

    std::vector<Entry> entry_list;

    std::unordered_map<size_t, std::vector<u8>> map_binaries;
    std::unordered_map<size_t, std::string> map_strings;
    std::unordered_map<size_t, s32> map_integers;

    [[nodiscard]] std::pair<std::vector<Entry>::iterator, size_t> FindEntry(std::string_view key);
    [[nodiscard]] std::pair<std::vector<Entry>::const_iterator, size_t> FindEntry(
        std::string_view key) const;
};
