//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "frame_graph.h"

#include "common/config.h"
#include "common/singleton.h"
#include "core/debug_state.h"
#include "imgui.h"
#include "imgui_internal.h"

using namespace ImGui;

namespace Core::Devtools::Widget {

constexpr float TARGET_FPS = 60.0f;
constexpr float BAR_WIDTH_MULT = 1.4f;
constexpr float BAR_HEIGHT_MULT = 1.25f;
constexpr float FRAME_GRAPH_PADDING_Y = 3.0f;
constexpr static float FRAME_GRAPH_HEIGHT = 50.0f;

void FrameGraph::Draw() {
    if (!is_open) {
        return;
    }
    SetNextWindowSize({340.0, 185.0f}, ImGuiCond_FirstUseEver);
    if (Begin("Video debug info", &is_open)) {
        const auto& ctx = *GImGui;
        const auto& io = ctx.IO;
        const auto& window = *ctx.CurrentWindow;
        auto& draw_list = *window.DrawList;

        auto isSystemPaused = DebugState.IsGuestThreadsPaused();

        static float deltaTime;
        static float frameRate;

        if (!isSystemPaused) {
            deltaTime = io.DeltaTime * 1000.0f;
            frameRate = 1000.0f / deltaTime;
        }

        Text("Frame time: %.3f ms (%.1f FPS)", deltaTime, frameRate);
        Text("Flip frame: %d Gnm submit frame: %d", DebugState.flip_frame_count.load(),
             DebugState.gnm_frame_count.load());
        SeparatorText("Frame graph");

        const float full_width = GetContentRegionAvail().x;
        // Frame graph - inspired by
        // https://asawicki.info/news_1758_an_idea_for_visualization_of_frame_times
        auto pos = GetCursorScreenPos();
        const ImVec2 size{full_width, FRAME_GRAPH_HEIGHT + FRAME_GRAPH_PADDING_Y * 2.0f};
        ItemSize(size);
        if (!ItemAdd({pos, pos + size}, GetID("FrameGraph"))) {
            return;
        }

        float target_dt = 1.0f / (TARGET_FPS * (float)Config::vblankDiv());
        float cur_pos_x = pos.x + full_width;
        pos.y += FRAME_GRAPH_PADDING_Y;
        const float final_pos_y = pos.y + FRAME_GRAPH_HEIGHT;

        draw_list.AddRectFilled({pos.x, pos.y - FRAME_GRAPH_PADDING_Y},
                                {pos.x + full_width, final_pos_y + FRAME_GRAPH_PADDING_Y},
                                IM_COL32(0x33, 0x33, 0x33, 0xFF));
        draw_list.PushClipRect({pos.x, pos.y}, {pos.x + full_width, final_pos_y}, true);
        for (u32 i = 0; i < FRAME_BUFFER_SIZE; ++i) {
            const auto& frame_info = frame_list[(DebugState.GetFrameNum() - i) % FRAME_BUFFER_SIZE];
            const float dt_factor = target_dt / frame_info.delta;

            const float width = std::ceil(BAR_WIDTH_MULT / dt_factor);
            const float height =
                std::min(std::log2(BAR_HEIGHT_MULT / dt_factor) / 3.0f, 1.0f) * FRAME_GRAPH_HEIGHT;

            ImU32 color;
            if (dt_factor >= 0.95f) { // BLUE
                color = IM_COL32(0x33, 0x33, 0xFF, 0xFF);
            } else if (dt_factor >= 0.5f) { // GREEN <> YELLOW
                float t = 1.0f - (dt_factor - 0.5f) * 2.0f;
                int r = (int)(0xFF * t);
                color = IM_COL32(r, 0xFF, 0, 0xFF);
            } else { // YELLOW <> RED
                float t = dt_factor * 2.0f;
                int g = (int)(0xFF * t);
                color = IM_COL32(0xFF, g, 0, 0xFF);
            }
            draw_list.AddRectFilled({cur_pos_x - width, final_pos_y - height},
                                    {cur_pos_x, final_pos_y}, color);
            cur_pos_x -= width;
            if (cur_pos_x < width) {
                break;
            }
        }
        draw_list.PopClipRect();
    }
    End();
}

} // namespace Core::Devtools::Widget
