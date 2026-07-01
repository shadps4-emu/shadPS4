// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::InvitationDialog {

s32 PS4_SYSV_ABI sceInvitationDialogClose();
s32 PS4_SYSV_ABI sceInvitationDialogGetResult();
s32 PS4_SYSV_ABI sceInvitationDialogGetResultA();
s32 PS4_SYSV_ABI sceInvitationDialogGetStatus();
s32 PS4_SYSV_ABI sceInvitationDialogInitialize();
s32 PS4_SYSV_ABI sceInvitationDialogOpen();
s32 PS4_SYSV_ABI sceInvitationDialogOpenA();
s32 PS4_SYSV_ABI sceInvitationDialogTerminate();
s32 PS4_SYSV_ABI sceInvitationDialogUpdateStatus();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::InvitationDialog