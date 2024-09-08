// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "imgui/imgui_layer.h"

namespace Vulkan {
class RendererVulkan;
}
namespace ImGui::Layers {

class VideoInfo : public Layer {
    bool m_show = false;
    ::Vulkan::RendererVulkan* renderer{};

public:
    explicit VideoInfo(::Vulkan::RendererVulkan* renderer) : renderer(renderer) {}

    void Draw() override;
};

} // namespace ImGui::Layers
