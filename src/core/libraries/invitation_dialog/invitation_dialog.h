// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <core/libraries/system/commondialog.h>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::InvitationDialog {

Libraries::CommonDialog::Error PS4_SYSV_ABI sceInvitationDialogClose();
s32 PS4_SYSV_ABI sceInvitationDialogGetResult();
s32 PS4_SYSV_ABI sceInvitationDialogGetResultA();
Libraries::CommonDialog::Status PS4_SYSV_ABI sceInvitationDialogGetStatus();
Libraries::CommonDialog::Error PS4_SYSV_ABI sceInvitationDialogInitialize();
Libraries::CommonDialog::Error PS4_SYSV_ABI sceInvitationDialogOpen();
Libraries::CommonDialog::Error PS4_SYSV_ABI sceInvitationDialogOpenA();
Libraries::CommonDialog::Error PS4_SYSV_ABI sceInvitationDialogTerminate();
Libraries::CommonDialog::Status PS4_SYSV_ABI sceInvitationDialogUpdateStatus();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::InvitationDialog