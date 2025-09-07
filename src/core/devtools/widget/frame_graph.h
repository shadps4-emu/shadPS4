//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Devtools::Widget {

class FrameGraph {
    static constexpr u32 FRAME_BUFFER_SIZE = 1024;
    struct FrameInfo {
        u32 num;
        float delta;
    };

    std::array<FrameInfo, FRAME_BUFFER_SIZE> frame_list{};

    float deltaTime{};
    float frameRate{};

    void DrawFrameGraph();

public:
    bool is_open = true;

    void Draw();

    void AddFrame(u32 num, float delta) {
        frame_list[num % FRAME_BUFFER_SIZE] = FrameInfo{num, delta};
    }
};

} // namespace Core::Devtools::Widget