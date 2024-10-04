// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>
#include <magic_enum.hpp>
#include <string>
#include <cwchar>

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/dialogs/ime_dialog.h"
#include "core/libraries/dialogs/ime_dialog_ui.h"
#include "core/linker.h"
#include "imgui/imgui_std.h"

#ifndef _WIN32
#define IME_UTF8_ENCODING "UTF-8"
#define IME_ORBIS_ENCODING "UTF-16LE"
#else
#include <Windows.h>
#endif

using namespace ImGui;

static constexpr ImVec2 BUTTON_SIZE{100.0f, 30.0f};

namespace Libraries::ImeDialog {

ImeDialogState::ImeDialogState(const OrbisImeDialogParam* param, const OrbisImeParamExtended* extended) {
    if (!param) return;

    userId = param->userId;
    is_multiLine = True(param->option & OrbisImeDialogOption::MULTILINE);
    type = param->type;
    enter_label = param->enterLabel;
    text_filter = param->filter;
    keyboard_filter = extended ? extended->extKeyboardFilter : nullptr;
    max_text_length = param->maxTextLength;
    text_buffer = param->inputTextBuffer;

#ifndef _WIN32
    orbis_to_utf8 = iconv_open(IME_UTF8_ENCODING, IME_ORBIS_ENCODING);
    utf8_to_orbis = iconv_open(IME_ORBIS_ENCODING , IME_UTF8_ENCODING);

    ASSERT_MSG(orbis_to_utf8 != (iconv_t)-1, "Failed to open iconv orbis_to_utf8");
    ASSERT_MSG(utf8_to_orbis != (iconv_t)-1, "Failed to open iconv utf8_to_orbis");
#endif

    std::size_t title_len = std::char_traits<char16_t>::length(param->title);
    title = new char[title_len * 4 + 1];

    if (!ConvertOrbisToUTF8(param->title, title_len, title, title_len)) {
        LOG_ERROR(Lib_ImeDialog, "Failed to convert title to utf8 encoding");
        return;
    }

    if (!param->placeholder) {
        return;
    }

    std::size_t placeholder_len = std::char_traits<char16_t>::length(param->placeholder);
    placeholder = new char[placeholder_len * 4 + 1];

    if (!ConvertOrbisToUTF8(param->placeholder, placeholder_len, placeholder, placeholder_len)) {
        LOG_ERROR(Lib_ImeDialog, "Failed to convert placeholder to utf8 encoding");
    }
}

ImeDialogState::~ImeDialogState() {
#ifndef _WIN32
    if (orbis_to_utf8 != (iconv_t)-1) {
        iconv_close(orbis_to_utf8);
    }
    if (utf8_to_orbis != (iconv_t)-1) {
        iconv_close(utf8_to_orbis);
    }
#endif

    if (title) {
        delete[] title;
    }

    if (placeholder) {
        delete[] placeholder;
    }
}

bool ImeDialogState::CallTextFilter() {
    std::scoped_lock lock(mutex);
    
    if (!text_filter || !input_changed) {
        return true;
    }

    input_changed = false;

    char16_t src_text[ORBIS_IME_DIALOG_MAX_TEXT_LENGTH + 1] = {0};
    u32 src_text_length = std::strlen(current_text);
    char16_t out_text[ORBIS_IME_DIALOG_MAX_TEXT_LENGTH + 1] = {0};
    u32 out_text_length = ORBIS_IME_DIALOG_MAX_TEXT_LENGTH;

    if (!ConvertUTF8ToOrbis(current_text, src_text_length, src_text, ORBIS_IME_DIALOG_MAX_TEXT_LENGTH)) {
        LOG_ERROR(Lib_ImeDialog, "Failed to convert text to orbis encoding");
        return false;
    }

    auto* linker = Common::Singleton<Core::Linker>::Instance();
    int ret = linker->ExecuteGuest(text_filter, out_text, &out_text_length, src_text, src_text_length);

    if (ret != 0) {
        return false;
    }

    if (!ConvertOrbisToUTF8(out_text, out_text_length, current_text, ORBIS_IME_DIALOG_MAX_TEXT_LENGTH)) {
        LOG_ERROR(Lib_ImeDialog, "Failed to convert text to utf8 encoding");
        return false;
    }

    return true;
}

bool ImeDialogState::CallKeyboardFilter(const OrbisImeKeycode* src_keycode, u16* out_keycode, u32* out_status) {
    if (!keyboard_filter) {
        return true;
    }

    auto* linker = Common::Singleton<Core::Linker>::Instance();
    int ret = linker->ExecuteGuest(keyboard_filter, src_keycode, out_keycode, out_status, nullptr);

    return ret == 0;
}

bool ImeDialogState::ConvertOrbisToUTF8(const char16_t* orbis_text, std::size_t orbis_text_len, char* utf8_text, std::size_t utf8_text_len) {
#ifndef _WIN32
    std::size_t orbis_text_len_bytes = orbis_text_len * sizeof(char16_t);
    std::size_t utf8_text_len_bytes = utf8_text_len * sizeof(char);

    char16_t* orbis_text_ptr = const_cast<char16_t*>(orbis_text);
    char* utf8_text_ptr = utf8_text;

    std::size_t result = iconv(orbis_to_utf8, (char**)&orbis_text_ptr, &orbis_text_len_bytes, (char**)&utf8_text_ptr, &utf8_text_len_bytes);

    if (result == (std::size_t)-1) {
        return false;
    }

    *utf8_text_ptr = '\0'; // Null-terminate the string
    return true;
#else
    int required_size = WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t*>(orbis_text), orbis_text_len, nullptr, 0, nullptr, nullptr);
    if (required_size > utf8_text_len) {
        return false;
    }

    int converted_size = WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t*>(orbis_text), orbis_text_len, utf8_text, utf8_text_len, nullptr, nullptr);
    
    if (required_size == 0) {
        return false;
    }
    
    utf8_text[converted_size] = '\0';

    return true;
#endif
}

bool ImeDialogState::ConvertUTF8ToOrbis(const char* utf8_text, std::size_t utf8_text_len, char16_t* orbis_text, std::size_t orbis_text_len) {
#ifndef _WIN32
    std::size_t utf8_text_len_bytes = utf8_text_len * sizeof(char);
    std::size_t orbis_text_len_bytes = orbis_text_len * sizeof(char16_t);

    char* utf8_text_ptr = const_cast<char*>(utf8_text);
    char16_t* orbis_text_ptr = orbis_text;

    std::size_t result = iconv(utf8_to_orbis, (char**)&utf8_text_ptr, &utf8_text_len_bytes, (char**)&orbis_text_ptr, &orbis_text_len_bytes);

    if (result == (std::size_t)-1) {
        return false;
    }

    *orbis_text_ptr = u'\0'; // Null-terminate the string
    return true;
#else
    int required_size = MultiByteToWideChar(CP_UTF8, 0, utf8_text, utf8_text_len, nullptr, 0);
    if (required_size > orbis_text_len) {
        return false;
    }
    
    int converted_size = MultiByteToWideChar(CP_UTF8, 0, utf8_text, utf8_text_len, reinterpret_cast<wchar_t*>(orbis_text), orbis_text_len);

    if (required_size == 0) {
        return false;
    }

    orbis_text[converted_size] = u'\0';

    return true;
#endif
}

bool ImeDialogState::ConvertOrbisCharToUTF8(const char16_t orbis_char, char* utf8_char, std::size_t& utf8_char_len) {
    std::fill(utf8_char, utf8_char + 4, '\0');
#ifndef _WIN32
    std::size_t orbis_char_len_bytes = sizeof(char16_t);
    std::size_t utf8_char_len_bytes = utf8_char_len;

    char16_t orbis_char_ptr = orbis_char;
    char* utf8_char_ptr = utf8_char;

    std::size_t result = iconv(orbis_to_utf8, (char**)&orbis_char_ptr, &orbis_char_len_bytes, (char**)&utf8_char_ptr, &utf8_char_len_bytes);

    if (result == (std::size_t)-1) {
        utf8_char_len = 0;
        return false;
    }

    utf8_char_len = 4 - utf8_char_len_bytes;
    return true;
#else
    int required_size = WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t*>(&orbis_char), 1, nullptr, 0, nullptr, nullptr);
    if (required_size > 4) {
        UNREACHABLE_MSG("UTF-8 character is never more than 4 bytes");
    }

    *utf8_char_len = WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t*>(&orbis_char), 1, utf8_char, 4, nullptr, nullptr);

    return *utf8_char_len != 0;
#endif
}

bool ImeDialogState::ConvertUTF8CharToOrbis(const char* utf8_char, char16_t& orbis_char) {
#ifndef _WIN32
    std::size_t utf8_char_len_bytes = 4 * sizeof(char);
    std::size_t orbis_char_len_bytes = sizeof(char16_t);

    char* utf8_char_ptr = const_cast<char*>(utf8_char);
    char16_t* orbis_char_ptr = &orbis_char;

    std::size_t result = iconv(utf8_to_orbis, (char**)&utf8_char_ptr, &utf8_char_len_bytes, (char**)&orbis_char_ptr, &orbis_char_len_bytes);

    if (result == (std::size_t)-1) {
        return false;
    }

    return true;
#else
    int required_size = MultiByteToWideChar(CP_UTF8, 0, utf8_char, std::strlen(utf8_char), reinterpret_cast<wchar_t*>(&orbis_char), 1);
    return required_size != 0;
#endif
}

ImeDialogUi::ImeDialogUi(ImeDialogState* state, OrbisImeDialogStatus* status, OrbisImeDialogResult* result)
    : state(state), status(status), result(result) {
    
    if (state && *status == OrbisImeDialogStatus::RUNNING) {
        AddLayer(this);
    }
}

ImeDialogUi::~ImeDialogUi() {
    RemoveLayer(this);
}

ImeDialogUi::ImeDialogUi(ImeDialogUi&& other) noexcept
    : state(other.state), status(other.status), result(other.result) {
    
    if (state) std::scoped_lock lock(state->mutex);
    if (other.state) std::scoped_lock lock2(other.state->mutex);
    other.state = nullptr;
    other.status = nullptr;
    other.result = nullptr;

    if (state && *status == OrbisImeDialogStatus::RUNNING) {
        AddLayer(this);
    }
}

ImeDialogUi& ImeDialogUi::operator=(ImeDialogUi other) {
    if (state) std::scoped_lock lock(state->mutex);
    if (other.state) std::scoped_lock lock2(other.state->mutex);
    std::swap(state, other.state);
    std::swap(status, other.status);
    std::swap(result, other.result);

    if (state) {
        AddLayer(this);
    }

    return *this;
}

void ImeDialogUi::Draw() {
    if (!state) {
        return;
    }

    std::unique_lock lock{state->mutex};

    if (!status || *status != OrbisImeDialogStatus::RUNNING) {
        return;
    }

    const auto& ctx = *GetCurrentContext();
    const auto& io = ctx.IO;

    ImVec2 window_size;

    if (state->is_multiLine) {
        window_size = {400.0f, 200.0f};
    } else {
        window_size = {400.0f, 100.0f};
    }

    CentralizeWindow();
    SetNextWindowSize(window_size);
    SetNextWindowCollapsed(false);

    if (first_render || !io.NavActive) {
        SetNextWindowFocus();
    }

    first_render = false;

    if (Begin("IME Dialog#ImeDialog", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
        DrawPrettyBackground();
        Separator();

        if (state->title) {
            SetWindowFontScale(1.7f);
            TextUnformatted(state->title);
            SetWindowFontScale(1.0f);
            Separator();
        }

        if (state->is_multiLine) {
            DrawMultiLineInputText();
        } else {
            DrawInputText();
        }

        Separator();

        const char* button_text;

        switch (state->enter_label) {
        case OrbisImeEnterLabel::GO:
            button_text = "Go#ImeDialogOK";
            break;
        case OrbisImeEnterLabel::SEARCH:
            button_text = "Search#ImeDialogOK";
            break;
        case OrbisImeEnterLabel::SEND:
            button_text = "Send#ImeDialogOK";
            break;
        case OrbisImeEnterLabel::DEFAULT:
        default:
            button_text = "OK#ImeDialogOK";
            break;
        }

        if (Button(button_text, BUTTON_SIZE)) {
            *status = OrbisImeDialogStatus::FINISHED;
            result->endstatus = OrbisImeDialogEndStatus::OK;
        }

        if (Button("Cancel#ImeDialogCancel", BUTTON_SIZE)) {
            *status = OrbisImeDialogStatus::FINISHED;
            result->endstatus = OrbisImeDialogEndStatus::USER_CANCELED;

        }
    }
    End();
}

void ImeDialogUi::DrawInputText() {
    if (InputTextEx("##ImeDialogInput", state->placeholder, state->current_text, ORBIS_IME_DIALOG_MAX_TEXT_LENGTH, ImVec2(0, 0), ImGuiInputTextFlags_CallbackCharFilter, InputTextCallback, this)) {
        state->input_changed = true;
    }
}

void ImeDialogUi::DrawMultiLineInputText() {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
    if (InputTextEx("##ImeDialogInput", state->placeholder, state->current_text, ORBIS_IME_DIALOG_MAX_TEXT_LENGTH, ImVec2(380.0f, 100.0f), ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_Multiline, InputTextCallback, this)) {
        state->input_changed = true;
    }
#pragma clang diagnostic pop
}

int ImeDialogUi::InputTextCallback(ImGuiInputTextCallbackData* data) {
    ImeDialogUi* ui = static_cast<ImeDialogUi*>(data->UserData);

    ASSERT(ui);

    // ImGui encodes ImWchar32 as multi-byte UTF-8 characters
    char* event_char = reinterpret_cast<char*>(data->EventChar);

    // Call the keyboard filter
    OrbisImeKeycode src_keycode = {
        .keycode = 0,
        .character = 0,
        .status = 1, // ??? 1 = key pressed, 0 = key released
        .type = OrbisImeKeyboardType::ENGLISH_US, //TODO set this to the correct value (maybe use the current language?)
        .userId = ui->state->userId,
        .resourceId = 0,
        .timestamp = 0
    };
    
    if (!ui->state->ConvertUTF8CharToOrbis(event_char, src_keycode.character)) {
        LOG_ERROR(Lib_ImeDialog, "Failed to convert orbis char to utf8");
        return 0;
    }
    src_keycode.keycode = src_keycode.character; //TODO set this to the correct value

    u16 out_keycode;
    u32 out_status;

    ui->state->CallKeyboardFilter(&src_keycode, &out_keycode, &out_status);

    //TODO. set the keycode
    
    return 0;
}

} // namespace Libraries::ImeDialog