// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::SysCore {

s32 PS4_SYSV_ABI sceApplicationInitialize();
s32 PS4_SYSV_ABI sceApplicationFinalize();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SysCore
