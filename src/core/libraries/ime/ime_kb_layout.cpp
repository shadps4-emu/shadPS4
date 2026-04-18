// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ime/ime_kb_layout.h"

#include <algorithm>
#include <array>

#include "core/debug_state.h"

namespace Libraries::Ime {
namespace {
struct KeySpec {
    int span;
    const char* label;
    bool is_done;
};

constexpr float kPanelBaseW = 793.0f;
constexpr float kPanelBaseHSingle = 528.0f;
constexpr float kPanelBaseHMulti = 628.0f;
constexpr float kLabelH = 57.0f;
constexpr float kInputHSingle = 50.0f;
constexpr float kInputHMulti = 151.0f;
constexpr float kPredictH = 53.0f;
constexpr float kPredictW = 740.0f;
constexpr float kCloseW = 53.0f;
constexpr float kKeysH = 316.0f;
constexpr float kKeyGap = 9.0f;
constexpr float kPadX = 26.0f;
constexpr float kPadBottomSingle = 26.0f;
constexpr float kPadBottomMulti = 26.0f;
constexpr float kKeyFontRatio = 0.035f;
constexpr float kCornerRatio = 0.004f;
constexpr float kSingleLineTextFill = 0.85f;
constexpr float kMultiLineTextFill = 0.85f;
constexpr int kMultiLineVisibleLines = 4;
constexpr int kKeyRows = 6;
constexpr int kKeyCols = 10;

const char* GetEnterLabel(OrbisImeEnterLabel label) {
    switch (label) {
    case OrbisImeEnterLabel::Go:
        return "Go";
    case OrbisImeEnterLabel::Search:
        return "Search";
    case OrbisImeEnterLabel::Send:
        return "Send";
    case OrbisImeEnterLabel::Default:
    default:
        return "Done";
    }
}
} // namespace

ImeViewportMetrics ComputeImeViewportMetrics(bool use_over2k) {
    ImeViewportMetrics metrics{};
    metrics.base_w = use_over2k ? 3840.0f : 1920.0f;
    metrics.base_h = use_over2k ? 2160.0f : 1080.0f;

    const ImGuiIO& io = ImGui::GetIO();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 base_pos = viewport ? viewport->WorkPos : ImVec2{0.0f, 0.0f};
    ImVec2 base_size = viewport ? viewport->WorkSize : io.DisplaySize;
    if (base_size.x <= 0.0f || base_size.y <= 0.0f) {
        base_pos = {0.0f, 0.0f};
        base_size = io.DisplaySize;
    }

    metrics.size = base_size;
    metrics.offset = base_pos;

    const auto out_res = DebugState.output_resolution;
    if (out_res.first != 0 && out_res.second != 0) {
        const float fb_scale_x =
            io.DisplayFramebufferScale.x > 0.0f ? io.DisplayFramebufferScale.x : 1.0f;
        const float fb_scale_y =
            io.DisplayFramebufferScale.y > 0.0f ? io.DisplayFramebufferScale.y : 1.0f;
        const float viewport_w = static_cast<float>(out_res.first) / fb_scale_x;
        const float viewport_h = static_cast<float>(out_res.second) / fb_scale_y;

        float offset_x = (base_size.x - viewport_w) * 0.5f;
        float offset_y = (base_size.y - viewport_h) * 0.5f;
        if (offset_x < 0.0f) {
            offset_x = 0.0f;
        }
        if (offset_y < 0.0f) {
            offset_y = 0.0f;
        }

        metrics.size = {viewport_w, viewport_h};
        metrics.offset = {base_pos.x + offset_x, base_pos.y + offset_y};
    }

    metrics.scale_x = metrics.size.x / metrics.base_w;
    metrics.scale_y = metrics.size.y / metrics.base_h;
    metrics.ui_scale = std::min(metrics.scale_x, metrics.scale_y);
    return metrics;
}

ImePanelMetrics ComputeImePanelMetrics(const ImePanelMetricsConfig& config) {
    ImePanelMetrics metrics{};
    metrics.panel_w = config.panel_w;
    metrics.panel_h = config.panel_h;
    metrics.padding_x = metrics.panel_w * (kPadX / kPanelBaseW);
    metrics.padding_bottom =
        metrics.panel_h * (config.multiline ? (kPadBottomMulti / kPanelBaseHMulti)
                                            : (kPadBottomSingle / kPanelBaseHSingle));

    const float panel_scale = metrics.panel_w / kPanelBaseW;
    metrics.label_h = config.show_title ? (kLabelH * panel_scale) : metrics.padding_bottom;
    metrics.input_h = metrics.panel_h * (config.multiline ? (kInputHMulti / kPanelBaseHMulti)
                                                          : (kInputHSingle / kPanelBaseHSingle));
    metrics.predict_h = metrics.panel_h * (config.multiline ? (kPredictH / kPanelBaseHMulti)
                                                            : (kPredictH / kPanelBaseHSingle));
    metrics.close_w = metrics.panel_w * (kCloseW / kPanelBaseW);
    metrics.keys_h = metrics.panel_h * (config.multiline ? (kKeysH / kPanelBaseHMulti)
                                                         : (kKeysH / kPanelBaseHSingle));
    metrics.key_gap = metrics.panel_w * (kKeyGap / kPanelBaseW);
    metrics.corner_radius = metrics.panel_w * kCornerRatio;

    const float base_font_size = config.base_font_size > 0.0f ? config.base_font_size : 1.0f;
    metrics.label_font_scale = (metrics.label_h * 0.85f) / base_font_size;
    metrics.input_font_scale =
        (metrics.input_h * (config.multiline
                                ? (kMultiLineTextFill / static_cast<float>(kMultiLineVisibleLines))
                                : kSingleLineTextFill)) /
        base_font_size;
    metrics.key_font_scale = (metrics.panel_h * kKeyFontRatio) / base_font_size;

    metrics.input_pos_local = {metrics.padding_x, metrics.label_h};
    metrics.input_size = {metrics.panel_w - metrics.padding_x * 2.0f, metrics.input_h};
    metrics.input_pos_screen = {config.window_pos.x + metrics.input_pos_local.x,
                                config.window_pos.y + metrics.input_pos_local.y};

    const float remaining_gap =
        metrics.panel_h - (metrics.label_h + metrics.input_h + metrics.predict_h + metrics.keys_h +
                           metrics.padding_bottom);
    metrics.predict_gap = std::max(0.0f, remaining_gap * 0.5f);

    metrics.predict_pos = {config.window_pos.x, config.window_pos.y + metrics.label_h +
                                                    metrics.input_h + metrics.predict_gap};
    metrics.predict_size = {metrics.panel_w * (kPredictW / kPanelBaseW), metrics.predict_h};
    metrics.close_pos = {config.window_pos.x + metrics.panel_w - metrics.close_w,
                         metrics.predict_pos.y};
    metrics.close_size = {metrics.close_w, metrics.predict_h};

    metrics.kb_pos = {config.window_pos.x + metrics.padding_x,
                      metrics.predict_pos.y + metrics.predict_h + metrics.predict_gap};
    metrics.kb_size = {metrics.panel_w - metrics.padding_x * 2.0f, metrics.keys_h};

    metrics.key_h =
        (metrics.kb_size.y - metrics.key_gap * static_cast<float>(kKeyRows - 1)) / kKeyRows;
    if (metrics.key_h < 8.0f) {
        metrics.key_h = 8.0f;
    }

    return metrics;
}

void DrawImeKeyboardGrid(const ImeKbGridLayout& layout, const ImeKbDrawParams& params,
                         ImeKbDrawState& state) {
    auto* draw = ImGui::GetWindowDrawList();
    if (!draw || layout.cols <= 0 || layout.rows <= 0 || layout.size.x <= 0.0f ||
        layout.size.y <= 0.0f) {
        return;
    }

    const float key_gap_x = layout.key_gap_x;
    const float key_gap_y = layout.key_gap_y;
    const float key_h = layout.key_h;
    const float key_w = (layout.size.x - key_gap_x * (layout.cols - 1)) / layout.cols;

    const auto draw_key = [&](ImVec2 pos, ImVec2 size, ImU32 bg) {
        draw->AddRectFilled(pos, {pos.x + size.x, pos.y + size.y}, bg, layout.corner_radius);
        draw->AddRect(pos, {pos.x + size.x, pos.y + size.y}, params.key_border,
                      layout.corner_radius);
    };
    const auto draw_key_label = [&](ImVec2 pos, ImVec2 size, ImU32 bg, const char* label) {
        draw_key(pos, size, bg);
        if (label && label[0] != '\0') {
            ImVec2 text_size = ImGui::CalcTextSize(label);
            ImVec2 text_pos{pos.x + (size.x - text_size.x) * 0.5f,
                            pos.y + (size.y - text_size.y) * 0.5f};
            draw->AddText(text_pos, params.key_text, label);
        }
    };

    constexpr KeySpec blank{1, nullptr, false};
    const std::array<KeySpec, kKeyCols> row_default = {blank, blank, blank, blank, blank,
                                                       blank, blank, blank, blank, blank};
    const std::array<KeySpec, 6> row_space = {
        blank, blank, blank, {4, nullptr, false}, blank, {2, nullptr, false},
    };
    const std::array<KeySpec, 9> row_done = {
        blank, blank, blank, blank, blank, blank, blank, blank, {2, nullptr, true}};

    auto draw_row = [&](int row_index, const auto& row) {
        float x = layout.pos.x;
        float y = layout.pos.y + row_index * (key_h + key_gap_y);
        for (const auto& key : row) {
            const float span_w = key_w * key.span + key_gap_x * (key.span - 1);
            ImVec2 pos{x, y};
            ImVec2 size{span_w, key_h};
            ImU32 bg = params.key_bg;
            if (row_index >= layout.rows - 2) {
                bg = params.key_bg_alt;
            }
            if (key.is_done) {
                bg = params.key_done;
            }
            const char* label = key.is_done ? GetEnterLabel(params.enter_label) : key.label;
            if (label && label[0] != '\0') {
                draw_key_label(pos, size, bg, label);
            } else {
                draw_key(pos, size, bg);
            }
            if (key.is_done) {
                ImGui::SetCursorScreenPos(pos);
                ImGui::InvisibleButton("##ImeDialogDoneKey", size);
                if (ImGui::IsItemClicked()) {
                    state.done_pressed = true;
                }
            }
            x += span_w + key_gap_x;
        }
    };

    draw_row(0, row_default);
    draw_row(1, row_default);
    draw_row(2, row_default);
    draw_row(3, row_default);
    draw_row(4, row_space);
    draw_row(5, row_done);
}

} // namespace Libraries::Ime
