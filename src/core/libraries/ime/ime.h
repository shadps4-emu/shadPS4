// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"
#include "core/libraries/ime/ime_common.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Ime {

int PS4_SYSV_ABI FinalizeImeModule();
int PS4_SYSV_ABI InitializeImeModule();
int PS4_SYSV_ABI sceImeCheckFilterText();
int PS4_SYSV_ABI sceImeCheckRemoteEventParam();
int PS4_SYSV_ABI sceImeCheckUpdateTextInfo();
Error PS4_SYSV_ABI sceImeClose();
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
Error PS4_SYSV_ABI sceImeGetPanelSize(const OrbisImeParam* param, u32* width, u32* height);
Error PS4_SYSV_ABI sceImeKeyboardClose(Libraries::UserService::OrbisUserServiceUserId userId);
int PS4_SYSV_ABI sceImeKeyboardGetInfo();
Error PS4_SYSV_ABI
sceImeKeyboardGetResourceId(Libraries::UserService::OrbisUserServiceUserId userId,
                            OrbisImeKeyboardResourceIdArray* resourceIdArray);
Error PS4_SYSV_ABI sceImeKeyboardOpen(Libraries::UserService::OrbisUserServiceUserId userId,
                                      const OrbisImeKeyboardParam* param);
int PS4_SYSV_ABI sceImeKeyboardOpenInternal();
int PS4_SYSV_ABI sceImeKeyboardSetMode();
int PS4_SYSV_ABI sceImeKeyboardUpdate();
Error PS4_SYSV_ABI sceImeOpen(const OrbisImeParam* param, const OrbisImeParamExtended* extended);
int PS4_SYSV_ABI sceImeOpenInternal();
void PS4_SYSV_ABI sceImeParamInit(OrbisImeParam* param);
int PS4_SYSV_ABI sceImeSetCandidateIndex();
Error PS4_SYSV_ABI sceImeSetCaret(const OrbisImeCaret* caret);
Error PS4_SYSV_ABI sceImeSetText(const char16_t* text, u32 length);
int PS4_SYSV_ABI sceImeSetTextGeometry();
Error PS4_SYSV_ABI sceImeUpdate(OrbisImeEventHandler handler);
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

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Ime
