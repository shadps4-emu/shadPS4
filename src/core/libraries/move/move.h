// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Move {

int PS4_SYSV_ABI sceMoveOpen();
int PS4_SYSV_ABI sceMoveGetDeviceInfo();
int PS4_SYSV_ABI sceMoveReadStateRecent();
int PS4_SYSV_ABI sceMoveTerm();
int PS4_SYSV_ABI sceMoveInit();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Move