// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>
#include "video_info.h"

void ImGui::Layers::VideoInfo::Draw() {
    const ImGuiIO& io = GetIO();

    m_show = IsKeyPressed(ImGuiKey_F10, false) ^ m_show;

    if (m_show) {
        if (Begin("Video Info")) {
            Text("Frame time: %.3f ms (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        }
        End();
    }
}
