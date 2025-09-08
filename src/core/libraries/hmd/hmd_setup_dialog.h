// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/system/commondialog.h"
#include "core/libraries/system/userservice.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::HmdSetupDialog {

struct OrbisHmdSetupDialogParam {
    Libraries::CommonDialog::BaseParam base_param;
    u64 size;
    Libraries::UserService::OrbisUserServiceUserId user_id;
    bool disable_handover_screen;
    u8 reserved[40];
};

struct OrbisHmdSetupDialogResult {
    Libraries::CommonDialog::Result result;
    u8 reserved[32];
};

s32 PS4_SYSV_ABI sceHmdSetupDialogClose();
s32 PS4_SYSV_ABI sceHmdSetupDialogGetResult(OrbisHmdSetupDialogResult* result);
Libraries::CommonDialog::Status PS4_SYSV_ABI sceHmdSetupDialogGetStatus();
s32 PS4_SYSV_ABI sceHmdSetupDialogInitialize();
s32 PS4_SYSV_ABI sceHmdSetupDialogOpen(const OrbisHmdSetupDialogParam* param);
s32 PS4_SYSV_ABI sceHmdSetupDialogTerminate();
Libraries::CommonDialog::Status PS4_SYSV_ABI sceHmdSetupDialogUpdateStatus();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::HmdSetupDialog