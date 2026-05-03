// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <SDL3/SDL.h>

#include "imgui/imgui_layer.h"
#include "settings_dialog_imgui.h"

namespace ImGuiEmuSettings {

class SettingsLayer final : public ImGui::Layer {
public:
    SettingsLayer();
    ~SettingsLayer() override;

    void Draw() override;

private:
    void Finish();
    SettingsWindow settingsWindow = SettingsWindow(true);
};

void OpenInGameSettingsDialog();

}; // namespace  ImGuiEmuSettings
