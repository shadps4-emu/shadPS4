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

inline void CentralizeNextWindow() {
    const auto display_size = GetIO().DisplaySize;
    SetNextWindowPos(display_size / 2.0f, ImGuiCond_Always, {0.5f});
}

inline void CentralizeWindow() {
    const auto display_size = GetIO().DisplaySize - GetCurrentWindowRead()->SizeFull;
    SetWindowPos(display_size / 2.0f);
}

inline void KeepWindowInside(ImVec2 display_size = GetIO().DisplaySize) {
    const auto cur_pos = GetWindowPos();
    if (cur_pos.x < 0.0f || cur_pos.y < 0.0f) {
        SetWindowPos(ImMax(cur_pos, ImVec2(0.0f, 0.0f)));
        return;
    }
    const auto cur_size = GetCurrentWindowRead()->SizeFull;
    const auto bottom_right = cur_pos + cur_size;
    if (bottom_right.x > display_size.x || bottom_right.y > display_size.y) {
        const auto max_pos = display_size - cur_size;
        SetWindowPos(ImMin(cur_pos, max_pos));
    }
}

inline void KeepNavHighlight() {
    GetCurrentContext()->NavCursorVisible = true;
}

inline void SetItemCurrentNavFocus(const ImGuiID id = -1) {
    const auto ctx = GetCurrentContext();
    SetFocusID(id == -1 ? ctx->LastItemData.ID : id, ctx->CurrentWindow);
    ctx->NavInitResult.Clear();
    ctx->NavCursorVisible = true;
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

// Limited-length InputTextEx wrapper (limits UTF-8 code points)
// - max_chars counts Unicode code points, not bytes
// - Works for single-line and multi-line
// - Chains user callbacks if provided
struct InputTextLimitCtx {
    int max_chars;
    ImGuiInputTextCallback user_cb;
    void* user_user_data;
    ImGuiInputTextFlags forward_flags; // original flags requested by caller
};

inline int InputTextLimitCallback(ImGuiInputTextCallbackData* data) {
    InputTextLimitCtx* ctx = static_cast<InputTextLimitCtx*>(data->UserData);
    if (ctx && ctx->user_cb) {
        const ImGuiInputTextFlags ev = data->EventFlag;
        const ImGuiInputTextFlags ff = ctx->forward_flags;
        const bool should_forward =
            ((ev == ImGuiInputTextFlags_CallbackAlways) &&
             (ff & ImGuiInputTextFlags_CallbackAlways)) ||
            ((ev == ImGuiInputTextFlags_CallbackEdit) && (ff & ImGuiInputTextFlags_CallbackEdit)) ||
            ((ev == ImGuiInputTextFlags_CallbackCharFilter) &&
             (ff & ImGuiInputTextFlags_CallbackCharFilter)) ||
            ((ev == ImGuiInputTextFlags_CallbackCompletion) &&
             (ff & ImGuiInputTextFlags_CallbackCompletion)) ||
            ((ev == ImGuiInputTextFlags_CallbackHistory) &&
             (ff & ImGuiInputTextFlags_CallbackHistory)) ||
            ((ev == ImGuiInputTextFlags_CallbackResize) &&
             (ff & ImGuiInputTextFlags_CallbackResize));
        if (should_forward) {
            void* orig = data->UserData;
            data->UserData = ctx->user_user_data;
            int user_ret = ctx->user_cb(data);
            data->UserData = orig;
            if (user_ret != 0) {
                return user_ret;
            }
        }
    }

    if (!ctx || ctx->max_chars < 0) {
        return 0;
    }

    // Enforce limit: discard extra characters on filter, trim on edit
    if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
        ImGuiContext* g = data->Ctx;
        if (!g) {
            return 0;
        }
        ImGuiInputTextState* st = &g->InputTextState;
        if (st == nullptr || st->TextSrc == nullptr) {
            return 0;
        }
        int cur_chars = ImTextCountCharsFromUtf8(st->TextSrc, st->TextSrc + st->TextLen);
        int sel_chars = 0;
        if (st->HasSelection()) {
            const int ib = st->GetSelectionStart();
            const int ie = st->GetSelectionEnd();
            sel_chars = ImTextCountCharsFromUtf8(st->TextSrc + ib, st->TextSrc + ie);
        }
        const int remaining = ctx->max_chars - (cur_chars - sel_chars);
        if (remaining <= 0) {
            data->EventChar = 0;
            return 1; // discard
        }
        return 0;
    }

    if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
        // Trim tail to ensure text length <= max_chars code points
        const char* s = data->Buf;
        const char* end = s + data->BufTextLen;
        const char* p = s;
        int codepoints = 0;
        while (p < end && codepoints < ctx->max_chars) {
            unsigned int c;
            int len = ImTextCharFromUtf8(&c, p, end);
            if (len <= 0)
                break;
            p += len;
            codepoints++;
        }
        if (p < end) {
            const int keep_bytes = static_cast<int>(p - s);
            data->DeleteChars(keep_bytes, data->BufTextLen - keep_bytes);
            if (data->CursorPos > data->BufTextLen)
                data->CursorPos = data->BufTextLen;
            if (data->SelectionStart > data->BufTextLen)
                data->SelectionStart = data->BufTextLen;
            if (data->SelectionEnd > data->BufTextLen)
                data->SelectionEnd = data->BufTextLen;
        }
        return 0;
    }

    return 0;
}

inline bool InputTextExLimited(const char* label, const char* hint, char* buf, int buf_size,
                               const ImVec2& size_arg, ImGuiInputTextFlags flags, int max_chars,
                               ImGuiInputTextCallback callback = nullptr,
                               void* user_data = nullptr) {
    InputTextLimitCtx ctx{max_chars, callback, user_data, flags};
    ImGuiInputTextFlags flags2 =
        flags | ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackEdit;
    return InputTextEx(label, hint, buf, buf_size, size_arg, flags2, InputTextLimitCallback, &ctx);
}

} // namespace ImGui
