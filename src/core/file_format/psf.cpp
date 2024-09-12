// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

#include "common/assert.h"
#include "common/io_file.h"
#include "common/logging/log.h"
#include "core/file_format/psf.h"

static const std::unordered_map<std::string_view, u32> psf_known_max_sizes = {
    {"ACCOUNT_ID", 8},  {"CATEGORY", 4},  {"DETAIL", 1024},       {"FORMAT", 4},
    {"MAINTITLE", 128}, {"PARAMS", 1024}, {"SAVEDATA_BLOCKS", 8}, {"SAVEDATA_DIRECTORY", 32},
    {"SUBTITLE", 128},  {"TITLE_ID", 12},
};
static inline u32 get_max_size(std::string_view key, u32 default_value) {
    if (const auto& v = psf_known_max_sizes.find(key); v != psf_known_max_sizes.end()) {
        return v->second;
    }
    return default_value;
}

PSF::PSF() = default;

PSF::~PSF() = default;

PSF::PSF(const PSF& other) = default;

PSF::PSF(PSF&& other) noexcept
    : entry_list{std::move(other.entry_list)}, map_binaries{std::move(other.map_binaries)},
      map_strings{std::move(other.map_strings)}, map_integers{std::move(other.map_integers)} {}

PSF& PSF::operator=(const PSF& other) {
    if (this == &other)
        return *this;
    entry_list = other.entry_list;
    map_binaries = other.map_binaries;
    map_strings = other.map_strings;
    map_integers = other.map_integers;
    return *this;
}

PSF& PSF::operator=(PSF&& other) noexcept {
    if (this == &other)
        return *this;
    entry_list = std::move(other.entry_list);
    map_binaries = std::move(other.map_binaries);
    map_strings = std::move(other.map_strings);
    map_integers = std::move(other.map_integers);
    return *this;
}

bool PSF::Open(const std::filesystem::path& filepath) {
    last_write = std::filesystem::last_write_time(filepath);

    Common::FS::IOFile file(filepath, Common::FS::FileAccessMode::Read);
    if (!file.IsOpen()) {
        return false;
    }

    const u64 psfSize = file.GetSize();
    std::vector<u8> psf(psfSize);
    file.Seek(0);
    file.Read(psf);
    file.Close();
    return Open(psf);
}

bool PSF::Open(const std::vector<u8>& psf_buffer) {
    const u8* psf_data = psf_buffer.data();

    entry_list.clear();
    map_binaries.clear();
    map_strings.clear();
    map_integers.clear();

    // Parse file contents
    PSFHeader header{};
    std::memcpy(&header, psf_data, sizeof(header));

    if (header.magic != PSF_MAGIC) {
        LOG_ERROR(Core, "Invalid PSF magic number");
        return false;
    }
    if (header.version != PSF_VERSION_1_1 && header.version != PSF_VERSION_1_0) {
        LOG_ERROR(Core, "Unsupported PSF version: 0x{:08x}", header.version);
        return false;
    }

    for (u32 i = 0; i < header.index_table_entries; i++) {
        PSFRawEntry raw_entry{};
        std::memcpy(&raw_entry, psf_data + sizeof(PSFHeader) + i * sizeof(PSFRawEntry),
                    sizeof(raw_entry));

        Entry& entry = entry_list.emplace_back();
        entry.key = std::string{(char*)(psf_data + header.key_table_offset + raw_entry.key_offset)};
        entry.param_fmt = static_cast<PSFEntryFmt>(raw_entry.param_fmt.Raw());
        entry.max_len = raw_entry.param_max_len;

        const u8* data = psf_data + header.data_table_offset + raw_entry.data_offset;

        switch (entry.param_fmt) {
        case Binary: {
            std::vector<u8> value(raw_entry.param_len);
            std::memcpy(value.data(), data, raw_entry.param_len);
            map_binaries.emplace(i, std::move(value));
        } break;
        case Text: {
            std::string c_str{reinterpret_cast<const char*>(data)};
            map_strings.emplace(i, std::move(c_str));
        } break;
        case Integer: {
            ASSERT_MSG(raw_entry.param_len == sizeof(s32), "PSF integer entry size mismatch");
            s32 integer = *(s32*)data;
            map_integers.emplace(i, integer);
        } break;
        default:
            UNREACHABLE_MSG("Unknown PSF entry format");
        }
    }
    return true;
}

bool PSF::Encode(const std::filesystem::path& filepath) const {
    Common::FS::IOFile file(filepath, Common::FS::FileAccessMode::Write);
    if (!file.IsOpen()) {
        return false;
    }

    last_write = std::filesystem::file_time_type::clock::now();

    const auto psf_buffer = Encode();
    return file.Write(psf_buffer) == psf_buffer.size();
}

std::vector<u8> PSF::Encode() const {
    std::vector<u8> psf_buffer;
    Encode(psf_buffer);
    return psf_buffer;
}

void PSF::Encode(std::vector<u8>& psf_buffer) const {
    psf_buffer.resize(sizeof(PSFHeader) + sizeof(PSFRawEntry) * entry_list.size());

    {
        auto& header = *(PSFHeader*)psf_buffer.data();
        header.magic = PSF_MAGIC;
        header.version = PSF_VERSION_1_1;
        header.index_table_entries = entry_list.size();
    }

    const size_t key_table_offset = psf_buffer.size();
    ((PSFHeader*)psf_buffer.data())->key_table_offset = key_table_offset;
    for (size_t i = 0; i < entry_list.size(); i++) {
        auto& raw_entry = ((PSFRawEntry*)(psf_buffer.data() + sizeof(PSFHeader)))[i];
        const Entry& entry = entry_list[i];
        raw_entry.key_offset = psf_buffer.size() - key_table_offset;
        raw_entry.param_fmt.FromRaw(entry.param_fmt);
        raw_entry.param_max_len = entry.max_len;
        std::ranges::copy(entry.key, std::back_inserter(psf_buffer));
        psf_buffer.push_back(0); // NULL terminator
    }

    const size_t data_table_offset = psf_buffer.size();
    ((PSFHeader*)psf_buffer.data())->data_table_offset = data_table_offset;
    for (size_t i = 0; i < entry_list.size(); i++) {
        if (psf_buffer.size() % 4 != 0) {
            std::ranges::fill_n(std::back_inserter(psf_buffer), 4 - psf_buffer.size() % 4, 0);
        }
        auto& raw_entry = ((PSFRawEntry*)(psf_buffer.data() + sizeof(PSFHeader)))[i];
        const Entry& entry = entry_list[i];
        raw_entry.data_offset = psf_buffer.size() - data_table_offset;

        s32 additional_padding = s32(raw_entry.param_max_len);

        switch (entry.param_fmt) {
        case Binary: {
            const auto& value = map_binaries.at(i);
            raw_entry.param_len = value.size();
            additional_padding -= s32(raw_entry.param_len);
            std::ranges::copy(value, std::back_inserter(psf_buffer));
        } break;
        case Text: {
            const auto& value = map_strings.at(i);
            raw_entry.param_len = value.size() + 1;
            additional_padding -= s32(raw_entry.param_len);
            std::ranges::copy(value, std::back_inserter(psf_buffer));
            psf_buffer.push_back(0); // NULL terminator
        } break;
        case Integer: {
            const auto& value = map_integers.at(i);
            raw_entry.param_len = sizeof(s32);
            additional_padding -= s32(raw_entry.param_len);
            const auto value_bytes = reinterpret_cast<const u8*>(&value);
            std::ranges::copy(value_bytes, value_bytes + sizeof(s32),
                              std::back_inserter(psf_buffer));
        } break;
        default:
            UNREACHABLE_MSG("Unknown PSF entry format");
        }
        ASSERT_MSG(additional_padding >= 0, "PSF entry max size mismatch");
        std::ranges::fill_n(std::back_inserter(psf_buffer), additional_padding, 0);
    }
}

std::optional<std::span<const u8>> PSF::GetBinary(std::string_view key) const {
    const auto& [it, index] = FindEntry(key);
    if (it == entry_list.end()) {
        return {};
    }
    ASSERT(it->param_fmt == Binary);
    return std::span{map_binaries.at(index)};
}

std::optional<std::string_view> PSF::GetString(std::string_view key) const {
    const auto& [it, index] = FindEntry(key);
    if (it == entry_list.end()) {
        return {};
    }
    ASSERT(it->param_fmt == Text);
    return std::string_view{map_strings.at(index)};
}

std::optional<s32> PSF::GetInteger(std::string_view key) const {
    const auto& [it, index] = FindEntry(key);
    if (it == entry_list.end()) {
        return {};
    }
    ASSERT(it->param_fmt == Integer);
    return map_integers.at(index);
}

void PSF::AddBinary(std::string key, std::vector<u8> value, bool update) {
    auto [it, index] = FindEntry(key);
    bool exist = it != entry_list.end();
    if (exist && !update) {
        LOG_ERROR(Core, "PSF: Tried to add binary key that already exists: {}", key);
        return;
    }
    if (exist) {
        ASSERT_MSG(it->param_fmt == Binary, "PSF: Change format is not supported");
        it->max_len = get_max_size(key, value.size());
        map_binaries.at(index) = std::move(value);
        return;
    }
    Entry& entry = entry_list.emplace_back();
    entry.max_len = get_max_size(key, value.size());
    entry.key = std::move(key);
    entry.param_fmt = Binary;
    map_binaries.emplace(entry_list.size() - 1, std::move(value));
}

void PSF::AddString(std::string key, std::string value, bool update) {
    auto [it, index] = FindEntry(key);
    bool exist = it != entry_list.end();
    if (exist && !update) {
        LOG_ERROR(Core, "PSF: Tried to add string key that already exists: {}", key);
        return;
    }
    if (exist) {
        ASSERT_MSG(it->param_fmt == Text, "PSF: Change format is not supported");
        it->max_len = get_max_size(key, value.size() + 1);
        map_strings.at(index) = std::move(value);
        return;
    }
    Entry& entry = entry_list.emplace_back();
    entry.max_len = get_max_size(key, value.size() + 1);
    entry.key = std::move(key);
    entry.param_fmt = Text;
    map_strings.emplace(entry_list.size() - 1, std::move(value));
}

void PSF::AddInteger(std::string key, s32 value, bool update) {
    auto [it, index] = FindEntry(key);
    bool exist = it != entry_list.end();
    if (exist && !update) {
        LOG_ERROR(Core, "PSF: Tried to add integer key that already exists: {}", key);
        return;
    }
    if (exist) {
        ASSERT_MSG(it->param_fmt == Integer, "PSF: Change format is not supported");
        it->max_len = sizeof(s32);
        map_integers.at(index) = value;
        return;
    }
    Entry& entry = entry_list.emplace_back();
    entry.key = std::move(key);
    entry.param_fmt = Integer;
    entry.max_len = sizeof(s32);
    map_integers.emplace(entry_list.size() - 1, value);
}

std::pair<std::vector<PSF::Entry>::iterator, size_t> PSF::FindEntry(std::string_view key) {
    auto entry =
        std::ranges::find_if(entry_list, [&](const auto& entry) { return entry.key == key; });
    return {entry, std::distance(entry_list.begin(), entry)};
}

std::pair<std::vector<PSF::Entry>::const_iterator, size_t> PSF::FindEntry(
    std::string_view key) const {
    auto entry =
        std::ranges::find_if(entry_list, [&](const auto& entry) { return entry.key == key; });
    return {entry, std::distance(entry_list.begin(), entry)};
}
