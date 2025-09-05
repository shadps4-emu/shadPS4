// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpProfileDialog {

s32 PS4_SYSV_ABI sceNpProfileDialogOpen();
s32 PS4_SYSV_ABI sceNpProfileDialogClose();
s32 PS4_SYSV_ABI sceNpProfileDialogGetResult();
s32 PS4_SYSV_ABI sceNpProfileDialogGetStatus();
s32 PS4_SYSV_ABI sceNpProfileDialogInitialize();
s32 PS4_SYSV_ABI sceNpProfileDialogOpenA();
s32 PS4_SYSV_ABI sceNpProfileDialogTerminate();
s32 PS4_SYSV_ABI sceNpProfileDialogUpdateStatus();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpProfileDialog