// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::ImeDialog {

int PS4_SYSV_ABI sceImeDialogAbort();
int PS4_SYSV_ABI sceImeDialogForceClose();
int PS4_SYSV_ABI sceImeDialogForTestFunction();
int PS4_SYSV_ABI sceImeDialogGetCurrentStarState();
int PS4_SYSV_ABI sceImeDialogGetPanelPositionAndForm();
int PS4_SYSV_ABI sceImeDialogGetPanelSize();
int PS4_SYSV_ABI sceImeDialogGetPanelSizeExtended();
int PS4_SYSV_ABI sceImeDialogGetResult();
int PS4_SYSV_ABI sceImeDialogGetStatus();
int PS4_SYSV_ABI sceImeDialogInit();
int PS4_SYSV_ABI sceImeDialogInitInternal();
int PS4_SYSV_ABI sceImeDialogInitInternal2();
int PS4_SYSV_ABI sceImeDialogInitInternal3();
int PS4_SYSV_ABI sceImeDialogSetPanelPosition();
int PS4_SYSV_ABI sceImeDialogTerm();

void RegisterlibSceImeDialog(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::ImeDialog