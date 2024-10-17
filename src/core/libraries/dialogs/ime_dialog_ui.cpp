// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cwchar>
#include <string>
#include <imgui.h>
#include <magic_enum.hpp>

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/dialogs/ime_dialog.h"
#include "core/libraries/dialogs/ime_dialog_ui.h"
#include "core/linker.h"
#include "imgui/imgui_std.h"

using namespace ImGui;

static constexpr ImVec2 BUTTON_SIZE{100.0f, 30.0f};

namespace Libraries::ImeDialog {

ImeDialogState::ImeDialogState(const OrbisImeDialogParam* param,
                               const OrbisImeParamExtended* extended) {
    if (!param)
        return;

    userId = param->userId;
    is_multiLine = True(param->option & OrbisImeDialogOption::MULTILINE);
    is_numeric = param->type == OrbisImeType::NUMBER;
    type = param->type;
    enter_label = param->enterLabel;
    text_filter = param->filter;
    keyboard_filter = extended ? extended->extKeyboardFilter : nullptr;
    max_text_length = param->maxTextLength;
    text_buffer = param->inputTextBuffer;

    if (param->title) {
        std::size_t title_len = std::char_traits<char16_t>::length(param->title);
        title.resize(title_len * 4 + 1);
        title[title_len * 4] = '\0';

        if (!ConvertOrbisToUTF8(param->title, title_len, &title[0], title_len * 4)) {
            LOG_ERROR(Lib_ImeDialog, "Failed to convert title to utf8 encoding");
        }
    }

    if (param->placeholder) {
        std::size_t placeholder_len = std::char_traits<char16_t>::length(param->placeholder);
        placeholder.resize(placeholder_len * 4 + 1);
        placeholder[placeholder_len * 4] = '\0';

        if (!ConvertOrbisToUTF8(param->placeholder, placeholder_len, &placeholder[0],
                                placeholder_len * 4)) {
            LOG_ERROR(Lib_ImeDialog, "Failed to convert placeholder to utf8 encoding");
        }
    }

    std::size_t text_len = std::char_traits<char16_t>::length(text_buffer);
    if (!ConvertOrbisToUTF8(text_buffer, text_len, current_text.begin(),
                            ORBIS_IME_DIALOG_MAX_TEXT_LENGTH * 4)) {
        LOG_ERROR(Lib_ImeDialog, "Failed to convert text to utf8 encoding");
    }
}

ImeDialogState::ImeDialogState(ImeDialogState&& other) noexcept
    : input_changed(other.input_changed), userId(other.userId), is_multiLine(other.is_multiLine),
      is_numeric(other.is_numeric), type(other.type), enter_label(other.enter_label),
      text_filter(other.text_filter), keyboard_filter(other.keyboard_filter),
      max_text_length(other.max_text_length), text_buffer(other.text_buffer),
      title(std::move(other.title)), placeholder(std::move(other.placeholder)),
      current_text(other.current_text) {

    other.text_buffer = nullptr;
}

ImeDialogState& ImeDialogState::operator=(ImeDialogState&& other) {
    if (this != &other) {
        input_changed = other.input_changed;
        userId = other.userId;
        is_multiLine = other.is_multiLine;
        is_numeric = other.is_numeric;
        type = other.type;
        enter_label = other.enter_label;
        text_filter = other.text_filter;
        keyboard_filter = other.keyboard_filter;
        max_text_length = other.max_text_length;
        text_buffer = other.text_buffer;
        title = std::move(other.title);
        placeholder = std::move(other.placeholder);
        current_text = other.current_text;

        other.text_buffer = nullptr;
    }

    return *this;
}

bool ImeDialogState::CopyTextToOrbisBuffer() {
    if (!text_buffer) {
        return false;
    }

    return ConvertUTF8ToOrbis(current_text.begin(), current_text.capacity(), text_buffer,
                              max_text_length);
}

bool ImeDialogState::CallTextFilter() {
    if (!text_filter || !input_changed) {
        return true;
    }

    input_changed = false;

    char16_t src_text[ORBIS_IME_DIALOG_MAX_TEXT_LENGTH + 1] = {0};
    u32 src_text_length = current_text.size();
    char16_t out_text[ORBIS_IME_DIALOG_MAX_TEXT_LENGTH + 1] = {0};
    u32 out_text_length = ORBIS_IME_DIALOG_MAX_TEXT_LENGTH;

    if (!ConvertUTF8ToOrbis(current_text.begin(), src_text_length, src_text,
                            ORBIS_IME_DIALOG_MAX_TEXT_LENGTH)) {
        LOG_ERROR(Lib_ImeDialog, "Failed to convert text to orbis encoding");
        return false;
    }

    auto* linker = Common::Singleton<Core::Linker>::Instance();
    int ret =
        linker->ExecuteGuest(text_filter, out_text, &out_text_length, src_text, src_text_length);

    if (ret != 0) {
        return false;
    }

    if (!ConvertOrbisToUTF8(out_text, out_text_length, current_text.begin(),
                            ORBIS_IME_DIALOG_MAX_TEXT_LENGTH * 4)) {
        LOG_ERROR(Lib_ImeDialog, "Failed to convert text to utf8 encoding");
        return false;
    }

    return true;
}

bool ImeDialogState::CallKeyboardFilter(const OrbisImeKeycode* src_keycode, u16* out_keycode,
                                        u32* out_status) {
    if (!keyboard_filter) {
        return true;
    }

    auto* linker = Common::Singleton<Core::Linker>::Instance();
    int ret = linker->ExecuteGuest(keyboard_filter, src_keycode, out_keycode, out_status, nullptr);

    return ret == 0;
}

bool ImeDialogState::ConvertOrbisToUTF8(const char16_t* orbis_text, std::size_t orbis_text_len,
                                        char* utf8_text, std::size_t utf8_text_len) {

    std::fill(utf8_text, utf8_text + utf8_text_len, '\0');
    const ImWchar* orbis_text_ptr = reinterpret_cast<const ImWchar*>(orbis_text);
    ImTextStrToUtf8(utf8_text, utf8_text_len, orbis_text_ptr, orbis_text_ptr + orbis_text_len);

    return true;
}

bool ImeDialogState::ConvertUTF8ToOrbis(const char* utf8_text, std::size_t utf8_text_len,
                                        char16_t* orbis_text, std::size_t orbis_text_len) {

    std::fill(orbis_text, orbis_text + orbis_text_len, u'\0');
    ImTextStrFromUtf8(reinterpret_cast<ImWchar*>(orbis_text), orbis_text_len, utf8_text, nullptr);

    return true;
}

ImeDialogUi::ImeDialogUi(ImeDialogState* state, OrbisImeDialogStatus* status,
                         OrbisImeDialogResult* result)
    : state(state), status(status), result(result) {

    if (state && *status == OrbisImeDialogStatus::RUNNING) {
        AddLayer(this);
    }
}

ImeDialogUi::~ImeDialogUi() {
    std::scoped_lock lock(draw_mutex);

    Free();
}

ImeDialogUi::ImeDialogUi(ImeDialogUi&& other) noexcept
    : state(other.state), status(other.status), result(other.result),
      first_render(other.first_render) {

    std::scoped_lock lock(draw_mutex, other.draw_mutex);
    other.state = nullptr;
    other.status = nullptr;
    other.result = nullptr;

    if (state && *status == OrbisImeDialogStatus::RUNNING) {
        AddLayer(this);
    }
}

ImeDialogUi& ImeDialogUi::operator=(ImeDialogUi&& other) {
    std::scoped_lock lock(draw_mutex, other.draw_mutex);
    Free();

    state = other.state;
    status = other.status;
    result = other.result;
    first_render = other.first_render;
    other.state = nullptr;
    other.status = nullptr;
    other.result = nullptr;

    if (state && *status == OrbisImeDialogStatus::RUNNING) {
        AddLayer(this);
    }

    return *this;
}

void ImeDialogUi::Free() {
    RemoveLayer(this);
}

void ImeDialogUi::Draw() {
    std::unique_lock lock{draw_mutex};

    if (!state) {
        return;
    }

    if (!status || *status != OrbisImeDialogStatus::RUNNING) {
        return;
    }

    const auto& ctx = *GetCurrentContext();
    const auto& io = ctx.IO;

    ImVec2 window_size;

    if (state->is_multiLine) {
        window_size = {500.0f, 300.0f};
    } else {
        window_size = {500.0f, 150.0f};
    }

    CentralizeWindow();
    SetNextWindowSize(window_size);
    SetNextWindowCollapsed(false);

    if (first_render || !io.NavActive) {
        SetNextWindowFocus();
    }

    if (Begin("IME Dialog##ImeDialog", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
        DrawPrettyBackground();

        if (!state->title.empty()) {
            SetWindowFontScale(1.7f);
            TextUnformatted(state->title.data());
            SetWindowFontScale(1.0f);
        }

        if (state->is_multiLine) {
            DrawMultiLineInputText();
        } else {
            DrawInputText();
        }

        SetCursorPosY(GetCursorPosY() + 10.0f);

        const char* button_text;

        switch (state->enter_label) {
        case OrbisImeEnterLabel::GO:
            button_text = "Go##ImeDialogOK";
            break;
        case OrbisImeEnterLabel::SEARCH:
            button_text = "Search##ImeDialogOK";
            break;
        case OrbisImeEnterLabel::SEND:
            button_text = "Send##ImeDialogOK";
            break;
        case OrbisImeEnterLabel::DEFAULT:
        default:
            button_text = "OK##ImeDialogOK";
            break;
        }

        float button_spacing = 10.0f;
        float total_button_width = BUTTON_SIZE.x * 2 + button_spacing;
        float button_start_pos = (window_size.x - total_button_width) / 2.0f;

        SetCursorPosX(button_start_pos);

        if (Button(button_text, BUTTON_SIZE) ||
            (!state->is_multiLine && IsKeyPressed(ImGuiKey_Enter))) {
            *status = OrbisImeDialogStatus::FINISHED;
            result->endstatus = OrbisImeDialogEndStatus::OK;
        }

        SameLine(0.0f, button_spacing);

        if (Button("Cancel##ImeDialogCancel", BUTTON_SIZE)) {
            *status = OrbisImeDialogStatus::FINISHED;
            result->endstatus = OrbisImeDialogEndStatus::USER_CANCELED;
        }
    }
    End();

    first_render = false;
}

void ImeDialogUi::DrawInputText() {
    ImVec2 input_size = {GetWindowWidth() - 40.0f, 0.0f};
    SetCursorPosX(20.0f);
    if (first_render) {
        SetKeyboardFocusHere();
    }
    const char* placeholder = state->placeholder.empty() ? nullptr : state->placeholder.data();
    if (InputTextEx("##ImeDialogInput", placeholder, state->current_text.begin(),
                    state->max_text_length, input_size, ImGuiInputTextFlags_CallbackCharFilter,
                    InputTextCallback, this)) {
        state->input_changed = true;
    }
}

void ImeDialogUi::DrawMultiLineInputText() {
    ImVec2 input_size = {GetWindowWidth() - 40.0f, 200.0f};
    SetCursorPosX(20.0f);
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackCharFilter |
                                static_cast<ImGuiInputTextFlags>(ImGuiInputTextFlags_Multiline);
    if (first_render) {
        SetKeyboardFocusHere();
    }
    const char* placeholder = state->placeholder.empty() ? nullptr : state->placeholder.data();
    if (InputTextEx("##ImeDialogInput", placeholder, state->current_text.begin(),
                    state->max_text_length, input_size, flags, InputTextCallback, this)) {
        state->input_changed = true;
    }
}

int ImeDialogUi::InputTextCallback(ImGuiInputTextCallbackData* data) {
    ImeDialogUi* ui = static_cast<ImeDialogUi*>(data->UserData);

    ASSERT(ui);

    // Should we filter punctuation?
    if (ui->state->is_numeric && (data->EventChar < '0' || data->EventChar > '9') &&
        data->EventChar != '\b' && data->EventChar != ',' && data->EventChar != '.') {
        return 1;
    }

    if (!ui->state->keyboard_filter) {
        return 0;
    }

    // ImGui encodes ImWchar32 as multi-byte UTF-8 characters
    char* event_char = reinterpret_cast<char*>(&data->EventChar);

    // Call the keyboard filter
    OrbisImeKeycode src_keycode = {
        .keycode = 0,
        .character = 0,
        .status = 1,                              // ??? 1 = key pressed, 0 = key released
        .type = OrbisImeKeyboardType::ENGLISH_US, // TODO set this to the correct value (maybe use
                                                  // the current language?)
        .userId = ui->state->userId,
        .resourceId = 0,
        .timestamp = 0};

    if (!ui->state->ConvertUTF8ToOrbis(event_char, 4, &src_keycode.character, 1)) {
        LOG_ERROR(Lib_ImeDialog, "Failed to convert orbis char to utf8");
        return 0;
    }
    src_keycode.keycode = src_keycode.character; // TODO set this to the correct value

    u16 out_keycode;
    u32 out_status;

    ui->state->CallKeyboardFilter(&src_keycode, &out_keycode, &out_status);

    // TODO. set the keycode

    return 0;
}

} // namespace Libraries::ImeDialog