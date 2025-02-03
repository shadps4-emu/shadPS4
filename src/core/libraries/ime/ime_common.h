// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/rtc/rtc.h"

enum class OrbisImeType : u32 {
    Default = 0,
    BasicLatin = 1,
    Url = 2,
    Mail = 3,
    Number = 4,
};

enum class OrbisImeHorizontalAlignment : u32 {
    Left = 0,
    Center = 1,
    Right = 2,
};

enum class OrbisImeVerticalAlignment : u32 {
    Top = 0,
    Center = 1,
    Bottom = 2,
};

enum class OrbisImeEnterLabel : u32 {
    Default = 0,
    Send = 1,
    Search = 2,
    Go = 3,
};

enum class OrbisImeInputMethod : u32 {
    Default = 0,
};

enum class OrbisImeEventId : u32 {
    Open = 0,
    UpdateText = 1,
    UpdateCaret = 2,
    PressClose = 4,
    PressEnter = 5,
    Abort = 6,
    CandidateListStart = 7,
    CandidateListEnd = 8,
    CandidateWord = 9,
    CandidateIndex = 10,
    CandidateDone = 11,
    CandidateCancel = 12,
    ChangeDevice = 14,
    ChangeInputMethodState = 18,

    KeyboardOpen = 256,
    KeyboardKeycodeDoen = 257,
    KeyboardKeycodeUp = 258,
    KeyboardKeycodeRepeat = 259,
    KeyboardConnection = 260,
    KeyboardDisconnection = 261,
    KeyboardAbort = 262,
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
    None = 0,
    Controller = 1,
    ExtKeyboard = 2,
    RemoteOsk = 3,
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
    u32 caret_index;
    u32 area_num;
    OrbisImeTextAreaProperty text_area[4];
};

struct OrbisImeKeycode {
    u16 keycode;
    char16_t character;
    u32 status;
    OrbisImeKeyboardType type;
    s32 user_id;
    u32 resource_id;
    Libraries::Rtc::OrbisRtcTick timestamp;
};

struct OrbisImeKeyboardResourceIdArray {
    s32 userId;
    u32 resourceId[5];
};

enum class OrbisImeCaretMovementDirection : u32 {
    Still = 0,
    Left = 1,
    Right = 2,
    Up = 3,
    Down = 4,
    Home = 5,
    End = 6,
    PageUp = 7,
    PageDown = 8,
    Top = 9,
    Bottom = 10,
};

union OrbisImeEventParam {
    OrbisImeRect rect;
    OrbisImeEditText text;
    OrbisImeCaretMovementDirection caret_move;
    OrbisImeKeycode keycode;
    OrbisImeKeyboardResourceIdArray resource_id_array;
    char16_t* candidate_word;
    s32 candidate_index;
    OrbisImeDeviceType device_type;
    u32 input_method_state;
    s8 reserved[64];
};

struct OrbisImeEvent {
    OrbisImeEventId id;
    OrbisImeEventParam param;
};

using OrbisImeTextFilter = PS4_SYSV_ABI int (*)(char16_t* outText, u32* outTextLength,
                                                const char16_t* srcText, u32 srcTextLength);

using OrbisImeEventHandler = PS4_SYSV_ABI void (*)(void* arg, const OrbisImeEvent* e);
