// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Random {

constexpr s32 SCE_RANDOM_MAX_SIZE = 64;

s32 PS4_SYSV_ABI sceRandomGetRandomNumber(u8* buf, std::size_t size);

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Random
