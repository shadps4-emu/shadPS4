// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <cmath>
#include <cstring>
#include <imgui_internal.h>

#include "common/singleton.h"
#include "common/types.h"
#include "core/libraries/system/userservice.h"
#include "input/controller.h"

namespace Libraries::Ime {

struct VirtualLeftStickDirections {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
};

struct VirtualPadSnapshot {
    u32 buttons = 0;
    VirtualLeftStickDirections left_stick_dirs{};
    ImVec2 left_stick{};
    ImVec2 panel_delta{};
};

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
constexpr double kPanelEdgeWrapHoldDelaySec = 0.5;
constexpr double kRepeatIntentWindowSec = 0.45;

namespace UiShared {

constexpr float kAxisDeadzone = 0.24f;
constexpr float kPanelMoveSpeed = 900.0f; // pixels per second at full tilt

inline float ToAxisUnit(const s32 axis) {
    const float centered = (static_cast<float>(axis) - 128.0f) / 127.0f;
    return std::clamp(centered, -1.0f, 1.0f);
}

inline bool ReadControllerState(Libraries::UserService::OrbisUserServiceUserId user_id,
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

inline ImVec4 BrightenColor(ImU32 color, float delta) {
    ImVec4 out = ImGui::ColorConvertU32ToFloat4(color);
    out.x = std::clamp(out.x + delta, 0.0f, 1.0f);
    out.y = std::clamp(out.y + delta, 0.0f, 1.0f);
    out.z = std::clamp(out.z + delta, 0.0f, 1.0f);
    return out;
}

inline int Utf16CountFromUtf8Range(const char* text, const char* end = nullptr) {
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

inline int Utf8ByteIndexFromUtf16Index(const char* text, int utf16_index) {
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

inline bool RejectInputCharByUtf16Limit(const ImGuiInputTextCallbackData* data, int max_utf16) {
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

inline bool ClampInputBufferToUtf16Limit(ImGuiInputTextCallbackData* data, int max_utf16) {
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

inline void DrawInactiveCaretOverlay(const ImRect& frame_rect, const char* text, int caret_byte,
                                     int selection_start_byte, int selection_end_byte,
                                     bool multiline = false) {
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

inline float ApplyAxisDeadzone(const float v) {
    const float av = std::abs(v);
    if (av <= UiShared::kAxisDeadzone) {
        return 0.0f;
    }
    const float scaled = (av - UiShared::kAxisDeadzone) / (1.0f - UiShared::kAxisDeadzone);
    return std::copysign(std::clamp(scaled, 0.0f, 1.0f), v);
}

inline VirtualPadSnapshot ReadVirtualPadSnapshot(
    Libraries::UserService::OrbisUserServiceUserId user_id, const float delta_time) {
    VirtualPadSnapshot snapshot{};

    const float imgui_rx =
        ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadRStickRight)->AnalogValue -
                          ImGui::GetKeyData(ImGuiKey_GamepadRStickLeft)->AnalogValue);
    const float imgui_ry =
        ApplyAxisDeadzone(ImGui::GetKeyData(ImGuiKey_GamepadRStickDown)->AnalogValue -
                          ImGui::GetKeyData(ImGuiKey_GamepadRStickUp)->AnalogValue);

    float virtual_rx = 0.0f;
    float virtual_ry = 0.0f;
    Input::State state{};
    if (UiShared::ReadControllerState(user_id, &state)) {
        snapshot.buttons = static_cast<u32>(state.buttonsState);
        const float lx = ApplyAxisDeadzone(
            UiShared::ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::LeftX)]));
        const float ly = ApplyAxisDeadzone(
            UiShared::ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::LeftY)]));
        snapshot.left_stick_dirs.left = lx <= -kNavAxisThreshold;
        snapshot.left_stick_dirs.right = lx >= kNavAxisThreshold;
        snapshot.left_stick_dirs.up = ly <= -kNavAxisThreshold;
        snapshot.left_stick_dirs.down = ly >= kNavAxisThreshold;
        snapshot.left_stick = {lx, ly};
        virtual_rx = ApplyAxisDeadzone(
            UiShared::ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::RightX)]));
        virtual_ry = ApplyAxisDeadzone(
            UiShared::ToAxisUnit(state.axes[static_cast<std::size_t>(Input::Axis::RightY)]));
    }

    if (delta_time > 0.0f) {
        const float rx = (std::abs(virtual_rx) > std::abs(imgui_rx)) ? virtual_rx : imgui_rx;
        const float ry = (std::abs(virtual_ry) > std::abs(imgui_ry)) ? virtual_ry : imgui_ry;
        snapshot.panel_delta = {rx * UiShared::kPanelMoveSpeed * delta_time,
                                ry * UiShared::kPanelMoveSpeed * delta_time};
    }
    return snapshot;
}

inline StickNavDirection ResolveStickNavDirection(const float lx, const float ly, float* strength) {
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

struct VirtualButtonRepeatResult {
    bool pressed = false;
    bool repeat = false;
};

inline VirtualButtonRepeatResult ProcessRepeatButton(const bool down, const bool edge_pressed,
                                                     const double now, const double repeat_delay,
                                                     const double repeat_rate,
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

inline bool IsStickDirectionEdgePressed(const StickNavDirection direction, const bool left_edge,
                                        const bool right_edge, const bool up_edge,
                                        const bool down_edge) {
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

inline StickNavPulseResult ProcessStickNavPulse(const bool enabled,
                                                const StickNavDirection direction,
                                                const float strength,
                                                const bool direction_edge_pressed,
                                                StickNavPulseState& state, const double now) {
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

inline void DisarmMenuActivate(bool& menu_activate_armed) {
    menu_activate_armed = false;
}

inline void RearmMenuActivateOnRelease(const bool activate_down, bool& menu_activate_armed) {
    if (!menu_activate_armed && !activate_down) {
        menu_activate_armed = true;
    }
}

inline bool ConsumeMenuActivatePress(const bool panel_activate_pressed,
                                     const bool opened_menu_this_frame, bool& menu_activate_armed) {
    if (!menu_activate_armed || opened_menu_this_frame || !panel_activate_pressed) {
        return false;
    }
    menu_activate_armed = false;
    return true;
}

} // namespace Libraries::Ime
