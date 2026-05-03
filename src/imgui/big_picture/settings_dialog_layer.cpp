// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>

#include "imgui/imgui_std.h"
#include "settings_dialog_layer.h"

namespace ImGuiEmuSettings {

static bool running = false;
static std::optional<SettingsLayer> settingsLayer = std::nullopt;

SettingsLayer::SettingsLayer() {
    AddLayer(this);
}

SettingsLayer::~SettingsLayer() {}

void SettingsLayer::Finish() {
    RemoveLayer(this);
    settingsLayer.reset();
}

void SettingsLayer::Draw() {
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[IMGUI_FONT_SETTINGS_WINDOW]);
    settingsWindow.DrawSettings(&running);
    ImGui::PopFont();

    if (!running) {
        Finish();
    }
}

void OpenInGameSettingsDialog() {
    if (settingsLayer.has_value()) {
        return;
    }

    settingsLayer.emplace();
    running = true;
}

} // namespace ImGuiEmuSettings
