// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Ime {

int PS4_SYSV_ABI FinalizeImeModule();
int PS4_SYSV_ABI InitializeImeModule();
int PS4_SYSV_ABI sceImeCheckFilterText();
int PS4_SYSV_ABI sceImeCheckRemoteEventParam();
int PS4_SYSV_ABI sceImeCheckUpdateTextInfo();
int PS4_SYSV_ABI sceImeClose();
int PS4_SYSV_ABI sceImeConfigGet();
int PS4_SYSV_ABI sceImeConfigSet();
int PS4_SYSV_ABI sceImeConfirmCandidate();
int PS4_SYSV_ABI sceImeDicAddWord();
int PS4_SYSV_ABI sceImeDicDeleteLearnDics();
int PS4_SYSV_ABI sceImeDicDeleteUserDics();
int PS4_SYSV_ABI sceImeDicDeleteWord();
int PS4_SYSV_ABI sceImeDicGetWords();
int PS4_SYSV_ABI sceImeDicReplaceWord();
int PS4_SYSV_ABI sceImeDisableController();
int PS4_SYSV_ABI sceImeFilterText();
int PS4_SYSV_ABI sceImeForTestFunction();
int PS4_SYSV_ABI sceImeGetPanelPositionAndForm();
int PS4_SYSV_ABI sceImeGetPanelSize();
int PS4_SYSV_ABI sceImeKeyboardClose();
int PS4_SYSV_ABI sceImeKeyboardGetInfo();
int PS4_SYSV_ABI sceImeKeyboardGetResourceId();
int PS4_SYSV_ABI sceImeKeyboardOpen();
int PS4_SYSV_ABI sceImeKeyboardOpenInternal();
int PS4_SYSV_ABI sceImeKeyboardSetMode();
int PS4_SYSV_ABI sceImeKeyboardUpdate();
int PS4_SYSV_ABI sceImeOpen();
int PS4_SYSV_ABI sceImeOpenInternal();
int PS4_SYSV_ABI sceImeParamInit();
int PS4_SYSV_ABI sceImeSetCandidateIndex();
int PS4_SYSV_ABI sceImeSetCaret();
int PS4_SYSV_ABI sceImeSetText();
int PS4_SYSV_ABI sceImeSetTextGeometry();
int PS4_SYSV_ABI sceImeUpdate();
int PS4_SYSV_ABI sceImeVshClearPreedit();
int PS4_SYSV_ABI sceImeVshClose();
int PS4_SYSV_ABI sceImeVshConfirmPreedit();
int PS4_SYSV_ABI sceImeVshDisableController();
int PS4_SYSV_ABI sceImeVshGetPanelPositionAndForm();
int PS4_SYSV_ABI sceImeVshInformConfirmdString();
int PS4_SYSV_ABI sceImeVshInformConfirmdString2();
int PS4_SYSV_ABI sceImeVshOpen();
int PS4_SYSV_ABI sceImeVshSendTextInfo();
int PS4_SYSV_ABI sceImeVshSetCaretGeometry();
int PS4_SYSV_ABI sceImeVshSetCaretIndexInPreedit();
int PS4_SYSV_ABI sceImeVshSetPanelPosition();
int PS4_SYSV_ABI sceImeVshSetParam();
int PS4_SYSV_ABI sceImeVshSetPreeditGeometry();
int PS4_SYSV_ABI sceImeVshSetSelectGeometry();
int PS4_SYSV_ABI sceImeVshSetSelectionText();
int PS4_SYSV_ABI sceImeVshUpdate();
int PS4_SYSV_ABI sceImeVshUpdateContext();
int PS4_SYSV_ABI sceImeVshUpdateContext2();

void RegisterlibSceIme(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Ime