// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::ImeDialog {

enum OrbisImeDialogStatus {
    ORBIS_IME_DIALOG_STATUS_NONE = 0,
    ORBIS_IME_DIALOG_STATUS_RUNNING = 1,
    ORBIS_IME_DIALOG_STATUS_FINISHED = 2
};

enum OrbisImeDialogEndStatus {
    ORBIS_IME_DIALOG_END_STATUS_OK = 0,
    ORBIS_IME_DIALOG_END_STATUS_USER_CANCELED = 1,
    ORBIS_IME_DIALOG_END_STATUS_ABORTED = 2
};

struct OrbisImeDialogResult {
    OrbisImeDialogEndStatus endstatus;
    s32 reserved[12];
};

enum OrbisImeType {
    ORBIS_IME_TYPE_DEFAULT = 0,
    ORBIS_IME_TYPE_BASIC_LATIN = 1,
    ORBIS_IME_TYPE_URL = 2,
    ORBIS_IME_TYPE_MAIL = 3,
    ORBIS_IME_TYPE_NUMBER = 4
};

enum OrbisImeEnterLabel {
    ORBIS_IME_ENTER_LABEL_DEFAULT = 0,
    ORBIS_IME_ENTER_LABEL_SEND = 1,
    ORBIS_IME_ENTER_LABEL_SEARCH = 2,
    ORBIS_IME_ENTER_LABEL_GO = 3
};
enum OrbiImeInputMethod { ORBIS_IME_INPUT_METHOD_DEFAULT = 0 };

typedef int (*OrbisImeTextFilter)(wchar_t* outText, u32* outTextLength, const wchar_t* srcText,
                                  u32 srcTextLength);

enum OrbisImeHorizontalAlignment {
    ORBIS_IME_HALIGN_LEFT = 0,
    ORBIS_IME_HALIGN_CENTER = 1,
    ORBIS_IME_HALIGN_RIGHT = 2
};

enum OrbisImeVerticalAlignment {
    ORBIS_IME_VALIGN_TOP = 0,
    ORBIS_IME_VALIGN_CENTER = 1,
    ORBIS_IME_VALIGN_BOTTOM = 2
};

struct OrbisImeDialogParam {
    s32 userId;
    OrbisImeType type;
    u64 supportedLanguages;
    OrbisImeEnterLabel enterLabel;
    OrbiImeInputMethod inputMethod;
    OrbisImeTextFilter filter;
    u32 option;
    u32 maxTextLength;
    wchar_t* inputTextBuffer;
    float posx;
    float posy;
    OrbisImeHorizontalAlignment horizontalAlignment;
    OrbisImeVerticalAlignment verticalAlignment;
    const wchar_t* placeholder;
    const wchar_t* title;
    s8 reserved[16];
};

struct OrbisImeColor {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

enum OrbisImePanelPriority {
    ORBIS_IME_PANEL_PRIORITY_DEFAULT = 0,
    ORBIS_IME_PANEL_PRIORITY_ALPHABET = 1,
    ORBIS_IME_PANEL_PRIORITY_SYMBOL = 2,
    ORBIS_IME_PANEL_PRIORITY_ACCENT = 3
};

enum OrbisImeKeyboardType {
    ORBIS_IME_KEYBOARD_TYPE_NONE = 0,
    ORBIS_IME_KEYBOARD_TYPE_DANISH = 1,
    ORBIS_IME_KEYBOARD_TYPE_GERMAN = 2,
    ORBIS_IME_KEYBOARD_TYPE_GERMAN_SW = 3,
    ORBIS_IME_KEYBOARD_TYPE_ENGLISH_US = 4,
    ORBIS_IME_KEYBOARD_TYPE_ENGLISH_GB = 5,
    ORBIS_IME_KEYBOARD_TYPE_SPANISH = 6,
    ORBIS_IME_KEYBOARD_TYPE_SPANISH_LA = 7,
    ORBIS_IME_KEYBOARD_TYPE_FINNISH = 8,
    ORBIS_IME_KEYBOARD_TYPE_FRENCH = 9,
    ORBIS_IME_KEYBOARD_TYPE_FRENCH_BR = 10,
    ORBIS_IME_KEYBOARD_TYPE_FRENCH_CA = 11,
    ORBIS_IME_KEYBOARD_TYPE_FRENCH_SW = 12,
    ORBIS_IME_KEYBOARD_TYPE_ITALIAN = 13,
    ORBIS_IME_KEYBOARD_TYPE_DUTCH = 14,
    ORBIS_IME_KEYBOARD_TYPE_NORWEGIAN = 15,
    ORBIS_IME_KEYBOARD_TYPE_POLISH = 16,
    ORBIS_IME_KEYBOARD_TYPE_PORTUGUESE_BR = 17,
    ORBIS_IME_KEYBOARD_TYPE_PORTUGUESE_PT = 18,
    ORBIS_IME_KEYBOARD_TYPE_RUSSIAN = 19,
    ORBIS_IME_KEYBOARD_TYPE_SWEDISH = 20,
    ORBIS_IME_KEYBOARD_TYPE_TURKISH = 21,
    ORBIS_IME_KEYBOARD_TYPE_JAPANESE_ROMAN = 22,
    ORBIS_IME_KEYBOARD_TYPE_JAPANESE_KANA = 23,
    ORBIS_IME_KEYBOARD_TYPE_KOREAN = 24,
    ORBIS_IME_KEYBOARD_TYPE_SM_CHINESE = 25,
    ORBIS_IME_KEYBOARD_TYPE_TR_CHINESE_ZY = 26,
    ORBIS_IME_KEYBOARD_TYPE_TR_CHINESE_PY_HK = 27,
    ORBIS_IME_KEYBOARD_TYPE_TR_CHINESE_PY_TW = 28,
    ORBIS_IME_KEYBOARD_TYPE_TR_CHINESE_CG = 29,
    ORBIS_IME_KEYBOARD_TYPE_ARABIC_AR = 30,
    ORBIS_IME_KEYBOARD_TYPE_THAI = 31,
    ORBIS_IME_KEYBOARD_TYPE_CZECH = 32,
    ORBIS_IME_KEYBOARD_TYPE_GREEK = 33,
    ORBIS_IME_KEYBOARD_TYPE_INDONESIAN = 34,
    ORBIS_IME_KEYBOARD_TYPE_VIETNAMESE = 35,
    ORBIS_IME_KEYBOARD_TYPE_ROMANIAN = 36,
    ORBIS_IME_KEYBOARD_TYPE_HUNGARIAN = 37
};

struct OrbisImeKeycode {
    u16 keycode;
    wchar_t character;
    u32 status;
    OrbisImeKeyboardType type;
    s32 userId;
    u32 resourceId;
    u64 timestamp;
};

typedef int (*OrbisImeExtKeyboardFilter)(const OrbisImeKeycode* srcKeycode, u16* outKeycode,
                                         u32* outStatus, void* reserved);

struct OrbisImeParamExtended {
    u32 option;
    OrbisImeColor colorBase;
    OrbisImeColor colorLine;
    OrbisImeColor colorTextField;
    OrbisImeColor colorPreedit;
    OrbisImeColor colorButtonDefault;
    OrbisImeColor colorButtonFunction;
    OrbisImeColor colorButtonSymbol;
    OrbisImeColor colorText;
    OrbisImeColor colorSpecial;
    OrbisImePanelPriority priority;
    char* additionalDictionaryPath;
    OrbisImeExtKeyboardFilter extKeyboardFilter;
    uint32_t disableDevice;
    uint32_t extKeyboardMode;
    int8_t reserved[60];
};

int PS4_SYSV_ABI sceImeDialogAbort();
int PS4_SYSV_ABI sceImeDialogForceClose();
int PS4_SYSV_ABI sceImeDialogForTestFunction();
int PS4_SYSV_ABI sceImeDialogGetCurrentStarState();
int PS4_SYSV_ABI sceImeDialogGetPanelPositionAndForm();
int PS4_SYSV_ABI sceImeDialogGetPanelSize();
int PS4_SYSV_ABI sceImeDialogGetPanelSizeExtended();
int PS4_SYSV_ABI sceImeDialogGetResult(OrbisImeDialogResult* result);
/*OrbisImeDialogStatus*/ int PS4_SYSV_ABI sceImeDialogGetStatus();
int PS4_SYSV_ABI sceImeDialogInit(OrbisImeDialogParam* param, OrbisImeParamExtended* extended);
int PS4_SYSV_ABI sceImeDialogInitInternal();
int PS4_SYSV_ABI sceImeDialogInitInternal2();
int PS4_SYSV_ABI sceImeDialogInitInternal3();
int PS4_SYSV_ABI sceImeDialogSetPanelPosition();
int PS4_SYSV_ABI sceImeDialogTerm();

void RegisterlibSceImeDialog(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::ImeDialog