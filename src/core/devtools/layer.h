// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "imgui/imgui_layer.h"

namespace Core::Devtools {

class Layer final : public ImGui::Layer {

    static void DrawMenuBar();

    static void DrawAdvanced();

    static void DrawSimple();

public:
    static void SetupSettings();

    void Draw() override;
};

} // namespace Core::Devtools
