// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"
#include "ime_common.h"

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
    None = 0,
    Running = 1,
    Finished = 2,
};

enum class OrbisImeDialogEndStatus : u32 {
    Ok = 0,
    UserCanceled = 1,
    Aborted = 2,
};

enum class OrbisImeDialogOption : u32 {
    Default = 0,
    Multiline = 1,
    NoAutoCorrection = 2,
    NoAutoCompletion = 4,
    // TODO: Document missing options
    LargeResolution = 1024,
};
DECLARE_ENUM_FLAG_OPERATORS(OrbisImeDialogOption)

enum class OrbisImePanelPriority : u32 {
    Default = 0,
    Alphabet = 1,
    Symbol = 2,
    Accent = 3,
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
    s32 user_id;
    u32 resource_id;
    u64 timestamp;
};

using OrbisImeExtKeyboardFilter = PS4_SYSV_ABI int (*)(const OrbisImeKeycode* srcKeycode,
                                                       u16* outKeycode, u32* outStatus,
                                                       void* reserved);

struct OrbisImeDialogParam {
    s32 user_id;
    OrbisImeType type;
    u64 supported_languages;
    OrbisImeEnterLabel enter_label;
    OrbisImeInputMethod input_method;
    OrbisImeTextFilter filter;
    OrbisImeDialogOption option;
    u32 max_text_length;
    char16_t* input_text_buffer;
    float posx;
    float posy;
    OrbisImeHorizontalAlignment horizontal_alignment;
    OrbisImeVerticalAlignment vertical_alignment;
    const char16_t* placeholder;
    const char16_t* title;
    s8 reserved[16];
};

struct OrbisImeParamExtended {
    u32 option; // OrbisImeDialogOptionExtended
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
    uint32_t disable_device;
    uint32_t ext_keyboard_mode;
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
