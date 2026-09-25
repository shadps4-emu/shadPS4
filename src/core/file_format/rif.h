// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <vector>
#include "common/endian.h"

constexpr u32 RIF_MAGIC = 0x52494600;
constexpr u32 RIFA_MAGIC = 0x72696661;
constexpr u32 RIDX_MAGIC = 0x72696478;
constexpr u8 RIDX_VERSION = 0x1;

constexpr u16 RIF_CONTENT_TYPE_PS4AL = 0x1C00;

struct RIFData {
    u32_be magic;
    u16_le version;
    u8 unk1[0x2];
    u64_le np_account_id;
    u64_le start_timestamp;
    u64_le end_timestamp;
    char content_id[0x30];
    u16_le type;
    u16_le drm_type;
    u16_le content_type;
    u16_le sku_flag;
    u64_le extra_flags;
    u8 unk2[0x1E0];
    u8 disk_key_hash[0x20];
    u8 secret_encryption_iv[0x10];
    u8 encrypted_secret[0x90];
    u8 rsa_signature[0x100];
};
static_assert(sizeof(RIFData) == 0x400);

struct RIFAHeader {
    u32_be magic;
    char service_id[0x14];
    u8 _reserved[0x3E8];
};
static_assert(sizeof(RIFAHeader) == 0x400);

struct RIDXHeader {
    u32_be magic;
    u32_le entry_count;
    u8 version;
    char service_id[0x13];
    u8 unk1[0x4];
};
static_assert(sizeof(RIDXHeader) == 0x20);

struct RIDXEntry {
    char entitlement_label[0x10];
    u64_le rif_offset;
    u64_le rif_size;
    u8 entitlement_control;
    u8 unk1;
    u8 rif_digest[0x8];
    u8 unk2[0x6];
};
static_assert(sizeof(RIDXEntry) == 0x30);

class RIF {
public:
    RIF() = default;
    ~RIF() = default;

    RIF(const RIF& other) = default;
    RIF(RIF&& other) noexcept = default;
    RIF& operator=(const RIF& other) = default;
    RIF& operator=(RIF&& other) noexcept = default;

    bool Open(const std::filesystem::path& ridx_path, const std::filesystem::path& rifa_path);
    bool Open(const std::vector<u8>& ridx_buffer, const std::vector<u8>& rifa_buffer);

    const std::string& GetServiceId() {
        return service_id;
    }

    const std::vector<std::string>& GetLicenseOnlyEntitlements() {
        return license_only_entitlements;
    }

private:
    std::string service_id;
    std::vector<std::string> license_only_entitlements;
};
