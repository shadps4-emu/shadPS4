// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"
#include "ime_common.h"
#include "ime_dialog_result.h"
#include "ime_dialog_status.h"
#include "ime_dialog_ui.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::ImeDialog {

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

struct Library {
    Library(Core::Loader::SymbolsResolver* sym);

    OrbisImeDialogStatus g_ime_dlg_status = OrbisImeDialogStatus::None;
    OrbisImeDialogResult g_ime_dlg_result{};
    ImeDialogState g_ime_dlg_state{};
    ImeDialogUi g_ime_dlg_ui{};
};
} // namespace Libraries::ImeDialog
