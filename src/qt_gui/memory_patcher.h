// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <cstring>
#include <string>
#include <vector>
#include <QString>

namespace MemoryPatcher {

extern uintptr_t g_eboot_address;
extern u64 g_eboot_image_size;
extern std::string g_game_serial;

enum PatchMask : uint8_t {
    None,
    Mask,
    Mask_Jump32,
};

struct patchInfo {
    std::string gameSerial;
    std::string modNameStr;
    std::string offsetStr;
    std::string valueStr;
    bool isOffset;
    bool littleEndian;
    PatchMask patchMask;
    int maskOffset;
};

extern std::vector<patchInfo> pending_patches;

QString convertValueToHex(const QString& type, const QString& valueStr);

void OnGameLoaded();
void AddPatchToQueue(patchInfo patchToAdd);
void ApplyPendingPatches();

void PatchMemory(std::string modNameStr, std::string offsetStr, std::string valueStr, bool isOffset,
                 bool littleEndian, PatchMask patchMask = PatchMask::None, int maskOffset = 0);

static std::vector<int32_t> PatternToByte(const std::string& pattern);
uintptr_t PatternScan(const std::string& signature);

} // namespace MemoryPatcher