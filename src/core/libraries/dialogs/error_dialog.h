// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}
namespace Libraries::ErrorDialog {

enum OrbisErrorDialogStatus {
    ORBIS_ERROR_DIALOG_STATUS_NONE = 0,
    ORBIS_ERROR_DIALOG_STATUS_INITIALIZED = 1,
    ORBIS_ERROR_DIALOG_STATUS_RUNNING = 2,
    ORBIS_ERROR_DIALOG_STATUS_FINISHED = 3
};

struct OrbisErrorDialogParam {
    s32 size;
    u32 errorCode;
    s32 userId;
    s32 reserved;
};

int PS4_SYSV_ABI sceErrorDialogClose();
OrbisErrorDialogStatus PS4_SYSV_ABI sceErrorDialogGetStatus();
int PS4_SYSV_ABI sceErrorDialogInitialize(OrbisErrorDialogParam* param);
int PS4_SYSV_ABI sceErrorDialogOpen(OrbisErrorDialogParam* param);
int PS4_SYSV_ABI sceErrorDialogOpenDetail();
int PS4_SYSV_ABI sceErrorDialogOpenWithReport();
int PS4_SYSV_ABI sceErrorDialogTerminate();
OrbisErrorDialogStatus PS4_SYSV_ABI sceErrorDialogUpdateStatus();

void RegisterlibSceErrorDialog(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::ErrorDialog