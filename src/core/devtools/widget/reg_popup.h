//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <imgui.h>
#include "common/types.h"
#include "video_core/amdgpu/regs_color.h"
#include "video_core/amdgpu/regs_depth.h"

namespace Core::Devtools::Widget {

class RegPopup {
    int id;
    ImGuiWindowFlags flags{ImGuiWindowFlags_NoSavedSettings};

    ImVec2 last_pos;
    AmdGpu::ColorBuffer color;
    struct {
        AmdGpu::DepthBuffer buffer;
        AmdGpu::DepthControl control;
    } depth;
    enum class DataType {
        None = 0,
        Color = 1,
        Depth = 2,
    };
    DataType type{};
    std::string title{};

    static void DrawColorBuffer(const AmdGpu::ColorBuffer& buffer);

    static void DrawDepthBuffer(const AmdGpu::DepthBuffer& buffer,
                                const AmdGpu::DepthControl control);

public:
    bool open = false;
    bool moved = false;

    RegPopup();

    void SetData(const std::string& base_title, AmdGpu::ColorBuffer color_buffer, u32 cb_id);

    void SetData(const std::string& base_title, AmdGpu::DepthBuffer depth_buffer,
                 AmdGpu::DepthControl depth_control);

    void SetPos(ImVec2 pos, bool auto_resize = false);

    void Draw();
};

} // namespace Core::Devtools::Widget
