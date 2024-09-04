// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <imgui.h>

namespace ImGui {

inline void CentralizeWindow() {
    const auto display_size = GetIO().DisplaySize;
    SetNextWindowPos(display_size / 2.0f, ImGuiCond_Always, {0.5f});
}
} // namespace ImGui
