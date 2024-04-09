// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "library_common.h"

namespace Libraries::SaveDataDialog {

int PS4_SYSV_ABI sceSaveDataDialogClose();
int PS4_SYSV_ABI sceSaveDataDialogGetResult();
int PS4_SYSV_ABI sceSaveDataDialogGetStatus();
int PS4_SYSV_ABI sceSaveDataDialogInitialize();
int PS4_SYSV_ABI sceSaveDataDialogIsReadyToDisplay();
int PS4_SYSV_ABI sceSaveDataDialogOpen();
int PS4_SYSV_ABI sceSaveDataDialogProgressBarInc();
int PS4_SYSV_ABI sceSaveDataDialogProgressBarSetValue();
int PS4_SYSV_ABI sceSaveDataDialogTerminate();
int PS4_SYSV_ABI sceSaveDataDialogUpdateStatus();

void RegisterlibSceSaveDataDialog(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SaveDataDialog