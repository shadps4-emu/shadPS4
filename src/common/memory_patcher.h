// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <cstring>
#include <string>
#include <vector>

#if defined(WIN32)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

namespace MemoryPatcher {

extern EXPORT uintptr_t g_eboot_address;
extern uint64_t g_eboot_image_size;
extern std::string g_game_serial;
extern std::string patch_file;

enum PatchMask : uint8_t {
    None,
    Mask,
    Mask_Jump32,
};

struct patchInfo {
    const std::string& gameSerial;
    const std::string& modNameStr;
    const std::string& offsetStr;
    const std::string& valueStr;
    const std::string& targetStr;
    const std::string& sizeStr;
    bool isOffset;
    bool littleEndian;
    PatchMask patchMask;
    int maskOffset;
};

std::string convertValueToHex(const std::string type, const std::string valueStr);

void OnGameLoaded();
void AddPatchToQueue(const patchInfo& patchToAdd);

void PatchMemory(const patchInfo& patch);

static std::vector<int32_t> PatternToByte(const std::string& pattern);
uintptr_t PatternScan(const std::string& signature);

} // namespace MemoryPatcher