// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include "save_backup.h"

class PSF;

namespace Libraries::SaveData {
using OrbisUserServiceUserId = s32;
}

namespace Libraries::SaveData::SaveMemory {

void SetDirectories(OrbisUserServiceUserId user_id, std::string game_serial);

[[nodiscard]] const std::filesystem::path& GetSavePath();

// returns the size of the existed save memory
size_t CreateSaveMemory(size_t memory_size);

// Initialize the icon. Set buf to null to read the standard icon.
void SetIcon(void* buf, size_t buf_size);

// Update the icon
void WriteIcon(void* buf, size_t buf_size);

[[nodiscard]] bool IsSaveMemoryInitialized();

[[nodiscard]] PSF& GetParamSFO();

[[nodiscard]] std::span<u8> GetIcon();

// Save now or wait for the background thread to save
void SaveSFO(bool sync = false);

[[nodiscard]] bool IsSaving();

bool TriggerSaveWithoutEvent();

bool TriggerSave();

void ReadMemory(void* buf, size_t buf_size, int64_t offset);

void WriteMemory(void* buf, size_t buf_size, int64_t offset);

} // namespace Libraries::SaveData::SaveMemory