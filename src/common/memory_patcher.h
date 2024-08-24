// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <cstring>
#include <string>
#include <vector>

namespace MemoryPatcher {

extern uintptr_t g_eboot_address;

struct patchInfo {
    std::string modNameStr;
    std::string offsetStr;
    std::string valueStr;
    bool isOffset;
    bool littleEndian;
};

extern std::vector<patchInfo> pending_patches;

void AddPatchToQueue(patchInfo patchToAdd);
void ApplyPendingPatches();

void PatchMemory(std::string modNameStr, std::string offsetStr, std::string valueStr, bool isOffset,
                 bool littleEndian);

} // namespace MemoryPatcher