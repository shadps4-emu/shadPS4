// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Piglet {

// Size of the configuration structure used by scePigletSet/GetConfigurationVSH.
constexpr u32 PIGLET_CONFIG_VSH_SIZE = 0x88;

struct OrbisPigletConfigVSH {
    u32 size;
    u8 reserved[PIGLET_CONFIG_VSH_SIZE - sizeof(u32)];
};
static_assert(sizeof(OrbisPigletConfigVSH) == PIGLET_CONFIG_VSH_SIZE);

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Piglet
