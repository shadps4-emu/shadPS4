// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/system/commondialog.h"

namespace Core::Loader {
class SymbolsResolver;
}
namespace Libraries::ErrorDialog {

using OrbisUserServiceUserId = s32;

struct Param;

CommonDialog::Error PS4_SYSV_ABI sceErrorDialogClose();
CommonDialog::Status PS4_SYSV_ABI sceErrorDialogGetStatus();
CommonDialog::Error PS4_SYSV_ABI sceErrorDialogInitialize();
CommonDialog::Error PS4_SYSV_ABI sceErrorDialogOpen(const Param* param);
int PS4_SYSV_ABI sceErrorDialogOpenDetail();
int PS4_SYSV_ABI sceErrorDialogOpenWithReport();
CommonDialog::Error PS4_SYSV_ABI sceErrorDialogTerminate();
CommonDialog::Status PS4_SYSV_ABI sceErrorDialogUpdateStatus();

void RegisterlibSceErrorDialog(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::ErrorDialog