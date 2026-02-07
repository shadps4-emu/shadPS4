// SPDX-FileCopyrightText: Copyright 2025-2026 shadLauncher4 Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "common/endian.h"
#include "common/types.h"

#define NPBIND_MAGIC 0xD294A018u

#pragma pack(push, 1)
struct NpBindHeader {
    u32_be magic;
    u32_be version;
    u64_be file_size;
    u64_be entry_size;
    u64_be num_entries;
    char padding[0x60]; // 96 bytes
};
#pragma pack(pop)

struct NPBindEntryRaw {
    u16_be type;
    u16_be size; // includes internal padding
    std::vector<u8> data;
};

struct NPBindBody {
    NPBindEntryRaw npcommid; // expected type 0x0010, size 12
    NPBindEntryRaw trophy;   // expected type 0x0011, size 12
    NPBindEntryRaw unk1;     // expected type 0x0012, size 176
    NPBindEntryRaw unk2;     // expected type 0x0013, size 16
    // The 0x98 padding after these entries is skipped while parsing
};

class NPBindFile {
private:
    NpBindHeader m_header;
    std::vector<NPBindBody> m_bodies;
    u8 m_digest[20]; // zeroed if absent

public:
    NPBindFile() {
        memset(m_digest, 0, sizeof(m_digest));
    }

    // Load from file
    bool Load(const std::string& path);

    // Accessors
    const NpBindHeader& Header() const {
        return m_header;
    }
    const std::vector<NPBindBody>& Bodies() const {
        return m_bodies;
    }
    const u8* Digest() const {
        return m_digest;
    }

    // Get npcommid data
    std::vector<std::string> GetNpCommIds() const;

    // Get specific body
    const NPBindBody& GetBody(size_t index) const {
        return m_bodies.at(index);
    }

    // Get number of bodies
    u64 BodyCount() const {
        return m_bodies.size();
    }

    // Check if file was loaded successfully
    bool IsValid() const {
        return m_header.magic == NPBIND_MAGIC;
    }

    // Clear all data
    void Clear() {
        m_header = NpBindHeader{};
        m_bodies.clear();
        memset(m_digest, 0, sizeof(m_digest));
    }
};