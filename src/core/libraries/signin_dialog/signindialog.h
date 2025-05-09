// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::SigninDialog {

s32 PS4_SYSV_ABI sceSigninDialogInitialize();
s32 PS4_SYSV_ABI sceSigninDialogOpen();
s32 PS4_SYSV_ABI sceSigninDialogGetStatus();
s32 PS4_SYSV_ABI sceSigninDialogUpdateStatus();
s32 PS4_SYSV_ABI sceSigninDialogGetResult();
s32 PS4_SYSV_ABI sceSigninDialogClose();
s32 PS4_SYSV_ABI sceSigninDialogTerminate();

void RegisterlibSceSigninDialog(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SigninDialog
