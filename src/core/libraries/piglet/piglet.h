// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Piglet {

// Size of the configuration structure used by scePigletSet/GetConfigurationVSH.
constexpr u32 PIGLET_CONFIG_VSH_SIZE = 0x88;

// Callbacks Piglet uses for its memory when the application provides them. Type is 0x802 for system
// memory and 0x804 for video memory.
using OrbisPigletMemoryAllocFunc = void*(PS4_SYSV_ABI*)(u32 size, u32 alignment, u32 type,
                                                        void* userdata);
using OrbisPigletMemoryFreeFunc = void(PS4_SYSV_ABI*)(void* ptr, u32 type, void* userdata);

struct OrbisPigletConfigVSH {
    u32 size;
    u8 reserved0[0x6C];
    OrbisPigletMemoryAllocFunc memory_alloc;
    OrbisPigletMemoryFreeFunc memory_free;
    u8 reserved1[0x8];
};
static_assert(sizeof(OrbisPigletConfigVSH) == PIGLET_CONFIG_VSH_SIZE);
static_assert(offsetof(OrbisPigletConfigVSH, memory_alloc) == 0x70);
static_assert(offsetof(OrbisPigletConfigVSH, memory_free) == 0x78);

// Version expected in the first field of the shader cache configuration.
constexpr u16 PIGLET_SHADER_CACHE_CONFIG_VERSION = 0x64;
constexpr u32 PIGLET_SHADER_CACHE_CONFIG_SIZE = 0x10C;

struct OrbisPigletShaderCacheConfiguration {
    u16 version;
    u8 reserved[PIGLET_SHADER_CACHE_CONFIG_SIZE - sizeof(u16)];
};
static_assert(sizeof(OrbisPigletShaderCacheConfiguration) == PIGLET_SHADER_CACHE_CONFIG_SIZE);

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Piglet
