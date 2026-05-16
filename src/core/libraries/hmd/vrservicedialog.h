// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::VrServiceDialog {

s32 PS4_SYSV_ABI sceVrServiceDialogClose();
s32 PS4_SYSV_ABI sceVrServiceDialogGetResult();
s32 PS4_SYSV_ABI sceVrServiceDialogGetStatus();
s32 PS4_SYSV_ABI sceVrServiceDialogInitialize();
s32 PS4_SYSV_ABI sceVrServiceDialogOpen();
s32 PS4_SYSV_ABI sceVrServiceDialogTerminate();
s32 PS4_SYSV_ABI sceVrServiceDialogUpdateStatus();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::VrServiceDialog