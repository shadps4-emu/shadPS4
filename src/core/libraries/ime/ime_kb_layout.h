// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <imgui.h>

#include "core/libraries/ime/ime_common.h"

namespace Libraries::Ime {

struct ImeViewportMetrics {
    ImVec2 size{};
    ImVec2 offset{};
    float scale_x = 1.0f;
    float scale_y = 1.0f;
    float ui_scale = 1.0f;
    float base_w = 1920.0f;
    float base_h = 1080.0f;
};

struct ImeKbGridLayout {
    ImVec2 pos{};
    ImVec2 size{};
    float key_gap_x = 0.0f;
    float key_gap_y = 0.0f;
    float key_h = 0.0f;
    int cols = 10;
    int rows = 6;
    float corner_radius = 0.0f;
};

struct ImeKbDrawParams {
    OrbisImeEnterLabel enter_label = OrbisImeEnterLabel::Default;
    ImU32 key_bg = IM_COL32(35, 35, 35, 255);
    ImU32 key_bg_alt = IM_COL32(50, 50, 50, 255);
    ImU32 key_border = IM_COL32(80, 80, 80, 255);
    ImU32 key_done = IM_COL32(30, 90, 170, 255);
    ImU32 key_text = IM_COL32(230, 230, 230, 255);
};

struct ImeKbDrawState {
    bool done_pressed = false;
};

struct ImePanelMetricsConfig {
    float panel_w = 0.0f;
    float panel_h = 0.0f;
    bool multiline = false;
    bool show_title = true;
    float base_font_size = 0.0f;
    ImVec2 window_pos{};
};

struct ImePanelMetrics {
    float panel_w = 0.0f;
    float panel_h = 0.0f;
    float padding_x = 0.0f;
    float padding_bottom = 0.0f;
    float label_h = 0.0f;
    float input_h = 0.0f;
    float predict_h = 0.0f;
    float close_w = 0.0f;
    float keys_h = 0.0f;
    float key_gap = 0.0f;
    float key_h = 0.0f;
    float corner_radius = 0.0f;
    float label_font_scale = 1.0f;
    float input_font_scale = 1.0f;
    float key_font_scale = 1.0f;
    float predict_gap = 0.0f;
    ImVec2 input_pos_local{};
    ImVec2 input_size{};
    ImVec2 input_pos_screen{};
    ImVec2 predict_pos{};
    ImVec2 predict_size{};
    ImVec2 close_pos{};
    ImVec2 close_size{};
    ImVec2 kb_pos{};
    ImVec2 kb_size{};
};

ImeViewportMetrics ComputeImeViewportMetrics(bool use_over2k);
ImePanelMetrics ComputeImePanelMetrics(const ImePanelMetricsConfig& config);

void DrawImeKeyboardGrid(const ImeKbGridLayout& layout, const ImeKbDrawParams& params,
                         ImeKbDrawState& state);

} // namespace Libraries::Ime
