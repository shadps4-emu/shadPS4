// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <magic_enum/magic_enum.hpp>

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "ime_dialog.h"
#include "ime_dialog_ui.h"

static constexpr std::array<float, 2> MAX_X_POSITIONS = {3840.0f, 1920.0f};
static constexpr std::array<float, 2> MAX_Y_POSITIONS = {2160.0f, 1080.0f};

namespace Libraries::ImeDialog {

static OrbisImeDialogStatus g_ime_dlg_status = OrbisImeDialogStatus::None;
static OrbisImeDialogResult g_ime_dlg_result{};
static ImeDialogState g_ime_dlg_state{};
static ImeDialogUi g_ime_dlg_ui;

static bool IsValidOption(OrbisImeOption option, OrbisImeType type) {
    if (False(~option & (OrbisImeOption::MULTILINE |
                         OrbisImeOption::NO_AUTO_CAPITALIZATION /* NoAutoCompletion */))) {
        return false;
    }

    if (True(option & OrbisImeOption::MULTILINE) && type != OrbisImeType::Default &&
        type != OrbisImeType::BasicLatin) {
        return false;
    }

    if (True(option & OrbisImeOption::NO_AUTO_CAPITALIZATION /* NoAutoCompletion */) &&
        type != OrbisImeType::Number && type != OrbisImeType::BasicLatin) {
        return false;
    }

    return true;
}

Error PS4_SYSV_ABI sceImeDialogAbort() {
    if (g_ime_dlg_status == OrbisImeDialogStatus::None) {
        LOG_INFO(Lib_ImeDialog, "IME dialog not in use");
        return Error::DIALOG_NOT_IN_USE;
    }

    if (g_ime_dlg_status != OrbisImeDialogStatus::Running) {
        LOG_INFO(Lib_ImeDialog, "IME dialog not running");
        return Error::DIALOG_NOT_RUNNING;
    }

    g_ime_dlg_status = OrbisImeDialogStatus::Finished;
    g_ime_dlg_result.endstatus = OrbisImeDialogEndStatus::Aborted;

    return Error::OK;
}

Error PS4_SYSV_ABI sceImeDialogForceClose() {
    if (g_ime_dlg_status == OrbisImeDialogStatus::None) {
        LOG_INFO(Lib_ImeDialog, "IME dialog not in use");
        return Error::DIALOG_NOT_IN_USE;
    }

    g_ime_dlg_status = OrbisImeDialogStatus::None;
    g_ime_dlg_ui = ImeDialogUi();
    g_ime_dlg_state = ImeDialogState();

    return Error::OK;
}

Error PS4_SYSV_ABI sceImeDialogForTestFunction() {
    return Error::INTERNAL;
}

int PS4_SYSV_ABI sceImeDialogGetCurrentStarState() {
    LOG_ERROR(Lib_ImeDialog, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeDialogGetPanelPositionAndForm() {
    LOG_ERROR(Lib_ImeDialog, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceImeDialogGetPanelSize(const OrbisImeDialogParam* param, u32* width,
                                            u32* height) {
    LOG_INFO(Lib_ImeDialog, "called");

    if (!width || !height) {
        return Error::INVALID_ADDRESS;
    }
    switch (param->type) {
    case OrbisImeType::Default:
    case OrbisImeType::BasicLatin:
    case OrbisImeType::Url:
    case OrbisImeType::Mail:
        *width = 500; // original: 793
        if (True(param->option & OrbisImeOption::MULTILINE)) {
            *height = 300; // original: 576
        } else {
            *height = 150; // original: 476
        }
        break;
    case OrbisImeType::Number:
        *width = 370;
        *height = 470;
        break;
    default:
        LOG_ERROR(Lib_ImeDialog, "Unknown OrbisImeType: {}", (u32)param->type);
        return Error::INVALID_PARAM;
    }

    return Error::OK;
}

int PS4_SYSV_ABI sceImeDialogGetPanelSizeExtended() {
    LOG_ERROR(Lib_ImeDialog, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceImeDialogGetResult(OrbisImeDialogResult* result) {
    if (g_ime_dlg_status == OrbisImeDialogStatus::None) {
        LOG_INFO(Lib_ImeDialog, "IME dialog is not running");
        return Error::DIALOG_NOT_IN_USE;
    }

    if (result == nullptr) {
        LOG_INFO(Lib_ImeDialog, "called with result (NULL)");
        return Error::INVALID_ADDRESS;
    }

    result->endstatus = g_ime_dlg_result.endstatus;

    if (g_ime_dlg_status == OrbisImeDialogStatus::Running) {
        return Error::DIALOG_NOT_FINISHED;
    }

    g_ime_dlg_state.CopyTextToOrbisBuffer();
    return Error::OK;
}

OrbisImeDialogStatus PS4_SYSV_ABI sceImeDialogGetStatus() {
    if (g_ime_dlg_status == OrbisImeDialogStatus::Running) {
        g_ime_dlg_state.CallTextFilter();
    }

    return g_ime_dlg_status;
}

#include <string>

#include <string>

static std::string ConvertUtf16ToUtf8(const char16_t* src) {
    if (!src) {
        return "(null)";
    }

    std::string result;
    while (*src) {
        char16_t c = *src++;

        if (c < 0x80) {
            result += static_cast<char>(c);
        } else if (c < 0x800) {
            result += static_cast<char>(0xC0 | (c >> 6));
            result += static_cast<char>(0x80 | (c & 0x3F));
        } else {
            result += static_cast<char>(0xE0 | (c >> 12));
            result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (c & 0x3F));
        }
    }

    return result;
}

void DumpImeDialogParam(const Libraries::ImeDialog::OrbisImeDialogParam* param,
                        const Libraries::ImeDialog::OrbisImeParamExtended* ext_param) {
    if (!param) {
        LOG_INFO(Lib_ImeDialog, "OpenImeDialog called with null param.");
        return;
    }

    // UTF-16 to UTF-8 conversion using your safe method
    std::string title_utf8 = "(null)";
    std::string placeholder_utf8 = "(null)";
    title_utf8 = ConvertUtf16ToUtf8(param->title);
    placeholder_utf8 = ConvertUtf16ToUtf8(param->placeholder);

    LOG_INFO(
        Lib_ImeDialog,
        "OpenImeDialog:\n"
        "  user_id={}, type={}, option=0x{:X}, max_text_length={}, supported_languages=0x{:X}\n"
        "  title=\"{}\", placeholder=\"{}\", input_text_buffer={}",
        param->user_id, static_cast<int>(param->type), static_cast<u32>(param->option),
        param->max_text_length, param->supported_languages, title_utf8, placeholder_utf8,
        param->input_text_buffer ? reinterpret_cast<uintptr_t>(param->input_text_buffer) : 0);

    if (ext_param) {
        LOG_INFO(Lib_ImeDialog,
                 "ExtendedParam:\n"
                 "  color_base=({}, {}, {}, {}) color_line=({}, {}, {}, {}) color_text_field=({}, "
                 "{}, {}, {})\n"
                 "  color_preedit=({}, {}, {}, {}) color_button_default=({}, {}, {}, {}) "
                 "color_button_function=({}, {}, {}, {})\n"
                 "  color_button_symbol=({}, {}, {}, {}) color_text=({}, {}, {}, {}) "
                 "color_special=({}, {}, {}, {})\n"
                 "  priority={}",
                 ext_param->color_base.r, ext_param->color_base.g, ext_param->color_base.b,
                 ext_param->color_base.a, ext_param->color_line.r, ext_param->color_line.g,
                 ext_param->color_line.b, ext_param->color_line.a, ext_param->color_text_field.r,
                 ext_param->color_text_field.g, ext_param->color_text_field.b,
                 ext_param->color_text_field.a, ext_param->color_preedit.r,
                 ext_param->color_preedit.g, ext_param->color_preedit.b, ext_param->color_preedit.a,
                 ext_param->color_button_default.r, ext_param->color_button_default.g,
                 ext_param->color_button_default.b, ext_param->color_button_default.a,
                 ext_param->color_button_function.r, ext_param->color_button_function.g,
                 ext_param->color_button_function.b, ext_param->color_button_function.a,
                 ext_param->color_button_symbol.r, ext_param->color_button_symbol.g,
                 ext_param->color_button_symbol.b, ext_param->color_button_symbol.a,
                 ext_param->color_text.r, ext_param->color_text.g, ext_param->color_text.b,
                 ext_param->color_text.a, ext_param->color_special.r, ext_param->color_special.g,
                 ext_param->color_special.b, ext_param->color_special.a,
                 static_cast<int>(ext_param->priority));
    } else {
        LOG_INFO(Lib_ImeDialog, "ExtendedParam: (none)");
    }
}

Error PS4_SYSV_ABI sceImeDialogInit(OrbisImeDialogParam* param, OrbisImeParamExtended* extended) {
    LOG_INFO(Lib_ImeDialog, ">> sceImeDialogInit: entering, param={}, extended={}",
             static_cast<void*>(param), static_cast<void*>(extended));
    if (g_ime_dlg_status != OrbisImeDialogStatus::None) {
        LOG_ERROR(Lib_ImeDialog, "sceImeDialogInit: busy (status=%u)", (u32)g_ime_dlg_status);
        return Error::BUSY;
    }

    if (param == nullptr) {
        LOG_ERROR(Lib_ImeDialog, "sceImeDialogInit: param is null");
        return Error::INVALID_ADDRESS;
    }

    if (!magic_enum::enum_contains(param->type)) {
        LOG_ERROR(Lib_ImeDialog, "sceImeDialogInit: invalid param->type=%u", (u32)param->type);
        return Error::INVALID_ADDRESS;
    }

    // TODO: do correct param->option validation
    // TODO: do correct param->supportedLanguages validation

    if (param->posx < 0.0f ||
        param->posx >= MAX_X_POSITIONS[False(param->option & OrbisImeOption::USE_2K_COORDINATES)]) {
        LOG_ERROR(Lib_ImeDialog, "sceImeDialogInit: invalid posx=%f", param->posx);
        return Error::INVALID_POSX;
    }

    if (param->posy < 0.0f ||
        param->posy >= MAX_Y_POSITIONS[False(param->option & OrbisImeOption::USE_2K_COORDINATES)]) {
        LOG_ERROR(Lib_ImeDialog, "sceImeDialogInit: invalid posy=%f", param->posy);
        return Error::INVALID_POSY;
    }

    if (!magic_enum::enum_contains(param->horizontal_alignment)) {
        LOG_INFO(Lib_ImeDialog, "Invalid param->horizontalAlignment");
        return Error::INVALID_HORIZONTALIGNMENT;
    }

    if (!magic_enum::enum_contains(param->vertical_alignment)) {
        LOG_INFO(Lib_ImeDialog, "Invalid param->verticalAlignment");
        return Error::INVALID_VERTICALALIGNMENT;
    }

    if (!IsValidOption(param->option, param->type)) {
        LOG_ERROR(Lib_ImeDialog, "sceImeDialogInit: invalid option=0x%X for type=%u",
                  static_cast<u32>(param->option), (u32)param->type);
        return Error::INVALID_PARAM;
    }

    if (param->input_text_buffer == nullptr) {
        LOG_ERROR(Lib_ImeDialog, "sceImeDialogInit: input_text_buffer is null");
        return Error::INVALID_INPUT_TEXT_BUFFER;
    }

    if (extended) {
        if (!magic_enum::enum_contains(extended->priority)) {
            LOG_INFO(Lib_ImeDialog, "Invalid extended->priority");
            return Error::INVALID_EXTENDED;
        }

        // TODO: do correct extended->option validation

        if ((extended->ext_keyboard_mode & 0xe3fffffc) != 0) {
            LOG_INFO(Lib_ImeDialog, "Invalid extended->extKeyboardMode");
            return Error::INVALID_EXTENDED;
        }

        if (extended->disable_device > 7) {
            LOG_INFO(Lib_ImeDialog, "Invalid extended->disableDevice");
            return Error::INVALID_EXTENDED;
        }
    }

    if (param->max_text_length == 0 || param->max_text_length > ORBIS_IME_MAX_TEXT_LENGTH) {
        LOG_ERROR(Lib_ImeDialog, "sceImeDialogInit: invalid max_text_length=%u",
                  param->max_text_length);
        return Error::INVALID_MAX_TEXT_LENGTH;
    }

    // Title string validation
    if (param->title != nullptr && !std::char_traits<char16_t>::length(param->title)) {
        LOG_ERROR(Lib_ImeDialog, "sceImeDialogInit: title is empty");
        return Error::INVALID_PARAM;
    }

    g_ime_dlg_result = {};
    g_ime_dlg_state = ImeDialogState(param, extended);
    g_ime_dlg_status = OrbisImeDialogStatus::Running;
    g_ime_dlg_ui = ImeDialogUi(&g_ime_dlg_state, &g_ime_dlg_status, &g_ime_dlg_result);

    LOG_INFO(Lib_ImeDialog, "<< sceImeDialogInit: successful, status now=Running");
    return Error::OK;
}

int PS4_SYSV_ABI sceImeDialogInitInternal() {
    LOG_ERROR(Lib_ImeDialog, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeDialogInitInternal2() {
    LOG_ERROR(Lib_ImeDialog, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeDialogInitInternal3() {
    LOG_ERROR(Lib_ImeDialog, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceImeDialogSetPanelPosition() {
    LOG_ERROR(Lib_ImeDialog, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceImeDialogTerm() {
    if (g_ime_dlg_status == OrbisImeDialogStatus::None) {
        LOG_INFO(Lib_ImeDialog, "IME dialog not in use");
        return Error::DIALOG_NOT_IN_USE;
    }

    if (g_ime_dlg_status == OrbisImeDialogStatus::Running) {
        LOG_INFO(Lib_ImeDialog, "IME dialog is still running");
        return Error::DIALOG_NOT_FINISHED;
    }

    g_ime_dlg_status = OrbisImeDialogStatus::None;
    g_ime_dlg_ui = ImeDialogUi();
    g_ime_dlg_state = ImeDialogState();

    return Error::OK;
}

void RegisterlibSceImeDialog(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("oBmw4xrmfKs", "libSceImeDialog", 1, "libSceImeDialog", 1, 1, sceImeDialogAbort);
    LIB_FUNCTION("bX4H+sxPI-o", "libSceImeDialog", 1, "libSceImeDialog", 1, 1,
                 sceImeDialogForceClose);
    LIB_FUNCTION("UFcyYDf+e88", "libSceImeDialog", 1, "libSceImeDialog", 1, 1,
                 sceImeDialogForTestFunction);
    LIB_FUNCTION("fy6ntM25pEc", "libSceImeDialog", 1, "libSceImeDialog", 1, 1,
                 sceImeDialogGetCurrentStarState);
    LIB_FUNCTION("8jqzzPioYl8", "libSceImeDialog", 1, "libSceImeDialog", 1, 1,
                 sceImeDialogGetPanelPositionAndForm);
    LIB_FUNCTION("wqsJvRXwl58", "libSceImeDialog", 1, "libSceImeDialog", 1, 1,
                 sceImeDialogGetPanelSize);
    LIB_FUNCTION("CRD+jSErEJQ", "libSceImeDialog", 1, "libSceImeDialog", 1, 1,
                 sceImeDialogGetPanelSizeExtended);
    LIB_FUNCTION("x01jxu+vxlc", "libSceImeDialog", 1, "libSceImeDialog", 1, 1,
                 sceImeDialogGetResult);
    LIB_FUNCTION("IADmD4tScBY", "libSceImeDialog", 1, "libSceImeDialog", 1, 1,
                 sceImeDialogGetStatus);
    LIB_FUNCTION("NUeBrN7hzf0", "libSceImeDialog", 1, "libSceImeDialog", 1, 1, sceImeDialogInit);
    LIB_FUNCTION("KR6QDasuKco", "libSceImeDialog", 1, "libSceImeDialog", 1, 1,
                 sceImeDialogInitInternal);
    LIB_FUNCTION("oe92cnJQ9HE", "libSceImeDialog", 1, "libSceImeDialog", 1, 1,
                 sceImeDialogInitInternal2);
    LIB_FUNCTION("IoKIpNf9EK0", "libSceImeDialog", 1, "libSceImeDialog", 1, 1,
                 sceImeDialogInitInternal3);
    LIB_FUNCTION("-2WqB87KKGg", "libSceImeDialog", 1, "libSceImeDialog", 1, 1,
                 sceImeDialogSetPanelPosition);
    LIB_FUNCTION("gyTyVn+bXMw", "libSceImeDialog", 1, "libSceImeDialog", 1, 1, sceImeDialogTerm);
};

} // namespace Libraries::ImeDialog
