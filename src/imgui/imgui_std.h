// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <imgui.h>

#include "imgui_internal.h"

namespace ImGui {

inline void CentralizeWindow() {
    const auto display_size = GetIO().DisplaySize;
    SetNextWindowPos(display_size / 2.0f, ImGuiCond_Always, {0.5f});
}

inline void KeepNavHighlight() {
    GetCurrentContext()->NavDisableHighlight = false;
}

inline void SetItemCurrentNavFocus() {
    const auto ctx = GetCurrentContext();
    SetFocusID(ctx->LastItemData.ID, ctx->CurrentWindow);
    ctx->NavInitResult.Clear();
}

} // namespace ImGui
