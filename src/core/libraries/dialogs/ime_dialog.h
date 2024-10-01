// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::ImeDialog {

enum class OrbisImeDialogStatus : s32 {
    NONE = 0,
    RUNNING = 1,
    FINISHED = 2
};

enum class OrbisImeDialogEndStatus : s32 {
    OK = 0,
    USER_CANCELED = 1,
    ABORTED = 2
};

enum class OrbisImeType : s32 {
    DEFAULT = 0,
    BASIC_LATIN = 1,
    URL = 2,
    MAIL = 3,
    NUMBER = 4
};

enum class OrbisImeEnterLabel : s32 {
    DEFAULT = 0,
    SEND = 1,
    SEARCH = 2,
    GO = 3
};

enum class OrbisImeInputMethod : s32 {
    DEFAULT = 0
};

enum class OrbisImeHorizontalAlignment : s32 {
    LEFT = 0,
    CENTER = 1,
    RIGHT = 2
};

enum class OrbisImeVerticalAlignment : s32 {
    TOP = 0,
    CENTER = 1,
    BOTTOM = 2
};

enum class OrbisImePanelPriority : s32 {
    DEFAULT = 0,
    ALPHABET = 1,
    SYMBOL = 2,
    ACCENT = 3
};

enum class OrbisImeKeyboardType : s32 {
    NONE = 0,
    DANISH = 1,
    GERMAN = 2,
    GERMAN_SW = 3,
    ENGLISH_US = 4,
    ENGLISH_GB = 5,
    SPANISH = 6,
    SPANISH_LA = 7,
    FINNISH = 8,
    FRENCH = 9,
    FRENCH_BR = 10,
    FRENCH_CA = 11,
    FRENCH_SW = 12,
    ITALIAN = 13,
    DUTCH = 14,
    NORWEGIAN = 15,
    POLISH = 16,
    PORTUGUESE_BR = 17,
    PORTUGUESE_PT = 18,
    RUSSIAN = 19,
    SWEDISH = 20,
    TURKISH = 21,
    JAPANESE_ROMAN = 22,
    JAPANESE_KANA = 23,
    KOREAN = 24,
    SM_CHINESE = 25,
    TR_CHINESE_ZY = 26,
    TR_CHINESE_PY_HK = 27,
    TR_CHINESE_PY_TW = 28,
    TR_CHINESE_CG = 29,
    ARABIC_AR = 30,
    THAI = 31,
    CZECH = 32,
    GREEK = 33,
    INDONESIAN = 34,
    VIETNAMESE = 35,
    ROMANIAN = 36,
    HUNGARIAN = 37
};

struct OrbisImeColor {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

struct OrbisImeDialogResult {
    OrbisImeDialogEndStatus endstatus;
    s32 reserved[12];
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

typedef int (*OrbisImeTextFilter)(wchar_t* outText, u32* outTextLength, const wchar_t* srcText,
                                  u32 srcTextLength);

typedef int (*OrbisImeExtKeyboardFilter)(const OrbisImeKeycode* srcKeycode, u16* outKeycode,
                                         u32* outStatus, void* reserved);

struct OrbisImeDialogParam {
    s32 userId;
    OrbisImeType type;
    u64 supportedLanguages;
    OrbisImeEnterLabel enterLabel;
    OrbisImeInputMethod inputMethod;
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
OrbisImeDialogStatus PS4_SYSV_ABI sceImeDialogGetStatus();
int PS4_SYSV_ABI sceImeDialogInit(OrbisImeDialogParam* param, OrbisImeParamExtended* extended);
int PS4_SYSV_ABI sceImeDialogInitInternal();
int PS4_SYSV_ABI sceImeDialogInitInternal2();
int PS4_SYSV_ABI sceImeDialogInitInternal3();
int PS4_SYSV_ABI sceImeDialogSetPanelPosition();
int PS4_SYSV_ABI sceImeDialogTerm();

void RegisterlibSceImeDialog(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::ImeDialog