// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include "common/io_file.h"
#include "core/file_format/psf.h"

PSF::PSF() = default;

PSF::~PSF() = default;

bool PSF::open(const std::string& filepath, std::vector<u8> psfBuffer) {
    if (!psfBuffer.empty()) {
        psf.resize(psfBuffer.size());
        psf = psfBuffer;
    } else {
        Common::FS::IOFile file(filepath, Common::FS::FileAccessMode::Read);
        if (!file.IsOpen()) {
            return false;
        }

        const u64 psfSize = file.GetSize();
        psf.resize(psfSize);
        file.Seek(0);
        file.Read(psf);
        file.Close();
    }

    // Parse file contents
    PSFHeader header;
    std::memcpy(&header, psf.data(), sizeof(header));
    for (u32 i = 0; i < header.index_table_entries; i++) {
        PSFEntry entry;
        std::memcpy(&entry, &psf[sizeof(PSFHeader) + i * sizeof(PSFEntry)], sizeof(entry));

        const std::string key = (char*)&psf[header.key_table_offset + entry.key_offset];
        if (entry.param_fmt == PSFEntry::Fmt::TextRaw ||
            entry.param_fmt == PSFEntry::Fmt::TextNormal) {
            map_strings[key] = (char*)&psf[header.data_table_offset + entry.data_offset];
        }
        if (entry.param_fmt == PSFEntry::Fmt::Integer) {
            u32 value;
            std::memcpy(&value, &psf[header.data_table_offset + entry.data_offset], sizeof(value));
            map_integers[key] = value;
        }
    }
    return true;
}

std::string PSF::GetString(const std::string& key) {
    if (map_strings.find(key) != map_strings.end()) {
        return map_strings.at(key);
    }
    return "";
}

u32 PSF::GetInteger(const std::string& key) {
    if (map_integers.find(key) != map_integers.end()) {
        return map_integers.at(key);
    }
    return -1;
}
