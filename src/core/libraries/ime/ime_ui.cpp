// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <vector>
#include "core/libraries/ime/ime_kb_layout.h"
#include "ime_ui.h"
#include "imgui/imgui_std.h"

namespace Libraries::Ime {

using namespace ImGui;

ImeState::ImeState(const OrbisImeParam* param, const OrbisImeParamExtended* extended) {
    if (!param) {
        LOG_ERROR(Lib_Ime, "Invalid IME parameters");
        return;
    }
    if (!param->work) {
        LOG_ERROR(Lib_Ime, "Invalid work buffer pointer");
        return;
    }
    if (!param->inputTextBuffer) {
        LOG_ERROR(Lib_Ime, "Invalid text buffer pointer");
        return;
    }
    work_buffer = param->work;
    text_buffer = param->inputTextBuffer;
    // Respect both the absolute IME limit and the caller-provided limit
    max_text_length = std::min(param->maxTextLength, ORBIS_IME_MAX_TEXT_LENGTH);

    if (extended) {
        LOG_INFO(Lib_Ime, "Extended IME parameters provided");
    }

    if (text_buffer) {
        const std::size_t text_len = std::char_traits<char16_t>::length(text_buffer);
        if (!ConvertOrbisToUTF8(text_buffer, text_len, current_text.begin(),
                                ORBIS_IME_MAX_TEXT_LENGTH * 4 + 1)) {
            LOG_ERROR(Lib_Ime, "Failed to convert text to utf8 encoding");
        }
    }
}

ImeState::ImeState(ImeState&& other) noexcept
    : work_buffer(other.work_buffer), text_buffer(other.text_buffer),
      max_text_length(other.max_text_length), current_text(std::move(other.current_text)),
      event_queue(std::move(other.event_queue)) {
    other.text_buffer = nullptr;
    other.max_text_length = 0;
}

ImeState& ImeState::operator=(ImeState&& other) noexcept {
    if (this != &other) {
        work_buffer = other.work_buffer;
        text_buffer = other.text_buffer;
        max_text_length = other.max_text_length;
        current_text = std::move(other.current_text);
        event_queue = std::move(other.event_queue);

        other.text_buffer = nullptr;
        other.max_text_length = 0;
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

    // Include current text payload for consumers expecting text with Enter
    OrbisImeEditText text{};
    text.str = reinterpret_cast<char16_t*>(work_buffer);
    // Sync work and input buffers with the latest UTF-8 text
    if (current_text.begin()) {
        ConvertUTF8ToOrbis(current_text.begin(), current_text.size(),
                           reinterpret_cast<char16_t*>(work_buffer), max_text_length + 1);
        if (text_buffer) {
            ConvertUTF8ToOrbis(current_text.begin(), current_text.size(), text_buffer,
                               max_text_length + 1);
        }
    }
    if (text.str) {
        const u32 len = static_cast<u32>(std::char_traits<char16_t>::length(text.str));
        // 0-based caret at end
        text.caret_index = len;
        text.area_num = 1;
        text.text_area[0].mode = OrbisImeTextAreaMode::Edit;
        // No edit happening on Enter: length=0; index can be caret
        text.text_area[0].index = len;
        text.text_area[0].length = 0;
        enterEvent.param.text = text;
    }

    LOG_DEBUG(Lib_Ime,
              "IME Event queued: PressEnter caret={} area_num={} edit.index={} edit.length={}",
              text.caret_index, text.area_num, text.text_area[0].index, text.text_area[0].length);
    SendEvent(&enterEvent);
}

void ImeState::SendCloseEvent() {
    OrbisImeEvent closeEvent{};
    closeEvent.id = OrbisImeEventId::PressClose;

    // Populate text payload with current buffer snapshot
    OrbisImeEditText text{};
    text.str = reinterpret_cast<char16_t*>(work_buffer);
    // Sync work and input buffers with the latest UTF-8 text
    if (current_text.begin()) {
        ConvertUTF8ToOrbis(current_text.begin(), current_text.size(),
                           reinterpret_cast<char16_t*>(work_buffer), max_text_length + 1);
        if (text_buffer) {
            ConvertUTF8ToOrbis(current_text.begin(), current_text.size(), text_buffer,
                               max_text_length + 1);
        }
    }
    if (text.str) {
        const u32 len = static_cast<u32>(std::char_traits<char16_t>::length(text.str));
        // 0-based caret at end
        text.caret_index = len;
        text.area_num = 1;
        text.text_area[0].mode = OrbisImeTextAreaMode::Edit;
        // No edit happening on Close: length=0; index can be caret
        text.text_area[0].index = len;
        text.text_area[0].length = 0;
        closeEvent.param.text = text;
    }

    LOG_DEBUG(Lib_Ime,
              "IME Event queued: PressClose caret={} area_num={} edit.index={} edit.length={}",
              text.caret_index, text.area_num, text.text_area[0].index, text.text_area[0].length);
    SendEvent(&closeEvent);
}

void ImeState::SetText(const char16_t* text, u32 length) {
    if (!text) {
        LOG_WARNING(Lib_Ime, "ImeState::SetText received null text pointer");
        return;
    }

    // Clamp to the effective maximum number of characters
    const u32 clamped_len = std::min(length, max_text_length) + 1;
    if (!ConvertOrbisToUTF8(text, clamped_len, current_text.begin(), current_text.capacity())) {
        LOG_ERROR(Lib_Ime, "ImeState::SetText failed to convert updated text to UTF-8");
        return;
    }
}
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
    const char* end = utf8_text ? (utf8_text + utf8_text_len) : nullptr;
    ImTextStrFromUtf8(reinterpret_cast<ImWchar*>(orbis_text), orbis_text_len, utf8_text, end);

    return true;
}

ImeUi::ImeUi(ImeState* state, const OrbisImeParam* param, const OrbisImeParamExtended* extended)
    : state(state), ime_param(param), extended_param(extended) {
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

    const bool use_over2k =
        (ime_param->option & OrbisImeOption::USE_OVER_2K_COORDINATES) != OrbisImeOption::DEFAULT;
    const auto viewport = Libraries::Ime::ComputeImeViewportMetrics(use_over2k);
    const float scale_x = viewport.scale_x;
    const float scale_y = viewport.scale_y;

    u32 panel_req_w = 0;
    u32 panel_req_h = 0;
    (void)sceImeGetPanelSize(ime_param, &panel_req_w, &panel_req_h);

    ImVec2 window_size{};
    if (panel_req_w > 0 && panel_req_h > 0) {
        window_size = {panel_req_w * scale_x, panel_req_h * scale_y};
    } else {
        window_size = {std::min(std::max(0.0f, viewport.size.x - 40.0f), 640.0f),
                       std::min(std::max(0.0f, viewport.size.y - 40.0f), 420.0f)};
        window_size.x = std::max(window_size.x, 320.0f);
        window_size.y = std::max(window_size.y, 240.0f);
    }

    float pos_x = viewport.offset.x + ime_param->posx * scale_x;
    float pos_y = viewport.offset.y + ime_param->posy * scale_y;
    if (ime_param->horizontal_alignment == OrbisImeHorizontalAlignment::Center) {
        pos_x -= window_size.x * 0.5f;
    } else if (ime_param->horizontal_alignment == OrbisImeHorizontalAlignment::Right) {
        pos_x -= window_size.x;
    }
    if (ime_param->vertical_alignment == OrbisImeVerticalAlignment::Center) {
        pos_y -= window_size.y * 0.5f;
    } else if (ime_param->vertical_alignment == OrbisImeVerticalAlignment::Bottom) {
        pos_y -= window_size.y;
    }
    pos_x = std::clamp(pos_x, viewport.offset.x,
                       viewport.offset.x + std::max(0.0f, viewport.size.x - window_size.x));
    pos_y = std::clamp(pos_y, viewport.offset.y,
                       viewport.offset.y + std::max(0.0f, viewport.size.y - window_size.y));

    SetNextWindowPos({pos_x, pos_y});
    SetNextWindowSize(window_size);
    SetNextWindowCollapsed(false);

    if (first_render || !io.NavActive) {
        SetNextWindowFocus();
    }

    if (Begin("IME##Ime", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
        DrawPrettyBackground();
        const Libraries::Ime::ImePanelMetricsConfig metrics_cfg{
            .panel_w = window_size.x,
            .panel_h = window_size.y,
            .multiline = True(ime_param->option & OrbisImeOption::MULTILINE),
            .show_title = false,
            .base_font_size = GetFontSize(),
            .window_pos = GetWindowPos(),
        };
        const Libraries::Ime::ImePanelMetrics metrics =
            Libraries::Ime::ComputeImePanelMetrics(metrics_cfg);

        SetWindowFontScale(std::max(viewport.ui_scale, metrics.input_font_scale));
        DrawInputText(metrics);

        auto* draw = GetWindowDrawList();
        const ImU32 pane_bg = IM_COL32(18, 18, 18, 255);
        const ImU32 pane_border = IM_COL32(70, 70, 70, 255);
        draw->AddRectFilled(metrics.predict_pos,
                            {metrics.predict_pos.x + metrics.predict_size.x,
                             metrics.predict_pos.y + metrics.predict_size.y},
                            pane_bg, metrics.corner_radius);
        draw->AddRect(metrics.predict_pos,
                      {metrics.predict_pos.x + metrics.predict_size.x,
                       metrics.predict_pos.y + metrics.predict_size.y},
                      pane_border, metrics.corner_radius);
        PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
        PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
        SetCursorScreenPos(metrics.close_pos);
        const bool cancel_pressed =
            Button("X##ImeClose", {metrics.close_size.x, metrics.close_size.y});
        PopStyleColor(3);

        SetCursorScreenPos(metrics.kb_pos);

        if (!accept_armed) {
            if (!IsKeyDown(ImGuiKey_Enter) && !IsKeyDown(ImGuiKey_GamepadFaceDown)) {
                accept_armed = true;
            }
        }
        const bool allow_key_accept = accept_armed;

        bool accept_pressed =
            (allow_key_accept && !metrics_cfg.multiline && IsKeyPressed(ImGuiKey_Enter)) ||
            (allow_key_accept && IsKeyPressed(ImGuiKey_GamepadFaceDown));

        Libraries::Ime::ImeKbGridLayout kb_layout{};
        kb_layout.pos = metrics.kb_pos;
        kb_layout.size = metrics.kb_size;
        kb_layout.key_gap_x = metrics.key_gap;
        kb_layout.key_gap_y = metrics.key_gap;
        kb_layout.key_h = metrics.key_h;
        kb_layout.cols = 10;
        kb_layout.rows = 6;
        kb_layout.corner_radius = metrics.corner_radius;

        Libraries::Ime::ImeKbDrawParams kb_params{};
        kb_params.enter_label = ime_param->enter_label;
        kb_params.key_bg_alt = IM_COL32(45, 45, 45, 255);

        Libraries::Ime::ImeKbDrawState kb_state{};
        SetWindowFontScale(metrics.key_font_scale);
        Libraries::Ime::DrawImeKeyboardGrid(kb_layout, kb_params, kb_state);
        SetWindowFontScale(metrics.input_font_scale);
        if (kb_state.done_pressed) {
            accept_pressed = true;
        }

        Dummy({metrics.kb_size.x, metrics.kb_size.y + metrics.padding_bottom});

        if (accept_pressed) {
            state->SendEnterEvent();
        } else if (cancel_pressed) {
            state->SendCloseEvent();
        }

        SetWindowFontScale(1.0f);
    }
    End();

    first_render = false;
}

void ImeUi::DrawInputText(const ImePanelMetrics& metrics) {
    const ImVec2 input_size = metrics.input_size;
    SetCursorPos(metrics.input_pos_local);
    if (first_render) {
        SetKeyboardFocusHere();
    }
    if (InputTextExLimited("##ImeInput", nullptr, state->current_text.begin(),
                           ime_param->maxTextLength * 4 + 1, input_size,
                           ImGuiInputTextFlags_CallbackAlways, ime_param->maxTextLength,
                           InputTextCallback, this)) {
    }
}

int ImeUi::InputTextCallback(ImGuiInputTextCallbackData* data) {
    ImeUi* ui = static_cast<ImeUi*>(data->UserData);
    ASSERT(ui);

    static std::string lastText;
    static int lastCaretPos = -1;
    std::string currentText(data->Buf, data->BufTextLen);
    if (currentText != lastText) {
        OrbisImeEditText eventParam{};
        eventParam.str = reinterpret_cast<char16_t*>(ui->ime_param->work);
        eventParam.area_num = 1;
        eventParam.text_area[0].mode = OrbisImeTextAreaMode::Edit;

        if (!ui->state->ConvertUTF8ToOrbis(data->Buf, data->BufTextLen, eventParam.str,
                                           ui->state->max_text_length + 1)) {
            LOG_ERROR(Lib_Ime, "Failed to convert UTF-8 to Orbis for eventParam.str");
            return 0;
        }

        if (!ui->state->ConvertUTF8ToOrbis(data->Buf, data->BufTextLen,
                                           ui->ime_param->inputTextBuffer,
                                           ui->state->max_text_length + 1)) {
            LOG_ERROR(Lib_Ime, "Failed to convert UTF-8 to Orbis for inputTextBuffer");
            return 0;
        }

        eventParam.caret_index = data->CursorPos;
        eventParam.text_area[0].index = data->CursorPos;
        eventParam.text_area[0].length =
            (data->CursorPos > lastCaretPos) ? 1 : -1; // data->CursorPos;

        OrbisImeEvent event{};
        event.id = OrbisImeEventId::UpdateText;
        event.param.text = eventParam;
        LOG_DEBUG(Lib_Ime,
                  "IME Event queued: UpdateText(type, "
                  "delete)\neventParam.caret_index={}\narea_num={}\neventParam.text_area[0].mode={}"
                  "\neventParam.text_area[0].index={}\neventParam.text_area[0].length={}",
                  eventParam.caret_index, eventParam.area_num,
                  static_cast<s32>(eventParam.text_area[0].mode), eventParam.text_area[0].index,
                  eventParam.text_area[0].length);

        lastText = currentText;
        lastCaretPos = -1;
        ui->state->SendEvent(&event);
    }

    if (lastCaretPos == -1) {
        lastCaretPos = data->CursorPos;
    } else if (data->CursorPos != lastCaretPos) {
        const int delta = data->CursorPos - lastCaretPos;

        // Emit one UpdateCaret per delta step (delta may be ±1 or a jump)
        const bool move_right = delta > 0;
        const u32 steps = static_cast<u32>(std::abs(delta));
        OrbisImeCaretMovementDirection dir = move_right ? OrbisImeCaretMovementDirection::Right
                                                        : OrbisImeCaretMovementDirection::Left;

        for (u32 i = 0; i < steps; ++i) {
            OrbisImeEvent caret_step{};
            caret_step.id = OrbisImeEventId::UpdateCaret;
            caret_step.param.caret_move = dir;
            LOG_DEBUG(Lib_Ime, "IME Event queued: UpdateCaret(step {}/{}), dir={}", i + 1, steps,
                      static_cast<u32>(dir));
            ui->state->SendEvent(&caret_step);
        }

        lastCaretPos = data->CursorPos;
    }

    return 0;
}

void ImeUi::Free() {
    RemoveLayer(this);
}

}; // namespace Libraries::Ime
