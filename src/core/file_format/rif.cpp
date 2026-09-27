// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <ranges>

#include "common/assert.h"
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
    ASSERT_MSG(ridxSize != 0, "RIDX file at {} is empty!", ridx_path.string());
    std::vector<u8> ridx(ridxSize);
    ridx_file.Seek(0);
    ridx_file.Read(ridx);
    ridx_file.Close();

    const u64 rifaSize = rifa_file.GetSize();
    ASSERT_MSG(rifaSize != 0, "RIFA file at {} is empty!", rifa_path.string());
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

        // License-only content has a specific type ID. Ignore anything else.
        if (rif.content_type == RIF_CONTENT_TYPE_PS4AL) {
            std::string entitlement_id(entry.entitlement_label, sizeof(entry.entitlement_label));
            license_only_entitlements.push_back(entitlement_id);
        }
    }

    return true;
}
