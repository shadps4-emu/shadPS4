// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include <core/libraries/system/userservice.h>
#include "core/libraries/save_data/save_backup.h"

class PSF;

namespace Libraries::SaveData::SaveMemory {

void PersistMemory(u32 slot_id, bool lock = true);

[[nodiscard]] std::string GetSaveDir(u32 slot_id);

[[nodiscard]] std::filesystem::path GetSavePath(
    Libraries::UserService::OrbisUserServiceUserId user_id, u32 slot_id,
    std::string_view game_serial);

// returns the size of the save memory if exists
size_t SetupSaveMemory(Libraries::UserService::OrbisUserServiceUserId user_id, u32 slot_id,
                       std::string_view game_serial, size_t memory_size);

// Write the icon. Set buf to null to read the standard icon.
void SetIcon(u32 slot_id, void* buf = nullptr, size_t buf_size = 0);

[[nodiscard]] bool IsSaveMemoryInitialized(u32 slot_id);

[[nodiscard]] PSF& GetParamSFO(u32 slot_id);

[[nodiscard]] std::vector<u8> GetIcon(u32 slot_id);

// Save now or wait for the background thread to save
void SaveSFO(u32 slot_id);

void ReadMemory(u32 slot_id, void* buf, size_t buf_size, int64_t offset);

void WriteMemory(u32 slot_id, void* buf, size_t buf_size, int64_t offset);

} // namespace Libraries::SaveData::SaveMemory