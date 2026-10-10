// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <string>

#include "imgui/imgui_layer.h"

union SDL_Event;

namespace Core::Devtools {

class Layer final : public ImGui::Layer {
public:
    static void SetupSettings();

    void Draw() override;
    bool ShouldKeepDrawing() override;

    // Must be inside a window
    static void DrawNullGpuNotice();

private:
    static void DrawMenuBar();
    static void DrawAdvanced();
    static void DrawSimple();
};

} // namespace Core::Devtools

namespace Overlay {

void ToggleSimpleFps();
void SetSimpleFps(bool enabled);
void ToggleQuitWindow();
bool ProcessQuitEvent(const SDL_Event& event);
bool IsQuitInputCaptured();
void ShowVolume();

void TextCentered(const std::string& text);

} // namespace Overlay
