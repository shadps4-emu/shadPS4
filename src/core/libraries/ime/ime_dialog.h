// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"
#include "ime_common.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::ImeDialog {

enum class OrbisImeDialogStatus : u32 {
    None = 0,
    Running = 1,
    Finished = 2,
};

enum class OrbisImeDialogEndStatus : u32 {
    Ok = 0,
    UserCanceled = 1,
    Aborted = 2,
};

struct OrbisImeDialogResult {
    OrbisImeDialogEndStatus endstatus;
    s32 reserved[12];
};

Error PS4_SYSV_ABI sceImeDialogAbort();
Error PS4_SYSV_ABI sceImeDialogForceClose();
Error PS4_SYSV_ABI sceImeDialogForTestFunction();
int PS4_SYSV_ABI sceImeDialogGetCurrentStarState();
int PS4_SYSV_ABI sceImeDialogGetPanelPositionAndForm();
Error PS4_SYSV_ABI sceImeDialogGetPanelSize(const OrbisImeDialogParam* param, u32* width,
                                            u32* height);
Error PS4_SYSV_ABI sceImeDialogGetPanelSizeExtended(const OrbisImeDialogParam* param,
                                                    const OrbisImeParamExtended* extended,
                                                    u32* width, u32* height);
Error PS4_SYSV_ABI sceImeDialogGetResult(OrbisImeDialogResult* result);
OrbisImeDialogStatus PS4_SYSV_ABI sceImeDialogGetStatus();
Error PS4_SYSV_ABI sceImeDialogInit(OrbisImeDialogParam* param, OrbisImeParamExtended* extended);
int PS4_SYSV_ABI sceImeDialogInitInternal();
int PS4_SYSV_ABI sceImeDialogInitInternal2();
int PS4_SYSV_ABI sceImeDialogInitInternal3();
int PS4_SYSV_ABI sceImeDialogSetPanelPosition();
Error PS4_SYSV_ABI sceImeDialogTerm();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::ImeDialog
