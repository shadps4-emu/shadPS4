//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <variant>

#include "common/types.h"
#include "video_core/renderer_vulkan/liverpool_to_vk.h"

namespace Core::Devtools::Widget {

class RegPopup {
    int id;

    std::variant<AmdGpu::Liverpool::ColorBuffer> data;
    std::string title{};

    void DrawColorBuffer(const AmdGpu::Liverpool::ColorBuffer& buffer);

public:
    bool open = false;

    RegPopup();

    void SetData(AmdGpu::Liverpool::ColorBuffer color_buffer, u32 batch_id, u32 cb_id);

    void Draw();
};

} // namespace Core::Devtools::Widget
