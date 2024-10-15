//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <variant>

#include <imgui.h>

#include "common/types.h"
#include "video_core/renderer_vulkan/liverpool_to_vk.h"

namespace Core::Devtools::Widget {

class RegPopup {
    int id;
    ImGuiWindowFlags flags{ImGuiWindowFlags_NoSavedSettings};

    using DepthBuffer = std::tuple<AmdGpu::Liverpool::DepthBuffer, AmdGpu::Liverpool::DepthControl>;

    std::variant<AmdGpu::Liverpool::ColorBuffer, DepthBuffer> data;
    std::string title{};

    static void DrawColorBuffer(const AmdGpu::Liverpool::ColorBuffer& buffer);

    static void DrawDepthBuffer(const DepthBuffer& depth_data);

public:
    bool open = false;

    RegPopup();

    void SetData(const std::string& base_title, AmdGpu::Liverpool::ColorBuffer color_buffer,
                 u32 cb_id);

    void SetData(const std::string& base_title, AmdGpu::Liverpool::DepthBuffer depth_buffer,
                 AmdGpu::Liverpool::DepthControl depth_control);

    void Draw(bool auto_resize = false);
};

} // namespace Core::Devtools::Widget
