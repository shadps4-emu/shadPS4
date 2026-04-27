// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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

ImVec4 BrightenColor(ImU32 color, float delta);
int Utf16CountFromUtf8Range(const char* text, const char* end = nullptr);
int Utf8ByteIndexFromUtf16Index(const char* text, int utf16_index);
bool RejectInputCharByUtf16Limit(const ImGuiInputTextCallbackData* data, int max_utf16);
bool ClampInputBufferToUtf16Limit(ImGuiInputTextCallbackData* data, int max_utf16);
void DrawInactiveCaretOverlay(const ImRect& frame_rect, const char* text, int caret_byte,
                              int selection_start_byte, int selection_end_byte,
                              bool multiline = false);
ImeKbLayoutSelection ResolveInitialKbLayoutSelection(OrbisImeExtOption ext_option,
                                                     OrbisImePanelPriority panel_priority);
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

void ResetOskShortcutRepeatState(OskShortcutRepeatState& state);
bool ConsumeTriggerShortcutPress(bool trigger_down, bool trigger_edge_pressed,
                                 float trigger_analog_value, bool& shortcut_armed);
OskShortcutActionResult EvaluateOskShortcutAction(bool allow_osk_shortcuts, bool menu_modal,
                                                  bool evaluate_action,
                                                  const OskPadInputFrame& panel_input,
                                                  const OskVirtualPadInputView& virtual_pad_input,
                                                  u32 prev_virtual_buttons,
                                                  ImeKbLayoutFamily layout_family,
                                                  OskShortcutRepeatState& repeat_state);

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

} // namespace Libraries::Ime
