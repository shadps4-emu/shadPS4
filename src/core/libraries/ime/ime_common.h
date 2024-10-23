// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <sys/types.h>
#include "common/enum.h"
#include "common/types.h"
#include "core/libraries/rtc/rtc.h"

enum class OrbisImeType : u32 {
    DEFAULT = 0,
    BASIC_LATIN = 1,
    URL = 2,
    MAIL = 3,
    NUMBER = 4,
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

enum class OrbisImeEnterLabel : u32 {
    DEFAULT = 0,
    SEND = 1,
    SEARCH = 2,
    GO = 3,
};

enum class OrbisImeInputMethod : u32 {
    DEFAULT = 0,
};

enum class OrbisImeEventId : u32 {
    OPEN = 0,
    UPDATE_TEXT = 1,
    UPDATE_CARET = 2,
    PRESS_CLOSE = 4,
    PRESS_ENTER = 5,
    ABORT = 6,
    CANDIDATE_LIST_START = 7,
    CANDIDATE_LIST_END = 8,
    CANDIDATE_WORD = 9,
    CANDIDATE_INDEX = 10,
    CANDIDATE_DONE = 11,
    CANDIDATE_CANCEL = 12,
    CHANGE_DEVICE = 14,
    CHANGE_INPUT_METHOD_STATE = 18,

    KEYBOARD_OPEN = 256,
    KEYBOARD_KEYCODE_DOWN = 257,
    KEYBOARD_KEYCODE_UP = 258,
    KEYBOARD_KEYCODE_REPEAT = 259,
    KEYBOARD_CONNECTION = 260,
    KEYBOARD_DISCONNECTION = 261,
    KEYBOARD_ABORT = 262,
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

enum class OrbisImeDeviceType : u32 {
    NONE = 0,
    CONTROLLER = 1,
    EXT_KEYBOARD = 2,
    REMOTE_OSK = 3,
};

struct OrbisImeRect {
    f32 x;
    f32 y;
    u32 width;
    u32 height;
};

struct OrbisImeTextAreaProperty {
    u32 mode; // OrbisImeTextAreaMode
    u32 index;
    s32 length;
};

struct OrbisImeEditText {
    char16_t* str;
    u32 caretIndex;
    u32 areaNum;
    OrbisImeTextAreaProperty textArea[4];
};

struct OrbisImeKeycode {
    u16 keycode;
    char16_t character;
    u32 status;
    OrbisImeKeyboardType type;
    s32 userId;
    u32 resourceId;
    Libraries::Rtc::OrbisRtcTick timestamp;
};

struct OrbisImeKeyboardResourceIdArray {
    s32 userId;
    u32 resourceId[6];
};

enum class OrbisImeCaretMovementDirection : u32 {
    STILL = 0,
    LEFT = 1,
    RIGHT = 2,
    UP = 3,
    DOWN = 4,
    HOME = 5,
    END = 6,
    PAGE_UP = 7,
    PAGE_DOWN = 8,
    TOP = 9,
    BOTTOM = 10,
};

union OrbisImeEventParam {
    OrbisImeRect rect;
    OrbisImeEditText text;
    OrbisImeCaretMovementDirection caretMove;
    OrbisImeKeycode keycode;
    OrbisImeKeyboardResourceIdArray resourceIdArray;
    char16_t* candidateWord;
    s32 candidateIndex;
    OrbisImeDeviceType deviceType;
    u32 inputMethodState;
    s8 reserved[64];
};

struct OrbisImeEvent {
    OrbisImeEventId id;
    OrbisImeEventParam param;
};

typedef PS4_SYSV_ABI int (*OrbisImeTextFilter)(char16_t* outText, u32* outTextLength,
                                               const char16_t* srcText, u32 srcTextLength);

typedef PS4_SYSV_ABI void (*OrbisImeEventHandler)(void* arg, const OrbisImeEvent* e);
