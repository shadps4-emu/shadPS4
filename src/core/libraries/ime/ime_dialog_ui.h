// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <vector>
#include <imgui.h>
#include "common/cstring.h"
#include "common/types.h"
#include "core/libraries/ime/ime_dialog.h"
#include "core/libraries/ime/ime_kb_layout.h"
#include "imgui/imgui_layer.h"

namespace Libraries::ImeDialog {

class ImeDialogUi;
} // namespace Libraries::ImeDialog

namespace Libraries::Ime {
struct ImePanelMetrics;
}

namespace Libraries::ImeDialog {

class ImeDialogState final {
    friend ImeDialogUi;

    bool input_changed = false;
    int caret_index = 0;
    int caret_byte_index = 0;
    bool caret_dirty = false;
    bool use_over2k = false;
    OrbisImePositionAndForm panel_layout{};
    bool panel_layout_valid = false;
    u32 panel_req_width = 0;
    u32 panel_req_height = 0;
    OrbisImeExtOption ext_option = OrbisImeExtOption::DEFAULT;
    OrbisImeDisableDevice disable_device = OrbisImeDisableDevice::DEFAULT;
    OrbisImePanelPriority panel_priority = OrbisImePanelPriority::Default;
    Libraries::Ime::ImeStyleConfig style_config{};

    s32 user_id{};
    bool is_multi_line{};
    bool is_numeric{};
    bool fixed_position{};
    OrbisImeType type{};
    OrbisImeLanguage supported_languages{};
    OrbisImeEnterLabel enter_label{};
    OrbisImeTextFilter text_filter{};
    OrbisImeExtKeyboardFilter keyboard_filter{};
    u32 max_text_length{};
    char16_t* text_buffer{};
    std::vector<char16_t> original_text;
    std::vector<char> title;
    std::vector<char> placeholder;

    // A character can hold up to 4 bytes in UTF-8
    Common::CString<ORBIS_IME_DIALOG_MAX_TEXT_LENGTH * 4 + 1> current_text;

public:
    /*
     * Use default constructor ImeDialogState() to initialize default values instead of
     * ImeDialogState(const OrbisImeDialogParam* param = nullptr,const OrbisImeParamExtended*
     * extended = nullptr) to avoid validation errors in log
     */
    ImeDialogState();
    ImeDialogState(const OrbisImeDialogParam* param /*= nullptr*/,
                   const OrbisImeParamExtended* extended /*= nullptr*/);
    ImeDialogState(const ImeDialogState& other) = delete;
    ImeDialogState(ImeDialogState&& other) noexcept;
    ImeDialogState& operator=(ImeDialogState&& other);

    bool CopyTextToOrbisBuffer(bool use_original);
    bool CallTextFilter();
    bool NormalizeNewlines();
    bool ClampCurrentTextToMaxLen();

private:
    bool CallKeyboardFilter(const OrbisImeKeycode* src_keycode, u16* out_keycode, u32* out_status);

    bool ConvertOrbisToUTF8(const char16_t* orbis_text, std::size_t orbis_text_len, char* utf8_text,
                            std::size_t native_text_len);
    bool ConvertUTF8ToOrbis(const char* native_text, std::size_t utf8_text_len,
                            char16_t* orbis_text, std::size_t orbis_text_len);
};

class ImeDialogUi final : public ImGui::Layer {
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

    ImeDialogState* state{};
    OrbisImeDialogStatus* status{};
    OrbisImeDialogResult* result{};

    bool first_render = true;
    bool accept_armed = false;
    bool native_input_active = false;
    bool pointer_navigation_active = true;
    EditMenuPopup edit_menu_popup = EditMenuPopup::None;
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
    u32 prev_virtual_buttons = 0;
    bool prev_virtual_square_down = false;
    bool prev_virtual_l1_down = false;
    bool prev_virtual_r1_down = false;
    double virtual_square_next_repeat_time = 0.0;
    double virtual_l1_next_repeat_time = 0.0;
    double virtual_r1_next_repeat_time = 0.0;
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
    Libraries::Ime::ImeKbLayoutSelection kb_layout_selection{};
    std::mutex draw_mutex;

public:
    explicit ImeDialogUi(ImeDialogState* state = nullptr, OrbisImeDialogStatus* status = nullptr,
                         OrbisImeDialogResult* result = nullptr);
    ~ImeDialogUi() override;
    ImeDialogUi(const ImeDialogUi& other) = delete;
    ImeDialogUi(ImeDialogUi&& other) noexcept;
    ImeDialogUi& operator=(ImeDialogUi&& other);

    void Draw() override;

private:
    void FinishDialog(OrbisImeDialogEndStatus endstatus, bool restore_original, const char* reason);
    void Free();

    bool DrawInputText(const Libraries::Ime::ImePanelMetrics& metrics,
                       bool pointer_selection_enabled);
    bool DrawMultiLineInputText(const Libraries::Ime::ImePanelMetrics& metrics,
                                bool pointer_selection_enabled);

    static int InputTextCallback(ImGuiInputTextCallbackData* data);
};

} // namespace Libraries::ImeDialog
