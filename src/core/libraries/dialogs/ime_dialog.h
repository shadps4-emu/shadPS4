// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::ImeDialog {

constexpr u32 ORBIS_IME_DIALOG_MAX_TEXT_LENGTH = 0x78;

enum class Error : u32 {
    OK = 0x0,
    BUSY = 0x80bc0001,
    NOT_OPENED = 0x80bc0002,
    NO_MEMORY = 0x80bc0003,
    CONNECTION_FAILED = 0x80bc0004,
    TOO_MANY_REQUESTS = 0x80bc0005,
    INVALID_TEXT = 0x80bc0006,
    EVENT_OVERFLOW = 0x80bc0007,
    NOT_ACTIVE = 0x80bc0008,
    IME_SUSPENDING = 0x80bc0009,
    DEVICE_IN_USE = 0x80bc000a,
    INVALID_USER_ID = 0x80bc0010,
    INVALID_TYPE = 0x80bc0011,
    INVALID_SUPPORTED_LANGUAGES = 0x80bc0012,
    INVALID_ENTER_LABEL = 0x80bc0013,
    INVALID_INPUT_METHOD = 0x80bc0014,
    INVALID_OPTION = 0x80bc0015,
    INVALID_MAX_TEXT_LENGTH = 0x80bc0016,
    INVALID_INPUT_TEXT_BUFFER = 0x80bc0017,
    INVALID_POSX = 0x80bc0018,
    INVALID_POSY = 0x80bc0019,
    INVALID_HORIZONTALIGNMENT = 0x80bc001a,
    INVALID_VERTICALALIGNMENT = 0x80bc001b,
    INVALID_EXTENDED = 0x80bc001c,
    INVALID_KEYBOARD_TYPE = 0x80bc001d,
    INVALID_WORK = 0x80bc0020,
    INVALID_ARG = 0x80bc0021,
    INVALID_HANDLER = 0x80bc0022,
    NO_RESOURCE_ID = 0x80bc0023,
    INVALID_MODE = 0x80bc0024,
    INVALID_PARAM = 0x80bc0030,
    INVALID_ADDRESS = 0x80bc0031,
    INVALID_RESERVED = 0x80bc0032,
    INVALID_TIMING = 0x80bc0033,
    INTERNAL = 0x80bc00ff,
    DIALOG_INVALID_TITLE = 0x80bc0101,
    DIALOG_NOT_RUNNING = 0x80bc0105,
    DIALOG_NOT_FINISHED = 0x80bc0106,
    DIALOG_NOT_IN_USE = 0x80bc0107,
};

enum class OrbisImeDialogStatus : u32 {
    NONE = 0,
    RUNNING = 1,
    FINISHED = 2,
};

enum class OrbisImeDialogEndStatus : u32 {
    OK = 0,
    USER_CANCELED = 1,
    ABORTED = 2,
};

enum class OrbisImeType : u32 {
    DEFAULT = 0,
    BASIC_LATIN = 1,
    URL = 2,
    MAIL = 3,
    NUMBER = 4,
};

enum class OrbisImeEnterLabel : u32 {
    DEFAULT = 0,
    SEND = 1,
    SEARCH = 2,
    GO = 3,
};

enum class OrbisImeDialogOption : u32 {
    DEFAULT = 0,
    MULTILINE = 1,
    NO_AUTO_CORRECTION = 2,
    NO_AUTO_COMPLETION = 4,
    // TODO: Document missing options
    LARGE_RESOLUTION = 1024,
};

DECLARE_ENUM_FLAG_OPERATORS(OrbisImeDialogOption)

enum class OrbisImeInputMethod : u32 {
    DEFAULT = 0,
};

enum class OrbisImeHorizontalAlignment : u32 {
    LEFT = 0,
    CENTER = 1,
    RIGHT = 2,
};

enum class OrbisImeVerticalAlignment : u32 {
    TOP = 0,
    CENTER = 1,
    BOTTOM = 2,
};

enum class OrbisImePanelPriority : u32 {
    DEFAULT = 0,
    ALPHABET = 1,
    SYMBOL = 2,
    ACCENT = 3,
};

enum class OrbisImeKeyboardType : u32 {
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
    HUNGARIAN = 37,
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
    char16_t character;
    u32 status;
    OrbisImeKeyboardType type;
    s32 userId;
    u32 resourceId;
    u64 timestamp;
};

typedef PS4_SYSV_ABI int (*OrbisImeTextFilter)(char16_t* outText, u32* outTextLength,
                                               const char16_t* srcText, u32 srcTextLength);

typedef PS4_SYSV_ABI int (*OrbisImeExtKeyboardFilter)(const OrbisImeKeycode* srcKeycode,
                                                      u16* outKeycode, u32* outStatus,
                                                      void* reserved);

struct OrbisImeDialogParam {
    s32 userId;
    OrbisImeType type;
    u64 supportedLanguages;
    OrbisImeEnterLabel enterLabel;
    OrbisImeInputMethod inputMethod;
    OrbisImeTextFilter filter;
    OrbisImeDialogOption option;
    u32 maxTextLength;
    char16_t* inputTextBuffer;
    float posx;
    float posy;
    OrbisImeHorizontalAlignment horizontalAlignment;
    OrbisImeVerticalAlignment verticalAlignment;
    const char16_t* placeholder;
    const char16_t* title;
    s8 reserved[16];
};

struct OrbisImeParamExtended {
    u32 option; // OrbisImeDialogOptionExtended
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

Error PS4_SYSV_ABI sceImeDialogAbort();
Error PS4_SYSV_ABI sceImeDialogForceClose();
Error PS4_SYSV_ABI sceImeDialogForTestFunction();
int PS4_SYSV_ABI sceImeDialogGetCurrentStarState();
int PS4_SYSV_ABI sceImeDialogGetPanelPositionAndForm();
int PS4_SYSV_ABI sceImeDialogGetPanelSize();
int PS4_SYSV_ABI sceImeDialogGetPanelSizeExtended();
Error PS4_SYSV_ABI sceImeDialogGetResult(OrbisImeDialogResult* result);
OrbisImeDialogStatus PS4_SYSV_ABI sceImeDialogGetStatus();
Error PS4_SYSV_ABI sceImeDialogInit(OrbisImeDialogParam* param, OrbisImeParamExtended* extended);
int PS4_SYSV_ABI sceImeDialogInitInternal();
int PS4_SYSV_ABI sceImeDialogInitInternal2();
int PS4_SYSV_ABI sceImeDialogInitInternal3();
int PS4_SYSV_ABI sceImeDialogSetPanelPosition();
Error PS4_SYSV_ABI sceImeDialogTerm();

void RegisterlibSceImeDialog(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::ImeDialog