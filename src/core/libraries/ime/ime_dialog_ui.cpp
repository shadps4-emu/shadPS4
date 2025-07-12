// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cwchar>
#include <string>
#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/ime/ime_dialog.h"
#include "core/libraries/ime/ime_dialog_ui.h"
#include "core/tls.h"
#include "imgui/imgui_std.h"

#include "core/libraries/ime/ime_keyboard_layouts.h" // c16rtomb, layout tables
#include "core/libraries/ime/ime_keyboard_ui.h"      // DrawVirtualKeyboard, Utf8SafeBackspace

using namespace ImGui;

/* small helper for the OK/Cancel buttons */
static constexpr ImVec2 BUTTON_SIZE{100.0f, 30.0f};

/* convert palette colour from Orbis struct to ImGui format */
static ImU32 ConvertColor(const OrbisImeColor& c) {
    return IM_COL32(c.r, c.g, c.b, c.a);
}

/*─────────────────────────────────────────────────────────────*
 *  Libraries::ImeDialog implementation
 *─────────────────────────────────────────────────────────────*/
namespace Libraries::ImeDialog {

/* ----------------------------------------------------------
 *  class‑static pointer – single definition
 * ----------------------------------------------------------*/
ImeDialogUi* ImeDialogUi::g_activeImeDialogUi = nullptr;

/* ----------------------------------------------------------
 *  keyboard‑to‑dialog event bridge
 * ----------------------------------------------------------*/
static void KeyboardCallbackBridge(const VirtualKeyEvent* evt) {
    if (ImeDialogUi::g_activeImeDialogUi && evt)
        ImeDialogUi::g_activeImeDialogUi->OnVirtualKeyEvent(evt);
}

/*─────────────────────────────────────────────────────────────*
 *  ImeDialogState : constructors, helpers
 *─────────────────────────────────────────────────────────────*/
ImeDialogState::ImeDialogState(const OrbisImeDialogParam* param,
                               const OrbisImeParamExtended* extended) {
    LOG_INFO(Lib_ImeDialog, ">> ImeDialogState::Ctor: param={}, text_buffer={}",
             static_cast<const void*>(param),
             static_cast<void*>(param ? param->input_text_buffer : nullptr));
    if (!param) {
        LOG_ERROR(Lib_ImeDialog, "   param==nullptr, returning without init");
        return;
    }

    /* basic param copy */
    user_id = param->user_id;
    is_multi_line = True(param->option & OrbisImeOption::MULTILINE);
    is_numeric = param->type == OrbisImeType::Number;
    type = param->type;
    enter_label = param->enter_label;
    text_filter = param->filter;
    keyboard_filter = extended ? extended->ext_keyboard_filter : nullptr;
    max_text_length = param->max_text_length;
    text_buffer = param->input_text_buffer;

    /* UTF‑16 → UTF‑8 conversions */
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
    : input_changed(other.input_changed), user_id(other.user_id),
      is_multi_line(other.is_multi_line), is_numeric(other.is_numeric), type(other.type),
      enter_label(other.enter_label), text_filter(other.text_filter),
      keyboard_filter(other.keyboard_filter), max_text_length(other.max_text_length),
      text_buffer(other.text_buffer), title(std::move(other.title)),
      placeholder(std::move(other.placeholder)), current_text(other.current_text) {

    other.text_buffer = nullptr;
}

ImeDialogState& ImeDialogState::operator=(ImeDialogState&& other) {
    if (this != &other) {
        input_changed = other.input_changed;
        user_id = other.user_id;
        is_multi_line = other.is_multi_line;
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

    int ret =
        Core::ExecuteGuest(text_filter, out_text, &out_text_length, src_text, src_text_length);

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

    int ret = Core::ExecuteGuest(keyboard_filter, src_keycode, out_keycode, out_status, nullptr);
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

/*─────────────────────────────────────────────────────────────*
 *  ImeDialogUi : constructor / destructor / move
 *─────────────────────────────────────────────────────────────*/
ImeDialogUi::ImeDialogUi(ImeDialogState* state, OrbisImeDialogStatus* status,
                         OrbisImeDialogResult* result)
    : state(state), status(status), result(result) {
    const OrbisImeParamExtended* incoming = state ? state->GetExtendedParam() : nullptr;
    if (incoming) {
        // copy caller’s palette
        ext_ = *incoming;
    } else {
        // zero-init and then overwrite the color fields you need
        std::memset(&ext_, 0, sizeof(ext_));
        ext_.color_base = {19, 19, 21, 240};
        ext_.color_line = {255, 255, 255, 255};
        ext_.color_text_field = {26, 26, 28, 240};
        ext_.color_preedit = {0, 0, 0, 255};
        ext_.color_button_default = {45, 45, 45, 255};
        ext_.color_button_function = {72, 72, 74, 255};
        ext_.color_button_symbol = {96, 96, 98, 255};
        ext_.color_text = {255, 255, 255, 255};
        ext_.color_special = {0, 123, 200, 255};
        ext_.priority = OrbisImePanelPriority::Default;
    }

    // For text, lines, etc.
    kb_style.color_text = ConvertColor(ext_.color_text);
    kb_style.color_line = ConvertColor(ext_.color_line);

    // Button colors
    kb_style.color_button_default = ConvertColor(ext_.color_button_default);
    kb_style.color_button_symbol = ConvertColor(ext_.color_button_symbol);
    kb_style.color_button_function = ConvertColor(ext_.color_button_function);
    kb_style.color_special = ConvertColor(ext_.color_special);

    if (state && *status == OrbisImeDialogStatus::Running) {
        AddLayer(this);
        ImeDialogUi::g_activeImeDialogUi = this;
    }
}

ImeDialogUi::~ImeDialogUi() {
    std::scoped_lock lock(draw_mutex);

    Free();

    if (ImeDialogUi::g_activeImeDialogUi == this) {
        ImeDialogUi::g_activeImeDialogUi = nullptr;
    }
}

ImeDialogUi::ImeDialogUi(ImeDialogUi&& other) noexcept
    : state(other.state), status(other.status), result(other.result),
      first_render(other.first_render) {

    std::scoped_lock lock(draw_mutex, other.draw_mutex);
    other.state = nullptr;
    other.status = nullptr;
    other.result = nullptr;

    if (state && *status == OrbisImeDialogStatus::Running) {
        AddLayer(this);
        ImeDialogUi::g_activeImeDialogUi = this;
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

    if (state && *status == OrbisImeDialogStatus::Running) {
        AddLayer(this);
        ImeDialogUi::g_activeImeDialogUi = this;
    }

    return *this;
}

void ImeDialogUi::Free() {
    RemoveLayer(this);
}

/*─────────────────────────────────────────────────────────────*
 *  ImeDialogUi : main ImGui draw routine
 *─────────────────────────────────────────────────────────────*/
void ImeDialogUi::Draw() {
    std::unique_lock lock{draw_mutex};
    LOG_INFO(Lib_ImeDialog, ">> ImeDialogUi::Draw: first_render=%d", first_render);

    if (!state) {
        return;
    }

    if (!status || *status != OrbisImeDialogStatus::Running) {
        return;
    }
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ConvertColor(ext_.color_base));
    const auto& ctx = *GetCurrentContext();
    const auto& io = ctx.IO;

    ImVec2 window_size;

    if (state->is_multi_line) {
        window_size = {500.0f, 500.0f};
    } else {
        window_size = {500.0f, 350.0f};
    }

    CentralizeNextWindow();
    SetNextWindowSize(window_size);
    SetNextWindowCollapsed(false);

    if (first_render || !io.NavActive) {
        SetNextWindowFocus();
    }

    if (Begin("IME Dialog##ImeDialog", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
        // DrawPrettyBackground();

        /* ---------- title ---------- */
        DrawTitle();

        /* ---------- input box ---------- */
        if (state->is_multi_line) {
            LOG_INFO(Lib_ImeDialog, "   Drawing multi-line widget…");
            DrawMultiLineInputText();
            LOG_INFO(Lib_ImeDialog, "   Done DrawMultiLineInputText");
        } else {
            LOG_INFO(Lib_ImeDialog, "   Drawing input text widget…");
            DrawInputText();
            LOG_INFO(Lib_ImeDialog, "   Done DrawInputText");
        }

        /* ---------- dummy prediction bar with Cancel button ---------- */
        DrawPredictionBarAnCancelButton();

        /* ---------- on‑screen keyboard ---------- */
        DrawVirtualKeyboardSection();

        /* ---------- OK / Cancel buttons ---------- */
        /*
        DrawOkAndCancelButtons();
        */

        End();
    }

    ImGui::PopStyleColor();

    first_render = false;
    LOG_INFO(Lib_ImeDialog, "<< ImeDialogUi::Draw complete");
}

/*─────────────────────────────────────────────────────────────*
 *  helper draw functions (unchanged)
 *─────────────────────────────────────────────────────────────*/
void ImeDialogUi::DrawInputText() {
    ImGui::BeginGroup();
    // ─── Apply ext_ colors ───────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_NavHighlight, ConvertColor(ext_.color_line));

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ConvertColor(ext_.color_text_field)); // background
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ConvertColor(ext_.color_text_field));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ConvertColor(ext_.color_text_field));
    ImGui::PushStyleColor(ImGuiCol_Border, ConvertColor(ext_.color_line)); // border line
    ImGui::PushStyleColor(ImGuiCol_Text, ConvertColor(ext_.color_text));   // typed text
    // ─────────────────────────────────────────────────────────────────────────
    ImVec2 input_size = {GetWindowWidth() - 40.0f, 0.0f};
    SetCursorPosX(20.0f);
    if (first_render) {
        SetKeyboardFocusHere();
    }

    const char* placeholder = state->placeholder.empty() ? nullptr : state->placeholder.data();
    if (InputTextEx("##ImeDialogInput", placeholder, state->current_text.begin(),
                    state->max_text_length + 1, input_size, ImGuiInputTextFlags_CallbackCharFilter,
                    InputTextCallback, this)) {
        state->input_changed = true;
    }

    // CARET: manually render even if not focused
    if (!ImGui::IsItemActive()) {
        // Calculate input field position
        ImVec2 input_pos = ImGui::GetItemRectMin();

        // Find where to draw the caret
        DrawCaretForInputText(state->current_text.begin(), state->caret_index, input_pos);
    }

    // ────── replicate keyboard’s hover→nav focus highlight ──────
    if (ImGui::IsItemHovered()) {
        ImGui::SetItemCurrentNavFocus();
        ImGui::KeepNavHighlight();
    }

    // ────── pop ALL style colors (5 for input + 1 for NavHighlight) ──────
    ImGui::PopStyleColor(6);
    ImGui::EndGroup();
}

void ImeDialogUi::DrawMultiLineInputText() {
    ImGui::BeginGroup();
    // ─── Apply the same ext_ colors ───────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_NavHighlight, ConvertColor(ext_.color_line));

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ConvertColor(ext_.color_text_field));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ConvertColor(ext_.color_text_field));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ConvertColor(ext_.color_text_field));
    ImGui::PushStyleColor(ImGuiCol_Border, ConvertColor(ext_.color_line));
    ImGui::PushStyleColor(ImGuiCol_Text, ConvertColor(ext_.color_text));
    // ─────────────────────────────────────────────────────────────────────────
    ImVec2 input_size = {GetWindowWidth() - 40.0f, 200.0f};
    SetCursorPosX(20.0f);
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackCharFilter |
                                static_cast<ImGuiInputTextFlags>(ImGuiInputTextFlags_Multiline);
    if (first_render) {
        SetKeyboardFocusHere();
    }
    const char* placeholder = state->placeholder.empty() ? nullptr : state->placeholder.data();

    if (InputTextEx("##ImeDialogInput", placeholder, state->current_text.begin(),
                    state->max_text_length + 1, input_size, flags, InputTextCallback, this)) {
        state->input_changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetItemCurrentNavFocus();
        ImGui::KeepNavHighlight();
    }

    ImGui::PopStyleColor(6);
    ImGui::EndGroup();
}

int ImeDialogUi::InputTextCallback(ImGuiInputTextCallbackData* data) {
    ImeDialogUi* ui = static_cast<ImeDialogUi*>(data->UserData);
    ASSERT(ui);

    LOG_DEBUG(Lib_ImeDialog, ">> InputTextCallback: EventFlag={}, EventChar={}", data->EventFlag,
              data->EventChar);

    // Should we filter punctuation?
    if (ui->state->is_numeric && (data->EventChar < '0' || data->EventChar > '9') &&
        data->EventChar != '\b' && data->EventChar != ',' && data->EventChar != '.') {
        LOG_INFO(Lib_ImeDialog, "InputTextCallback: rejecting non-digit char '{}'",
                 static_cast<char>(data->EventChar));
        return 1;
    }

    if (!ui->state->keyboard_filter) {
        LOG_DEBUG(Lib_ImeDialog, "InputTextCallback: no keyboard_filter, accepting char");
        return 0;
    }

    // ImGui encodes ImWchar32 as multi-byte UTF-8 characters
    char* event_char = reinterpret_cast<char*>(&data->EventChar);

    // Call the keyboard filter
    OrbisImeKeycode src_keycode = {
        .keycode = 0,
        .character = 0,
        .status = 1,                              // ??? 1 = key pressed, 0 = key released
        .type = OrbisImeKeyboardType::ENGLISH_US, // TODO set this to the correct value (maybe
                                                  // use the current language?)
        .user_id = ui->state->user_id,
        .resource_id = 0,
        .timestamp = {0},
    };

    if (!ui->state->ConvertUTF8ToOrbis(event_char, 4, &src_keycode.character, 1)) {
        LOG_ERROR(Lib_ImeDialog, "InputTextCallback: ConvertUTF8ToOrbis failed");
        return 0;
    }
    LOG_DEBUG(Lib_ImeDialog, "InputTextCallback: converted to Orbis char={:#X}",
              static_cast<uint16_t>(src_keycode.character));
    src_keycode.keycode = src_keycode.character; // TODO set this to the correct value

    u16 out_keycode;
    u32 out_status;

    bool keep = ui->state->CallKeyboardFilter(&src_keycode, &out_keycode, &out_status);
    LOG_DEBUG(Lib_ImeDialog,
              "InputTextCallback: CallKeyboardFilter returned %s (keycode=0x%X, status=0x%X)",
              keep ? "true" : "false", out_keycode, out_status);
    // TODO. set the keycode

    return 0;
}

void ImeDialogUi::DrawTitle() {
    if (!state->title.empty()) {
        SetCursorPosX(20.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ConvertColor(ext_.color_text));
        SetWindowFontScale(1.7f);
        TextUnformatted(state->title.data());
        SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
    }
}

void ImeDialogUi::DrawOkAndCancelButtons() {
    SetCursorPosY(GetCursorPosY() + 10.0f);

    const char* button_text;

    switch (state->enter_label) {
    case OrbisImeEnterLabel::Go:
        button_text = "Go##ImeDialogOK";
        break;
    case OrbisImeEnterLabel::Search:
        button_text = "Search##ImeDialogOK";
        break;
    case OrbisImeEnterLabel::Send:
        button_text = "Send##ImeDialogOK";
        break;
    case OrbisImeEnterLabel::Default:
    default:
        button_text = "OK##ImeDialogOK";
        break;
    }

    float button_spacing = 10.0f;
    float total_button_width = BUTTON_SIZE.x * 2 + button_spacing;
    float button_start_pos = (GetWindowWidth() - total_button_width) / 2.0f;

    SetCursorPosX(button_start_pos);

    if (Button(button_text, BUTTON_SIZE) ||
        (!state->is_multi_line && IsKeyPressed(ImGuiKey_Enter))) {
        *status = OrbisImeDialogStatus::Finished;
        result->endstatus = OrbisImeDialogEndStatus::Ok;
    }

    SameLine(0.0f, button_spacing);

    if (Button("Cancel##ImeDialogCancel", BUTTON_SIZE)) {
        *status = OrbisImeDialogStatus::Finished;
        result->endstatus = OrbisImeDialogEndStatus::UserCanceled;
    }
}

/* draw keyboard in a sub‑ID scope */
void ImeDialogUi::DrawVirtualKeyboardSection() {
    ImGui::PushID("VirtualKeyboardSection");
    DrawVirtualKeyboard(kb_mode, state->type, shift_state, kb_language, KeyboardCallbackBridge,
                        kb_style);
    ImGui::PopID();
}

void ImeDialogUi::DrawPredictionBarAnCancelButton() {
    const float pad = 5.0f;
    const float width = kb_style.layout_width;
    const float bar_h = 25.0f;
    SetCursorPosY(GetCursorPosY() + 5.0f);
    SetCursorPosX(0.0f);
    ImVec2 p0 = GetCursorScreenPos();
    ImVec2 p1 = ImVec2(p0.x + width - bar_h - 2 * pad, p0.y + bar_h);
    // GetWindowDrawList()->AddRectFilled(p0, p1, IM_COL32(0, 0, 0, 255));

    /* label */
    // ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y));
    // ImGui::PushStyleColor(ImGuiCol_Text, kb_style.color_text);
    // Selectable("dummy prediction", false, 0, ImVec2(width - bar_h, bar_h));
    // ImGui::PopStyleColor();

    SetCursorPosX(pad);
    ImGui::PushStyleColor(ImGuiCol_NavHighlight, ConvertColor(ext_.color_line));
    ImGui::PushStyleColor(ImGuiCol_Button, ConvertColor(ext_.color_preedit));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ConvertColor(ext_.color_preedit));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ConvertColor(ext_.color_preedit));
    ImGui::PushStyleColor(ImGuiCol_Text, ConvertColor(ext_.color_text));

    if (ImGui::Button("predict", ImVec2(width - bar_h - 3 * pad, bar_h))) {
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetItemCurrentNavFocus();
        ImGui::KeepNavHighlight();
    }

    ImGui::PopStyleColor(4);

    /* X button */
    // ImGui::SameLine(width - bar_h);
    ImGui::SetCursorScreenPos(ImVec2(p0.x + width - bar_h - pad, p0.y));

    ImGui::PushStyleColor(ImGuiCol_Button, kb_style.color_button_function);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kb_style.color_button_function);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, kb_style.color_button_function);
    ImGui::PushStyleColor(ImGuiCol_Text, kb_style.color_text);

    if (ImGui::Button("╳", ImVec2(bar_h, bar_h))) {
        *status = OrbisImeDialogStatus::Finished;
        result->endstatus = OrbisImeDialogEndStatus::UserCanceled;
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetItemCurrentNavFocus();
        ImGui::KeepNavHighlight();
    }

    ImGui::PopStyleColor(5);

    SetCursorPosX(0.0f);
    SetCursorPosY(GetCursorPosY() + 5.0f);
}

/*─────────────────────────────────────────────────────────────*
 *  helper draw functions (new)
 *─────────────────────────────────────────────────────────────*/
void ImeDialogUi::OnVirtualKeyEvent(const VirtualKeyEvent* evt) {
    if (!evt || !state || !evt->key)
        return;

    const KeyEntry* key = evt->key;

    /* Treat Repeat exactly like Down */
    if (evt->type == VirtualKeyEventType::Down || evt->type == VirtualKeyEventType::Repeat) {
        switch (key->type) {
        case KeyType::Character: {
            char utf8[8]{};
            int n = c16rtomb(utf8, key->character);
            if (n > 0) {
                state->InsertUtf8AtCaret(utf8, (size_t)n);
            }
            break;
        }
        case KeyType::Function:
            switch (key->keycode) {
            case KC_LEFT: // Your custom code for ◀ button
                if (state->caret_index > 0)
                    state->caret_index--;
                LOG_INFO(Lib_ImeDialog, "Caret index = {}", state->caret_index);
                break;
            case KC_RIGHT: // Your custom code for ▶ button
                if (state->caret_index < (int)state->current_text.size())
                    state->caret_index++;
                LOG_INFO(Lib_ImeDialog, "Caret index = {}", state->caret_index);
                break;
            case 0x08:
                state->BackspaceUtf8AtCaret();
                break; // Backspace
            case 0x0D:
                *status = OrbisImeDialogStatus::Finished; // Enter
                result->endstatus = OrbisImeDialogEndStatus::Ok;
                break;

            case KC_SYM1:
                kb_mode = KeyboardMode::Symbols1;
                break;
            case KC_SYM2:
                kb_mode = KeyboardMode::Symbols2;
                break;
            case KC_ACCENTS:
                kb_mode = KeyboardMode::AccentLetters;
                break;
            case KC_LETTERS:
                kb_mode = KeyboardMode::Letters;
                break;

            case 0x10: // Shift / Caps
            case 0x105:
                shift_state = (shift_state == ShiftState::None)
                                  ? ShiftState::Shift
                                  : (shift_state == ShiftState::Shift ? ShiftState::CapsLock
                                                                      : ShiftState::None);
                break;
            default:
                break;
            }
            break;

        default:
            break;
        }
    }
    /* Up is available if you need it later; currently ignored */
}
} // namespace Libraries::ImeDialog
