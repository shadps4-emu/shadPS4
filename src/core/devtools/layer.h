// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <string>

#include "imgui/imgui_layer.h"

namespace Core::Devtools {

class Layer final : public ImGui::Layer {
public:
    static void SetupSettings();

    void Draw() override;

    // Must be inside a window
    static void DrawNullGpuNotice();

private:
    static void DrawMenuBar();
    static void DrawAdvanced();
    static void DrawSimple();

    static void TextCentered(const std::string& text);
};

} // namespace Core::Devtools

namespace Overlay {

void ToggleSimpleFps();
void ToggleQuitWindow();

} // namespace Overlay
