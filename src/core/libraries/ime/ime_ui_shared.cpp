// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ime/ime_ui_shared.h"

#include <cmath>
#include <cstring>

#include "common/singleton.h"
#include "input/controller.h"

namespace Libraries::Ime {

namespace {

enum class StickNavDirection : int {
    None = 0,
    Left = 1,
    Right = 2,
    Up = 3,
    Down = 4,
};

constexpr float kNavAxisThreshold = 0.50f;
constexpr double kStickNavInitialDelaySlow = 0.26;
constexpr double kStickNavInitialDelayFast = 0.16;
constexpr double kStickNavRepeatSlow = 0.20;
constexpr double kStickNavRepeatFast = 0.11;

struct VirtualButtonRepeatResult {
    bool pressed = false;
    bool repeat = false;
};

struct StickNavPulseState {
    int repeat_dir = 0;
    double next_repeat_time = 0.0;
};

struct StickNavPulseResult {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool left_repeat = false;
    bool right_repeat = false;
    bool up_repeat = false;
    bool down_repeat = false;
};

} // namespace

namespace UiShared {

constexpr float kAxisDeadzone = 0.24f;
constexpr float kPanelMoveSpeed = 900.0f; // pixels per second at full tilt

float ToAxisUnit(const s32 axis) {
    const float centered = (static_cast<float>(axis) - 128.0f) / 127.0f;
    return std::clamp(centered, -1.0f, 1.0f);
}

bool ReadControllerState(Libraries::UserService::OrbisUserServiceUserId user_id,
                         Input::State* out_state) {
    if (!out_state) {
        return false;
    }
    auto* controllers = Common::Singleton<Input::GameControllers>::Instance();
    if (!controllers) {
        return false;
    }

    const auto read_state = [&](u8 index, bool* has_state) {
        if (has_state) {
            *has_state = false;
        }
        Input::State pad_state{};
        bool connected = false;
        int connected_count = 0;
        (*controllers)[index]->ReadState(&pad_state, &connected, &connected_count);
        if (!connected) {
            return false;
        }
        if (has_state) {
            *has_state = true;
        }
        *out_state = pad_state;
        return true;
    };

    const auto mapped = Input::GameControllers::GetControllerIndexFromUserID(user_id);
    if (mapped.has_value() && *mapped < 5) {
        bool has_state = false;
        if (read_state(*mapped, &has_state) && has_state) {
            return true;
        }
    }

    for (u8 index = 0; index < 5; ++index) {
        if (mapped.has_value() && index == *mapped) {
            continue;
        }
        bool has_state = false;
        if (read_state(index, &has_state) && has_state) {
            return true;
        }
    }
    return false;
}

} // namespace UiShared

ImVec4 BrightenColor(ImU32 color, float delta) {
    ImVec4 out = ImGui::ColorConvertU32ToFloat4(color);
    out.x = std::clamp(out.x + delta, 0.0f, 1.0f);
    out.y = std::clamp(out.y + delta, 0.0f, 1.0f);
    out.z = std::clamp(out.z + delta, 0.0f, 1.0f);
    return out;
}

ImeKbLayoutSelection ResolveInitialKbLayoutSelection(OrbisImeExtOption ext_option,
                                                     OrbisImePanelPriority panel_priority) {
    ImeKbLayoutSelection selection{};
    const bool set_priority = True(ext_option & OrbisImeExtOption::SET_PRIORITY);
    if (set_priority) {
        switch (panel_priority) {
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

    const bool shift_lock = set_priority && True(ext_option & OrbisImeExtOption::PRIORITY_SHIFT);
    if (shift_lock && selection.family != ImeKbLayoutFamily::Symbols) {
        // SDK docs: PRIORITY_SHIFT starts the initial panel in Shift-lock state.
        selection.case_state = ImeKbCaseState::CapsLock;
    }
    return selection;
}

int Utf16CountFromUtf8Range(const char* text, const char* end) {
    if (!text) {
        return 0;
    }
    const char* const range_end = end ? end : (text + std::strlen(text));
    const char* p = text;
    int utf16_units = 0;
    while (p < range_end && *p) {
        unsigned int c = 0;
        const int step = ImTextCharFromUtf8(&c, p, range_end);
        if (step <= 0) {
            break;
        }
        utf16_units += (c > 0xFFFF) ? 2 : 1;
        p += step;
    }
    return utf16_units;
}

int Utf8ByteIndexFromUtf16Index(const char* text, int utf16_index) {
    if (!text || utf16_index <= 0) {
        return 0;
    }
    const char* p = text;
    int count = 0;
    while (*p && count < utf16_index) {
        unsigned int c = 0;
        const int step = ImTextCharFromUtf8(&c, p, nullptr);
        if (step <= 0) {
            break;
        }
        const int utf16_units = (c > 0xFFFF) ? 2 : 1;
        if (count + utf16_units > utf16_index) {
            break;
        }
        count += utf16_units;
        p += step;
    }
    return static_cast<int>(p - text);
}

bool RejectInputCharByUtf16Limit(const ImGuiInputTextCallbackData* data, int max_utf16) {
    if (!data || max_utf16 < 0 || data->EventChar == 0) {
        return false;
    }
    ImGuiContext* g = data->Ctx;
    if (!g) {
        return false;
    }
    ImGuiInputTextState* st = &g->InputTextState;
    if (!st || !st->TextSrc) {
        return false;
    }

    const int cur_units = Utf16CountFromUtf8Range(st->TextSrc, st->TextSrc + st->TextLen);
    int selected_units = 0;
    if (st->HasSelection()) {
        const int sel_begin = st->GetSelectionStart();
        const int sel_end = st->GetSelectionEnd();
        selected_units = Utf16CountFromUtf8Range(st->TextSrc + sel_begin, st->TextSrc + sel_end);
    }

    const int incoming_units = data->EventChar > 0xFFFF ? 2 : 1;
    const int remaining_units = max_utf16 - (cur_units - selected_units);
    return remaining_units < incoming_units;
}

bool ClampInputBufferToUtf16Limit(ImGuiInputTextCallbackData* data, int max_utf16) {
    if (!data || max_utf16 < 0) {
        return false;
    }
    const int utf16_len = Utf16CountFromUtf8Range(data->Buf, data->Buf + data->BufTextLen);
    if (utf16_len <= max_utf16) {
        return false;
    }

    const int keep_bytes = Utf8ByteIndexFromUtf16Index(data->Buf, max_utf16);
    if (keep_bytes < data->BufTextLen) {
        data->DeleteChars(keep_bytes, data->BufTextLen - keep_bytes);
    }
    data->CursorPos = std::clamp(data->CursorPos, 0, data->BufTextLen);
    data->SelectionStart = std::clamp(data->SelectionStart, 0, data->BufTextLen);
    data->SelectionEnd = std::clamp(data->SelectionEnd, 0, data->BufTextLen);
    return true;
}

void DrawInactiveCaretOverlay(const ImRect& frame_rect, const char* text, int caret_byte,
                              int selection_start_byte, int selection_end_byte, bool multiline) {
    if (!text) {
        return;
    }
    if (selection_start_byte != selection_end_byte) {
        return;
    }

    const int text_len = static_cast<int>(std::strlen(text));
    const int clamped_byte = std::clamp(caret_byte, 0, text_len);
    const char* const caret_ptr = text + clamped_byte;

    const ImGuiStyle& style = ImGui::GetStyle();
    const float left = frame_rect.Min.x + style.FramePadding.x;
    const float right = frame_rect.Max.x - style.FramePadding.x;
    if (right <= left) {
        return;
    }

    const float top = frame_rect.Min.y + style.FramePadding.y;
    const float bottom = frame_rect.Max.y - style.FramePadding.y;
    if (bottom <= top) {
        return;
    }

    float caret_x = left;
    float caret_y = top;
    float caret_bottom = bottom;
    if (multiline) {
        const char* line_begin = text;
        int line_index = 0;
        const char* p = text;
        while (p < caret_ptr) {
            if (*p == '\n') {
                line_begin = p + 1;
                ++line_index;
            }
            ++p;
        }
        caret_x = left + ImGui::CalcTextSize(line_begin, caret_ptr, false, -1.0f).x;
        caret_y = top + static_cast<float>(line_index) * ImGui::GetTextLineHeight();
        caret_bottom = caret_y + ImGui::GetTextLineHeight();
    } else {
        caret_x = left + ImGui::CalcTextSize(text, caret_ptr, false, -1.0f).x;
    }

    caret_x = std::clamp(caret_x, left, right);
    caret_y = std::clamp(caret_y, top, bottom);
    caret_bottom = std::clamp(caret_bottom, caret_y, bottom);
    if (caret_bottom <= caret_y) {
        return;
    }

    const ImU32 caret_col = ImGui::GetColorU32(ImGuiCol_Text);
    ImGui::GetWindowDrawList()->AddLine({caret_x, caret_y}, {caret_x, caret_bottom}, caret_col,
                                        1.5f);
}

static float ApplyAxisDeadzone(float v) {
    const float av = std::abs(v);
    if (av <= UiShared::kAxisDeadzone) {
        return 0.0f;
    }
    const float scaled = (av - UiShared::kAxisDeadzone) / (1.0f - UiShared::kAxisDeadzone);
    return std::copysign(std::clamp(scaled, 0.0f, 1.0f), v);
}

VirtualPadSnapshot ReadVirtualPadSnapshot(Libraries::UserService::OrbisUserServiceUserId user_id,
                                          float delta_time, bool include_imgui_fallback) {
    VirtualPadSnapshot snapshot{};
    constexpr u32 kMaskLeft = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Left);
    constexpr u32 kMaskRight = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Right);
    constexpr u32 kMaskUp = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Up);
    constexpr u32 kMaskDown = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Down);
    constexpr u32 kMaskCross = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Cross);
    constexpr u32 kMaskTriangle =
        static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Triangle);
    constexpr u32 kMaskSquare = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Square);
    constexpr u32 kMaskCircle = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Circle);
    constexpr u32 kMaskL1 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::L1);
    constexpr u32 kMaskR1 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::R1);
    constexpr u32 kMaskL2 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::L2);
    constexpr u32 kMaskR2 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::R2);
    constexpr u32 kMaskL3 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::L3);
    constexpr u32 kMaskR3 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::R3);
    constexpr float kTriggerButtonThreshold = 0.25f;

    float lx = 0.0f;
    float ly = 0.0f;
    float rx = 0.0f;
    float ry = 0.0f;
    float l2_analog = 0.0f;
    float r2_analog = 0.0f;
    u32 buttons = 0;
    Input::State state{};
    if (UiShared::ReadControllerState(user_id, &state)) {
        buttons = static_cast<u32>(state.buttonsState);
        lx = ApplyAxisDeadzone(
            UiShared::ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::LeftX)]));
        ly = ApplyAxisDeadzone(
            UiShared::ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::LeftY)]));
        rx = ApplyAxisDeadzone(
            UiShared::ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::RightX)]));
        ry = ApplyAxisDeadzone(
            UiShared::ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::RightY)]));
        l2_analog = std::clamp(
            static_cast<float>(state.axes[static_cast<std::size_t>(Input::Axis::TriggerLeft)]) /
                255.0f,
            0.0f, 1.0f);
        r2_analog = std::clamp(
            static_cast<float>(state.axes[static_cast<std::size_t>(Input::Axis::TriggerRight)]) /
                255.0f,
            0.0f, 1.0f);
    }

    if (include_imgui_fallback) {
        const float imgui_lx =
            ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadLStickRight)->AnalogValue -
                              ImGui::GetKeyData(ImGuiKey_GamepadLStickLeft)->AnalogValue);
        const float imgui_ly =
            ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadLStickDown)->AnalogValue -
                              ImGui::GetKeyData(ImGuiKey_GamepadLStickUp)->AnalogValue);
        const float imgui_rx =
            ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadRStickRight)->AnalogValue -
                              ImGui::GetKeyData(ImGuiKey_GamepadRStickLeft)->AnalogValue);
        const float imgui_ry =
            ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadRStickDown)->AnalogValue -
                              ImGui::GetKeyData(ImGuiKey_GamepadRStickUp)->AnalogValue);
        const float imgui_l2 =
            std::clamp(ImGui::GetKeyData(ImGuiKey_GamepadL2)->AnalogValue, 0.0f, 1.0f);
        const float imgui_r2 =
            std::clamp(ImGui::GetKeyData(ImGuiKey_GamepadR2)->AnalogValue, 0.0f, 1.0f);

        if (std::abs(imgui_lx) > std::abs(lx)) {
            lx = imgui_lx;
        }
        if (std::abs(imgui_ly) > std::abs(ly)) {
            ly = imgui_ly;
        }
        if (std::abs(imgui_rx) > std::abs(rx)) {
            rx = imgui_rx;
        }
        if (std::abs(imgui_ry) > std::abs(ry)) {
            ry = imgui_ry;
        }
        l2_analog = std::max(l2_analog, imgui_l2);
        r2_analog = std::max(r2_analog, imgui_r2);

        const auto merge_imgui_button = [&](const ImGuiKey key, const u32 mask) {
            if (ImGui::IsKeyDown(key)) {
                buttons |= mask;
            }
        };
        merge_imgui_button(ImGuiKey_GamepadDpadLeft, kMaskLeft);
        merge_imgui_button(ImGuiKey_GamepadDpadRight, kMaskRight);
        merge_imgui_button(ImGuiKey_GamepadDpadUp, kMaskUp);
        merge_imgui_button(ImGuiKey_GamepadDpadDown, kMaskDown);
        merge_imgui_button(ImGuiKey_GamepadFaceDown, kMaskCross);
        merge_imgui_button(ImGuiKey_GamepadFaceUp, kMaskTriangle);
        merge_imgui_button(ImGuiKey_GamepadFaceLeft, kMaskSquare);
        merge_imgui_button(ImGuiKey_GamepadFaceRight, kMaskCircle);
        merge_imgui_button(ImGuiKey_GamepadL1, kMaskL1);
        merge_imgui_button(ImGuiKey_GamepadR1, kMaskR1);
        merge_imgui_button(ImGuiKey_GamepadL3, kMaskL3);
        merge_imgui_button(ImGuiKey_GamepadR3, kMaskR3);
        if (l2_analog >= kTriggerButtonThreshold) {
            buttons |= kMaskL2;
        }
        if (r2_analog >= kTriggerButtonThreshold) {
            buttons |= kMaskR2;
        }
    }

    snapshot.buttons = buttons;
    snapshot.left_stick = {lx, ly};
    snapshot.l2_analog = l2_analog;

    if (delta_time > 0.0f) {
        snapshot.panel_delta = {rx * UiShared::kPanelMoveSpeed * delta_time,
                                ry * UiShared::kPanelMoveSpeed * delta_time};
    }
    return snapshot;
}

static StickNavDirection ResolveStickNavDirection(float lx, float ly, float* strength) {
    const float abs_x = std::abs(lx);
    const float abs_y = std::abs(ly);
    if (abs_x < kNavAxisThreshold && abs_y < kNavAxisThreshold) {
        if (strength) {
            *strength = 0.0f;
        }
        return StickNavDirection::None;
    }

    if (abs_x >= abs_y) {
        if (strength) {
            *strength = abs_x;
        }
        return (lx < 0.0f) ? StickNavDirection::Left : StickNavDirection::Right;
    }

    if (strength) {
        *strength = abs_y;
    }
    return (ly < 0.0f) ? StickNavDirection::Up : StickNavDirection::Down;
}

static VirtualButtonRepeatResult ProcessRepeatButton(bool down, bool edge_pressed, double now,
                                                     double repeat_delay, double repeat_rate,
                                                     bool& prev_down_state,
                                                     double& next_repeat_time) {
    VirtualButtonRepeatResult result{};
    if (!down) {
        prev_down_state = false;
        next_repeat_time = 0.0;
        return result;
    }

    if (!prev_down_state) {
        prev_down_state = true;
        next_repeat_time = now + repeat_delay;
        result.pressed = edge_pressed;
        return result;
    }

    if (edge_pressed) {
        next_repeat_time = now + repeat_delay;
        result.pressed = true;
        return result;
    }

    if (repeat_rate <= 0.0) {
        return result;
    }

    if (now >= next_repeat_time) {
        next_repeat_time = now + repeat_rate;
        result.pressed = true;
        result.repeat = true;
    }
    return result;
}

static bool IsStickDirectionEdgePressed(StickNavDirection direction, bool left_edge,
                                        bool right_edge, bool up_edge, bool down_edge) {
    switch (direction) {
    case StickNavDirection::Left:
        return left_edge;
    case StickNavDirection::Right:
        return right_edge;
    case StickNavDirection::Up:
        return up_edge;
    case StickNavDirection::Down:
        return down_edge;
    case StickNavDirection::None:
    default:
        return false;
    }
}

static StickNavPulseResult ProcessStickNavPulse(bool enabled, StickNavDirection direction,
                                                float strength, bool direction_edge_pressed,
                                                StickNavPulseState& state, double now) {
    StickNavPulseResult result{};
    if (!enabled || direction == StickNavDirection::None) {
        state.repeat_dir = 0;
        state.next_repeat_time = 0.0;
        return result;
    }

    const float t =
        std::clamp((strength - kNavAxisThreshold) / (1.0f - kNavAxisThreshold), 0.0f, 1.0f);
    const double initial_delay =
        std::lerp(kStickNavInitialDelaySlow, kStickNavInitialDelayFast, static_cast<double>(t));
    const double repeat_interval =
        std::lerp(kStickNavRepeatSlow, kStickNavRepeatFast, static_cast<double>(t));

    const int dir_id = static_cast<int>(direction);
    const int prev_dir = state.repeat_dir;
    bool pulse = false;
    bool pulse_repeat = false;

    if (prev_dir != dir_id) {
        // Treat direction changes before the pending repeat tick as fast-rotate input.
        const bool fast_rotate_change = (prev_dir != 0) && (now < state.next_repeat_time);
        state.repeat_dir = dir_id;
        state.next_repeat_time = now + initial_delay;
        pulse = (prev_dir == 0) || direction_edge_pressed || fast_rotate_change;
    } else if (direction_edge_pressed) {
        pulse = true;
        state.next_repeat_time = now + initial_delay;
    } else if (now >= state.next_repeat_time) {
        pulse = true;
        pulse_repeat = true;
        state.next_repeat_time = now + repeat_interval;
    }

    if (!pulse) {
        return result;
    }

    switch (direction) {
    case StickNavDirection::Left:
        result.left = true;
        result.left_repeat = pulse_repeat;
        break;
    case StickNavDirection::Right:
        result.right = true;
        result.right_repeat = pulse_repeat;
        break;
    case StickNavDirection::Up:
        result.up = true;
        result.up_repeat = pulse_repeat;
        break;
    case StickNavDirection::Down:
        result.down = true;
        result.down_repeat = pulse_repeat;
        break;
    case StickNavDirection::None:
    default:
        break;
    }
    return result;
}

OskVirtualPadInputView::OskVirtualPadInputView(const OskPadInputFrame& input_frame,
                                               const ImGuiIO& io)
    : frame(input_frame), repeat_delay(static_cast<double>(io.KeyRepeatDelay)),
      repeat_rate(static_cast<double>(io.KeyRepeatRate)) {}

bool OskVirtualPadInputView::Down(Libraries::Pad::OrbisPadButtonDataOffset button) const {
    return (frame.virtual_buttons & static_cast<u32>(button)) != 0;
}

bool OskVirtualPadInputView::Pressed(Libraries::Pad::OrbisPadButtonDataOffset button) const {
    const u32 mask = static_cast<u32>(button);
    return (frame.virtual_buttons & mask) != 0 && (frame.prev_virtual_buttons & mask) == 0;
}

bool OskVirtualPadInputView::RepeatPressed(Libraries::Pad::OrbisPadButtonDataOffset button,
                                           bool& prev_down_state, double& next_repeat_time,
                                           bool* out_repeat) const {
    const VirtualButtonRepeatResult result =
        ProcessRepeatButton(Down(button), Pressed(button), ImGui::GetTime(), repeat_delay,
                            repeat_rate, prev_down_state, next_repeat_time);
    if (out_repeat) {
        *out_repeat = result.repeat;
    }
    return result.pressed;
}

void ResetOskShortcutRepeatState(OskShortcutRepeatState& state) {
    state.prev_square_down = false;
    state.prev_l1_down = false;
    state.prev_r1_down = false;
    state.l2_shortcut_armed = true;
    state.square_next_repeat_time = 0.0;
    state.l1_next_repeat_time = 0.0;
    state.r1_next_repeat_time = 0.0;
    state.triangle_next_repeat_time = 0.0;
}

bool ConsumeTriggerShortcutPress(bool trigger_down, bool trigger_edge_pressed,
                                 float trigger_analog_value, bool& shortcut_armed) {
    // Require clear release before accepting another trigger shortcut press.
    constexpr float kTriggerReleaseThreshold = 0.20f;
    if (!trigger_down && trigger_analog_value <= kTriggerReleaseThreshold) {
        shortcut_armed = true;
    }
    if (!shortcut_armed || !trigger_edge_pressed) {
        return false;
    }
    shortcut_armed = false;
    return true;
}

OskShortcutActionResult EvaluateOskShortcutAction(bool allow_osk_shortcuts, bool menu_modal,
                                                  bool evaluate_action,
                                                  const OskPadInputFrame& panel_input,
                                                  const OskVirtualPadInputView& virtual_pad_input,
                                                  u32 prev_virtual_buttons,
                                                  ImeKbLayoutFamily layout_family,
                                                  OskShortcutRepeatState& repeat_state) {
    OskShortcutActionResult result{};
    if (!(allow_osk_shortcuts && !menu_modal)) {
        ResetOskShortcutRepeatState(repeat_state);
        return result;
    }
    if (!evaluate_action) {
        return result;
    }

    constexpr u32 kMaskTriangle =
        static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Triangle);

    const bool l1_down = virtual_pad_input.Down(Libraries::Pad::OrbisPadButtonDataOffset::L1);
    const bool l1_edge_pressed =
        virtual_pad_input.Pressed(Libraries::Pad::OrbisPadButtonDataOffset::L1);
    const bool l1_repeat_pressed = virtual_pad_input.RepeatPressed(
        Libraries::Pad::OrbisPadButtonDataOffset::L1, repeat_state.prev_l1_down,
        repeat_state.l1_next_repeat_time);

    const bool square_down =
        virtual_pad_input.Down(Libraries::Pad::OrbisPadButtonDataOffset::Square);
    const bool square_edge_pressed =
        virtual_pad_input.Pressed(Libraries::Pad::OrbisPadButtonDataOffset::Square);
    const bool square_repeat_pressed = virtual_pad_input.RepeatPressed(
        Libraries::Pad::OrbisPadButtonDataOffset::Square, repeat_state.prev_square_down,
        repeat_state.square_next_repeat_time);

    const bool r1_repeat_pressed = virtual_pad_input.RepeatPressed(
        Libraries::Pad::OrbisPadButtonDataOffset::R1, repeat_state.prev_r1_down,
        repeat_state.r1_next_repeat_time);

    const bool clear_all_shortcut_pressed =
        (l1_down && square_edge_pressed) || (square_down && l1_edge_pressed);

    const bool l2_down = virtual_pad_input.Down(Libraries::Pad::OrbisPadButtonDataOffset::L2);
    const bool l2_edge_pressed =
        virtual_pad_input.Pressed(Libraries::Pad::OrbisPadButtonDataOffset::L2);
    const float l2_analog = std::max(panel_input.l2_analog, l2_down ? 1.0f : 0.0f);
    const bool l2_pressed = ConsumeTriggerShortcutPress(l2_down, l2_edge_pressed, l2_analog,
                                                        repeat_state.l2_shortcut_armed);

    const bool tri_down =
        virtual_pad_input.Down(Libraries::Pad::OrbisPadButtonDataOffset::Triangle);
    const bool tri_edge_pressed =
        virtual_pad_input.Pressed(Libraries::Pad::OrbisPadButtonDataOffset::Triangle);
    bool prev_virtual_triangle_down = (prev_virtual_buttons & kMaskTriangle) != 0;
    const bool tri_pressed = virtual_pad_input.RepeatPressed(
        Libraries::Pad::OrbisPadButtonDataOffset::Triangle, prev_virtual_triangle_down,
        repeat_state.triangle_next_repeat_time);
    const bool tri_space_pressed = tri_pressed && !l2_down;

    const bool symbols_shortcut_pressed = (l2_down && tri_edge_pressed) || (tri_down && l2_pressed);
    if (symbols_shortcut_pressed) {
        result.action = ImeKbKeyAction::SymbolsMode;
    } else if (tri_space_pressed) {
        result.action = ImeKbKeyAction::Space;
    } else if (layout_family != ImeKbLayoutFamily::Symbols &&
               virtual_pad_input.Pressed(Libraries::Pad::OrbisPadButtonDataOffset::L3)) {
        result.action = ImeKbKeyAction::SpecialsMode;
    } else if (virtual_pad_input.Pressed(Libraries::Pad::OrbisPadButtonDataOffset::R2)) {
        result.action = ImeKbKeyAction::Done;
    } else if (clear_all_shortcut_pressed) {
        result.clear_all = true;
    } else if (square_repeat_pressed) {
        result.action = ImeKbKeyAction::Backspace;
    } else if (l1_repeat_pressed) {
        result.action = ImeKbKeyAction::ArrowLeft;
    } else if (r1_repeat_pressed) {
        result.action = ImeKbKeyAction::ArrowRight;
    } else if (virtual_pad_input.Pressed(Libraries::Pad::OrbisPadButtonDataOffset::R3)) {
        result.action = ImeKbKeyAction::Settings;
    } else if (layout_family != ImeKbLayoutFamily::Symbols && l2_pressed) {
        result.action = ImeKbKeyAction::Shift;
    }
    return result;
}

namespace {

bool IsVirtualPadButtonDown(const u32 buttons, const u32 button_mask) {
    return (buttons & button_mask) != 0;
}

bool IsVirtualPadButtonPressed(const u32 buttons, const u32 prev_buttons, const u32 button_mask) {
    return IsVirtualPadButtonDown(buttons, button_mask) &&
           !IsVirtualPadButtonDown(prev_buttons, button_mask);
}

VirtualButtonRepeatResult ProcessVirtualPadButtonRepeat(const u32 buttons, const u32 prev_buttons,
                                                        const u32 button_mask, const double now,
                                                        const double repeat_delay,
                                                        const double repeat_rate,
                                                        bool& prev_down_state,
                                                        double& next_repeat_time) {
    return ProcessRepeatButton(IsVirtualPadButtonDown(buttons, button_mask),
                               IsVirtualPadButtonPressed(buttons, prev_buttons, button_mask), now,
                               repeat_delay, repeat_rate, prev_down_state, next_repeat_time);
}

} // namespace

OskPadInputFrame ComputeOskPadInputFrame(const VirtualPadSnapshot& virtual_pad,
                                         bool allow_osk_shortcuts, bool first_render,
                                         OskPadInputState& state) {
    constexpr u32 kMaskLeft = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Left);
    constexpr u32 kMaskRight = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Right);
    constexpr u32 kMaskUp = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Up);
    constexpr u32 kMaskDown = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Down);
    constexpr u32 kMaskCross = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Cross);
    constexpr u32 kMaskTriangle =
        static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Triangle);
    constexpr u32 kMaskSquare = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Square);
    constexpr u32 kMaskCircle = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::Circle);
    constexpr u32 kMaskL1 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::L1);
    constexpr u32 kMaskR1 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::R1);
    constexpr u32 kMaskL2 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::L2);
    constexpr u32 kMaskR2 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::R2);
    constexpr u32 kMaskL3 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::L3);
    constexpr u32 kMaskR3 = static_cast<u32>(Libraries::Pad::OrbisPadButtonDataOffset::R3);

    OskPadInputFrame frame{};
    frame.virtual_buttons = virtual_pad.buttons;
    frame.prev_virtual_buttons = first_render ? frame.virtual_buttons : state.prev_virtual_buttons;
    frame.l2_analog = virtual_pad.l2_analog;

    const auto virtual_down = [&](const u32 mask) {
        return IsVirtualPadButtonDown(frame.virtual_buttons, mask);
    };
    const auto virtual_pressed = [&](const u32 mask) {
        return IsVirtualPadButtonPressed(frame.virtual_buttons, frame.prev_virtual_buttons, mask);
    };

    const ImGuiIO& io = ImGui::GetIO();
    const double now = ImGui::GetTime();
    const double repeat_delay = static_cast<double>(io.KeyRepeatDelay);
    const double repeat_rate = static_cast<double>(io.KeyRepeatRate);

    frame.cross_down = virtual_down(kMaskCross);

    bool prev_virtual_cross_hold_down =
        IsVirtualPadButtonDown(frame.prev_virtual_buttons, kMaskCross);
    const VirtualButtonRepeatResult virtual_cross_repeat = ProcessRepeatButton(
        virtual_down(kMaskCross), virtual_pressed(kMaskCross), now, repeat_delay, repeat_rate,
        prev_virtual_cross_hold_down, state.virtual_cross_next_repeat_time);

    const bool prev_cross_down = first_render ? frame.cross_down : state.prev_virtual_cross_down;
    frame.panel_activate_pressed_raw = frame.cross_down && !prev_cross_down;
    frame.panel_activate_repeat_raw = virtual_cross_repeat.repeat;

    const VirtualButtonRepeatResult virtual_nav_left = ProcessVirtualPadButtonRepeat(
        frame.virtual_buttons, frame.prev_virtual_buttons, kMaskLeft, now, repeat_delay,
        repeat_rate, state.prev_virtual_dpad_left_down, state.virtual_dpad_left_next_repeat_time);
    const VirtualButtonRepeatResult virtual_nav_right = ProcessVirtualPadButtonRepeat(
        frame.virtual_buttons, frame.prev_virtual_buttons, kMaskRight, now, repeat_delay,
        repeat_rate, state.prev_virtual_dpad_right_down, state.virtual_dpad_right_next_repeat_time);
    const VirtualButtonRepeatResult virtual_nav_up = ProcessVirtualPadButtonRepeat(
        frame.virtual_buttons, frame.prev_virtual_buttons, kMaskUp, now, repeat_delay, repeat_rate,
        state.prev_virtual_dpad_up_down, state.virtual_dpad_up_next_repeat_time);
    const VirtualButtonRepeatResult virtual_nav_down = ProcessVirtualPadButtonRepeat(
        frame.virtual_buttons, frame.prev_virtual_buttons, kMaskDown, now, repeat_delay,
        repeat_rate, state.prev_virtual_dpad_down_down, state.virtual_dpad_down_next_repeat_time);

    frame.virtual_lstick_dirs.left = virtual_pad.left_stick.x <= -kNavAxisThreshold;
    frame.virtual_lstick_dirs.right = virtual_pad.left_stick.x >= kNavAxisThreshold;
    frame.virtual_lstick_dirs.up = virtual_pad.left_stick.y <= -kNavAxisThreshold;
    frame.virtual_lstick_dirs.down = virtual_pad.left_stick.y >= kNavAxisThreshold;
    const bool prev_virtual_lstick_left_down =
        first_render ? frame.virtual_lstick_dirs.left : state.prev_virtual_lstick_left_down;
    const bool prev_virtual_lstick_right_down =
        first_render ? frame.virtual_lstick_dirs.right : state.prev_virtual_lstick_right_down;
    const bool prev_virtual_lstick_up_down =
        first_render ? frame.virtual_lstick_dirs.up : state.prev_virtual_lstick_up_down;
    const bool prev_virtual_lstick_down_down =
        first_render ? frame.virtual_lstick_dirs.down : state.prev_virtual_lstick_down_down;

    const bool virtual_lstick_left_edge =
        frame.virtual_lstick_dirs.left && !prev_virtual_lstick_left_down;
    const bool virtual_lstick_right_edge =
        frame.virtual_lstick_dirs.right && !prev_virtual_lstick_right_down;
    const bool virtual_lstick_up_edge =
        frame.virtual_lstick_dirs.up && !prev_virtual_lstick_up_down;
    const bool virtual_lstick_down_edge =
        frame.virtual_lstick_dirs.down && !prev_virtual_lstick_down_down;

    const bool raw_virtual_control_input =
        virtual_pressed(kMaskLeft) || virtual_pressed(kMaskRight) || virtual_pressed(kMaskUp) ||
        virtual_pressed(kMaskDown) || virtual_lstick_left_edge || virtual_lstick_right_edge ||
        virtual_lstick_up_edge || virtual_lstick_down_edge || virtual_pressed(kMaskCross) ||
        virtual_pressed(kMaskTriangle) || virtual_pressed(kMaskSquare) ||
        virtual_pressed(kMaskCircle) || virtual_pressed(kMaskL1) || virtual_pressed(kMaskR1) ||
        virtual_pressed(kMaskL2) || virtual_pressed(kMaskR2) || virtual_pressed(kMaskL3) ||
        virtual_pressed(kMaskR3);
    frame.raw_osk_control_input = raw_virtual_control_input;

    float stick_strength = 0.0f;
    const StickNavDirection stick_dir = ResolveStickNavDirection(
        virtual_pad.left_stick.x, virtual_pad.left_stick.y, &stick_strength);
    const bool stick_dir_edge_pressed =
        IsStickDirectionEdgePressed(stick_dir, virtual_lstick_left_edge, virtual_lstick_right_edge,
                                    virtual_lstick_up_edge, virtual_lstick_down_edge);

    StickNavPulseState stick_nav_state{state.left_stick_repeat_dir,
                                       state.left_stick_next_repeat_time};
    const StickNavPulseResult stick_nav_pulse =
        ProcessStickNavPulse(allow_osk_shortcuts, stick_dir, stick_strength, stick_dir_edge_pressed,
                             stick_nav_state, now);
    state.left_stick_repeat_dir = stick_nav_state.repeat_dir;
    state.left_stick_next_repeat_time = stick_nav_state.next_repeat_time;

    frame.virtual_nav_left = virtual_nav_left.pressed;
    frame.virtual_nav_right = virtual_nav_right.pressed;
    frame.virtual_nav_up = virtual_nav_up.pressed;
    frame.virtual_nav_down = virtual_nav_down.pressed;
    frame.virtual_nav_left_repeat = virtual_nav_left.repeat;
    frame.virtual_nav_right_repeat = virtual_nav_right.repeat;
    frame.virtual_nav_up_repeat = virtual_nav_up.repeat;
    frame.virtual_nav_down_repeat = virtual_nav_down.repeat;
    frame.stick_nav_left = stick_nav_pulse.left;
    frame.stick_nav_right = stick_nav_pulse.right;
    frame.stick_nav_up = stick_nav_pulse.up;
    frame.stick_nav_down = stick_nav_pulse.down;
    frame.stick_nav_left_repeat = stick_nav_pulse.left_repeat;
    frame.stick_nav_right_repeat = stick_nav_pulse.right_repeat;
    frame.stick_nav_up_repeat = stick_nav_pulse.up_repeat;
    frame.stick_nav_down_repeat = stick_nav_pulse.down_repeat;

    frame.nav_left = allow_osk_shortcuts && (frame.virtual_nav_left || frame.stick_nav_left);
    frame.nav_right = allow_osk_shortcuts && (frame.virtual_nav_right || frame.stick_nav_right);
    frame.nav_up = allow_osk_shortcuts && (frame.virtual_nav_up || frame.stick_nav_up);
    frame.nav_down = allow_osk_shortcuts && (frame.virtual_nav_down || frame.stick_nav_down);
    frame.nav_left_repeat =
        allow_osk_shortcuts && (frame.virtual_nav_left_repeat || frame.stick_nav_left_repeat);
    frame.nav_right_repeat =
        allow_osk_shortcuts && (frame.virtual_nav_right_repeat || frame.stick_nav_right_repeat);
    frame.nav_up_repeat =
        allow_osk_shortcuts && (frame.virtual_nav_up_repeat || frame.stick_nav_up_repeat);
    frame.nav_down_repeat =
        allow_osk_shortcuts && (frame.virtual_nav_down_repeat || frame.stick_nav_down_repeat);

    frame.virtual_control_input =
        allow_osk_shortcuts &&
        (virtual_nav_left.pressed || virtual_nav_right.pressed || virtual_nav_up.pressed ||
         virtual_nav_down.pressed || stick_nav_pulse.left || stick_nav_pulse.right ||
         stick_nav_pulse.up || stick_nav_pulse.down || virtual_pressed(kMaskCross) ||
         virtual_pressed(kMaskTriangle) || virtual_pressed(kMaskSquare) ||
         virtual_pressed(kMaskCircle) || virtual_pressed(kMaskL1) || virtual_pressed(kMaskR1) ||
         virtual_pressed(kMaskL2) || virtual_pressed(kMaskR2) || virtual_pressed(kMaskL3) ||
         virtual_pressed(kMaskR3));

    frame.osk_control_input =
        frame.virtual_control_input || (allow_osk_shortcuts && (frame.panel_activate_pressed_raw ||
                                                                frame.panel_activate_repeat_raw));
    return frame;
}

void CommitOskPadInputFrame(const OskPadInputFrame& frame, OskPadInputState& state) {
    state.prev_virtual_buttons = frame.virtual_buttons;
    state.prev_virtual_cross_down = frame.cross_down;
    state.prev_virtual_lstick_left_down = frame.virtual_lstick_dirs.left;
    state.prev_virtual_lstick_right_down = frame.virtual_lstick_dirs.right;
    state.prev_virtual_lstick_up_down = frame.virtual_lstick_dirs.up;
    state.prev_virtual_lstick_down_down = frame.virtual_lstick_dirs.down;
}

void DisarmMenuActivate(bool& menu_activate_armed) {
    menu_activate_armed = false;
}

void RearmMenuActivateOnRelease(bool activate_down, bool& menu_activate_armed) {
    if (!menu_activate_armed && !activate_down) {
        menu_activate_armed = true;
    }
}

bool ConsumeMenuActivatePress(bool panel_activate_pressed, bool opened_menu_this_frame,
                              bool& menu_activate_armed) {
    if (!menu_activate_armed || opened_menu_this_frame || !panel_activate_pressed) {
        return false;
    }
    menu_activate_armed = false;
    return true;
}

} // namespace Libraries::Ime
