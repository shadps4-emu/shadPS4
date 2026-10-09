// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "common/types.h"
#include "signindialog_error.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::SigninDialog {

enum class Status : u32 {
    NONE = 0,
    INITIALIZED = 1,
    RUNNING = 2,
    FINISHED = 3,
};

enum class OrbisSigninDialogResultType {
    OK = 0,
    USER_CANCELED = 1,
};
struct OrbisSigninDialogResult {
    OrbisSigninDialogResultType result;
    int32_t reserved[3];
};

s32 PS4_SYSV_ABI sceSigninDialogInitialize();
s32 PS4_SYSV_ABI sceSigninDialogOpen();
Status PS4_SYSV_ABI sceSigninDialogGetStatus();
Status PS4_SYSV_ABI sceSigninDialogUpdateStatus();
s32 PS4_SYSV_ABI sceSigninDialogGetResult(OrbisSigninDialogResult* result);
s32 PS4_SYSV_ABI sceSigninDialogClose();
s32 PS4_SYSV_ABI sceSigninDialogTerminate();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SigninDialog
