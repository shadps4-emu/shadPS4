// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <imgui.h>
#include <queue>
#include "imgui/imgui_layer.h"

#include "common/cstring.h"
#include "common/types.h"

#include "core/libraries/ime/ime_kb_layout.h"
#include "ime.h"

namespace Libraries::Ime {

class ImeHandler;
class ImeUi;
struct ImePanelMetrics;

class ImeState {
    friend class ImeHandler;
    friend class ImeUi;

    void* work_buffer{};
    char16_t* text_buffer{};
    u32 max_text_length = 0;

    // A character can hold up to 4 bytes in UTF-8
    Common::CString<ORBIS_IME_MAX_TEXT_LENGTH * 4 + 1> current_text;
    int caret_index = 0;
    int caret_byte_index = 0;
    bool caret_dirty = false;

    std::queue<OrbisImeEvent> event_queue;
    std::mutex queue_mutex;

public:
    ImeState(const OrbisImeParam* param = nullptr, const OrbisImeParamExtended* extended = nullptr);
    ImeState(ImeState&& other) noexcept;
    ImeState& operator=(ImeState&& other) noexcept;

    void SendEvent(OrbisImeEvent* event);
    void SendEnterEvent();
    void SendCloseEvent();

    void SetText(const char16_t* text, u32 length);
    void SetCaret(u32 position);

private:
    bool ConvertOrbisToUTF8(const char16_t* orbis_text, std::size_t orbis_text_len, char* utf8_text,
                            std::size_t native_text_len);
    bool ConvertUTF8ToOrbis(const char* native_text, std::size_t utf8_text_len,
                            char16_t* orbis_text, std::size_t orbis_text_len);
};

class ImeUi : public ImGui::Layer {
    enum class PanelSelectionTarget : u8 {
        Input = 0,
        Prediction = 1,
        Close = 2,
        Keyboard = 3,
    };

    enum class EditMenuPopup : u8 {
        None = 0,
        Main = 1,
        Actions = 2,
    };

    ImeState* state{};
    const OrbisImeParam* ime_param{};
    const OrbisImeParamExtended* extended_param{};
    ImeStyleConfig style_config{};

    bool first_render = true;
    bool accept_armed = false;
    bool native_input_active = false;
    bool pointer_navigation_active = true;
    EditMenuPopup edit_menu_popup = EditMenuPopup::None;
    bool menu_activate_armed = true;
    bool l2_shortcut_armed = true;
    bool request_input_focus = false;
    bool request_input_select_all = false;
    bool text_select_mode = false;
    bool pending_input_selection_apply = false;
    bool prev_virtual_cross_down = false;
    bool prev_virtual_lstick_left_down = false;
    bool prev_virtual_lstick_right_down = false;
    bool prev_virtual_lstick_up_down = false;
    bool prev_virtual_lstick_down_down = false;
    int left_stick_repeat_dir = 0;
    double left_stick_next_repeat_time = 0.0;
    double virtual_cross_next_repeat_time = 0.0;
    double virtual_triangle_next_repeat_time = 0.0;
    u32 prev_virtual_buttons = 0;
    bool prev_virtual_square_down = false;
    bool prev_virtual_l1_down = false;
    bool prev_virtual_r1_down = false;
    bool prev_virtual_dpad_left_down = false;
    bool prev_virtual_dpad_right_down = false;
    bool prev_virtual_dpad_up_down = false;
    bool prev_virtual_dpad_down_down = false;
    double virtual_square_next_repeat_time = 0.0;
    double virtual_l1_next_repeat_time = 0.0;
    double virtual_r1_next_repeat_time = 0.0;
    double virtual_dpad_left_next_repeat_time = 0.0;
    double virtual_dpad_right_next_repeat_time = 0.0;
    double virtual_dpad_up_next_repeat_time = 0.0;
    double virtual_dpad_down_next_repeat_time = 0.0;
    ImeEdgeWrapNavState panel_vertical_nav_state{};
    bool panel_position_initialized = false;
    bool panel_drag_active = false;
    bool gamepad_input_capture_active = false;
    ImVec2 panel_position{};
    int input_cursor_utf16 = 0;
    int input_cursor_byte = 0;
    int input_selection_start_byte = 0;
    int input_selection_end_byte = 0;
    int text_select_anchor_utf16 = -1;
    int text_select_focus_utf16 = -1;
    int top_virtual_col = 0;
    PanelSelectionTarget panel_selection = PanelSelectionTarget::Keyboard;
    int pending_keyboard_row = -1;
    int pending_keyboard_col = -1;
    int last_keyboard_selected_row = 0;
    int last_keyboard_selected_col = 0;
    int edit_menu_index = 0;
    ImeKbLayoutSelection kb_layout_selection{};
    ImeKbLayoutSelection last_nav_layout_selection{};
    bool nav_layout_selection_initialized = false;
    ImeKbLayoutFamily kb_alpha_family = ImeKbLayoutFamily::Latin;
    std::mutex draw_mutex;

public:
    explicit ImeUi(ImeState* state = nullptr, const OrbisImeParam* param = nullptr,
                   const OrbisImeParamExtended* extended = nullptr);
    ~ImeUi() override;
    ImeUi(const ImeUi& other) = delete;
    ImeUi& operator=(ImeUi&& other);

    void Draw() override;

private:
    void Free();

    bool DrawInputText(const ImePanelMetrics& metrics, bool pointer_selection_enabled);

    static int InputTextCallback(ImGuiInputTextCallbackData* data);
};

}; // namespace Libraries::Ime
