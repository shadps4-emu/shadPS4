// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cmath>
#include <imgui.h>

#include "imgui_internal.h"

#define IM_COL32_GRAY(x) IM_COL32(x, x, x, 0xFF)

#define IMGUI_FONT_TEXT 0
#define IMGUI_FONT_MONO 1

namespace ImGui {

namespace Easing {

inline float FastInFastOutCubic(float x) {
    constexpr float c4 = 1.587401f; // 4^(1/3)
    constexpr float c05 = 0.7937f;  // 0.5^(1/3)
    return std::pow(c4 * x - c05, 3.0f) + 0.5f;
}

} // namespace Easing

inline void CentralizeWindow() {
    const auto display_size = GetIO().DisplaySize;
    SetNextWindowPos(display_size / 2.0f, ImGuiCond_Always, {0.5f});
}

inline void KeepWindowInside(ImVec2 display_size = GetIO().DisplaySize) {
    const auto cur_pos = GetWindowPos();
    if (cur_pos.x < 0.0f || cur_pos.y < 0.0f) {
        SetWindowPos(ImMax(cur_pos, ImVec2(0.0f, 0.0f)));
        return;
    }
    const auto cur_size = GetWindowSize();
    const auto bottom_right = cur_pos + cur_size;
    if (bottom_right.x > display_size.x || bottom_right.y > display_size.y) {
        const auto max_pos = display_size - cur_size;
        SetWindowPos(ImMin(cur_pos, max_pos));
    }
}

inline void KeepNavHighlight() {
    GetCurrentContext()->NavDisableHighlight = false;
}

inline void SetItemCurrentNavFocus(const ImGuiID id = -1) {
    const auto ctx = GetCurrentContext();
    SetFocusID(id == -1 ? ctx->LastItemData.ID : id, ctx->CurrentWindow);
    ctx->NavInitResult.Clear();
    ctx->NavDisableHighlight = false;
}

inline void DrawPrettyBackground() {
    const double time = GetTime() / 1.5f;
    const float x = ((float)std::cos(time) + 1.0f) / 2.0f;
    const float d = Easing::FastInFastOutCubic(x);
    u8 top_left = ImLerp(0x13, 0x05, d);
    u8 top_right = ImLerp(0x00, 0x07, d);
    u8 bottom_right = ImLerp(0x03, 0x27, d);
    u8 bottom_left = ImLerp(0x05, 0x00, d);

    auto& window = *GetCurrentWindowRead();
    auto inner_pos = window.DC.CursorPos - window.WindowPadding;
    auto inner_size = GetContentRegionAvail() + window.WindowPadding * 2.0f;
    GetWindowDrawList()->AddRectFilledMultiColor(
        inner_pos, inner_pos + inner_size, IM_COL32_GRAY(top_left), IM_COL32_GRAY(top_right),
        IM_COL32_GRAY(bottom_right), IM_COL32_GRAY(bottom_left));
}

static void DrawCenteredText(const char* text, const char* text_end = nullptr,
                             ImVec2 content = GetContentRegionAvail()) {
    auto pos = GetCursorPos();
    const auto text_size = CalcTextSize(text, text_end, false, content.x - 40.0f);
    PushTextWrapPos(content.x);
    SetCursorPos(pos + (content - text_size) / 2.0f);
    TextEx(text, text_end, ImGuiTextFlags_NoWidthForLargeClippedText);
    PopTextWrapPos();
    SetCursorPos(pos + content);
}

} // namespace ImGui
