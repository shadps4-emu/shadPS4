// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibSceGnmDriver {

int32_t PS4_SYSV_ABI sceGnmSubmitDone();
void PS4_SYSV_ABI sceGnmFlushGarlic();

void LibSceGnmDriver_Register(Loader::SymbolsResolver* sym);

}; // namespace Core::Libraries::LibSceGnmDriver
