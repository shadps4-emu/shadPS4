// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

#include "ime_common.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Ime {

constexpr u32 ORBIS_IME_MAX_TEXT_LENGTH = 2048;

enum class OrbisImeKeyboardOption : u32 {
    DEFAULT = 0,
    REPEAT = 1,
    REPEAT_EACH_KEY = 2,
    ADD_OSK = 4,
    EFFECTIVE_WITH_TIME = 8,
    DISABLE_RESUME = 16,
    DISABLE_CAPSLOCK_WITHOUT_SHIFT = 32,
};
DECLARE_ENUM_FLAG_OPERATORS(OrbisImeKeyboardOption)

struct OrbisImeKeyboardParam {
    OrbisImeKeyboardOption option;
    s8 reserved1[4];
    void* arg;
    OrbisImeEventHandler handler;
    s8 reserved2[8];
};

struct OrbisImeParam {
    s32 userId;
    OrbisImeType type;
    u64 supportedLanguages;
    OrbisImeEnterLabel enterLabel;
    OrbisImeInputMethod inputMethod;
    OrbisImeTextFilter filter;
    u32 option;
    u32 maxTextLength;
    char16_t* inputTextBuffer;
    float posx;
    float posy;
    OrbisImeHorizontalAlignment horizontalAlignment;
    OrbisImeVerticalAlignment verticalAlignment;
    void* work;
    void* arg;
    OrbisImeEventHandler handler;
    s8 reserved[8];
};

struct OrbisImeCaret {
    f32 x;
    f32 y;
    u32 height;
    u32 index;
};

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
s32 PS4_SYSV_ABI sceImeGetPanelSize(const OrbisImeParam* param, u32* width, u32* height);
s32 PS4_SYSV_ABI sceImeKeyboardClose(s32 userId);
int PS4_SYSV_ABI sceImeKeyboardGetInfo();
int PS4_SYSV_ABI sceImeKeyboardGetResourceId();
s32 PS4_SYSV_ABI sceImeKeyboardOpen(s32 userId, const OrbisImeKeyboardParam* param);
int PS4_SYSV_ABI sceImeKeyboardOpenInternal();
int PS4_SYSV_ABI sceImeKeyboardSetMode();
int PS4_SYSV_ABI sceImeKeyboardUpdate();
s32 PS4_SYSV_ABI sceImeOpen(const OrbisImeParam* param, const void* extended);
int PS4_SYSV_ABI sceImeOpenInternal();
void PS4_SYSV_ABI sceImeParamInit(OrbisImeParam* param);
int PS4_SYSV_ABI sceImeSetCandidateIndex();
s32 PS4_SYSV_ABI sceImeSetCaret(const OrbisImeCaret* caret);
int PS4_SYSV_ABI sceImeSetText();
int PS4_SYSV_ABI sceImeSetTextGeometry();
s32 PS4_SYSV_ABI sceImeUpdate(OrbisImeEventHandler handler);
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