// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}
namespace Libraries::ErrorDialog {

int PS4_SYSV_ABI sceErrorDialogClose();
int PS4_SYSV_ABI sceErrorDialogGetStatus();
int PS4_SYSV_ABI sceErrorDialogInitialize();
int PS4_SYSV_ABI sceErrorDialogOpen();
int PS4_SYSV_ABI sceErrorDialogOpenDetail();
int PS4_SYSV_ABI sceErrorDialogOpenWithReport();
int PS4_SYSV_ABI sceErrorDialogTerminate();
int PS4_SYSV_ABI sceErrorDialogUpdateStatus();

void RegisterlibSceErrorDialog(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::ErrorDialog