// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include <imgui_internal.h>
#include "core/libraries/ime/ime_kb_layout.h"
#include "core/libraries/ime/ime_ui_shared.h"
#include "core/libraries/pad/pad.h"
#include "ime_ui.h"
#include "imgui/imgui_std.h"
#include "imgui/renderer/imgui_core.h"

namespace Libraries::Ime {

using namespace ImGui;
namespace {

constexpr ImU32 kSelectorBorderColor = IM_COL32(248, 248, 248, 255);
constexpr ImU32 kSelectorOverlayColor = IM_COL32(255, 255, 255, 18);
constexpr float kSelectorBorderThickness = 2.0f;
constexpr float kSelectorInnerMargin = 2.0f;

ImeKbLayoutSelection ResolveInitialKbLayoutSelection(const OrbisImeParamExtended* extended_param) {
    ImeKbLayoutSelection selection{};
    if (!extended_param) {
        return selection;
    }

    const bool set_priority = True(extended_param->option & OrbisImeExtOption::SET_PRIORITY);
    if (set_priority) {
        switch (extended_param->priority) {
        case OrbisImePanelPriority::Symbol:
            selection.family = ImeKbLayoutFamily::Symbols;
            break;
        case OrbisImePanelPriority::Accent:
            selection.family = ImeKbLayoutFamily::Specials;
            break;
        case OrbisImePanelPriority::Alphabet:
        case OrbisImePanelPriority::Default:
        default:
            selection.family = ImeKbLayoutFamily::Latin;
            break;
        }
    }

    const bool shift_lock =
        set_priority && True(extended_param->option & OrbisImeExtOption::PRIORITY_SHIFT);
    if (shift_lock && selection.family != ImeKbLayoutFamily::Symbols) {
        // SDK docs: PRIORITY_SHIFT starts the initial panel in Shift-lock state.
        selection.case_state = ImeKbCaseState::CapsLock;
    }
    return selection;
}

} // namespace

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
    const int current_len_utf16 = Utf16CountFromUtf8Range(
        current_text.begin(), current_text.begin() + static_cast<int>(current_text.size()));
    caret_index = current_len_utf16;
    caret_byte_index = Utf8ByteIndexFromUtf16Index(current_text.begin(), caret_index);
    caret_dirty = true;
}

ImeState::ImeState(ImeState&& other) noexcept
    : work_buffer(other.work_buffer), text_buffer(other.text_buffer),
      max_text_length(other.max_text_length), current_text(std::move(other.current_text)),
      caret_index(other.caret_index), caret_byte_index(other.caret_byte_index),
      caret_dirty(other.caret_dirty), event_queue(std::move(other.event_queue)) {
    other.text_buffer = nullptr;
    other.max_text_length = 0;
    other.caret_index = 0;
    other.caret_byte_index = 0;
    other.caret_dirty = false;
}

ImeState& ImeState::operator=(ImeState&& other) noexcept {
    if (this != &other) {
        work_buffer = other.work_buffer;
        text_buffer = other.text_buffer;
        max_text_length = other.max_text_length;
        current_text = std::move(other.current_text);
        caret_index = other.caret_index;
        caret_byte_index = other.caret_byte_index;
        caret_dirty = other.caret_dirty;
        event_queue = std::move(other.event_queue);

        other.text_buffer = nullptr;
        other.max_text_length = 0;
        other.caret_index = 0;
        other.caret_byte_index = 0;
        other.caret_dirty = false;
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
        const u32 caret = static_cast<u32>(std::clamp(caret_index, 0, static_cast<int>(len)));
        text.caret_index = caret;
        text.area_num = 1;
        text.text_area[0].mode = OrbisImeTextAreaMode::Edit;
        // No edit happening on Enter: length=0; index can be caret
        text.text_area[0].index = caret;
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
        const u32 caret = static_cast<u32>(std::clamp(caret_index, 0, static_cast<int>(len)));
        text.caret_index = caret;
        text.area_num = 1;
        text.text_area[0].mode = OrbisImeTextAreaMode::Edit;
        // No edit happening on Close: length=0; index can be caret
        text.text_area[0].index = caret;
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

    const int len_utf16 = Utf16CountFromUtf8Range(
        current_text.begin(), current_text.begin() + static_cast<int>(current_text.size()));
    if (caret_index < 0) {
        caret_index = 0;
    } else if (caret_index > len_utf16) {
        caret_index = len_utf16;
    }
    caret_byte_index = Utf8ByteIndexFromUtf16Index(current_text.begin(), caret_index);
    caret_dirty = true;
}

void ImeState::SetCaret(u32 position) {
    const int len_utf16 = Utf16CountFromUtf8Range(
        current_text.begin(), current_text.begin() + static_cast<int>(current_text.size()));
    const int next_caret = std::clamp(static_cast<int>(position), 0, len_utf16);
    caret_index = next_caret;
    caret_byte_index = Utf8ByteIndexFromUtf16Index(current_text.begin(), next_caret);
    caret_dirty = true;
    LOG_DEBUG(Lib_Ime, "ImeState::SetCaret requested={}, applied={} (len={})", position,
              caret_index, len_utf16);
}

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
    : state(state), ime_param(param), extended_param(extended),
      style_config(ResolveImeStyleConfig(extended)) {
    if (param) {
        kb_layout_selection = ResolveInitialKbLayoutSelection(extended_param);
        last_nav_layout_selection = kb_layout_selection;
        nav_layout_selection_initialized = true;
        kb_alpha_family = (kb_layout_selection.family == ImeKbLayoutFamily::Specials)
                              ? ImeKbLayoutFamily::Specials
                              : ImeKbLayoutFamily::Latin;
        AddLayer(this);
        ImGui::Core::AcquireGamepadInputCapture();
        gamepad_input_capture_active = true;
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
    extended_param = other.extended_param;
    style_config = other.style_config;
    first_render = other.first_render;
    accept_armed = other.accept_armed;
    native_input_active = other.native_input_active;
    pointer_navigation_active = other.pointer_navigation_active;
    edit_menu_popup = other.edit_menu_popup;
    request_input_focus = other.request_input_focus;
    request_input_select_all = other.request_input_select_all;
    text_select_mode = other.text_select_mode;
    pending_input_selection_apply = other.pending_input_selection_apply;
    prev_virtual_cross_down = other.prev_virtual_cross_down;
    prev_virtual_lstick_left_down = other.prev_virtual_lstick_left_down;
    prev_virtual_lstick_right_down = other.prev_virtual_lstick_right_down;
    prev_virtual_lstick_up_down = other.prev_virtual_lstick_up_down;
    prev_virtual_lstick_down_down = other.prev_virtual_lstick_down_down;
    left_stick_repeat_dir = other.left_stick_repeat_dir;
    left_stick_next_repeat_time = other.left_stick_next_repeat_time;
    prev_virtual_buttons = other.prev_virtual_buttons;
    prev_virtual_square_down = other.prev_virtual_square_down;
    prev_virtual_l1_down = other.prev_virtual_l1_down;
    prev_virtual_r1_down = other.prev_virtual_r1_down;
    prev_virtual_dpad_left_down = other.prev_virtual_dpad_left_down;
    prev_virtual_dpad_right_down = other.prev_virtual_dpad_right_down;
    prev_virtual_dpad_up_down = other.prev_virtual_dpad_up_down;
    prev_virtual_dpad_down_down = other.prev_virtual_dpad_down_down;
    virtual_square_next_repeat_time = other.virtual_square_next_repeat_time;
    virtual_l1_next_repeat_time = other.virtual_l1_next_repeat_time;
    virtual_r1_next_repeat_time = other.virtual_r1_next_repeat_time;
    virtual_dpad_left_next_repeat_time = other.virtual_dpad_left_next_repeat_time;
    virtual_dpad_right_next_repeat_time = other.virtual_dpad_right_next_repeat_time;
    virtual_dpad_up_next_repeat_time = other.virtual_dpad_up_next_repeat_time;
    virtual_dpad_down_next_repeat_time = other.virtual_dpad_down_next_repeat_time;
    panel_vertical_nav_state = other.panel_vertical_nav_state;
    selector_fade_state = other.selector_fade_state;
    panel_position_initialized = other.panel_position_initialized;
    panel_drag_active = other.panel_drag_active;
    panel_position = other.panel_position;
    input_cursor_utf16 = other.input_cursor_utf16;
    input_cursor_byte = other.input_cursor_byte;
    input_selection_start_byte = other.input_selection_start_byte;
    input_selection_end_byte = other.input_selection_end_byte;
    text_select_anchor_utf16 = other.text_select_anchor_utf16;
    text_select_focus_utf16 = other.text_select_focus_utf16;
    top_virtual_col = other.top_virtual_col;
    panel_selection = other.panel_selection;
    pending_keyboard_row = other.pending_keyboard_row;
    pending_keyboard_col = other.pending_keyboard_col;
    last_keyboard_selected_row = other.last_keyboard_selected_row;
    last_keyboard_selected_col = other.last_keyboard_selected_col;
    edit_menu_index = other.edit_menu_index;
    kb_layout_selection = other.kb_layout_selection;
    last_nav_layout_selection = other.last_nav_layout_selection;
    nav_layout_selection_initialized = other.nav_layout_selection_initialized;
    kb_alpha_family = other.kb_alpha_family;
    gamepad_input_capture_active = other.gamepad_input_capture_active;
    other.state = nullptr;
    other.ime_param = nullptr;
    other.extended_param = nullptr;
    other.style_config = GetDefaultImeStyleConfig();
    other.prev_virtual_lstick_left_down = false;
    other.prev_virtual_lstick_right_down = false;
    other.prev_virtual_lstick_up_down = false;
    other.prev_virtual_lstick_down_down = false;
    other.left_stick_repeat_dir = 0;
    other.left_stick_next_repeat_time = 0.0;
    other.prev_virtual_buttons = 0;
    other.prev_virtual_square_down = false;
    other.prev_virtual_l1_down = false;
    other.prev_virtual_r1_down = false;
    other.prev_virtual_dpad_left_down = false;
    other.prev_virtual_dpad_right_down = false;
    other.prev_virtual_dpad_up_down = false;
    other.prev_virtual_dpad_down_down = false;
    other.virtual_square_next_repeat_time = 0.0;
    other.virtual_l1_next_repeat_time = 0.0;
    other.virtual_r1_next_repeat_time = 0.0;
    other.virtual_dpad_left_next_repeat_time = 0.0;
    other.virtual_dpad_right_next_repeat_time = 0.0;
    other.virtual_dpad_up_next_repeat_time = 0.0;
    other.virtual_dpad_down_next_repeat_time = 0.0;
    ResetImeEdgeWrapNav(other.panel_vertical_nav_state);
    other.selector_fade_state = {};
    other.nav_layout_selection_initialized = false;
    other.kb_alpha_family = ImeKbLayoutFamily::Latin;
    other.gamepad_input_capture_active = false;

    AddLayer(this);
    if (!gamepad_input_capture_active && ime_param) {
        ImGui::Core::AcquireGamepadInputCapture();
        gamepad_input_capture_active = true;
    }
    return *this;
}

void ImeUi::Draw() {
    std::unique_lock<std::mutex> lock{draw_mutex};

    if (!state) {
        return;
    }

    const auto& ctx = *GetCurrentContext();
    const auto& io = ctx.IO;
    const bool imgui_typing_mode_active =
        native_input_active || request_input_focus || pending_input_selection_apply;
    const bool ps4_typing_mode_active = !imgui_typing_mode_active;
    const VirtualPadSnapshot virtual_pad = ReadVirtualPadSnapshot(ime_param->user_id, io.DeltaTime);

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
    const float min_x = viewport.offset.x;
    const float max_x = viewport.offset.x + std::max(0.0f, viewport.size.x - window_size.x);
    const float min_y = viewport.offset.y;
    const float max_y = viewport.offset.y + std::max(0.0f, viewport.size.y - window_size.y);
    pos_x = std::clamp(pos_x, min_x, max_x);
    pos_y = std::clamp(pos_y, min_y, max_y);

    const bool panel_position_locked = True(ime_param->option & OrbisImeOption::FIXED_POSITION);
    if (!panel_position_initialized) {
        panel_position = {pos_x, pos_y};
        panel_position_initialized = true;
    }
    if (panel_position_locked) {
        panel_position = {pos_x, pos_y};
        panel_drag_active = false;
    } else {
        if (!ps4_typing_mode_active) {
            panel_drag_active = false;
        } else {
            const ImVec2 mouse_pos = io.MousePos;
            const bool mouse_over_panel = mouse_pos.x >= panel_position.x &&
                                          mouse_pos.x <= (panel_position.x + window_size.x) &&
                                          mouse_pos.y >= panel_position.y &&
                                          mouse_pos.y <= (panel_position.y + window_size.y);
            if (!panel_drag_active && IsMouseClicked(ImGuiMouseButton_Left, false) &&
                mouse_over_panel) {
                panel_drag_active = true;
            }
            if (panel_drag_active) {
                if (IsMouseDown(ImGuiMouseButton_Left)) {
                    panel_position.x += io.MouseDelta.x;
                    panel_position.y += io.MouseDelta.y;
                } else {
                    panel_drag_active = false;
                }
            }
            const ImVec2 right_stick_delta = virtual_pad.panel_delta;
            panel_position.x += right_stick_delta.x;
            panel_position.y += right_stick_delta.y;
        }
    }
    panel_position.x = std::clamp(panel_position.x, min_x, max_x);
    panel_position.y = std::clamp(panel_position.y, min_y, max_y);

    SetNextWindowPos(panel_position);
    SetNextWindowSize(window_size);
    SetNextWindowDockID(0, ImGuiCond_Always);
    SetNextWindowCollapsed(false);

    if (first_render || !io.NavActive) {
        SetNextWindowFocus();
    }

    const auto lock_window_scroll = []() {
        SetScrollX(0.0f);
        SetScrollY(0.0f);
        ImGuiWindow* window = GetCurrentWindow();
        if (!window) {
            return;
        }
        window->Scroll = ImVec2(0.0f, 0.0f);
        const float max_f = std::numeric_limits<float>::max();
        window->ScrollTarget = ImVec2(max_f, max_f);
    };

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoNavInputs;
    if (Begin("IME##Ime", nullptr, window_flags)) {
        lock_window_scroll();
        KeepNavHighlight();
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

        const u32 virtual_buttons = virtual_pad.buttons;
        if (first_render) {
            prev_virtual_buttons = virtual_buttons;
        }
        const auto virtual_down = [&](Libraries::Pad::OrbisPadButtonDataOffset button) {
            const u32 mask = static_cast<u32>(button);
            return (virtual_buttons & mask) != 0;
        };
        const auto virtual_pressed = [&](Libraries::Pad::OrbisPadButtonDataOffset button) {
            const u32 mask = static_cast<u32>(button);
            return (virtual_buttons & mask) != 0 && (prev_virtual_buttons & mask) == 0;
        };
        const auto virtual_repeat_pressed = [&](Libraries::Pad::OrbisPadButtonDataOffset button,
                                                bool& prev_down_state, double& next_repeat_time,
                                                bool* out_repeat = nullptr) -> bool {
            if (out_repeat) {
                *out_repeat = false;
            }
            const bool down = virtual_down(button);
            if (!down) {
                prev_down_state = false;
                next_repeat_time = 0.0;
                return false;
            }

            const bool pressed = virtual_pressed(button);
            const double now = ImGui::GetTime();
            if (!prev_down_state) {
                prev_down_state = true;
                next_repeat_time = now + static_cast<double>(io.KeyRepeatDelay);
                return pressed;
            }
            if (pressed) {
                next_repeat_time = now + static_cast<double>(io.KeyRepeatDelay);
                return true;
            }
            if (io.KeyRepeatRate <= 0.0f) {
                return false;
            }
            if (now >= next_repeat_time) {
                next_repeat_time = now + static_cast<double>(io.KeyRepeatRate);
                if (out_repeat) {
                    *out_repeat = true;
                }
                return true;
            }
            return false;
        };
        const bool imgui_cross_down = IsKeyDown(ImGuiKey_GamepadFaceDown);
        const bool virtual_cross_down =
            virtual_down(Libraries::Pad::OrbisPadButtonDataOffset::Cross);
        const bool cross_down = imgui_cross_down || virtual_cross_down;
        if (first_render) {
            prev_virtual_cross_down = cross_down;
        }

        // Use an explicit edge detector for activation to avoid stale SDL key events triggering
        // immediate unintended key presses when OSK opens.
        const bool panel_activate_pressed_raw = cross_down && !prev_virtual_cross_down;

        const bool gamepad_nav_left_once = IsKeyPressed(ImGuiKey_GamepadDpadLeft, false);
        const bool gamepad_nav_right_once = IsKeyPressed(ImGuiKey_GamepadDpadRight, false);
        const bool gamepad_nav_up_once = IsKeyPressed(ImGuiKey_GamepadDpadUp, false);
        const bool gamepad_nav_down_once = IsKeyPressed(ImGuiKey_GamepadDpadDown, false);
        const bool gamepad_nav_left_with_repeat = IsKeyPressed(ImGuiKey_GamepadDpadLeft, true);
        const bool gamepad_nav_right_with_repeat = IsKeyPressed(ImGuiKey_GamepadDpadRight, true);
        const bool gamepad_nav_up_with_repeat = IsKeyPressed(ImGuiKey_GamepadDpadUp, true);
        const bool gamepad_nav_down_with_repeat = IsKeyPressed(ImGuiKey_GamepadDpadDown, true);
        const bool gamepad_nav_left = gamepad_nav_left_with_repeat;
        const bool gamepad_nav_right = gamepad_nav_right_with_repeat;
        const bool gamepad_nav_up = gamepad_nav_up_with_repeat;
        const bool gamepad_nav_down = gamepad_nav_down_with_repeat;
        const bool gamepad_nav_left_repeat = gamepad_nav_left_with_repeat && !gamepad_nav_left_once;
        const bool gamepad_nav_right_repeat =
            gamepad_nav_right_with_repeat && !gamepad_nav_right_once;
        const bool gamepad_nav_up_repeat = gamepad_nav_up_with_repeat && !gamepad_nav_up_once;
        const bool gamepad_nav_down_repeat = gamepad_nav_down_with_repeat && !gamepad_nav_down_once;
        bool virtual_nav_left_repeat = false;
        bool virtual_nav_right_repeat = false;
        bool virtual_nav_up_repeat = false;
        bool virtual_nav_down_repeat = false;
        const bool virtual_nav_left = virtual_repeat_pressed(
            Libraries::Pad::OrbisPadButtonDataOffset::Left, prev_virtual_dpad_left_down,
            virtual_dpad_left_next_repeat_time, &virtual_nav_left_repeat);
        const bool virtual_nav_right = virtual_repeat_pressed(
            Libraries::Pad::OrbisPadButtonDataOffset::Right, prev_virtual_dpad_right_down,
            virtual_dpad_right_next_repeat_time, &virtual_nav_right_repeat);
        const bool virtual_nav_up = virtual_repeat_pressed(
            Libraries::Pad::OrbisPadButtonDataOffset::Up, prev_virtual_dpad_up_down,
            virtual_dpad_up_next_repeat_time, &virtual_nav_up_repeat);
        const bool virtual_nav_down = virtual_repeat_pressed(
            Libraries::Pad::OrbisPadButtonDataOffset::Down, prev_virtual_dpad_down_down,
            virtual_dpad_down_next_repeat_time, &virtual_nav_down_repeat);
        const bool imgui_lstick_left_pressed = IsKeyPressed(ImGuiKey_GamepadLStickLeft, false);
        const bool imgui_lstick_right_pressed = IsKeyPressed(ImGuiKey_GamepadLStickRight, false);
        const bool imgui_lstick_up_pressed = IsKeyPressed(ImGuiKey_GamepadLStickUp, false);
        const bool imgui_lstick_down_pressed = IsKeyPressed(ImGuiKey_GamepadLStickDown, false);
        const float imgui_lx =
            ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadLStickRight)->AnalogValue -
                              ImGui::GetKeyData(ImGuiKey_GamepadLStickLeft)->AnalogValue);
        const float imgui_ly =
            ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadLStickDown)->AnalogValue -
                              ImGui::GetKeyData(ImGuiKey_GamepadLStickUp)->AnalogValue);
        const VirtualLeftStickDirections virtual_lstick_dirs = virtual_pad.left_stick_dirs;
        if (first_render) {
            prev_virtual_lstick_left_down = virtual_lstick_dirs.left;
            prev_virtual_lstick_right_down = virtual_lstick_dirs.right;
            prev_virtual_lstick_up_down = virtual_lstick_dirs.up;
            prev_virtual_lstick_down_down = virtual_lstick_dirs.down;
        }
        const bool virtual_lstick_left_edge =
            virtual_lstick_dirs.left && !prev_virtual_lstick_left_down;
        const bool virtual_lstick_right_edge =
            virtual_lstick_dirs.right && !prev_virtual_lstick_right_down;
        const bool virtual_lstick_up_edge = virtual_lstick_dirs.up && !prev_virtual_lstick_up_down;
        const bool virtual_lstick_down_edge =
            virtual_lstick_dirs.down && !prev_virtual_lstick_down_down;

        const bool raw_gamepad_control_input =
            gamepad_nav_left_once || gamepad_nav_right_once || gamepad_nav_up_once ||
            gamepad_nav_down_once || imgui_lstick_left_pressed || imgui_lstick_right_pressed ||
            imgui_lstick_up_pressed || imgui_lstick_down_pressed ||
            IsKeyPressed(ImGuiKey_GamepadFaceDown, false) ||
            IsKeyPressed(ImGuiKey_GamepadFaceUp, false) ||
            IsKeyPressed(ImGuiKey_GamepadFaceLeft, false) ||
            IsKeyPressed(ImGuiKey_GamepadFaceRight, false) ||
            IsKeyPressed(ImGuiKey_GamepadL1, false) || IsKeyPressed(ImGuiKey_GamepadR1, false) ||
            IsKeyPressed(ImGuiKey_GamepadL2, false) || IsKeyPressed(ImGuiKey_GamepadR2, false) ||
            IsKeyPressed(ImGuiKey_GamepadL3, false) || IsKeyPressed(ImGuiKey_GamepadR3, false);
        const bool raw_virtual_control_input =
            virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Left) ||
            virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Right) ||
            virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Up) ||
            virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Down) ||
            virtual_lstick_left_edge || virtual_lstick_right_edge || virtual_lstick_up_edge ||
            virtual_lstick_down_edge ||
            virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Cross) ||
            virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Triangle) ||
            virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Square) ||
            virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Circle) ||
            virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L1) ||
            virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::R1) ||
            virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L2) ||
            virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::R2) ||
            virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L3) ||
            virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::R3);
        const bool raw_osk_control_input = raw_gamepad_control_input || raw_virtual_control_input;
        if (native_input_active && raw_osk_control_input && !request_input_focus &&
            !pending_input_selection_apply) {
            native_input_active = false;
            request_input_focus = false;
            pending_input_selection_apply = false;
            ImGui::ClearActiveID();
        }

        const bool controller_shortcuts_disabled =
            extended_param &&
            True(extended_param->disable_device & OrbisImeDisableDevice::CONTROLLER);
        const bool allow_osk_shortcuts = ps4_typing_mode_active && !controller_shortcuts_disabled;
        const float combined_lx = (std::abs(virtual_pad.left_stick.x) > std::abs(imgui_lx))
                                      ? virtual_pad.left_stick.x
                                      : imgui_lx;
        const float combined_ly = (std::abs(virtual_pad.left_stick.y) > std::abs(imgui_ly))
                                      ? virtual_pad.left_stick.y
                                      : imgui_ly;
        float stick_strength = 0.0f;
        const StickNavDirection stick_dir =
            ResolveStickNavDirection(combined_lx, combined_ly, &stick_strength);
        bool stick_nav_left = false;
        bool stick_nav_right = false;
        bool stick_nav_up = false;
        bool stick_nav_down = false;
        bool stick_nav_left_repeat = false;
        bool stick_nav_right_repeat = false;
        bool stick_nav_up_repeat = false;
        bool stick_nav_down_repeat = false;
        if (!allow_osk_shortcuts || stick_dir == StickNavDirection::None) {
            left_stick_repeat_dir = 0;
            left_stick_next_repeat_time = 0.0;
        } else {
            const double now = ImGui::GetTime();
            const float t = std::clamp(
                (stick_strength - kNavAxisThreshold) / (1.0f - kNavAxisThreshold), 0.0f, 1.0f);
            const double initial_delay = std::lerp(
                kStickNavInitialDelaySlow, kStickNavInitialDelayFast, static_cast<double>(t));
            const double repeat_interval =
                std::lerp(kStickNavRepeatSlow, kStickNavRepeatFast, static_cast<double>(t));
            bool dir_edge_pressed = false;
            switch (stick_dir) {
            case StickNavDirection::Left:
                dir_edge_pressed = imgui_lstick_left_pressed || virtual_lstick_left_edge;
                break;
            case StickNavDirection::Right:
                dir_edge_pressed = imgui_lstick_right_pressed || virtual_lstick_right_edge;
                break;
            case StickNavDirection::Up:
                dir_edge_pressed = imgui_lstick_up_pressed || virtual_lstick_up_edge;
                break;
            case StickNavDirection::Down:
                dir_edge_pressed = imgui_lstick_down_pressed || virtual_lstick_down_edge;
                break;
            case StickNavDirection::None:
            default:
                break;
            }
            bool stick_pulse = false;
            bool stick_pulse_repeat = false;
            if (left_stick_repeat_dir != static_cast<int>(stick_dir) || dir_edge_pressed) {
                stick_pulse = true;
                left_stick_repeat_dir = static_cast<int>(stick_dir);
                left_stick_next_repeat_time = now + initial_delay;
            } else if (now >= left_stick_next_repeat_time) {
                stick_pulse = true;
                stick_pulse_repeat = true;
                left_stick_next_repeat_time = now + repeat_interval;
            }
            if (stick_pulse) {
                switch (stick_dir) {
                case StickNavDirection::Left:
                    stick_nav_left = true;
                    stick_nav_left_repeat = stick_pulse_repeat;
                    break;
                case StickNavDirection::Right:
                    stick_nav_right = true;
                    stick_nav_right_repeat = stick_pulse_repeat;
                    break;
                case StickNavDirection::Up:
                    stick_nav_up = true;
                    stick_nav_up_repeat = stick_pulse_repeat;
                    break;
                case StickNavDirection::Down:
                    stick_nav_down = true;
                    stick_nav_down_repeat = stick_pulse_repeat;
                    break;
                case StickNavDirection::None:
                default:
                    break;
                }
            }
        }
        const bool nav_left = (allow_osk_shortcuts && gamepad_nav_left) ||
                              (allow_osk_shortcuts && (virtual_nav_left || stick_nav_left));
        const bool nav_right = (allow_osk_shortcuts && gamepad_nav_right) ||
                               (allow_osk_shortcuts && (virtual_nav_right || stick_nav_right));
        const bool nav_up = (allow_osk_shortcuts && gamepad_nav_up) ||
                            (allow_osk_shortcuts && (virtual_nav_up || stick_nav_up));
        const bool nav_down = (allow_osk_shortcuts && gamepad_nav_down) ||
                              (allow_osk_shortcuts && (virtual_nav_down || stick_nav_down));
        const bool nav_left_repeat =
            (allow_osk_shortcuts && gamepad_nav_left_repeat) ||
            (allow_osk_shortcuts && (virtual_nav_left_repeat || stick_nav_left_repeat));
        const bool nav_right_repeat =
            (allow_osk_shortcuts && gamepad_nav_right_repeat) ||
            (allow_osk_shortcuts && (virtual_nav_right_repeat || stick_nav_right_repeat));
        const bool nav_up_repeat =
            (allow_osk_shortcuts && gamepad_nav_up_repeat) ||
            (allow_osk_shortcuts && (virtual_nav_up_repeat || stick_nav_up_repeat));
        const bool nav_down_repeat =
            (allow_osk_shortcuts && gamepad_nav_down_repeat) ||
            (allow_osk_shortcuts && (virtual_nav_down_repeat || stick_nav_down_repeat));
        const double nav_now = ImGui::GetTime();
        const bool cancel_shortcut_pressed =
            allow_osk_shortcuts &&
            (IsKeyPressed(ImGuiKey_GamepadFaceRight, false) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Circle));

        const bool gamepad_control_input =
            allow_osk_shortcuts &&
            (gamepad_nav_left || gamepad_nav_right || gamepad_nav_up || gamepad_nav_down ||
             stick_nav_left || stick_nav_right || stick_nav_up || stick_nav_down ||
             IsKeyPressed(ImGuiKey_GamepadFaceDown, false) ||
             IsKeyPressed(ImGuiKey_GamepadFaceUp, false) ||
             IsKeyPressed(ImGuiKey_GamepadFaceLeft, false) ||
             IsKeyPressed(ImGuiKey_GamepadFaceRight, false) ||
             IsKeyPressed(ImGuiKey_GamepadL1, false) || IsKeyPressed(ImGuiKey_GamepadR1, false) ||
             IsKeyPressed(ImGuiKey_GamepadL2, false) || IsKeyPressed(ImGuiKey_GamepadR2, false) ||
             IsKeyPressed(ImGuiKey_GamepadL3, false) || IsKeyPressed(ImGuiKey_GamepadR3, false));
        const bool virtual_control_input =
            allow_osk_shortcuts &&
            (virtual_nav_left || virtual_nav_right || virtual_nav_up || virtual_nav_down ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Cross) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Triangle) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Square) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Circle) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L1) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::R1) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L2) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::R2) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L3) ||
             virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::R3));
        const bool osk_control_input = gamepad_control_input || virtual_control_input ||
                                       (allow_osk_shortcuts && panel_activate_pressed_raw);
        const ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;
        const bool pointer_input = IsMouseClicked(ImGuiMouseButton_Left, false) ||
                                   IsMouseClicked(ImGuiMouseButton_Right, false) ||
                                   IsMouseClicked(ImGuiMouseButton_Middle, false) ||
                                   (mouse_delta.x != 0.0f || mouse_delta.y != 0.0f);
        if (pointer_input) {
            pointer_navigation_active = true;
        }
        if (osk_control_input) {
            pointer_navigation_active = false;
        }
        if (native_input_active && (gamepad_control_input || virtual_control_input) &&
            !request_input_focus && !pending_input_selection_apply) {
            native_input_active = false;
            ImGui::ClearActiveID();
        }
        if (!accept_armed) {
            if (!cross_down) {
                accept_armed = true;
            }
        }
        const bool panel_activate_pressed =
            allow_osk_shortcuts && accept_armed && panel_activate_pressed_raw;

        using SelectionIndex = ImeSelectionGridIndex;
        const auto& selected_kb_layout = Libraries::Ime::GetImeKeyboardLayout(kb_layout_selection);
        const bool nav_layout_changed =
            nav_layout_selection_initialized &&
            (last_nav_layout_selection.family != kb_layout_selection.family ||
             last_nav_layout_selection.case_state != kb_layout_selection.case_state ||
             last_nav_layout_selection.page != kb_layout_selection.page);
        if (nav_layout_changed) {
            ResetImeEdgeWrapNav(panel_vertical_nav_state);
        }
        last_nav_layout_selection = kb_layout_selection;
        nav_layout_selection_initialized = true;
        const int keyboard_min_col = 0;
        const int keyboard_min_row = 0;
        const int keyboard_cols = std::max(1, static_cast<int>(selected_kb_layout.cols));
        const int keyboard_rows = std::max(1, static_cast<int>(selected_kb_layout.rows));
        const int keyboard_max_col = keyboard_cols - 1;
        const int keyboard_max_row = keyboard_rows - 1;
        const auto keyboard_vertical_wraps_from = [&](int from_row, int from_col, int step_row) {
            return DoesImeKeyboardNavigationWrap(selected_kb_layout, from_row, from_col, step_row,
                                                 0);
        };

        const auto& top_layout_cfg =
            Libraries::Ime::GetImeTopPanelLayoutConfig(kb_layout_selection);
        const int top_panel_row =
            SelectionIndex::PanelTopRowFromConfig(static_cast<int>(top_layout_cfg.row));
        const int top_panel_rows =
            SelectionIndex::PanelTopRowsFromConfig(static_cast<int>(top_layout_cfg.row_span));
        const int keyboard_panel_min_row =
            SelectionIndex::PanelKeyboardMinRowForTopPanel(top_panel_row, top_panel_rows);
        const int keyboard_panel_max_row = SelectionIndex::PanelKeyboardMaxRowForKeyboardRows(
            keyboard_rows, top_panel_row, top_panel_rows);
        const int top_col_min = 0;
        const int top_cols_cfg = std::max(1, static_cast<int>(top_layout_cfg.cols));
        const int top_col_max = top_col_min + top_cols_cfg - 1;

        const auto top_to_keyboard_col = [&](int top_col) {
            const int clamped_top = std::clamp(top_col, top_col_min, top_col_max);
            if (keyboard_cols <= 1 || top_cols_cfg <= 1) {
                return keyboard_min_col;
            }
            const int top_offset = clamped_top - top_col_min;
            return keyboard_min_col + (top_offset * (keyboard_cols - 1)) / (top_cols_cfg - 1);
        };
        const auto keyboard_to_top_col = [&](int keyboard_col) {
            const int clamped_kb = std::clamp(keyboard_col, keyboard_min_col, keyboard_max_col);
            if (top_cols_cfg <= 1 || keyboard_cols <= 1) {
                return top_col_min;
            }
            const int kb_offset = clamped_kb - keyboard_min_col;
            return top_col_min + (kb_offset * (top_cols_cfg - 1)) / (keyboard_cols - 1);
        };

        struct TopNavElement {
            PanelSelectionTarget target = PanelSelectionTarget::Prediction;
            int min_col = 0;
            int max_col = 0;
        };

        std::vector<int> top_col_to_element_index(static_cast<std::size_t>(top_cols_cfg), -1);
        std::vector<TopNavElement> top_elements;
        top_elements.reserve(static_cast<std::size_t>(top_cols_cfg));

        const auto append_top_element = [&](ImeTopPanelElementId id, int min_col, int max_col) {
            PanelSelectionTarget target = PanelSelectionTarget::Prediction;
            switch (id) {
            case ImeTopPanelElementId::Prediction:
                target = PanelSelectionTarget::Prediction;
                break;
            case ImeTopPanelElementId::Close:
                target = PanelSelectionTarget::Close;
                break;
            default:
                return;
            }

            if (static_cast<int>(top_elements.size()) >= top_cols_cfg) {
                return;
            }
            if (max_col < min_col) {
                return;
            }

            const int clamped_min = std::clamp(min_col, top_col_min, top_col_max);
            const int clamped_max = std::clamp(max_col, top_col_min, top_col_max);
            if (clamped_max < clamped_min) {
                return;
            }

            top_elements.push_back({target, clamped_min, clamped_max});
            const int element_index = static_cast<int>(top_elements.size()) - 1;
            for (int col = clamped_min; col <= clamped_max; ++col) {
                top_col_to_element_index[static_cast<std::size_t>(col - top_col_min)] =
                    element_index;
            }
        };

        for (std::size_t i = 0; i < top_layout_cfg.element_count; ++i) {
            const auto& spec = top_layout_cfg.elements[i];
            const int min_col = std::clamp(static_cast<int>(spec.col), top_col_min, top_col_max);
            const int span = std::max(1, static_cast<int>(spec.col_span));
            const int max_col = std::clamp(min_col + span - 1, top_col_min, top_col_max);
            append_top_element(spec.id, min_col, max_col);
        }
        if (top_elements.empty()) {
            append_top_element(ImeTopPanelElementId::Prediction, top_col_min, top_col_max);
        }

        const auto element_index_for_col = [&](int col) {
            if (col < top_col_min || col > top_col_max) {
                return -1;
            }
            return top_col_to_element_index[static_cast<std::size_t>(col - top_col_min)];
        };
        const auto element_index_for_target = [&](PanelSelectionTarget target) {
            for (int i = 0; i < static_cast<int>(top_elements.size()); ++i) {
                if (top_elements[static_cast<std::size_t>(i)].target == target) {
                    return i;
                }
            }
            return -1;
        };
        const auto set_top_selection = [&](PanelSelectionTarget target, int preferred_col) {
            panel_selection = target;
            const int target_idx = element_index_for_target(target);
            if (target_idx < 0) {
                top_virtual_col = std::clamp(preferred_col, top_col_min, top_col_max);
                return;
            }
            const auto& element = top_elements[static_cast<std::size_t>(target_idx)];
            top_virtual_col = std::clamp(preferred_col, element.min_col, element.max_col);
        };
        const auto top_col_for_selection = [&](PanelSelectionTarget target) {
            const int target_idx = element_index_for_target(target);
            const int clamped_col = std::clamp(top_virtual_col, top_col_min, top_col_max);
            if (target_idx < 0) {
                return clamped_col;
            }
            const auto& element = top_elements[static_cast<std::size_t>(target_idx)];
            return std::clamp(clamped_col, element.min_col, element.max_col);
        };
        const bool menu_modal = (edit_menu_popup != EditMenuPopup::None);
        const int keyboard_row_before_panel_nav =
            (pending_keyboard_row >= keyboard_min_row && pending_keyboard_row <= keyboard_max_row)
                ? pending_keyboard_row
                : last_keyboard_selected_row;
        const int keyboard_col_before_panel_nav =
            (pending_keyboard_col >= keyboard_min_col && pending_keyboard_col <= keyboard_max_col)
                ? pending_keyboard_col
                : last_keyboard_selected_col;
        bool entered_top_from_keyboard = false;
        const auto move_keyboard_edge_to_top = [&](int wrap_dir_y, int keyboard_col) {
            const int top_col = keyboard_to_top_col(keyboard_col);
            const int element_idx = element_index_for_col(top_col);
            if (element_idx >= 0) {
                const auto& element = top_elements[static_cast<std::size_t>(element_idx)];
                set_top_selection(element.target, top_col);
            } else if (!top_elements.empty()) {
                const auto& first_element = top_elements[0];
                set_top_selection(first_element.target, first_element.min_col);
            }
            CommitImeEdgeWrapStep(panel_vertical_nav_state, wrap_dir_y, 0, nav_now);
            entered_top_from_keyboard = true;
        };
        if (!menu_modal && !text_select_mode && !pointer_navigation_active &&
            panel_selection == PanelSelectionTarget::Keyboard) {
            const int wrap_dir_y =
                (nav_up && keyboard_vertical_wraps_from(keyboard_row_before_panel_nav,
                                                        keyboard_col_before_panel_nav, -1))
                    ? -1
                    : ((nav_down && keyboard_vertical_wraps_from(keyboard_row_before_panel_nav,
                                                                 keyboard_col_before_panel_nav, 1))
                           ? 1
                           : 0);
            if (wrap_dir_y != 0) {
                const bool hold_before_wrap_to_top = wrap_dir_y > 0;
                const bool wrap_repeat = nav_down_repeat;
                if (!hold_before_wrap_to_top ||
                    !ShouldDelayImeEdgeWrap(panel_vertical_nav_state, wrap_dir_y, 0, wrap_repeat,
                                            true, nav_now, kPanelEdgeWrapHoldDelaySec,
                                            kRepeatIntentWindowSec)) {
                    move_keyboard_edge_to_top(wrap_dir_y, keyboard_col_before_panel_nav);
                }
            }
        }

        SetWindowFontScale(std::max(viewport.ui_scale, metrics.input_font_scale));
        const bool input_hovered = DrawInputText(metrics, pointer_navigation_active);
        const bool input_selected =
            pointer_navigation_active && (input_hovered || native_input_active);
        auto draw_selector = [&](ImVec2 pos, ImVec2 size, bool selected,
                                 PanelSelectionTarget fade_target) {
            const float inset = kSelectorInnerMargin;
            if (size.x <= inset * 2.0f || size.y <= inset * 2.0f) {
                return;
            }
            const int fade_index = static_cast<int>(fade_target);
            if (fade_index < 0 ||
                fade_index >= static_cast<int>(selector_fade_state.panel_alpha.size())) {
                return;
            }
            const float alpha = UpdateImeSelectorFadeAlpha(
                selector_fade_state.panel_alpha[static_cast<std::size_t>(fade_index)], selected,
                io.DeltaTime);
            if (alpha <= 0.0f) {
                return;
            }
            const ImVec2 sel_min{pos.x + inset, pos.y + inset};
            const ImVec2 sel_size{size.x - inset * 2.0f, size.y - inset * 2.0f};
            const float selector_corner_radius = std::max(0.0f, metrics.corner_radius - inset);
            auto* selector_draw = GetWindowDrawList();
            DrawImeSelectorFrame(selector_draw, sel_min, sel_size, selector_corner_radius,
                                 kSelectorOverlayColor, kSelectorBorderColor,
                                 kSelectorBorderThickness, alpha);
        };
        draw_selector(metrics.input_pos_screen, metrics.input_size, input_selected,
                      PanelSelectionTarget::Input);

        auto* draw = GetWindowDrawList();
        const ImU32 pane_bg = ImeColorToImU32(style_config.color_base);
        const ImU32 pane_border = ImeColorToImU32(style_config.color_line);
        draw->AddRectFilled(metrics.predict_pos,
                            {metrics.predict_pos.x + metrics.predict_size.x,
                             metrics.predict_pos.y + metrics.predict_size.y},
                            pane_bg, metrics.corner_radius);
        draw->AddRect(metrics.predict_pos,
                      {metrics.predict_pos.x + metrics.predict_size.x,
                       metrics.predict_pos.y + metrics.predict_size.y},
                      pane_border, metrics.corner_radius);
        SetCursorScreenPos(metrics.predict_pos);
        PushID("##ImePredict");
        PushItemFlag(ImGuiItemFlags_NoNav, true);
        InvisibleButton("##ImePredict", metrics.predict_size);
        PopItemFlag();
        const bool prediction_clicked =
            IsMouseClicked(ImGuiMouseButton_Left, false) && IsItemHovered();
        const int prediction_element_idx =
            element_index_for_target(PanelSelectionTarget::Prediction);
        if (pointer_navigation_active && prediction_clicked && prediction_element_idx >= 0) {
            const auto& prediction_element =
                top_elements[static_cast<std::size_t>(prediction_element_idx)];
            set_top_selection(PanelSelectionTarget::Prediction,
                              SelectionIndex::GridColumnFromX(
                                  io.MousePos.x, metrics.predict_pos.x, metrics.predict_size.x,
                                  prediction_element.min_col, prediction_element.max_col));
        }
        PopID();
        draw_selector(metrics.predict_pos, metrics.predict_size,
                      panel_selection == PanelSelectionTarget::Prediction,
                      PanelSelectionTarget::Prediction);
        const ImU32 close_button_bg = ImeColorToImU32(style_config.color_button_function);
        PushStyleColor(ImGuiCol_Button, BrightenColor(close_button_bg, 0.0f));
        PushStyleColor(ImGuiCol_ButtonHovered, BrightenColor(close_button_bg, 0.08f));
        PushStyleColor(ImGuiCol_ButtonActive, BrightenColor(close_button_bg, 0.16f));
        SetCursorScreenPos(metrics.close_pos);
        PushItemFlag(ImGuiItemFlags_NoNav, true);
        bool cancel_pressed = Button("X##ImeClose", {metrics.close_size.x, metrics.close_size.y});
        PopItemFlag();
        cancel_pressed = cancel_pressed || cancel_shortcut_pressed;
        const bool close_clicked = IsMouseClicked(ImGuiMouseButton_Left, false) && IsItemHovered();
        const int close_element_idx = element_index_for_target(PanelSelectionTarget::Close);
        if (pointer_navigation_active && close_clicked && close_element_idx >= 0) {
            const auto& close_element = top_elements[static_cast<std::size_t>(close_element_idx)];
            set_top_selection(PanelSelectionTarget::Close, close_element.min_col);
        }
        PopStyleColor(3);
        draw_selector(metrics.close_pos, metrics.close_size,
                      panel_selection == PanelSelectionTarget::Close, PanelSelectionTarget::Close);
        if (!cancel_pressed && panel_selection == PanelSelectionTarget::Close &&
            panel_activate_pressed) {
            cancel_pressed = true;
        }

        SetCursorScreenPos(metrics.kb_pos);

        bool entered_keyboard_from_top = false;
        const auto move_top_navigation = [&](int dir_x, int dir_y) {
            const PanelSelectionTarget current = panel_selection;
            int col = top_col_for_selection(current);
            int origin_element_idx = element_index_for_col(col);
            if (origin_element_idx < 0) {
                origin_element_idx = element_index_for_target(current);
            }

            if (dir_x != 0) {
                if (origin_element_idx >= 0) {
                    const int span = top_col_max - top_col_min + 1;
                    bool crossed_wrap = false;
                    for (int step = 1; step <= span; ++step) {
                        const int next_col_raw = col + dir_x * step;
                        crossed_wrap = crossed_wrap || next_col_raw < top_col_min ||
                                       next_col_raw > top_col_max;
                        const int next_col =
                            top_col_min + (col - top_col_min + dir_x * step + span) % span;
                        const int next_element_idx = element_index_for_col(next_col);
                        if (next_element_idx < 0 || next_element_idx == origin_element_idx) {
                            continue;
                        }
                        const bool repeat_hint = dir_x < 0 ? nav_left_repeat : nav_right_repeat;
                        if (ShouldDelayImeEdgeWrap(
                                panel_vertical_nav_state, 0, dir_x, repeat_hint, crossed_wrap,
                                nav_now, kPanelEdgeWrapHoldDelaySec, kRepeatIntentWindowSec)) {
                            return;
                        }
                        const auto& next_element =
                            top_elements[static_cast<std::size_t>(next_element_idx)];
                        set_top_selection(next_element.target, next_col);
                        CommitImeEdgeWrapStep(panel_vertical_nav_state, 0, dir_x, nav_now);
                        return;
                    }
                }
            }

            if (origin_element_idx >= 0) {
                if (dir_y > 0) {
                    pending_keyboard_row = SelectionIndex::PanelToKeyboardRow(
                        keyboard_panel_min_row, keyboard_rows, top_panel_row, top_panel_rows);
                    pending_keyboard_col = top_to_keyboard_col(col);
                    panel_selection = PanelSelectionTarget::Keyboard;
                    entered_keyboard_from_top = true;
                    CommitImeEdgeWrapStep(panel_vertical_nav_state, dir_y, 0, nav_now);
                    return;
                }
                if (dir_y < 0) {
                    if (ShouldDelayImeEdgeWrap(panel_vertical_nav_state, dir_y, 0, nav_up_repeat,
                                               true, nav_now, kPanelEdgeWrapHoldDelaySec,
                                               kRepeatIntentWindowSec)) {
                        return;
                    }
                    pending_keyboard_row = SelectionIndex::PanelToKeyboardRow(
                        keyboard_panel_max_row, keyboard_rows, top_panel_row, top_panel_rows);
                    pending_keyboard_col = top_to_keyboard_col(col);
                    panel_selection = PanelSelectionTarget::Keyboard;
                    entered_keyboard_from_top = true;
                    CommitImeEdgeWrapStep(panel_vertical_nav_state, dir_y, 0, nav_now);
                    return;
                }
            }
        };
        if (!menu_modal && !text_select_mode && !pointer_navigation_active &&
            !entered_top_from_keyboard && panel_selection != PanelSelectionTarget::Keyboard) {
            if (nav_left) {
                move_top_navigation(-1, 0);
            } else if (nav_right) {
                move_top_navigation(1, 0);
            } else if (nav_up) {
                move_top_navigation(0, -1);
            } else if (nav_down) {
                move_top_navigation(0, 1);
            }
        }

        bool accept_pressed = false;
        auto sync_text_buffers = [&]() {
            if (!state->current_text.begin()) {
                return;
            }
            state->ConvertUTF8ToOrbis(state->current_text.begin(), state->current_text.size(),
                                      reinterpret_cast<char16_t*>(state->work_buffer),
                                      state->max_text_length + 1);
            if (state->text_buffer) {
                state->ConvertUTF8ToOrbis(state->current_text.begin(), state->current_text.size(),
                                          state->text_buffer, state->max_text_length + 1);
            }
        };
        const auto text_length_utf16 = [&]() {
            const char* text = state->current_text.begin();
            const int byte_len = static_cast<int>(state->current_text.size());
            return Utf16CountFromUtf8Range(text, text ? (text + byte_len) : nullptr);
        };
        const auto emit_update_text_event = [&](int edit_index_utf16, int edit_delta_utf16,
                                                int caret_utf16) {
            OrbisImeEditText event_param{};
            event_param.str = reinterpret_cast<char16_t*>(state->work_buffer);
            event_param.area_num = 1;
            event_param.caret_index = static_cast<u32>(std::max(0, caret_utf16));
            event_param.text_area[0].mode = OrbisImeTextAreaMode::Edit;
            event_param.text_area[0].index = static_cast<u32>(std::max(0, edit_index_utf16));
            event_param.text_area[0].length = static_cast<s32>(edit_delta_utf16);

            OrbisImeEvent event{};
            event.id = OrbisImeEventId::UpdateText;
            event.param.text = event_param;
            state->SendEvent(&event);
        };
        const auto emit_update_caret_events = [&](int old_caret_utf16, int new_caret_utf16) {
            const int delta = new_caret_utf16 - old_caret_utf16;
            if (delta == 0) {
                return;
            }
            const bool move_right = delta > 0;
            const u32 steps = static_cast<u32>(std::abs(delta));
            const OrbisImeCaretMovementDirection direction =
                move_right ? OrbisImeCaretMovementDirection::Right
                           : OrbisImeCaretMovementDirection::Left;
            for (u32 i = 0; i < steps; ++i) {
                OrbisImeEvent caret_step{};
                caret_step.id = OrbisImeEventId::UpdateCaret;
                caret_step.param.caret_move = direction;
                state->SendEvent(&caret_step);
            }
        };
        const auto apply_selection_state = [&]() {
            const int len = text_length_utf16();
            const int caret = std::clamp(
                (text_select_focus_utf16 >= 0)
                    ? text_select_focus_utf16
                    : ((state->caret_index >= 0) ? state->caret_index : input_cursor_utf16),
                0, len);
            const int anchor =
                text_select_mode ? std::clamp(text_select_anchor_utf16, 0, len) : caret;
            const int focus =
                text_select_mode ? std::clamp(text_select_focus_utf16, 0, len) : caret;
            const char* text = state->current_text.begin();
            const int anchor_byte = Utf8ByteIndexFromUtf16Index(text ? text : "", anchor);
            const int focus_byte = Utf8ByteIndexFromUtf16Index(text ? text : "", focus);
            input_cursor_utf16 = focus;
            input_cursor_byte = focus_byte;
            input_selection_start_byte = std::min(anchor_byte, focus_byte);
            input_selection_end_byte = std::max(anchor_byte, focus_byte);
            state->caret_index = focus;
            state->caret_byte_index = focus_byte;
            state->caret_dirty = native_input_active;
            if (native_input_active) {
                pending_input_selection_apply = true;
                request_input_focus = true;
            } else {
                pending_input_selection_apply = false;
                request_input_focus = false;
            }
        };
        const auto apply_text_edit = [&](int start_utf16, int end_utf16,
                                         const char* insert_utf8) -> bool {
            const std::string current = state->current_text.to_string();
            const char* current_text = current.c_str();
            const int len_utf16 = Utf16CountFromUtf8Range(
                current_text, current_text + static_cast<int>(current.size()));
            const int start = std::clamp(start_utf16, 0, len_utf16);
            const int end = std::clamp(end_utf16, start, len_utf16);
            const int removed_utf16 = end - start;
            const int available_utf16 =
                static_cast<int>(state->max_text_length) - (len_utf16 - removed_utf16);

            std::string insert_clamped{};
            if (insert_utf8 && insert_utf8[0] != '\0' && available_utf16 > 0) {
                const char* p = insert_utf8;
                int used_utf16 = 0;
                while (*p && used_utf16 < available_utf16) {
                    unsigned int codepoint = 0;
                    const int step = ImTextCharFromUtf8(&codepoint, p, nullptr);
                    if (step <= 0) {
                        break;
                    }
                    const int units = (codepoint > 0xFFFF) ? 2 : 1;
                    if (used_utf16 + units > available_utf16) {
                        break;
                    }
                    insert_clamped.append(p, static_cast<std::size_t>(step));
                    p += step;
                    used_utf16 += units;
                }
            }

            if (removed_utf16 == 0 && insert_clamped.empty()) {
                return false;
            }

            const int start_byte = Utf8ByteIndexFromUtf16Index(current_text, start);
            const int end_byte = Utf8ByteIndexFromUtf16Index(current_text, end);
            std::string updated{};
            updated.reserve(current.size() - static_cast<std::size_t>(end_byte - start_byte) +
                            insert_clamped.size());
            updated.append(current, 0, static_cast<std::size_t>(start_byte));
            updated.append(insert_clamped);
            updated.append(current, static_cast<std::size_t>(end_byte), std::string::npos);

            state->current_text.FromString(updated);
            sync_text_buffers();
            const int inserted_utf16 = Utf16CountFromUtf8Range(
                insert_clamped.c_str(),
                insert_clamped.c_str() + static_cast<int>(insert_clamped.size()));
            const int new_caret_utf16 = start + inserted_utf16;
            text_select_mode = false;
            text_select_anchor_utf16 = new_caret_utf16;
            text_select_focus_utf16 = new_caret_utf16;
            apply_selection_state();
            emit_update_text_event(start, inserted_utf16 - removed_utf16, input_cursor_utf16);
            return true;
        };
        const auto selection_utf16_range = [&]() -> std::pair<int, int> {
            const int len = text_length_utf16();
            if (!text_select_mode) {
                const int caret = std::clamp(state->caret_index, 0, len);
                return {caret, caret};
            }
            const int anchor = std::clamp(text_select_anchor_utf16, 0, len);
            const int focus = std::clamp(text_select_focus_utf16, 0, len);
            return {std::min(anchor, focus), std::max(anchor, focus)};
        };
        const auto insert_text_at_caret = [&](const char* suffix) {
            const auto [sel_start, sel_end] = selection_utf16_range();
            return apply_text_edit(sel_start, sel_end, suffix);
        };
        const auto backspace_at_caret = [&]() {
            const auto [sel_start, sel_end] = selection_utf16_range();
            if (sel_end > sel_start) {
                return apply_text_edit(sel_start, sel_end, "");
            }
            if (sel_end <= 0) {
                return false;
            }
            const std::string current = state->current_text.to_string();
            const char* text = current.c_str();
            const int caret_byte = Utf8ByteIndexFromUtf16Index(text, sel_end);
            int prev_byte = caret_byte;
            do {
                --prev_byte;
            } while (prev_byte > 0 &&
                     (static_cast<unsigned char>(text[prev_byte]) & 0xC0u) == 0x80u);
            const int prev_utf16 = Utf16CountFromUtf8Range(text, text + prev_byte);
            return apply_text_edit(prev_utf16, sel_end, "");
        };
        const auto clear_all_text = [&]() {
            const int len = text_length_utf16();
            if (len <= 0) {
                return false;
            }
            // Matches libSceIme backend all-delete behavior (sceImeBackendAllDeleteConvertString):
            // clear the whole editable string in one operation.
            return apply_text_edit(0, len, "");
        };

        Libraries::Ime::ImeKbGridLayout kb_layout{};
        kb_layout.pos = metrics.kb_pos;
        kb_layout.size = metrics.kb_size;
        kb_layout.key_gap_x = metrics.key_gap;
        kb_layout.key_gap_y = metrics.key_gap;
        kb_layout.cols = keyboard_cols;
        kb_layout.rows = keyboard_rows;
        const int layout_rows = std::max(1, kb_layout.rows);
        kb_layout.fixed_bottom_rows = SelectionIndex::ResolveFunctionRows(
            layout_rows, static_cast<int>(selected_kb_layout.function_rows));
        if (kb_layout.fixed_bottom_rows > 0) {
            kb_layout.bottom_row_h = std::max(8.0f, metrics.key_h);
            const int typing_rows = std::max(1, layout_rows - kb_layout.fixed_bottom_rows);
            const float typing_area_h =
                kb_layout.size.y - kb_layout.key_gap_y * static_cast<float>(layout_rows - 1) -
                kb_layout.bottom_row_h * static_cast<float>(kb_layout.fixed_bottom_rows);
            const float computed_typing_key_h = typing_area_h / static_cast<float>(typing_rows);
            kb_layout.key_h = std::max(8.0f, computed_typing_key_h);
        } else {
            kb_layout.fixed_bottom_rows = 0;
            kb_layout.bottom_row_h = 0.0f;
            const float computed_key_h =
                (kb_layout.size.y - kb_layout.key_gap_y * static_cast<float>(layout_rows - 1)) /
                static_cast<float>(layout_rows);
            kb_layout.key_h = std::max(8.0f, computed_key_h);
        }
        kb_layout.corner_radius = metrics.corner_radius;

        ResizeImeKeyboardSelectorFade(selector_fade_state, keyboard_rows, keyboard_cols);
        Libraries::Ime::ImeKbDrawParams kb_params{};
        kb_params.selection = kb_layout_selection;
        kb_params.layout_model = &selected_kb_layout;
        kb_params.supported_languages = ime_param->supported_languages;
        kb_params.enter_label = ime_param->enter_label;
        kb_params.show_selection_highlight = (panel_selection == PanelSelectionTarget::Keyboard);
        kb_params.selection_fade_alpha = selector_fade_state.keyboard_alpha.data();
        kb_params.selection_fade_rows = selector_fade_state.keyboard_rows;
        kb_params.selection_fade_cols = selector_fade_state.keyboard_cols;
        kb_params.delta_time = io.DeltaTime;
        kb_params.allow_nav_input = allow_osk_shortcuts && !menu_modal && !text_select_mode &&
                                    (panel_selection == PanelSelectionTarget::Keyboard) &&
                                    !entered_keyboard_from_top;
        kb_params.allow_activate_input = allow_osk_shortcuts && accept_armed && !menu_modal &&
                                         !text_select_mode &&
                                         (panel_selection == PanelSelectionTarget::Keyboard);
        kb_params.external_nav_left = allow_osk_shortcuts && (virtual_nav_left || stick_nav_left);
        kb_params.external_nav_right =
            allow_osk_shortcuts && (virtual_nav_right || stick_nav_right);
        kb_params.external_nav_up = allow_osk_shortcuts && (virtual_nav_up || stick_nav_up);
        kb_params.external_nav_down = allow_osk_shortcuts && (virtual_nav_down || stick_nav_down);
        kb_params.external_nav_left_repeat =
            allow_osk_shortcuts && ((virtual_nav_left && virtual_nav_left_repeat) ||
                                    (stick_nav_left && stick_nav_left_repeat));
        kb_params.external_nav_right_repeat =
            allow_osk_shortcuts && ((virtual_nav_right && virtual_nav_right_repeat) ||
                                    (stick_nav_right && stick_nav_right_repeat));
        kb_params.external_nav_up_repeat =
            allow_osk_shortcuts &&
            ((virtual_nav_up && virtual_nav_up_repeat) || (stick_nav_up && stick_nav_up_repeat));
        kb_params.external_nav_down_repeat =
            allow_osk_shortcuts && ((virtual_nav_down && virtual_nav_down_repeat) ||
                                    (stick_nav_down && stick_nav_down_repeat));
        kb_params.external_activate_pressed = panel_activate_pressed;
        kb_params.reset_nav_state = nav_layout_changed;
        kb_params.requested_selected_row = pending_keyboard_row;
        kb_params.requested_selected_col = pending_keyboard_col;
        ApplyImeStyleToKeyboardDrawParams(style_config, kb_params);
        pending_keyboard_row = -1;
        pending_keyboard_col = -1;

        Libraries::Ime::ImeKbDrawState kb_state{};
        SetWindowFontScale(metrics.key_font_scale);
        Libraries::Ime::DrawImeKeyboardGrid(kb_layout, kb_params, kb_state);
        SetWindowFontScale(metrics.input_font_scale);
        if (kb_state.selected_row >= 0 && kb_state.selected_col >= 0) {
            last_keyboard_selected_row = kb_state.selected_row;
            last_keyboard_selected_col = kb_state.selected_col;
        }
        if (pointer_navigation_active && kb_state.clicked) {
            panel_selection = PanelSelectionTarget::Keyboard;
        }

        const auto cycle_case_state = [&]() {
            switch (kb_layout_selection.case_state) {
            case ImeKbCaseState::Lower:
                kb_layout_selection.case_state = ImeKbCaseState::Upper;
                break;
            case ImeKbCaseState::Upper:
                kb_layout_selection.case_state = ImeKbCaseState::CapsLock;
                break;
            case ImeKbCaseState::CapsLock:
            default:
                kb_layout_selection.case_state = ImeKbCaseState::Lower;
                break;
            }
        };
        const auto set_family_and_reset_page = [&](ImeKbLayoutFamily family) {
            kb_layout_selection.family = family;
            kb_layout_selection.page = 0;
            if (family == ImeKbLayoutFamily::Latin || family == ImeKbLayoutFamily::Specials) {
                kb_alpha_family = family;
            }
        };
        const auto toggle_family_mode = [&](ImeKbLayoutFamily target_family) {
            if (target_family == ImeKbLayoutFamily::Symbols) {
                if (kb_layout_selection.family == ImeKbLayoutFamily::Symbols) {
                    set_family_and_reset_page(kb_alpha_family);
                } else {
                    if (kb_layout_selection.family == ImeKbLayoutFamily::Latin ||
                        kb_layout_selection.family == ImeKbLayoutFamily::Specials) {
                        kb_alpha_family = kb_layout_selection.family;
                    }
                    set_family_and_reset_page(ImeKbLayoutFamily::Symbols);
                }
                return;
            }
            if (kb_layout_selection.family == target_family) {
                set_family_and_reset_page(ImeKbLayoutFamily::Latin);
            } else {
                set_family_and_reset_page(target_family);
            }
        };
        const auto focus_keyboard_action_key = [&](ImeKbKeyAction action) {
            const auto& focus_layout = GetImeKeyboardLayout(kb_layout_selection);
            if (!focus_layout.keys || focus_layout.key_count == 0) {
                return;
            }
            for (std::size_t i = 0; i < focus_layout.key_count; ++i) {
                const auto& key = focus_layout.keys[i];
                if (key.action != action) {
                    continue;
                }
                pending_keyboard_row = static_cast<int>(key.row);
                pending_keyboard_col = static_cast<int>(key.col);
                last_keyboard_selected_row = pending_keyboard_row;
                last_keyboard_selected_col = pending_keyboard_col;
                panel_selection = PanelSelectionTarget::Keyboard;
                return;
            }
        };
        const auto flip_mode_page = [&](int direction) {
            if (kb_layout_selection.family != ImeKbLayoutFamily::Symbols &&
                kb_layout_selection.family != ImeKbLayoutFamily::Specials) {
                return;
            }
            const int page = static_cast<int>(kb_layout_selection.page);
            kb_layout_selection.page = static_cast<u8>((page + direction + 2) % 2);
        };
        const auto consume_temporary_uppercase = [&](bool typed_character) {
            if (typed_character && kb_layout_selection.case_state == ImeKbCaseState::Upper) {
                kb_layout_selection.case_state = ImeKbCaseState::Lower;
            }
        };
        const auto has_clipboard_text = [&]() {
            const char* text = ImGui::GetClipboardText();
            return text && text[0] != '\0';
        };
        const auto get_selection_byte_range = [&]() {
            const int text_len = static_cast<int>(state->current_text.size());
            const int sel_start = std::clamp(input_selection_start_byte, 0, text_len);
            const int sel_end = std::clamp(input_selection_end_byte, 0, text_len);
            return std::pair{std::min(sel_start, sel_end), std::max(sel_start, sel_end)};
        };
        const auto copy_selected_text = [&]() {
            const std::string current = state->current_text.to_string();
            const auto [sel_start, sel_end] = get_selection_byte_range();
            if (sel_end > sel_start) {
                const std::string selection =
                    current.substr(static_cast<std::size_t>(sel_start),
                                   static_cast<std::size_t>(sel_end - sel_start));
                ImGui::SetClipboardText(selection.c_str());
            } else {
                ImGui::SetClipboardText(current.c_str());
            }
        };
        const auto collapse_selection_to_caret = [&](int caret_utf16) {
            const int len = text_length_utf16();
            const int clamped_caret = std::clamp(caret_utf16, 0, len);
            text_select_mode = false;
            text_select_anchor_utf16 = clamped_caret;
            text_select_focus_utf16 = clamped_caret;
            apply_selection_state();
            panel_selection = PanelSelectionTarget::Keyboard;
        };
        const auto move_text_caret = [&](int delta_utf16, bool preserve_selection) {
            const int len = text_length_utf16();
            int base = text_select_mode ? text_select_focus_utf16 : state->caret_index;
            if (base < 0) {
                base = state->caret_index;
            }
            base = std::clamp(base, 0, len);
            const int next = std::clamp(base + delta_utf16, 0, len);
            if (preserve_selection && text_select_mode) {
                text_select_focus_utf16 = next;
                apply_selection_state();
                panel_selection = PanelSelectionTarget::Keyboard;
                emit_update_caret_events(base, next);
                return next != base;
            }
            collapse_selection_to_caret(next);
            emit_update_caret_events(base, next);
            return next != base;
        };
        const auto begin_text_selection_from_caret = [&]() {
            text_select_mode = true;
            const int len = text_length_utf16();
            const int caret = std::clamp(state->caret_index, 0, len);
            text_select_anchor_utf16 = caret;
            text_select_focus_utf16 = caret;
            apply_selection_state();
            panel_selection = PanelSelectionTarget::Keyboard;
        };
        const auto select_all_text = [&]() {
            text_select_mode = true;
            const int len = text_length_utf16();
            text_select_anchor_utf16 = 0;
            text_select_focus_utf16 = len;
            apply_selection_state();
            panel_selection = PanelSelectionTarget::Keyboard;
        };
        const auto open_main_menu = [&]() {
            edit_menu_popup = EditMenuPopup::Main;
            edit_menu_index = 0;
        };
        const auto open_actions_menu = [&]() {
            edit_menu_popup = EditMenuPopup::Actions;
            edit_menu_index = 0;
        };
        const auto apply_main_menu_action = [&](int action_index) {
            switch (action_index) {
            case 0: // Select
                begin_text_selection_from_caret();
                edit_menu_popup = EditMenuPopup::None;
                break;
            case 1: // Select All
                select_all_text();
                open_actions_menu();
                break;
            case 2: // Paste
                if (has_clipboard_text()) {
                    (void)insert_text_at_caret(ImGui::GetClipboardText());
                }
                edit_menu_popup = EditMenuPopup::None;
                break;
            default:
                break;
            }
        };
        const auto apply_actions_menu_action = [&](int action_index) {
            switch (action_index) {
            case 0: // Copy
                copy_selected_text();
                collapse_selection_to_caret(text_select_focus_utf16 >= 0 ? text_select_focus_utf16
                                                                         : input_cursor_utf16);
                edit_menu_popup = EditMenuPopup::None;
                break;
            case 1: // Paste
                if (has_clipboard_text()) {
                    (void)insert_text_at_caret(ImGui::GetClipboardText());
                }
                edit_menu_popup = EditMenuPopup::None;
                break;
            default:
                break;
            }
        };

        bool opened_menu_this_frame = false;
        if (!(allow_osk_shortcuts && !menu_modal)) {
            prev_virtual_square_down = false;
            prev_virtual_l1_down = false;
            prev_virtual_r1_down = false;
            virtual_square_next_repeat_time = 0.0;
            virtual_l1_next_repeat_time = 0.0;
            virtual_r1_next_repeat_time = 0.0;
        }
        if (allow_osk_shortcuts && !menu_modal &&
            kb_state.pressed_action == Libraries::Ime::ImeKbKeyAction::None) {
            const bool l1_down = IsKeyDown(ImGuiKey_GamepadL1) ||
                                 virtual_down(Libraries::Pad::OrbisPadButtonDataOffset::L1);
            const bool l1_edge_pressed =
                IsKeyPressed(ImGuiKey_GamepadL1, false) ||
                virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L1);
            const bool l1_repeat_pressed =
                IsKeyPressed(ImGuiKey_GamepadL1, true) ||
                virtual_repeat_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L1,
                                       prev_virtual_l1_down, virtual_l1_next_repeat_time);
            const bool square_down = IsKeyDown(ImGuiKey_GamepadFaceLeft) ||
                                     virtual_down(Libraries::Pad::OrbisPadButtonDataOffset::Square);
            const bool square_edge_pressed =
                IsKeyPressed(ImGuiKey_GamepadFaceLeft, false) ||
                virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Square);
            const bool square_repeat_pressed =
                IsKeyPressed(ImGuiKey_GamepadFaceLeft, true) ||
                virtual_repeat_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Square,
                                       prev_virtual_square_down, virtual_square_next_repeat_time);
            const bool r1_repeat_pressed =
                IsKeyPressed(ImGuiKey_GamepadR1, true) ||
                virtual_repeat_pressed(Libraries::Pad::OrbisPadButtonDataOffset::R1,
                                       prev_virtual_r1_down, virtual_r1_next_repeat_time);
            const bool clear_all_shortcut_pressed =
                (l1_down && square_edge_pressed) || (square_down && l1_edge_pressed);
            const bool l2_down = IsKeyDown(ImGuiKey_GamepadL2) ||
                                 virtual_down(Libraries::Pad::OrbisPadButtonDataOffset::L2);
            const bool l2_pressed = IsKeyPressed(ImGuiKey_GamepadL2, false) ||
                                    virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L2);
            const bool tri_down = IsKeyDown(ImGuiKey_GamepadFaceUp) ||
                                  virtual_down(Libraries::Pad::OrbisPadButtonDataOffset::Triangle);
            const bool tri_pressed =
                IsKeyPressed(ImGuiKey_GamepadFaceUp, false) ||
                virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::Triangle);
            const bool symbols_shortcut_pressed =
                (l2_down && tri_pressed) || (tri_down && l2_pressed);
            if (symbols_shortcut_pressed) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::SymbolsMode;
            } else if (tri_pressed) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::Space;
            } else if (kb_layout_selection.family != Libraries::Ime::ImeKbLayoutFamily::Symbols &&
                       (IsKeyPressed(ImGuiKey_GamepadL3, false) ||
                        virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L3))) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::SpecialsMode;
            } else if (IsKeyPressed(ImGuiKey_GamepadR2, false) ||
                       virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::R2)) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::Done;
            } else if (clear_all_shortcut_pressed) {
                (void)clear_all_text();
            } else if (square_repeat_pressed) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::Backspace;
            } else if (l1_repeat_pressed) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::ArrowLeft;
            } else if (r1_repeat_pressed) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::ArrowRight;
            } else if (IsKeyPressed(ImGuiKey_GamepadR3, false) ||
                       virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::R3)) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::Settings;
            } else if (kb_layout_selection.family != Libraries::Ime::ImeKbLayoutFamily::Symbols &&
                       (IsKeyPressed(ImGuiKey_GamepadL2, false) ||
                        virtual_pressed(Libraries::Pad::OrbisPadButtonDataOffset::L2))) {
                kb_state.pressed_action = Libraries::Ime::ImeKbKeyAction::Shift;
            }
        }
        switch (kb_state.pressed_action) {
        case Libraries::Ime::ImeKbKeyAction::Character:
            consume_temporary_uppercase(insert_text_at_caret(kb_state.pressed_label));
            break;
        case Libraries::Ime::ImeKbKeyAction::Shift:
            cycle_case_state();
            break;
        case Libraries::Ime::ImeKbKeyAction::SymbolsMode:
            toggle_family_mode(ImeKbLayoutFamily::Symbols);
            break;
        case Libraries::Ime::ImeKbKeyAction::SpecialsMode:
            toggle_family_mode(ImeKbLayoutFamily::Specials);
            focus_keyboard_action_key(ImeKbKeyAction::SpecialsMode);
            break;
        case Libraries::Ime::ImeKbKeyAction::ArrowLeft:
            (void)move_text_caret(-1, text_select_mode);
            break;
        case Libraries::Ime::ImeKbKeyAction::ArrowRight:
            (void)move_text_caret(1, text_select_mode);
            break;
        case Libraries::Ime::ImeKbKeyAction::PagePrev:
            flip_mode_page(-1);
            break;
        case Libraries::Ime::ImeKbKeyAction::PageNext:
            flip_mode_page(1);
            break;
        case Libraries::Ime::ImeKbKeyAction::Space:
            (void)insert_text_at_caret(" ");
            break;
        case Libraries::Ime::ImeKbKeyAction::Backspace:
            (void)backspace_at_caret();
            break;
        case Libraries::Ime::ImeKbKeyAction::NewLine:
            if (metrics_cfg.multiline) {
                (void)insert_text_at_caret("\n");
            } else {
                accept_pressed = true;
            }
            break;
        case Libraries::Ime::ImeKbKeyAction::Menu:
            if (edit_menu_popup == EditMenuPopup::None) {
                open_main_menu();
                opened_menu_this_frame = true;
            } else {
                edit_menu_popup = EditMenuPopup::None;
            }
            break;
        case Libraries::Ime::ImeKbKeyAction::Settings:
            pointer_navigation_active = !pointer_navigation_active;
            if (!pointer_navigation_active) {
                if (native_input_active) {
                    native_input_active = false;
                    ImGui::ClearActiveID();
                }
                panel_selection = PanelSelectionTarget::Keyboard;
            }
            break;
        case Libraries::Ime::ImeKbKeyAction::Done:
            accept_pressed = true;
            break;
        default:
            break;
        }
        if (kb_state.done_pressed) {
            accept_pressed = true;
        }

        if (text_select_mode && edit_menu_popup == EditMenuPopup::None &&
            !pointer_navigation_active) {
            int delta = 0;
            if (nav_left || nav_up) {
                delta = -1;
            } else if (nav_right || nav_down) {
                delta = 1;
            }
            if (delta != 0) {
                const int len = text_length_utf16();
                if (text_select_anchor_utf16 < 0 || text_select_focus_utf16 < 0) {
                    const int caret = std::clamp(state->caret_index, 0, len);
                    text_select_anchor_utf16 = caret;
                    text_select_focus_utf16 = caret;
                }
                const int old_focus = std::clamp(text_select_focus_utf16, 0, len);
                text_select_focus_utf16 = std::clamp(text_select_focus_utf16 + delta, 0, len);
                apply_selection_state();
                emit_update_caret_events(old_focus, text_select_focus_utf16);
            } else if (panel_activate_pressed) {
                open_actions_menu();
                opened_menu_this_frame = true;
            }
        }

        if (edit_menu_popup != EditMenuPopup::None && cancel_pressed) {
            cancel_pressed = false;
            edit_menu_popup = EditMenuPopup::None;
        } else if (text_select_mode && cancel_pressed) {
            cancel_pressed = false;
            text_select_mode = false;
            const int len = text_length_utf16();
            const int caret = std::clamp(text_select_focus_utf16, 0, len);
            text_select_anchor_utf16 = caret;
            text_select_focus_utf16 = caret;
            apply_selection_state();
        }

        if (edit_menu_popup != EditMenuPopup::None) {
            constexpr std::array<const char*, 3> kMainMenuItems = {"Select", "Select All", "Paste"};
            constexpr std::array<const char*, 2> kActionMenuItems = {"Copy", "Paste"};
            const bool is_main_menu = (edit_menu_popup == EditMenuPopup::Main);
            const int item_count = is_main_menu ? static_cast<int>(kMainMenuItems.size())
                                                : static_cast<int>(kActionMenuItems.size());
            const bool clipboard_ready = has_clipboard_text();
            const auto item_label = [&](int index) -> const char* {
                return is_main_menu ? kMainMenuItems[static_cast<std::size_t>(index)]
                                    : kActionMenuItems[static_cast<std::size_t>(index)];
            };
            const auto item_enabled = [&](int index) {
                if (is_main_menu && index == 2) {
                    return clipboard_ready;
                }
                if (!is_main_menu && index == 1) {
                    return clipboard_ready;
                }
                return true;
            };

            if (!pointer_navigation_active) {
                if (nav_up) {
                    edit_menu_index = (edit_menu_index + item_count - 1) % item_count;
                } else if (nav_down) {
                    edit_menu_index = (edit_menu_index + 1) % item_count;
                }
            }
            edit_menu_index = std::clamp(edit_menu_index, 0, item_count - 1);

            const float menu_w = std::min(metrics.kb_size.x * 0.42f, 280.0f);
            const float menu_inner_pad = std::max(6.0f, metrics.key_gap * 0.7f);
            const float item_gap = std::max(3.0f, metrics.key_gap * 0.35f);
            const float item_h = std::max(28.0f, metrics.key_h * 0.70f);
            const float menu_h = menu_inner_pad * 2.0f + item_h * static_cast<float>(item_count) +
                                 item_gap * static_cast<float>(item_count - 1);
            const ImVec2 menu_pos{
                metrics.kb_pos.x + (metrics.kb_size.x - menu_w) * 0.5f,
                metrics.kb_pos.y + (metrics.kb_size.y - menu_h) * 0.5f,
            };
            const ImVec2 menu_max{menu_pos.x + menu_w, menu_pos.y + menu_h};
            draw->AddRectFilled(menu_pos, menu_max, IM_COL32(16, 16, 16, 245),
                                metrics.corner_radius);
            draw->AddRect(menu_pos, menu_max, IM_COL32(100, 100, 100, 255), metrics.corner_radius);

            const bool menu_activate = !opened_menu_this_frame && panel_activate_pressed;
            bool click_activate = false;
            for (int i = 0; i < item_count; ++i) {
                const float item_y = menu_pos.y + menu_inner_pad + i * (item_h + item_gap);
                const ImVec2 item_pos{menu_pos.x + menu_inner_pad, item_y};
                const ImVec2 item_size{menu_w - menu_inner_pad * 2.0f, item_h};
                const bool selected = (i == edit_menu_index);
                const bool enabled = item_enabled(i);
                const ImU32 item_bg = !enabled   ? IM_COL32(30, 30, 30, 255)
                                      : selected ? IM_COL32(60, 96, 146, 255)
                                                 : IM_COL32(45, 45, 45, 255);
                draw->AddRectFilled(item_pos, {item_pos.x + item_size.x, item_pos.y + item_size.y},
                                    item_bg, metrics.corner_radius * 0.6f);
                draw->AddRect(item_pos, {item_pos.x + item_size.x, item_pos.y + item_size.y},
                              IM_COL32(90, 90, 90, 255), metrics.corner_radius * 0.6f);

                ImGui::PushID(1000 + i);
                ImGui::SetCursorScreenPos(item_pos);
                ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
                ImGui::InvisibleButton("##ImeEditMenuItem", item_size);
                ImGui::PopItemFlag();
                if (ImGui::IsItemHovered()) {
                    edit_menu_index = i;
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && enabled) {
                    edit_menu_index = i;
                    click_activate = true;
                }
                ImGui::PopID();

                const char* label = item_label(i);
                const ImVec2 text_size = ImGui::CalcTextSize(label);
                const ImVec2 text_pos{
                    item_pos.x + (item_size.x - text_size.x) * 0.5f,
                    item_pos.y + (item_size.y - text_size.y) * 0.5f,
                };
                const ImU32 text_col =
                    enabled ? IM_COL32(232, 232, 232, 255) : IM_COL32(128, 128, 128, 255);
                draw->AddText(text_pos, text_col, label);
            }

            if (pointer_navigation_active && ImGui::IsMouseClicked(ImGuiMouseButton_Left, false) &&
                !ImGui::IsMouseHoveringRect(menu_pos, menu_max, false)) {
                edit_menu_popup = EditMenuPopup::None;
            } else if (menu_activate || click_activate) {
                if (item_enabled(edit_menu_index)) {
                    const EditMenuPopup previous_popup = edit_menu_popup;
                    if (previous_popup == EditMenuPopup::Main) {
                        apply_main_menu_action(edit_menu_index);
                    } else {
                        apply_actions_menu_action(edit_menu_index);
                    }
                    if (edit_menu_popup != EditMenuPopup::None &&
                        edit_menu_popup != previous_popup) {
                        opened_menu_this_frame = true;
                    }
                }
            }
        }

        Dummy({metrics.kb_size.x, metrics.kb_size.y + metrics.padding_bottom});

        if (accept_pressed) {
            state->SendEnterEvent();
        } else if (cancel_pressed) {
            state->SendCloseEvent();
        }

        prev_virtual_buttons = virtual_buttons;
        prev_virtual_cross_down = cross_down;
        prev_virtual_lstick_left_down = virtual_lstick_dirs.left;
        prev_virtual_lstick_right_down = virtual_lstick_dirs.right;
        prev_virtual_lstick_up_down = virtual_lstick_dirs.up;
        prev_virtual_lstick_down_down = virtual_lstick_dirs.down;
        lock_window_scroll();
        SetWindowFontScale(1.0f);
    }
    End();

    first_render = false;
}

bool ImeUi::DrawInputText(const ImePanelMetrics& metrics, bool pointer_selection_enabled) {
    const ImVec2 input_size = metrics.input_size;
    SetCursorPos(metrics.input_pos_local);
    if (request_input_focus) {
        SetKeyboardFocusHere();
        request_input_focus = false;
    }

    if (state->caret_dirty && !text_select_mode) {
        const char* text = state->current_text.begin();
        const int len_utf16 =
            Utf16CountFromUtf8Range(text, text + static_cast<int>(state->current_text.size()));
        int caret_utf16 = state->caret_index;
        if (caret_utf16 < 0) {
            caret_utf16 = 0;
        } else if (caret_utf16 > len_utf16) {
            caret_utf16 = len_utf16;
        }
        const int caret_byte = Utf8ByteIndexFromUtf16Index(text, caret_utf16);
        state->caret_index = caret_utf16;
        state->caret_byte_index = caret_byte;
        input_cursor_utf16 = caret_utf16;
        input_cursor_byte = caret_byte;
        input_selection_start_byte = caret_byte;
        input_selection_end_byte = caret_byte;
        text_select_anchor_utf16 = caret_utf16;
        text_select_focus_utf16 = caret_utf16;
        if (native_input_active) {
            pending_input_selection_apply = true;
            request_input_focus = true;
        } else {
            pending_input_selection_apply = false;
            request_input_focus = false;
            state->caret_dirty = false;
        }
    }

    const ImVec2 rect_min = metrics.input_pos_screen;
    const ImVec2 rect_max{rect_min.x + input_size.x, rect_min.y + input_size.y};
    const bool clicked_input = IsMouseClicked(ImGuiMouseButton_Left, false) &&
                               IsMouseHoveringRect(rect_min, rect_max, false);
    if (clicked_input) {
        native_input_active = true;
        SetKeyboardFocusHere();
    }

    ImGuiInputTextFlags flags =
        ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_CallbackCharFilter;
    if (!native_input_active) {
        flags |= ImGuiInputTextFlags_ReadOnly;
    }

    PushStyleColor(ImGuiCol_FrameBg, ImeColorToImVec4(style_config.color_text_field));
    PushStyleColor(ImGuiCol_FrameBgHovered, ImeColorToImVec4(style_config.color_preedit));
    PushStyleColor(ImGuiCol_FrameBgActive, ImeColorToImVec4(style_config.color_preedit));
    PushStyleColor(ImGuiCol_Text, ImeColorToImVec4(style_config.color_text));
    PushItemFlag(ImGuiItemFlags_NoNav, true);
    if (InputTextExLimited("##ImeInput", nullptr, state->current_text.begin(),
                           state->max_text_length * 4 + 1, input_size, flags,
                           state->max_text_length, InputTextCallback, this)) {
    }
    PopItemFlag();
    PopStyleColor(4);

    const ImRect frame_rect = {GetItemRectMin(), GetItemRectMax()};
    if (!IsItemActive()) {
        DrawInactiveCaretOverlay(frame_rect, state->current_text.begin(), input_cursor_byte,
                                 input_selection_start_byte, input_selection_end_byte);
    }

    const bool hovered = IsItemHovered();
    if (IsMouseClicked(ImGuiMouseButton_Left, false) && !hovered && native_input_active) {
        native_input_active = false;
    }
    return pointer_selection_enabled && (hovered || IsItemActive() || clicked_input);
}

int ImeUi::InputTextCallback(ImGuiInputTextCallbackData* data) {
    ImeUi* ui = static_cast<ImeUi*>(data->UserData);
    ASSERT(ui);

    if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
        if (!ui->state) {
            return 1;
        }
        const int max_utf16 = static_cast<int>(ui->state->max_text_length);
        if (RejectInputCharByUtf16Limit(data, max_utf16)) {
            return 1;
        }
        return 0;
    }

    static const ImeUi* last_ui = nullptr;
    static std::u16string last_text;
    static int lastCaretPos = -1;
    int buf_len = std::max(0, data->BufTextLen);
    if (!ui->native_input_active) {
        ui->request_input_select_all = false;
        ui->pending_input_selection_apply = false;
        last_ui = nullptr;
        last_text.clear();
        lastCaretPos = -1;
        const int cursor_byte = std::clamp(ui->input_cursor_byte, 0, buf_len);
        const int selection_start = std::clamp(ui->input_selection_start_byte, 0, buf_len);
        const int selection_end = std::clamp(ui->input_selection_end_byte, 0, buf_len);
        data->CursorPos = cursor_byte;
        data->SelectionStart = selection_start;
        data->SelectionEnd = selection_end;
        return 0;
    }

    if (ui->request_input_select_all) {
        data->SelectAll();
        ui->request_input_select_all = false;
    }

    if (ui->pending_input_selection_apply) {
        const int len_chars = Utf16CountFromUtf8Range(data->Buf, data->Buf + buf_len);
        int anchor = ui->text_select_anchor_utf16;
        int focus = ui->text_select_focus_utf16;
        if (anchor < 0 || focus < 0) {
            const int caret_utf16 = Utf16CountFromUtf8Range(data->Buf, data->Buf + data->CursorPos);
            anchor = caret_utf16;
            focus = caret_utf16;
        }
        anchor = std::clamp(anchor, 0, len_chars);
        focus = std::clamp(focus, 0, len_chars);
        const int anchor_byte = Utf8ByteIndexFromUtf16Index(data->Buf, anchor);
        const int focus_byte = Utf8ByteIndexFromUtf16Index(data->Buf, focus);
        data->CursorPos = focus_byte;
        data->SelectionStart = std::min(anchor_byte, focus_byte);
        data->SelectionEnd = std::max(anchor_byte, focus_byte);
        ui->text_select_anchor_utf16 = anchor;
        ui->text_select_focus_utf16 = focus;
        ui->pending_input_selection_apply = false;
    }

    bool caret_set_from_api = false;
    if (ui->state->caret_dirty && !ui->text_select_mode) {
        const int len_chars = Utf16CountFromUtf8Range(data->Buf, data->Buf + buf_len);
        int caret = ui->state->caret_index;
        if (caret < 0) {
            caret = 0;
        } else if (caret > len_chars) {
            caret = len_chars;
        }
        const int caret_byte = Utf8ByteIndexFromUtf16Index(data->Buf, caret);
        data->CursorPos = caret_byte;
        data->SelectionStart = caret_byte;
        data->SelectionEnd = caret_byte;
        ui->state->caret_index = caret;
        ui->state->caret_byte_index = caret_byte;
        ui->state->caret_dirty = false;
        caret_set_from_api = true;
    }

    if (ClampInputBufferToUtf16Limit(data, static_cast<int>(ui->state->max_text_length))) {
        buf_len = std::max(0, data->BufTextLen);
    }

    const int cursor_byte = std::clamp(data->CursorPos, 0, buf_len);

    constexpr std::size_t kImeTextCapacity = ORBIS_IME_MAX_TEXT_LENGTH + 1;
    const std::size_t max_orbis_len = std::min<std::size_t>(
        static_cast<std::size_t>(ui->state->max_text_length) + 1, kImeTextCapacity);

    if (last_ui != ui) {
        last_ui = ui;
        std::array<char16_t, kImeTextCapacity> snapshot{};
        if (ui->state->ConvertUTF8ToOrbis(data->Buf, data->BufTextLen, snapshot.data(),
                                          max_orbis_len)) {
            last_text.assign(snapshot.data());
        } else {
            last_text.clear();
        }
        lastCaretPos = cursor_byte;
    }

    std::array<char16_t, kImeTextCapacity> current_text_u16{};
    if (!ui->state->ConvertUTF8ToOrbis(data->Buf, data->BufTextLen, current_text_u16.data(),
                                       max_orbis_len)) {
        LOG_ERROR(Lib_Ime, "Failed to convert UTF-8 to Orbis for current text");
        return 0;
    }
    std::u16string current_text(current_text_u16.data());

    if (current_text != last_text) {
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

        std::size_t prefix = 0;
        while (prefix < last_text.size() && prefix < current_text.size() &&
               last_text[prefix] == current_text[prefix]) {
            ++prefix;
        }

        std::size_t old_tail = last_text.size();
        std::size_t new_tail = current_text.size();
        while (old_tail > prefix && new_tail > prefix &&
               last_text[old_tail - 1] == current_text[new_tail - 1]) {
            --old_tail;
            --new_tail;
        }

        const s32 removed = static_cast<s32>(old_tail - prefix);
        const s32 inserted = static_cast<s32>(new_tail - prefix);
        eventParam.caret_index =
            static_cast<u32>(Utf16CountFromUtf8Range(data->Buf, data->Buf + cursor_byte));
        eventParam.text_area[0].index = static_cast<u32>(prefix);
        eventParam.text_area[0].length = inserted - removed;

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

        last_text = current_text;
        lastCaretPos = cursor_byte;
        ui->state->SendEvent(&event);
    }

    if (lastCaretPos == -1) {
        lastCaretPos = cursor_byte;
    } else if (cursor_byte != lastCaretPos) {
        const int old_cursor_byte = std::clamp(lastCaretPos, 0, buf_len);
        const int old_cursor_utf16 =
            Utf16CountFromUtf8Range(data->Buf, data->Buf + old_cursor_byte);
        const int new_cursor_utf16 = Utf16CountFromUtf8Range(data->Buf, data->Buf + cursor_byte);
        const int delta = new_cursor_utf16 - old_cursor_utf16;

        if (delta != 0 && !caret_set_from_api) {
            // Emit one UpdateCaret per UTF-16 unit step (delta may be ±1 or a jump).
            const bool move_right = delta > 0;
            const u32 steps = static_cast<u32>(std::abs(delta));
            OrbisImeCaretMovementDirection dir = move_right ? OrbisImeCaretMovementDirection::Right
                                                            : OrbisImeCaretMovementDirection::Left;

            for (u32 i = 0; i < steps; ++i) {
                OrbisImeEvent caret_step{};
                caret_step.id = OrbisImeEventId::UpdateCaret;
                caret_step.param.caret_move = dir;
                LOG_DEBUG(Lib_Ime, "IME Event queued: UpdateCaret(step {}/{}), dir={}", i + 1,
                          steps, static_cast<u32>(dir));
                ui->state->SendEvent(&caret_step);
            }
        }

        lastCaretPos = cursor_byte;
    }

    const int selection_start_byte =
        std::clamp(std::min(data->SelectionStart, data->SelectionEnd), 0, buf_len);
    const int selection_end_byte =
        std::clamp(std::max(data->SelectionStart, data->SelectionEnd), 0, buf_len);
    ui->input_cursor_byte = cursor_byte;
    ui->input_cursor_utf16 = Utf16CountFromUtf8Range(data->Buf, data->Buf + cursor_byte);
    ui->input_selection_start_byte = selection_start_byte;
    ui->input_selection_end_byte = selection_end_byte;
    ui->state->caret_byte_index = cursor_byte;
    ui->state->caret_index = ui->input_cursor_utf16;
    ui->state->caret_dirty = false;

    if (!ui->text_select_mode) {
        ui->text_select_anchor_utf16 = ui->input_cursor_utf16;
        ui->text_select_focus_utf16 = ui->input_cursor_utf16;
    }

    return 0;
}

void ImeUi::Free() {
    RemoveLayer(this);
    if (gamepad_input_capture_active) {
        ImGui::Core::ReleaseGamepadInputCapture();
        gamepad_input_capture_active = false;
    }
}

}; // namespace Libraries::Ime
