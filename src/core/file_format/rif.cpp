// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <ranges>

#include "common/io_file.h"
#include "common/logging/log.h"
#include "core/file_format/rif.h"

bool RIF::Open(const std::filesystem::path& ridx_path, const std::filesystem::path& rifa_path) {
    Common::FS::IOFile ridx_file(ridx_path, Common::FS::FileAccessMode::Read);
    if (!ridx_file.IsOpen()) {
        return false;
    }

    Common::FS::IOFile rifa_file(rifa_path, Common::FS::FileAccessMode::Read);
    if (!rifa_file.IsOpen()) {
        return false;
    }

    const u64 ridxSize = ridx_file.GetSize();
    if (ridxSize == 0) {
        LOG_ERROR(Core, "RIDX file at {} is empty", ridx_path.string());
        return false;
    }

    std::vector<u8> ridx(ridxSize);
    ridx_file.Seek(0);
    ridx_file.Read(ridx);
    ridx_file.Close();

    const u64 rifaSize = rifa_file.GetSize();
    if (rifaSize == 0) {
        LOG_ERROR(Core, "RIFA file at {} is empty", rifa_path.string());
        return false;
    }

    std::vector<u8> rifa(rifaSize);
    rifa_file.Seek(0);
    rifa_file.Read(rifa);
    rifa_file.Close();

    return Open(ridx, rifa);
}

bool RIF::Open(const std::vector<u8>& ridx_buffer, const std::vector<u8>& rifa_buffer) {
    const u8* ridx_data = ridx_buffer.data();
    const u8* rifa_data = rifa_buffer.data();

    service_id = "";
    license_only_entitlements.clear();
    all_entitlements.clear();

    RIDXHeader ridx_header{};
    std::memcpy(&ridx_header, ridx_data, sizeof(ridx_header));

    if (ridx_header.magic != RIDX_MAGIC) {
        LOG_ERROR(Core, "Invalid RIDX magic number");
        return false;
    }
    if (ridx_header.version != RIDX_VERSION) {
        LOG_ERROR(Core, "Unsupported RIDX version: 0x{:08x}", ridx_header.version);
        return false;
    }

    RIFAHeader rifa_header{};
    std::memcpy(&rifa_header, rifa_data, sizeof(rifa_header));

    if (rifa_header.magic != RIFA_MAGIC) {
        LOG_ERROR(Core, "Invalid RIFA magic number");
        return false;
    }

    service_id = std::string(ridx_header.service_id, sizeof(ridx_header.service_id));

    for (u32 i = 0; i < ridx_header.entry_count; i++) {
        RIDXEntry entry{};
        std::memcpy(&entry, ridx_data + sizeof(RIDXHeader) + i * sizeof(RIDXEntry), sizeof(entry));

        if (entry.rif_size != sizeof(RIFData)) {
            LOG_ERROR(Core, "Invalid RIF size at {}", i);
            continue;
        }

        RIFData rif{};
        std::memcpy(&rif, rifa_data + entry.rif_offset, sizeof(rif));

        if (rif.magic != RIF_MAGIC) {
            LOG_ERROR(Core, "Invalid RIF magic number at {}", i);
            continue;
        }

        std::string entitlement_id(
            entry.entitlement_label,
            strnlen(entry.entitlement_label, sizeof(entry.entitlement_label)));
        all_entitlements.emplace(entitlement_id, rif);

        // License-only content has a specific type ID. Ignore anything else.
        if (rif.content_type == RIF_CONTENT_TYPE_PS4AL) {
            license_only_entitlements.emplace(entitlement_id, rif);
        }
    }

    return true;
}

std::optional<u32> RIF::GetSkuFlag() const {
    for (const auto& [label, rif] : all_entitlements) {
        if (rif.content_type == 0x1a00 || rif.content_type == 0x001a) {
            u16 val_le = static_cast<u16>(rif.sku_flag);
            u16 val_be = std::byteswap(val_le);
            u16 flag = (val_be != 0 && val_be < 0x100) ? val_be : val_le;
            if (flag != 0) {
                return flag;
            }
        }
    }
    return std::nullopt;
}

bool RIF::GetEntitlementKey(std::string_view label, std::span<u8, 16> out_key) const {
    auto it = all_entitlements.find(std::string(label));
    if (it != all_entitlements.end()) {
        std::memcpy(out_key.data(), &it->second.encrypted_secret[RIF_SECRET_ENTITLEMENT_KEY_OFFSET],
                    16);
        return true;
    }
    return false;
}
