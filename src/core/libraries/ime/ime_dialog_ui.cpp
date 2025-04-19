// ime_dialog_ui.cpp
// ----------------------------------------------------------
// Full implementation of IME dialog UI with on‑screen keyboard
// (all original logic intact, bugs fixed).
// ----------------------------------------------------------

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

#include "ime_keyboard_layouts.h" // c16rtomb, layout tables
#include "ime_keyboard_ui.h"      // DrawVirtualKeyboard, Utf8SafeBackspace

using namespace ImGui;

/* small helper for the OK/Cancel buttons */
static constexpr ImVec2 BUTTON_SIZE{100.0f, 30.0f};

/* convert palette colour from Orbis struct to ImGui format */
static ImU32 ConvertColor(const Libraries::ImeDialog::OrbisImeColor& c) {
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
    if (!param)
        return;

    /* basic param copy */
    user_id = param->user_id;
    is_multi_line = True(param->option & OrbisImeDialogOption::Multiline);
    is_numeric = (param->type == OrbisImeType::Number);
    type = param->type;
    enter_label = param->enter_label;
    text_filter = param->filter;
    keyboard_filter = extended ? extended->ext_keyboard_filter : nullptr;
    max_text_length = param->max_text_length;
    text_buffer = param->input_text_buffer;

    /* default keyboard style */
    has_custom_kb_style = false;
    custom_kb_style.layout_width = 485.0f;
    custom_kb_style.layout_height = 200.0f;
    custom_kb_style.key_spacing = 2.0f;
    custom_kb_style.color_text = IM_COL32(225, 225, 225, 255);
    custom_kb_style.color_line = IM_COL32(88, 88, 88, 255);
    custom_kb_style.color_button_default = IM_COL32(35, 35, 35, 255);
    custom_kb_style.color_button_function = IM_COL32(50, 50, 50, 255);
    custom_kb_style.color_special = IM_COL32(0, 140, 200, 255);
    custom_kb_style.color_button_symbol = IM_COL32(60, 60, 60, 255);
    custom_kb_style.use_button_symbol_color = false;

    /* optional extended palette */
    if (extended) {
        custom_kb_style.layout_width = 600.0f;
        custom_kb_style.layout_height = 220.0f;
        custom_kb_style.key_spacing = 3.0f;
        custom_kb_style.color_text = ConvertColor(extended->color_text);
        custom_kb_style.color_line = ConvertColor(extended->color_line);
        custom_kb_style.color_button_default = ConvertColor(extended->color_button_default);
        custom_kb_style.color_button_function = ConvertColor(extended->color_button_function);
        custom_kb_style.color_button_symbol = ConvertColor(extended->color_button_symbol);
        custom_kb_style.color_special = ConvertColor(extended->color_special);
        custom_kb_style.use_button_symbol_color = true;
        has_custom_kb_style = true;
    }

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

    if (state && *status == OrbisImeDialogStatus::Running) {
        AddLayer(this);
        ImeDialogUi::g_activeImeDialogUi = this;
    }

    if (state && state->has_custom_kb_style) {
        kb_style = state->custom_kb_style;
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

    if (state && status && *status == OrbisImeDialogStatus::Running) {
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

    if (state && status && *status == OrbisImeDialogStatus::Running) {
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

    if (!state) {
        return;
    }

    if (!status || *status != OrbisImeDialogStatus::Running) {
        return;
    }

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
        DrawPrettyBackground();
        /* ---------- title ---------- */
        if (!state->title.empty()) {
            SetCursorPosX(20.0f);
            SetWindowFontScale(1.7f);
            TextUnformatted(state->title.data());
            SetWindowFontScale(1.0f);
        }

        /* ---------- input box ---------- */
        if (state->is_multi_line) {
            DrawMultiLineInputText();
        } else {
            DrawInputText();
        }

        /* ---------- dummy prediction bar with Cancel button ---------- */
        DrawPredictionBarAnCancelButton();

        /* ---------- on‑screen keyboard ---------- */
        DrawVirtualKeyboardSection();

        /* ---------- OK / Cancel buttons ---------- */
        /* {
            SetCursorPosY(GetCursorPosY() + 10.0f);
            const char* ok_lbl = "OK##ImeDialogOK";
            switch (state->enter_label) {
            case OrbisImeEnterLabel::Go:
                ok_lbl = "Go##ImeDialogOK";
                break;
            case OrbisImeEnterLabel::Search:
                ok_lbl = "Search##ImeDialogOK";
                break;
            case OrbisImeEnterLabel::Send:
                ok_lbl = "Send##ImeDialogOK";
                break;
            default:
                break;
            }

            float spacing = 10.0f;
            float total_w = BUTTON_SIZE.x * 2 + spacing;
            float x_start = (window_size.x - total_w) / 2.0f;
            SetCursorPosX(x_start);

            if (Button(ok_lbl, BUTTON_SIZE) ||
                (!state->is_multi_line && IsKeyPressed(ImGuiKey_Enter))) {
                *status = OrbisImeDialogStatus::Finished;
                result->endstatus = OrbisImeDialogEndStatus::Ok;
            }

            SameLine(0.0f, spacing);

            if (Button("Cancel##ImeDialogCancel", BUTTON_SIZE)) {
                *status = OrbisImeDialogStatus::Finished;
                result->endstatus = OrbisImeDialogEndStatus::UserCanceled;
            }
        }*/

        End();
    }

    first_render = false;
}

/*─────────────────────────────────────────────────────────────*
 *  helper draw functions (unchanged)
 *─────────────────────────────────────────────────────────────*/
void ImeDialogUi::DrawInputText() {
    ImVec2 size(GetWindowWidth() - 40.0f, 0.0f);
    SetCursorPosX(20.0f);
    if (first_render)
        SetKeyboardFocusHere();

    const char* ph = state->placeholder.empty() ? nullptr : state->placeholder.data();
    if (InputTextEx("##ImeDialogInput", ph, state->current_text.begin(), state->max_text_length,
                    size, ImGuiInputTextFlags_CallbackCharFilter, InputTextCallback, this))
        state->input_changed = true;
}

void ImeDialogUi::DrawMultiLineInputText() {
    ImVec2 size(GetWindowWidth() - 40.0f, 200.0f);
    SetCursorPosX(20.0f);
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackCharFilter |
                                static_cast<ImGuiInputTextFlags>(ImGuiInputTextFlags_Multiline);
    if (first_render)
        SetKeyboardFocusHere();

    const char* ph = state->placeholder.empty() ? nullptr : state->placeholder.data();
    if (InputTextEx("##ImeDialogInput", ph, state->current_text.begin(), state->max_text_length,
                    size, flags, InputTextCallback, this))
        state->input_changed = true;
}

int ImeDialogUi::InputTextCallback(ImGuiInputTextCallbackData* data) {
    ImeDialogUi* ui = static_cast<ImeDialogUi*>(data->UserData);
    ASSERT(ui);

    /* numeric filter */
    if (ui->state->is_numeric && (data->EventChar < '0' || data->EventChar > '9') &&
        data->EventChar != '\b' && data->EventChar != ',' && data->EventChar != '.')
        return 1;

    if (!ui->state->keyboard_filter)
        return 0;

    char* ev_char = reinterpret_cast<char*>(&data->EventChar);

    OrbisImeKeycode src{
        .keycode = 0,
        .character = 0,
        .status = 1,
        .type = OrbisImeKeyboardType::ENGLISH_US,
        .user_id = ui->state->user_id,
        .resource_id = 0,
        .timestamp = 0,
    };

    if (!ui->state->ConvertUTF8ToOrbis(ev_char, 4, &src.character, 1))
        return 0;
    src.keycode = src.character;

    u16 out_code;
    u32 out_stat;
    ui->state->CallKeyboardFilter(&src, &out_code, &out_stat);
    return 0;
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
            if (n > 0)
                state->AppendUtf8(utf8, (size_t)n);
            break;
        }
        case KeyType::Function:
            switch (key->keycode) {
            case 0x08:
                state->BackspaceUtf8();
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
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 0, 255));
    ImGui::PushStyleColor(ImGuiCol_Text, kb_style.color_text);

    if (ImGui::Button("predict", ImVec2(width - bar_h - 3 * pad, bar_h))) {
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
    ImGui::PopStyleColor(4);
    SetCursorPosX(0.0f);
    SetCursorPosY(GetCursorPosY() + 5.0f);
}

} // namespace Libraries::ImeDialog
