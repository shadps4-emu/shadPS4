// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>

#include "common/config.h"
#include "common/types.h"
#include "imgui_internal.h"
#include "video_info.h"

using namespace ImGui;

struct FrameInfo {
    u32 num;
    float delta;
};

static bool show = false;
static bool show_advanced = false;

static u32 current_frame = 0;
constexpr float TARGET_FPS = 60.0f;
constexpr u32 FRAME_BUFFER_SIZE = 1024;
constexpr float BAR_WIDTH_MULT = 1.4f;
constexpr float BAR_HEIGHT_MULT = 1.25f;
constexpr float FRAME_GRAPH_PADDING_Y = 3.0f;
static std::array<FrameInfo, FRAME_BUFFER_SIZE> frame_list;
static float frame_graph_height = 50.0f;

static void DrawSimple() {
    const auto io = GetIO();
    Text("Frame time: %.3f ms (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
}

static void DrawAdvanced() {
    const auto& ctx = *GetCurrentContext();
    const auto& io = ctx.IO;
    const auto& window = *ctx.CurrentWindow;
    auto& draw_list = *window.DrawList;

    Text("Frame time: %.3f ms (%.1f FPS)", io.DeltaTime * 1000.0f, io.Framerate);

    SeparatorText("Frame graph");
    const float full_width = GetContentRegionAvail().x;
    { // Frame graph - inspired by
      // https://asawicki.info/news_1758_an_idea_for_visualization_of_frame_times
        auto pos = GetCursorScreenPos();
        const ImVec2 size{full_width, frame_graph_height + FRAME_GRAPH_PADDING_Y * 2.0f};
        ItemSize(size);
        if (!ItemAdd({pos, pos + size}, GetID("FrameGraph"))) {
            return;
        }

        float target_dt = 1.0f / (TARGET_FPS * (float)Config::vblankDiv());
        float cur_pos_x = pos.x + full_width;
        pos.y += FRAME_GRAPH_PADDING_Y;
        const float final_pos_y = pos.y + frame_graph_height;

        draw_list.AddRectFilled({pos.x, pos.y - FRAME_GRAPH_PADDING_Y},
                                {pos.x + full_width, final_pos_y + FRAME_GRAPH_PADDING_Y},
                                IM_COL32(0x33, 0x33, 0x33, 0xFF));
        draw_list.PushClipRect({pos.x, pos.y}, {pos.x + full_width, final_pos_y}, true);
        for (u32 i = 0; i < FRAME_BUFFER_SIZE; ++i) {
            const auto& frame_info = frame_list[(current_frame - i) % FRAME_BUFFER_SIZE];
            const float dt_factor = target_dt / frame_info.delta;

            const float width = std::ceil(BAR_WIDTH_MULT / dt_factor);
            const float height =
                std::min(std::log2(BAR_HEIGHT_MULT / dt_factor) / 3.0f, 1.0f) * frame_graph_height;

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
}

void Layers::VideoInfo::Draw() {
    const auto io = GetIO();

    const FrameInfo frame_info{
        .num = ++current_frame,
        .delta = io.DeltaTime,
    };
    frame_list[current_frame % FRAME_BUFFER_SIZE] = frame_info;

    if (IsKeyPressed(ImGuiKey_F10, false)) {
        const bool changed_ctrl = io.KeyCtrl != show_advanced;
        show_advanced = io.KeyCtrl;
        show = changed_ctrl || !show;
    }

    if (show) {
        if (show_advanced) {
            if (Begin("Video debug info", &show, 0)) {
                DrawAdvanced();
            }
        } else {
            if (Begin("Video Info", nullptr,
                      ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration |
                          ImGuiWindowFlags_AlwaysAutoResize)) {
                DrawSimple();
            }
        }
        End();
    }
}
