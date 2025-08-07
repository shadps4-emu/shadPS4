// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <core/libraries/system/userservice.h>
#include <magic_enum/magic_enum.hpp>
#include "common/enum.h"
#include "common/types.h"
#include "core/libraries/rtc/rtc.h"

constexpr u32 ORBIS_IME_MAX_TEXT_LENGTH = 2048;
constexpr u32 ORBIS_IME_DIALOG_MAX_TEXT_LENGTH = 2048;

template <typename E>
const std::underlying_type_t<E> generate_full_mask() {
    static_assert(std::is_enum_v<E>, "E must be an enum type.");
    static_assert(magic_enum::customize::enum_range<E>::is_flags,
                  "E must be marked as is_flags = true.");

    using U = std::underlying_type_t<E>;
    const auto values = magic_enum::enum_values<E>();
    U mask = 0;

    // Use index-based loop for better constexpr compatibility
    for (std::size_t i = 0; i < values.size(); ++i) {
        mask |= static_cast<U>(values[i]);
    }

    return mask;
}

enum class Error : u32 {
    OK = 0x0,

    // ImeDialog library
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

    // Ime library
    DIALOG_INVALID_TITLE = 0x80bc0101,
    DIALOG_NOT_RUNNING = 0x80bc0105,
    DIALOG_NOT_FINISHED = 0x80bc0106,
    DIALOG_NOT_IN_USE = 0x80bc0107
};

enum class OrbisImeOption : u32 {
    DEFAULT = 0,
    MULTILINE = 1,
    NO_AUTO_CAPITALIZATION = 2,
    PASSWORD = 4,
    LANGUAGES_FORCED = 8,
    EXT_KEYBOARD = 16,
    NO_LEARNING = 32,
    FIXED_POSITION = 64,
    DISABLE_COPY_PASTE = 128,
    DISABLE_RESUME = 256,
    DISABLE_AUTO_SPACE = 512,
    DISABLE_POSITION_ADJUSTMENT = 2048,
    EXPANDED_PREEDIT_BUFFER = 4096,
    USE_JAPANESE_EISUU_KEY_AS_CAPSLOCK = 8192,
    USE_OVER_2K_COORDINATES = 16384,
};
DECLARE_ENUM_FLAG_OPERATORS(OrbisImeOption);
template <>
struct magic_enum::customize::enum_range<OrbisImeOption> {
    static constexpr bool is_flags = true;
};
const u32 kValidImeOptionMask = generate_full_mask<OrbisImeOption>();

enum class OrbisImeExtOption : u32 {
    DEFAULT = 0x00000000,
    SET_COLOR = 0x00000001,
    SET_PRIORITY = 0x00000002,
    PRIORITY_SHIFT = 0x00000004,
    PRIORITY_FULL_WIDTH = 0x00000008,
    PRIORITY_FIXED_PANEL = 0x00000010,
    DISABLE_POINTER = 0x00000040,
    ENABLE_ADDITIONAL_DICTIONARY = 0x00000080,
    DISABLE_STARTUP_SE = 0x00000100,
    DISABLE_LIST_FOR_EXT_KEYBOARD = 0x00000200,
    HIDE_KEYPANEL_IF_EXT_KEYBOARD = 0x00000400,
    INIT_EXT_KEYBOARD_MODE = 0x00000800,

    ENABLE_ACCESSIBILITY = 0x00001000,                // ImeDialog unly
    ACCESSIBILITY_PANEL_FORCED = 0x00002000,          // ImeDialog only
    ADDITIONAL_DICTIONARY_PRIORITY_MODE = 0x00004000, // ImeDialog only
};
DECLARE_ENUM_FLAG_OPERATORS(OrbisImeExtOption);

constexpr u32 kValidImeExtOptionMask = static_cast<u32>(
    OrbisImeExtOption::SET_PRIORITY | OrbisImeExtOption::PRIORITY_FULL_WIDTH |
    OrbisImeExtOption::PRIORITY_FIXED_PANEL | OrbisImeExtOption::DISABLE_POINTER |
    OrbisImeExtOption::ENABLE_ADDITIONAL_DICTIONARY | OrbisImeExtOption::DISABLE_STARTUP_SE |
    OrbisImeExtOption::DISABLE_LIST_FOR_EXT_KEYBOARD |
    OrbisImeExtOption::HIDE_KEYPANEL_IF_EXT_KEYBOARD | OrbisImeExtOption::INIT_EXT_KEYBOARD_MODE);

template <>
struct magic_enum::customize::enum_range<OrbisImeExtOption> {
    static constexpr bool is_flags = true;
};
const u32 kValidImeDialogExtOptionMask = generate_full_mask<OrbisImeExtOption>();

enum class OrbisImeLanguage : u64 {
    DANISH = 0x0000000000000001,
    GERMAN = 0x0000000000000002,
    ENGLISH_US = 0x0000000000000004,
    SPANISH = 0x0000000000000008,
    FRENCH = 0x0000000000000010,
    ITALIAN = 0x0000000000000020,
    DUTCH = 0x0000000000000040,
    NORWEGIAN = 0x0000000000000080,
    POLISH = 0x0000000000000100,
    PORTUGUESE_PT = 0x0000000000000200,
    RUSSIAN = 0x0000000000000400,
    FINNISH = 0x0000000000000800,
    SWEDISH = 0x0000000000001000,
    JAPANESE = 0x0000000000002000,
    KOREAN = 0x0000000000004000,
    SIMPLIFIED_CHINESE = 0x0000000000008000,
    TRADITIONAL_CHINESE = 0x0000000000010000,
    PORTUGUESE_BR = 0x0000000000020000,
    ENGLISH_GB = 0x0000000000040000,
    TURKISH = 0x0000000000080000,
    SPANISH_LA = 0x0000000000100000,
    ARABIC = 0x0000000001000000,
    FRENCH_CA = 0x0000000002000000,
    THAI = 0x0000000004000000,
    CZECH = 0x0000000008000000,
    GREEK = 0x0000000010000000,
    INDONESIAN = 0x0000000020000000,
    VIETNAMESE = 0x0000000040000000,
    ROMANIAN = 0x0000000080000000,
    HUNGARIAN = 0x0000000100000000,
};
DECLARE_ENUM_FLAG_OPERATORS(OrbisImeLanguage);

template <>
struct magic_enum::customize::enum_range<OrbisImeLanguage> {
    static constexpr bool is_flags = true;
};
const u64 kValidOrbisImeLanguageMask = generate_full_mask<OrbisImeLanguage>();

enum class OrbisImeDisableDevice : u32 {
    DEFAULT = 0x00000000,
    CONTROLLER = 0x00000001,
    EXT_KEYBOARD = 0x00000002,
    REMOTE_OSK = 0x00000004,
};
DECLARE_ENUM_FLAG_OPERATORS(OrbisImeDisableDevice);
template <>
struct magic_enum::customize::enum_range<OrbisImeDisableDevice> {
    static constexpr bool is_flags = true;
};
const u32 kValidOrbisImeDisableDeviceMask = generate_full_mask<OrbisImeDisableDevice>();

enum class OrbisImeInputMethodState : u32 {
    PREEDIT = 0x01000000,
    SELECTED = 0x02000000,
    NATIVE = 0x04000000,
    NATIVE2 = 0x08000000,
    FULL_WIDTH = 0x10000000,
};
DECLARE_ENUM_FLAG_OPERATORS(OrbisImeInputMethodState);
template <>
struct magic_enum::customize::enum_range<OrbisImeInputMethodState> {
    static constexpr bool is_flags = true;
};
const u32 kValidOrbisImeInputMethodStateMask = generate_full_mask<OrbisImeInputMethodState>();

enum class OrbisImeInitExtKeyboardMode : u32 {
    ISABLE_ARABIC_INDIC_NUMERALS = 0x00000001,
    ENABLE_FORMAT_CHARACTERS = 0x00000002,
    INPUT_METHOD_STATE_NATIVE = 0x04000000,
    INPUT_METHOD_STATE_NATIVE2 = 0x08000000,
    INPUT_METHOD_STATE_FULL_WIDTH = 0x10000000,
};
DECLARE_ENUM_FLAG_OPERATORS(OrbisImeInitExtKeyboardMode);
template <>
struct magic_enum::customize::enum_range<OrbisImeInitExtKeyboardMode> {
    static constexpr bool is_flags = true;
};
const u32 kValidOrbisImeInitExtKeyboardModeMask = generate_full_mask<OrbisImeInitExtKeyboardMode>();

enum class OrbisImeKeycodeState : u32 {
    KEYCODE_VALID = 0x00000001,
    CHARACTER_VALID = 0x00000002,
    WITH_IME = 0x00000004,
    FROM_OSK = 0x00000008,
    FROM_OSK_SHORTCUT = 0x00000010,
    FROM_IME_OPERATION = 0x00000020,
    REPLACE_CHARACTER = 0x00000040,
    CONTINUOUS_EVENT = 0x00000080,
    MODIFIER_L_CTRL = 0x00000100,
    MODIFIER_L_SHIFT = 0x00000200,
    MODIFIER_L_ALT = 0x00000400,
    MODIFIER_L_GUI = 0x00000800,
    MODIFIER_R_CTRL = 0x00001000,
    MODIFIER_R_SHIFT = 0x00002000,
    MODIFIER_R_ALT = 0x00004000,
    MODIFIER_R_GUI = 0x00008000,
    LED_NUM_LOCK = 0x00010000,
    LED_CAPS_LOCK = 0x00020000,
    LED_SCROLL_LOCK = 0x00040000,
    RESERVED1 = 0x00080000,
    RESERVED2 = 0x00100000,
    FROM_IME_INPUT = 0x00200000,
};
DECLARE_ENUM_FLAG_OPERATORS(OrbisImeKeycodeState);
template <>
struct magic_enum::customize::enum_range<OrbisImeKeycodeState> {
    static constexpr bool is_flags = true;
};
const u32 kValidOrbisImeKeycodeStateMask = generate_full_mask<OrbisImeKeycodeState>();

enum class OrbisImeKeyboardOption : u32 {
    Default = 0,
    Repeat = 1,
    RepeatEachKey = 2,
    AddOsk = 4,
    EffectiveWithIme = 8,
    DisableResume = 16,
    DisableCapslockWithoutShift = 32,
};
DECLARE_ENUM_FLAG_OPERATORS(OrbisImeKeyboardOption)
template <>
struct magic_enum::customize::enum_range<OrbisImeKeyboardOption> {
    static constexpr bool is_flags = true;
};
const u32 kValidOrbisImeKeyboardOptionMask = generate_full_mask<OrbisImeKeyboardOption>();

enum class OrbisImeKeyboardMode : u32 {
    Auto = 0,
    Manual = 1,
    Alphabet = 0,
    Native = 2,
    Part = 4,
    Katakana = 8,
    Hkana = 16,
    ArabicIndicNumerals = 32,
    DisableFormatCharacters = 64,
};

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
    ChangeSize = 3,
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
    JumpToNextObject = 15,
    JumpToBeforeObject = 16,
    ChangeWindowType = 17,

    ChangeInputMethodState = 18,

    KeyboardOpen = 256,
    KeyboardKeycodeDown = 257,
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

enum class OrbisImePanelPriority : u32 {
    Default = 0,
    Alphabet = 1,
    Symbol = 2,
    Accent = 3,
};

struct OrbisImeRect {
    f32 x;
    f32 y;
    u32 width;
    u32 height;
};

struct OrbisImeColor {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

enum class OrbisImeTextAreaMode : u32 {
    Disable = 0,
    Edit = 1,
    Preedit = 2,
    Select = 3,
};

struct OrbisImeTextAreaProperty {
    OrbisImeTextAreaMode mode;
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
    Libraries::UserService::OrbisUserServiceUserId user_id;
    u32 resource_id;
    Libraries::Rtc::OrbisRtcTick timestamp;
};

struct OrbisImeKeyboardResourceIdArray {
    Libraries::UserService::OrbisUserServiceUserId user_id;
    u32 resource_id[5];
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

enum class OrbisImePanelType : u32 {
    Hide = 0,
    Osk = 1,
    Dialog = 2,
    Candidate = 3,
    Edit = 4,
    EditAndCandidate = 5,
    Accessibility = 6,
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
    OrbisImePanelType panel_type;
    u32 input_method_state;
    s8 reserved[64];
};

struct OrbisImeEvent {
    OrbisImeEventId id;
    OrbisImeEventParam param;
};

using OrbisImeExtKeyboardFilter = PS4_SYSV_ABI int (*)(const OrbisImeKeycode* srcKeycode,
                                                       u16* outKeycode, u32* outStatus,
                                                       void* reserved);

using OrbisImeTextFilter = PS4_SYSV_ABI int (*)(char16_t* outText, u32* outTextLength,
                                                const char16_t* srcText, u32 srcTextLength);

using OrbisImeEventHandler = PS4_SYSV_ABI void (*)(void* arg, const OrbisImeEvent* e);

struct OrbisImeKeyboardParam {
    OrbisImeKeyboardOption option;
    s8 reserved1[4];
    void* arg;
    OrbisImeEventHandler handler;
    s8 reserved2[8];
};

struct OrbisImeParam {
    Libraries::UserService::OrbisUserServiceUserId user_id;
    OrbisImeType type;
    OrbisImeLanguage supported_languages;
    OrbisImeEnterLabel enter_label;
    OrbisImeInputMethod input_method;
    OrbisImeTextFilter filter;
    OrbisImeOption option;
    u32 maxTextLength;
    char16_t* inputTextBuffer;
    f32 posx;
    f32 posy;
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

struct OrbisImeDialogParam {
    Libraries::UserService::OrbisUserServiceUserId user_id;
    OrbisImeType type;
    OrbisImeLanguage supported_languages;
    OrbisImeEnterLabel enter_label;
    OrbisImeInputMethod input_method;
    OrbisImeTextFilter filter;
    OrbisImeOption option;
    u32 max_text_length;
    char16_t* input_text_buffer;
    f32 posx;
    f32 posy;
    OrbisImeHorizontalAlignment horizontal_alignment;
    OrbisImeVerticalAlignment vertical_alignment;
    const char16_t* placeholder;
    const char16_t* title;
    s8 reserved[16];
};

struct OrbisImeParamExtended {
    OrbisImeExtOption option;
    OrbisImeColor color_base;
    OrbisImeColor color_line;
    OrbisImeColor color_text_field;
    OrbisImeColor color_preedit;
    OrbisImeColor color_button_default;
    OrbisImeColor color_button_function;
    OrbisImeColor color_button_symbol;
    OrbisImeColor color_text;
    OrbisImeColor color_special;
    OrbisImePanelPriority priority;
    char* additional_dictionary_path;
    OrbisImeExtKeyboardFilter ext_keyboard_filter;
    OrbisImeDisableDevice disable_device;
    u32 ext_keyboard_mode;
    s8 reserved[60];
};
