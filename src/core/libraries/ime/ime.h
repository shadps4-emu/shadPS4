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

constexpr u32 ORBIS_IME_MAX_TEXT_LENGTH = 2048;

enum class OrbisImeKeyboardOption : u32 {
    Default = 0,
    Repeat = 1,
    RepeatEachKey = 2,
    AddOsk = 4,
    EffectiveWithTime = 8,
    DisableResume = 16,
    DisableCapslockWithoutShift = 32,
};
DECLARE_ENUM_FLAG_OPERATORS(OrbisImeKeyboardOption)

enum class OrbisImeOption : u32 {
    DEFAULT = 0,
    MULTILINE = 1,
    NO_AUTO_CAPITALIZATION = 2,
    PASSWORD = 4,
    LANGUAGES_FORCED = 8,
    EXT_KEYBOARD = 16,
    NO_LEARNING = 32,
    FIXED_POSITION = 64,
    DISABLE_RESUME = 256,
    DISABLE_AUTO_SPACE = 512,
    DISABLE_POSITION_ADJUSTMENT = 2048,
    EXPANDED_PREEDIT_BUFFER = 4096,
    USE_JAPANESE_EISUU_KEY_AS_CAPSLOCK = 8192,
    USE_2K_COORDINATES = 16384,
};
DECLARE_ENUM_FLAG_OPERATORS(OrbisImeOption)

struct OrbisImeKeyboardParam {
    OrbisImeKeyboardOption option;
    s8 reserved1[4];
    void* arg;
    OrbisImeEventHandler handler;
    s8 reserved2[8];
};

struct OrbisImeParam {
    s32 user_id;
    OrbisImeType type;
    u64 supported_languages;
    OrbisImeEnterLabel enter_label;
    OrbisImeInputMethod input_method;
    OrbisImeTextFilter filter;
    OrbisImeOption option;
    u32 maxTextLength;
    char16_t* inputTextBuffer;
    float posx;
    float posy;
    OrbisImeHorizontalAlignment horizontal_alignment;
    OrbisImeVerticalAlignment vertical_alignment;
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
s32 PS4_SYSV_ABI sceImeSetText(const char16_t* text, u32 length);
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
