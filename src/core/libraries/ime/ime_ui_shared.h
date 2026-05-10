// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <imgui_internal.h>

#include "common/types.h"
#include "core/libraries/ime/ime_kb_layout.h"
#include "core/libraries/pad/pad.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Ime {

struct VirtualPadSnapshot {
    u32 buttons = 0;
    ImVec2 left_stick{};
    ImVec2 panel_delta{};
    float l2_analog = 0.0f;
};

constexpr double kPanelEdgeWrapHoldDelaySec = 0.5;
constexpr double kRepeatIntentWindowSec = 0.45;
constexpr float kSelectorFadeOutDurationSec = 0.2f;
constexpr float kSelectorPressPulseDurationSec = 0.2f;
constexpr float kSelectorPressPulseExpandBorderFactor = 1.0f;

ImVec4 BrightenColor(ImU32 color, float delta);
int Utf16CountFromUtf8Range(const char* text, const char* end = nullptr);
int Utf8ByteIndexFromUtf16Index(const char* text, int utf16_index);
bool RejectInputCharByUtf16Limit(const ImGuiInputTextCallbackData* data, int max_utf16);
bool ClampInputBufferToUtf16Limit(ImGuiInputTextCallbackData* data, int max_utf16);
void DrawInactiveCaretOverlay(const ImRect& frame_rect, const char* text, int caret_byte,
                              int selection_start_byte, int selection_end_byte,
                              bool multiline = false);

struct SelectorFadeState {
    ImVec2 current_min{};
    ImVec2 current_max{};
    float current_corner_radius = 0.0f;
    bool current_visible = false;

    ImVec2 previous_min{};
    ImVec2 previous_max{};
    float previous_corner_radius = 0.0f;
    bool previous_visible = false;
    double previous_started_at = 0.0;
    double press_pulse_started_at = -1.0;
};

void TriggerSelectorPressPulse(SelectorFadeState& state, double now);
float ComputePressPulseExpand(double pulse_started_at, double now, float pulse_duration_sec,
                              float max_expand_px);
void UpdateSelectorFadeState(SelectorFadeState& state, ImVec2 pos, ImVec2 size, float inset,
                             float corner_radius, bool selected, double now);
void DrawSelectorFadeState(const SelectorFadeState& state, ImDrawList* draw_list,
                           ImU32 overlay_color, ImU32 border_color, float border_thickness,
                           float fade_duration_sec, double now, float current_expand_px = 0.0f);

ImeKbLayoutSelection ResolveInitialKbLayoutSelection(OrbisImeExtOption ext_option,
                                                     OrbisImePanelPriority panel_priority);
void InitializeDefaultOskSelectionAnchor(const ImeKbLayoutSelection& layout_selection,
                                         OrbisImeExtOption ext_option, int& pending_row,
                                         int& pending_col, int& last_row, int& last_col);
VirtualPadSnapshot ReadVirtualPadSnapshot(Libraries::UserService::OrbisUserServiceUserId user_id,
                                          float delta_time, bool include_imgui_fallback = true);

struct OskPadInputState {
    u32& prev_virtual_buttons;
    bool& prev_virtual_cross_down;
    bool& prev_virtual_lstick_left_down;
    bool& prev_virtual_lstick_right_down;
    bool& prev_virtual_lstick_up_down;
    bool& prev_virtual_lstick_down_down;
    int& left_stick_repeat_dir;
    double& left_stick_next_repeat_time;
    double& virtual_cross_next_repeat_time;
    bool& prev_virtual_dpad_left_down;
    bool& prev_virtual_dpad_right_down;
    bool& prev_virtual_dpad_up_down;
    bool& prev_virtual_dpad_down_down;
    double& virtual_dpad_left_next_repeat_time;
    double& virtual_dpad_right_next_repeat_time;
    double& virtual_dpad_up_next_repeat_time;
    double& virtual_dpad_down_next_repeat_time;
};

struct OskPadInputFrame {
    u32 virtual_buttons = 0;
    u32 prev_virtual_buttons = 0;
    struct {
        bool left = false;
        bool right = false;
        bool up = false;
        bool down = false;
    } virtual_lstick_dirs{};
    bool cross_down = false;
    float l2_analog = 0.0f;
    bool panel_activate_pressed_raw = false;
    bool panel_activate_repeat_raw = false;
    bool virtual_nav_left = false;
    bool virtual_nav_right = false;
    bool virtual_nav_up = false;
    bool virtual_nav_down = false;
    bool virtual_nav_left_repeat = false;
    bool virtual_nav_right_repeat = false;
    bool virtual_nav_up_repeat = false;
    bool virtual_nav_down_repeat = false;
    bool stick_nav_left = false;
    bool stick_nav_right = false;
    bool stick_nav_up = false;
    bool stick_nav_down = false;
    bool stick_nav_left_repeat = false;
    bool stick_nav_right_repeat = false;
    bool stick_nav_up_repeat = false;
    bool stick_nav_down_repeat = false;
    bool nav_left = false;
    bool nav_right = false;
    bool nav_up = false;
    bool nav_down = false;
    bool nav_left_repeat = false;
    bool nav_right_repeat = false;
    bool nav_up_repeat = false;
    bool nav_down_repeat = false;
    bool raw_osk_control_input = false;
    bool virtual_control_input = false;
    bool osk_control_input = false;
};

struct OskVirtualPadInputView {
    const OskPadInputFrame& frame;
    double repeat_delay = 0.0;
    double repeat_rate = 0.0;

    explicit OskVirtualPadInputView(const OskPadInputFrame& input_frame, const ImGuiIO& io);
    bool Down(Libraries::Pad::OrbisPadButtonDataOffset button) const;
    bool Pressed(Libraries::Pad::OrbisPadButtonDataOffset button) const;
    bool RepeatPressed(Libraries::Pad::OrbisPadButtonDataOffset button, bool& prev_down_state,
                       double& next_repeat_time, bool* out_repeat = nullptr) const;
};

struct OskShortcutRepeatState {
    bool& prev_square_down;
    bool& prev_l1_down;
    bool& prev_r1_down;
    bool& l2_shortcut_armed;
    double& square_next_repeat_time;
    double& l1_next_repeat_time;
    double& r1_next_repeat_time;
    double& triangle_next_repeat_time;
};

struct OskShortcutActionResult {
    ImeKbKeyAction action = ImeKbKeyAction::None;
    bool clear_all = false;
};

OskShortcutActionResult EvaluateOskShortcutAction(bool allow_osk_shortcuts, bool menu_modal,
                                                  bool evaluate_action,
                                                  const OskPadInputFrame& panel_input,
                                                  const OskVirtualPadInputView& virtual_pad_input,
                                                  u32 prev_virtual_buttons,
                                                  ImeKbLayoutFamily layout_family,
                                                  OskShortcutRepeatState& repeat_state);

void CycleKeyboardCaseState(ImeKbLayoutSelection& selection);
void ToggleKeyboardFamilyMode(ImeKbLayoutSelection& selection, ImeKbLayoutFamily& alpha_family,
                              ImeKbLayoutFamily target_family);
bool FocusKeyboardActionKeySelection(const ImeKbLayoutSelection& selection, ImeKbKeyAction action,
                                     int& out_row, int& out_col);
void FlipKeyboardModePage(ImeKbLayoutSelection& selection, int direction);

template <typename KeyboardDrawParams>
inline void ApplyOskPanelNavToKeyboardParams(KeyboardDrawParams& kb_params,
                                             const bool allow_osk_shortcuts,
                                             const OskPadInputFrame& panel_input) {
    kb_params.external_nav_left =
        allow_osk_shortcuts && (panel_input.virtual_nav_left || panel_input.stick_nav_left);
    kb_params.external_nav_right =
        allow_osk_shortcuts && (panel_input.virtual_nav_right || panel_input.stick_nav_right);
    kb_params.external_nav_up =
        allow_osk_shortcuts && (panel_input.virtual_nav_up || panel_input.stick_nav_up);
    kb_params.external_nav_down =
        allow_osk_shortcuts && (panel_input.virtual_nav_down || panel_input.stick_nav_down);
    kb_params.external_nav_left_repeat =
        allow_osk_shortcuts &&
        ((panel_input.virtual_nav_left && panel_input.virtual_nav_left_repeat) ||
         (panel_input.stick_nav_left && panel_input.stick_nav_left_repeat));
    kb_params.external_nav_right_repeat =
        allow_osk_shortcuts &&
        ((panel_input.virtual_nav_right && panel_input.virtual_nav_right_repeat) ||
         (panel_input.stick_nav_right && panel_input.stick_nav_right_repeat));
    kb_params.external_nav_up_repeat =
        allow_osk_shortcuts && ((panel_input.virtual_nav_up && panel_input.virtual_nav_up_repeat) ||
                                (panel_input.stick_nav_up && panel_input.stick_nav_up_repeat));
    kb_params.external_nav_down_repeat =
        allow_osk_shortcuts &&
        ((panel_input.virtual_nav_down && panel_input.virtual_nav_down_repeat) ||
         (panel_input.stick_nav_down && panel_input.stick_nav_down_repeat));
}

OskPadInputFrame ComputeOskPadInputFrame(const VirtualPadSnapshot& virtual_pad,
                                         bool allow_osk_shortcuts, bool first_render,
                                         OskPadInputState& state);
void CommitOskPadInputFrame(const OskPadInputFrame& frame, OskPadInputState& state);

void DisarmMenuActivate(bool& menu_activate_armed);
void RearmMenuActivateOnRelease(bool activate_down, bool& menu_activate_armed);
bool ConsumeMenuActivatePress(bool panel_activate_pressed, bool opened_menu_this_frame,
                              bool& menu_activate_armed);

template <typename EditMenuPopupT>
inline void OpenOskMainEditMenu(EditMenuPopupT& popup, int& edit_menu_index,
                                bool& menu_activate_armed) {
    popup = EditMenuPopupT::Main;
    edit_menu_index = 0;
    DisarmMenuActivate(menu_activate_armed);
}

template <typename EditMenuPopupT>
inline void OpenOskActionsEditMenu(EditMenuPopupT& popup, int& edit_menu_index,
                                   bool& menu_activate_armed) {
    popup = EditMenuPopupT::Actions;
    edit_menu_index = 0;
    DisarmMenuActivate(menu_activate_armed);
}

template <typename EditMenuPopupT>
inline bool CloseOskEditMenuOnCancel(EditMenuPopupT& popup, bool& cancel_pressed,
                                     bool& menu_activate_armed) {
    if (popup == EditMenuPopupT::None || !cancel_pressed) {
        return false;
    }
    cancel_pressed = false;
    popup = EditMenuPopupT::None;
    menu_activate_armed = true;
    return true;
}

template <typename EditMenuPopupT, typename PanelMetricsT, typename ApplyActionFn>
inline bool DrawAndHandleOskEditMenuPopup(
    EditMenuPopupT& popup, int& edit_menu_index, const PanelMetricsT& metrics, ImDrawList* draw,
    bool pointer_navigation_active, bool nav_up, bool nav_down, bool cross_down,
    bool panel_activate_pressed, bool opened_menu_this_frame, bool& menu_activate_armed,
    bool clipboard_ready, int id_base, const char* item_button_id, bool close_on_outside_click,
    ApplyActionFn&& apply_action) {
    if (popup == EditMenuPopupT::None || draw == nullptr) {
        return false;
    }

    constexpr std::array<const char*, 3> kMainMenuItems = {"Select", "Select All", "Paste"};
    constexpr std::array<const char*, 2> kActionMenuItems = {"Copy", "Paste"};
    const bool is_main_menu = (popup == EditMenuPopupT::Main);
    const int item_count = is_main_menu ? static_cast<int>(kMainMenuItems.size())
                                        : static_cast<int>(kActionMenuItems.size());
    const auto item_label = [&](int index) -> const char* {
        return is_main_menu ? kMainMenuItems[static_cast<std::size_t>(index)]
                            : kActionMenuItems[static_cast<std::size_t>(index)];
    };
    const auto item_enabled = [&](int index) {
        if ((is_main_menu && index == 2) || (!is_main_menu && index == 1)) {
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
    draw->AddRectFilled(menu_pos, menu_max, IM_COL32(16, 16, 16, 245), metrics.corner_radius);
    draw->AddRect(menu_pos, menu_max, IM_COL32(100, 100, 100, 255), metrics.corner_radius);

    RearmMenuActivateOnRelease(cross_down, menu_activate_armed);
    const bool menu_activate = ConsumeMenuActivatePress(
        panel_activate_pressed, opened_menu_this_frame, menu_activate_armed);
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
        draw->AddRectFilled(item_pos, {item_pos.x + item_size.x, item_pos.y + item_size.y}, item_bg,
                            metrics.corner_radius * 0.6f);
        draw->AddRect(item_pos, {item_pos.x + item_size.x, item_pos.y + item_size.y},
                      IM_COL32(90, 90, 90, 255), metrics.corner_radius * 0.6f);

        ImGui::PushID(id_base + i);
        ImGui::SetCursorScreenPos(item_pos);
        ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
        ImGui::InvisibleButton(item_button_id, item_size);
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

    const bool outside_click_close = close_on_outside_click && pointer_navigation_active &&
                                     !opened_menu_this_frame &&
                                     ImGui::IsMouseClicked(ImGuiMouseButton_Left, false) &&
                                     !ImGui::IsMouseHoveringRect(menu_pos, menu_max, false);
    if (outside_click_close) {
        popup = EditMenuPopupT::None;
        menu_activate_armed = true;
        return true;
    }

    if ((menu_activate || click_activate) && item_enabled(edit_menu_index)) {
        const EditMenuPopupT previous_popup = popup;
        apply_action(previous_popup, edit_menu_index);
        if (popup != EditMenuPopupT::None && popup != previous_popup) {
            edit_menu_index = 0;
        }
        if (popup == EditMenuPopupT::None) {
            menu_activate_armed = true;
        }
    }
    return true;
}

} // namespace Libraries::Ime
