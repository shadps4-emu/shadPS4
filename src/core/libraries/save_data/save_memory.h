// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/file_format/psf.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::SaveData::SaveMemory {

void ClearAll();

bool PersistMemory(u32 slot_id, bool lock = true);

[[nodiscard]] std::string GetSaveDir(u32 slot_id);

[[nodiscard]] std::filesystem::path GetSavePath(
    Libraries::UserService::OrbisUserServiceUserId user_id, u32 slot_id,
    std::string_view game_serial);

size_t SetupSaveMemory(Libraries::UserService::OrbisUserServiceUserId user_id, u32 slot_id,
                       std::string_view game_serial, size_t memory_size);

bool SetIcon(u32 slot_id, const void* buf = nullptr, size_t buf_size = 0);

[[nodiscard]] bool IsSaveMemoryInitialized(u32 slot_id);

[[nodiscard]] bool IsSlotOwnedByUser(u32 slot_id,
                                     Libraries::UserService::OrbisUserServiceUserId user_id);

[[nodiscard]] size_t GetMemorySize(u32 slot_id);

[[nodiscard]] PSF& GetParamSFO(u32 slot_id);

[[nodiscard]] std::vector<u8> GetIcon(u32 slot_id);

bool SaveSFO(u32 slot_id);

bool ReadMemory(u32 slot_id, void* buf, size_t buf_size, int64_t offset);

bool WriteMemory(u32 slot_id, const void* buf, size_t buf_size, int64_t offset);

bool RestoreLoad(Libraries::UserService::OrbisUserServiceUserId user_id, u32 slot_id,
                 std::string_view game_serial);

} // namespace Libraries::SaveData::SaveMemory
