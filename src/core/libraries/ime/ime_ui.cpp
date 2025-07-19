// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ime_ui.h"
#include "imgui/imgui_std.h"

namespace Libraries::Ime {

using namespace ImGui;

static constexpr ImVec2 BUTTON_SIZE{100.0f, 30.0f};

ImeState::ImeState(const OrbisImeParam* param) {
    if (!param) {
        return;
    }

    work_buffer = param->work;
    text_buffer = param->inputTextBuffer;

    std::size_t text_len = std::char_traits<char16_t>::length(text_buffer);
    if (!ConvertOrbisToUTF8(text_buffer, text_len, current_text.begin(),
                            ORBIS_IME_MAX_TEXT_LENGTH * 4)) {
        LOG_ERROR(Lib_ImeDialog, "Failed to convert text to utf8 encoding");
    }
}

ImeState::ImeState(ImeState&& other) noexcept
    : work_buffer(other.work_buffer), text_buffer(other.text_buffer),
      current_text(std::move(other.current_text)), event_queue(std::move(other.event_queue)) {
    other.text_buffer = nullptr;
}

ImeState& ImeState::operator=(ImeState&& other) noexcept {
    if (this != &other) {
        work_buffer = other.work_buffer;
        text_buffer = other.text_buffer;
        current_text = std::move(other.current_text);
        event_queue = std::move(other.event_queue);

        other.text_buffer = nullptr;
    }
    return *this;
}

void ImeState::SendEvent(OrbisImeEvent* event) {
    std::unique_lock<std::mutex> lock{queue_mutex};
    event_queue.push(*event);
}

void ImeState::SendEnterEvent() {
    OrbisImeEvent enterEvent{};
    enterEvent.id = OrbisImeEventId::PressEnter;
    SendEvent(&enterEvent);
}

void ImeState::SendCloseEvent() {
    OrbisImeEvent closeEvent{};
    closeEvent.id = OrbisImeEventId::PressClose;
    closeEvent.param.text.str = reinterpret_cast<char16_t*>(work_buffer);
    SendEvent(&closeEvent);
}

void ImeState::SetText(const char16_t* text, u32 length) {}

void ImeState::SetCaret(u32 position) {}

bool ImeState::ConvertOrbisToUTF8(const char16_t* orbis_text, std::size_t orbis_text_len,
                                  char* utf8_text, std::size_t utf8_text_len) {
    std::fill(utf8_text, utf8_text + utf8_text_len, '\0');
    const ImWchar* orbis_text_ptr = reinterpret_cast<const ImWchar*>(orbis_text);
    ImTextStrToUtf8(utf8_text, utf8_text_len, orbis_text_ptr, orbis_text_ptr + orbis_text_len);

    return true;
}

bool ImeState::ConvertUTF8ToOrbis(const char* utf8_text, std::size_t utf8_text_len,
                                  char16_t* orbis_text, std::size_t orbis_text_len) {
    std::fill(orbis_text, orbis_text + orbis_text_len, u'\0');
    ImTextStrFromUtf8(reinterpret_cast<ImWchar*>(orbis_text), orbis_text_len, utf8_text, nullptr);

    return true;
}

ImeUi::ImeUi(ImeState* state, const OrbisImeParam* param) : state(state), ime_param(param) {
    if (param) {
        AddLayer(this);
    }
}

ImeUi::~ImeUi() {
    std::scoped_lock lock(draw_mutex);
    Free();
}

ImeUi& ImeUi::operator=(ImeUi&& other) {
    std::scoped_lock lock(draw_mutex, other.draw_mutex);
    Free();

    state = other.state;
    ime_param = other.ime_param;
    first_render = other.first_render;
    other.state = nullptr;
    other.ime_param = nullptr;

    AddLayer(this);
    return *this;
}

void ImeUi::Draw() {
    std::unique_lock<std::mutex> lock{draw_mutex};

    if (!state) {
        return;
    }

    const auto& ctx = *GetCurrentContext();
    const auto& io = ctx.IO;

    // TODO: Figure out how to properly translate the positions -
    //       for example, if a game wants to center the IME panel,
    //       we have to translate the panel position in a way that it
    //       still becomes centered, as the game normally calculates
    //       the position assuming a it's running on a 1920x1080 screen,
    //       whereas we are running on a 1280x720 window size (by default).
    //
    //       e.g. Panel position calculation from a game:
    //       param.posx = (1920 / 2) - (panelWidth  / 2);
    //       param.posy = (1080 / 2) - (panelHeight / 2);
    const auto size = GetIO().DisplaySize;
    f32 pos_x = (ime_param->posx / 1920.0f * (float)size.x);
    f32 pos_y = (ime_param->posy / 1080.0f * (float)size.y);

    ImVec2 window_pos = {pos_x, pos_y};
    ImVec2 window_size = {500.0f, 100.0f};

    // SetNextWindowPos(window_pos);
    SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                     ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    SetNextWindowSize(window_size);
    SetNextWindowCollapsed(false);

    if (first_render || !io.NavActive) {
        SetNextWindowFocus();
    }

    if (Begin("IME##Ime", nullptr,
              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                  ImGuiWindowFlags_NoSavedSettings)) {
        DrawPrettyBackground();

        DrawInputText();
        SetCursorPosY(GetCursorPosY() + 10.0f);

        const char* button_text;
        button_text = "Done##ImeDone";

        float button_spacing = 10.0f;
        float total_button_width = BUTTON_SIZE.x * 2 + button_spacing;
        float button_start_pos = (window_size.x - total_button_width) / 2.0f;

        SetCursorPosX(button_start_pos);

        if (Button(button_text, BUTTON_SIZE) || (IsKeyPressed(ImGuiKey_Enter))) {
            state->SendEnterEvent();
        }

        SameLine(0.0f, button_spacing);

        if (Button("Close##ImeClose", BUTTON_SIZE)) {
            state->SendCloseEvent();
        }
    }
    End();

    first_render = false;
}

void ImeUi::DrawInputText() {
    ImVec2 input_size = {GetWindowWidth() - 40.0f, 0.0f};
    SetCursorPosX(20.0f);
    if (first_render) {
        SetKeyboardFocusHere();
    }
    if (InputTextEx("##ImeInput", nullptr, state->current_text.begin(), ime_param->maxTextLength,
                    input_size, ImGuiInputTextFlags_CallbackAlways, InputTextCallback, this)) {
    }
}

int ImeUi::InputTextCallback(ImGuiInputTextCallbackData* data) {
    ImeUi* ui = static_cast<ImeUi*>(data->UserData);
    ASSERT(ui);

    static std::string lastText;
    std::string currentText(data->Buf, data->BufTextLen);
    if (currentText != lastText) {
        OrbisImeEditText eventParam{};
        eventParam.str = reinterpret_cast<char16_t*>(ui->ime_param->work);
        eventParam.caret_index = data->CursorPos;
        eventParam.area_num = 1;

        eventParam.text_area[0].mode = OrbisImeTextAreaMode::Edit;
        eventParam.text_area[0].index = data->CursorPos;
        eventParam.text_area[0].length = data->BufTextLen;

        if (!ui->state->ConvertUTF8ToOrbis(data->Buf, data->BufTextLen, eventParam.str,
                                           ui->ime_param->maxTextLength)) {
            LOG_ERROR(Lib_ImeDialog, "Failed to convert Orbis char to UTF-8");
            return 0;
        }

        if (!ui->state->ConvertUTF8ToOrbis(data->Buf, data->BufTextLen,
                                           ui->ime_param->inputTextBuffer,
                                           ui->ime_param->maxTextLength)) {
            LOG_ERROR(Lib_ImeDialog, "Failed to convert Orbis char to UTF-8");
            return 0;
        }

        OrbisImeEvent event{};
        event.id = OrbisImeEventId::UpdateText;
        event.param.text = eventParam;

        lastText = currentText;
        ui->state->SendEvent(&event);
    }

    static int lastCaretPos = -1;
    if (lastCaretPos == -1) {
        lastCaretPos = data->CursorPos;
    } else if (data->CursorPos != lastCaretPos) {
        OrbisImeCaretMovementDirection caretDirection = OrbisImeCaretMovementDirection::Still;
        if (data->CursorPos < lastCaretPos) {
            caretDirection = OrbisImeCaretMovementDirection::Left;
        } else if (data->CursorPos > lastCaretPos) {
            caretDirection = OrbisImeCaretMovementDirection::Right;
        }

        OrbisImeEvent event{};
        event.id = OrbisImeEventId::UpdateCaret;
        event.param.caret_move = caretDirection;

        lastCaretPos = data->CursorPos;
        ui->state->SendEvent(&event);
    }

    return 0;
}

void ImeUi::Free() {
    RemoveLayer(this);
}

}; // namespace Libraries::Ime
