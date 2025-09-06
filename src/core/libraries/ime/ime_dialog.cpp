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
    LOG_INFO(Lib_ImeDialog, "called");
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

Error PS4_SYSV_ABI sceImeDialogInit(OrbisImeDialogParam* param, OrbisImeParamExtended* extended) {
    LOG_INFO(Lib_ImeDialog, "called, param={}, extended={}", static_cast<void*>(param),
             static_cast<void*>(extended));

    if (param == nullptr) {
        LOG_ERROR(Lib_ImeDialog, "param is null");
        return Error::INVALID_ADDRESS;
    } else {
        LOG_DEBUG(Lib_ImeDialog, "param->user_id: {}", static_cast<u32>(param->user_id));
        LOG_DEBUG(Lib_ImeDialog, "param->type: {}", static_cast<u32>(param->type));
        LOG_DEBUG(Lib_ImeDialog, "param->supported_languages: {:064b}",
                  static_cast<u64>(param->supported_languages));
        LOG_DEBUG(Lib_ImeDialog, "param->enter_label: {}", static_cast<u32>(param->enter_label));
        LOG_DEBUG(Lib_ImeDialog, "param->input_method: {}", static_cast<u32>(param->input_method));
        LOG_DEBUG(Lib_ImeDialog, "param->filter: {}", (void*)param->filter);
        LOG_DEBUG(Lib_ImeDialog, "param->option: {:032b}", static_cast<u32>(param->option));
        LOG_DEBUG(Lib_ImeDialog, "param->max_text_length: {}", param->max_text_length);
        LOG_DEBUG(Lib_ImeDialog, "param->input_text_buffer: {}", (void*)param->input_text_buffer);
        LOG_DEBUG(Lib_ImeDialog, "param->posx: {}", param->posx);
        LOG_DEBUG(Lib_ImeDialog, "param->posy: {}", param->posy);
        LOG_DEBUG(Lib_ImeDialog, "param->horizontal_alignment: {}",
                  static_cast<u32>(param->horizontal_alignment));
        LOG_DEBUG(Lib_ImeDialog, "param->vertical_alignment: {}",
                  static_cast<u32>(param->vertical_alignment));
        LOG_DEBUG(Lib_ImeDialog, "param->placeholder: {}",
                  param->placeholder ? "<non-null>" : "NULL");
        LOG_DEBUG(Lib_ImeDialog, "param.title: {}", param->title ? "<non-null>" : "NULL");
    }

    if (g_ime_dlg_status != OrbisImeDialogStatus::None) {
        LOG_ERROR(Lib_ImeDialog, "busy (status={})", (u32)g_ime_dlg_status);
        return Error::BUSY;
    }

    if (!magic_enum::enum_contains(param->type)) {
        LOG_ERROR(Lib_ImeDialog, "invalid param->type={}", (u32)param->type);
        return Error::INVALID_ADDRESS;
    }

    // TODO: do correct param->option validation
    // TODO: do correct param->supportedLanguages validation

    if (param->posx < 0.0f ||
        param->posx >=
            MAX_X_POSITIONS[False(param->option & OrbisImeOption::USE_OVER_2K_COORDINATES)]) {
        LOG_ERROR(Lib_ImeDialog, "Invalid posx: {}", param->posx);
        return Error::INVALID_POSX;
    }

    if (param->posy < 0.0f ||
        param->posy >=
            MAX_Y_POSITIONS[False(param->option & OrbisImeOption::USE_OVER_2K_COORDINATES)]) {
        LOG_ERROR(Lib_ImeDialog, "invalid posy: {}", param->posy);
        return Error::INVALID_POSY;
    }

    if (!magic_enum::enum_contains(param->horizontal_alignment)) {
        LOG_INFO(Lib_ImeDialog, "Invalid param->horizontalAlignment: {}",
                 (u32)param->horizontal_alignment);
        return Error::INVALID_HORIZONTALIGNMENT;
    }

    if (!magic_enum::enum_contains(param->vertical_alignment)) {
        LOG_INFO(Lib_ImeDialog, "Invalid param->verticalAlignment: {}",
                 (u32)param->vertical_alignment);
        return Error::INVALID_VERTICALALIGNMENT;
    }

    if (!IsValidOption(param->option, param->type)) {
        LOG_ERROR(Lib_ImeDialog, "Invalid option: {:032b} for type={}",
                  static_cast<u32>(param->option), (u32)param->type);
        return Error::INVALID_PARAM;
    }

    if (param->input_text_buffer == nullptr) {
        LOG_ERROR(Lib_ImeDialog, "Invalid input_text_buffer: null");
        return Error::INVALID_INPUT_TEXT_BUFFER;
    }

    if (extended) {
        LOG_DEBUG(Lib_ImeDialog, "extended->option: {:032b}", static_cast<u32>(extended->option));
        LOG_DEBUG(Lib_ImeDialog, "extended->color_base: {{{},{},{},{}}}", extended->color_base.r,
                  extended->color_base.g, extended->color_base.b, extended->color_base.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_line: {{{},{},{},{}}}", extended->color_line.r,
                  extended->color_line.g, extended->color_line.b, extended->color_line.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_text_field: {{{},{},{},{}}}",
                  extended->color_text_field.r, extended->color_text_field.g,
                  extended->color_text_field.b, extended->color_text_field.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_preedit: {{{},{},{},{}}}",
                  extended->color_preedit.r, extended->color_preedit.g, extended->color_preedit.b,
                  extended->color_preedit.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_button_default: {{{},{},{},{}}}",
                  extended->color_button_default.r, extended->color_button_default.g,
                  extended->color_button_default.b, extended->color_button_default.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_button_function: {{{},{},{},{}}}",
                  extended->color_button_function.r, extended->color_button_function.g,
                  extended->color_button_function.b, extended->color_button_function.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_button_symbol: {{{},{},{},{}}}",
                  extended->color_button_symbol.r, extended->color_button_symbol.g,
                  extended->color_button_symbol.b, extended->color_button_symbol.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_text: {{{},{},{},{}}}", extended->color_text.r,
                  extended->color_text.g, extended->color_text.b, extended->color_text.a);
        LOG_DEBUG(Lib_ImeDialog, "extended->color_special: {{{},{},{},{}}}",
                  extended->color_special.r, extended->color_special.g, extended->color_special.b,
                  extended->color_special.a);

        LOG_DEBUG(Lib_ImeDialog, "extended->priority: {:032b}",
                  static_cast<u32>(extended->priority));
        LOG_DEBUG(Lib_ImeDialog, "extended->disable_device: {:032b}",
                  static_cast<u32>(extended->disable_device));
        LOG_DEBUG(Lib_ImeDialog, "extended->ext_keyboard_mode: {:032b}",
                  static_cast<u32>(extended->ext_keyboard_mode));

        LOG_DEBUG(Lib_ImeDialog, "extended->additional_dictionary_path: {}",
                  extended->additional_dictionary_path ? extended->additional_dictionary_path
                                                       : "NULL");
        LOG_DEBUG(Lib_ImeDialog, "param->filter: {}", (void*)param->filter);

        if (!magic_enum::enum_contains(extended->priority)) {
            LOG_INFO(Lib_ImeDialog, "Invalid extended->priority: {}", (u32)extended->priority);
            return Error::INVALID_EXTENDED;
        }

        // TODO: do correct extended->option validation

        if ((extended->ext_keyboard_mode & 0xe3fffffc) != 0) {
            LOG_INFO(Lib_ImeDialog, "Invalid extended->extKeyboardMode");
            return Error::INVALID_EXTENDED;
        }

        if (static_cast<u32>(extended->disable_device) & ~kValidOrbisImeDisableDeviceMask) {
            LOG_ERROR(Lib_ImeDialog,
                      "sceImeDialogInit: disable_device has invalid bits set (0x{:X})",
                      static_cast<u32>(extended->disable_device));
            return Error::INVALID_EXTENDED;
        }
    } else {
        LOG_DEBUG(Lib_ImeDialog, "extended: NULL");
    }

    if (param->max_text_length == 0 || param->max_text_length > ORBIS_IME_MAX_TEXT_LENGTH) {
        LOG_ERROR(Lib_ImeDialog, "sceImeDialogInit: invalid max_text_length={}",
                  param->max_text_length);
        return Error::INVALID_MAX_TEXT_LENGTH;
    }

    g_ime_dlg_result = {};
    g_ime_dlg_state = ImeDialogState(param, extended);
    g_ime_dlg_status = OrbisImeDialogStatus::Running;
    g_ime_dlg_ui = ImeDialogUi(&g_ime_dlg_state, &g_ime_dlg_status, &g_ime_dlg_result);

    LOG_INFO(Lib_ImeDialog, "sceImeDialogInit: successful, status now=Running");
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
    LOG_INFO(Lib_ImeDialog, "called");
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

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("oBmw4xrmfKs", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogAbort);
    LIB_FUNCTION("bX4H+sxPI-o", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogForceClose);
    LIB_FUNCTION("UFcyYDf+e88", "libSceImeDialog", 1, "libSceImeDialog",
                 sceImeDialogForTestFunction);
    LIB_FUNCTION("fy6ntM25pEc", "libSceImeDialog", 1, "libSceImeDialog",
                 sceImeDialogGetCurrentStarState);
    LIB_FUNCTION("8jqzzPioYl8", "libSceImeDialog", 1, "libSceImeDialog",
                 sceImeDialogGetPanelPositionAndForm);
    LIB_FUNCTION("wqsJvRXwl58", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogGetPanelSize);
    LIB_FUNCTION("CRD+jSErEJQ", "libSceImeDialog", 1, "libSceImeDialog",
                 sceImeDialogGetPanelSizeExtended);
    LIB_FUNCTION("x01jxu+vxlc", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogGetResult);
    LIB_FUNCTION("IADmD4tScBY", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogGetStatus);
    LIB_FUNCTION("NUeBrN7hzf0", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogInit);
    LIB_FUNCTION("KR6QDasuKco", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogInitInternal);
    LIB_FUNCTION("oe92cnJQ9HE", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogInitInternal2);
    LIB_FUNCTION("IoKIpNf9EK0", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogInitInternal3);
    LIB_FUNCTION("-2WqB87KKGg", "libSceImeDialog", 1, "libSceImeDialog",
                 sceImeDialogSetPanelPosition);
    LIB_FUNCTION("gyTyVn+bXMw", "libSceImeDialog", 1, "libSceImeDialog", sceImeDialogTerm);
};

} // namespace Libraries::ImeDialog
