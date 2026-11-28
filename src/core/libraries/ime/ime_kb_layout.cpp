// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ime/ime_kb_layout.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <span>
#include <unordered_map>
#include <vector>

#include "core/debug_state.h"
#include "core/libraries/ime/ime_kb_language.h"
#include "core/libraries/ime/ime_ui_shared.h"

namespace Libraries::Ime {
namespace {

constexpr float kPanelBaseW = 793.0f;
constexpr float kNumericPanelBaseW = 370.0f;
constexpr float kPanelBaseHSingle = 528.0f;
constexpr float kPanelBaseHMulti = 628.0f;
constexpr float kLabelH = 57.0f;
constexpr float kInputHSingle = 50.0f;
constexpr float kInputHMulti = 151.0f;
constexpr float kPredictH = 53.0f;
constexpr float kKeysH = 316.0f;
constexpr float kKeyGap = 9.0f;
constexpr float kPadX = 26.0f;
constexpr float kPadBottom = 26.0f;
constexpr float kRawPanelBaseH = 408.0f;
constexpr float kRawNumericPanelBaseH = 402.0f;
constexpr float kKeyFontRatio = 0.042f;
constexpr float kTypingKeyLabelScale = 1.60f;
constexpr float kFunctionKeyLabelScale = 1.26f;
constexpr float kHotkeyLabelScale = 0.74f;
constexpr float kCornerRatio = 0.004f;
constexpr float kInputFontSize = 32.0f;
constexpr int kKeyRows = 6;
u16 Utf8FirstCodeUnit(const char* text) {
    if (!text || text[0] == '\0') {
        return 0;
    }
    const unsigned char c0 = static_cast<unsigned char>(text[0]);
    if ((c0 & 0x80u) == 0) {
        return static_cast<u16>(c0);
    }
    if ((c0 & 0xE0u) == 0xC0u && text[1] != '\0') {
        const unsigned char c1 = static_cast<unsigned char>(text[1]);
        if ((c1 & 0xC0u) == 0x80u) {
            return static_cast<u16>(((c0 & 0x1Fu) << 6) | (c1 & 0x3Fu));
        }
        return 0;
    }
    if ((c0 & 0xF0u) == 0xE0u && text[1] != '\0' && text[2] != '\0') {
        const unsigned char c1 = static_cast<unsigned char>(text[1]);
        const unsigned char c2 = static_cast<unsigned char>(text[2]);
        if ((c1 & 0xC0u) == 0x80u && (c2 & 0xC0u) == 0x80u) {
            return static_cast<u16>(((c0 & 0x0Fu) << 12) | ((c1 & 0x3Fu) << 6) | (c2 & 0x3Fu));
        }
        return 0;
    }
    return 0;
}

ImeKbKeyGlyph ResolveShiftOverrideGlyph(const ImeKbLayoutSelection& selection,
                                        const ImeKbKeySpec& key, ImeKbKeyGlyph fallback_glyph) {
    if (key.action != ImeKbKeyAction::Shift) {
        return fallback_glyph;
    }

    switch (selection.case_state) {
    case ImeKbCaseState::Upper:
        return ImeKbKeyGlyph::ShiftFilled;
    case ImeKbCaseState::CapsLock:
        return ImeKbKeyGlyph::CapsLockFilled;
    case ImeKbCaseState::Lower:
    default:
        return ImeKbKeyGlyph::ShiftOutline;
    }
}

const char* ResolveLanguageOverrideLabel(const ImeKbLayoutSelection& selection,
                                         const ImeKbKeySpec& key, const char* fallback_label) {
    const auto& labels =
        GetImeKeyboardLanguageProfile(selection.language, selection.variant).GetLabels();
    switch (key.action) {
    case ImeKbKeyAction::Space:
        return labels.space_label;
    case ImeKbKeyAction::AlternateMode:
        return selection.family == ImeKbLayoutFamily::Alphabet ? labels.accents_label
                                                               : labels.letters_label;
    case ImeKbKeyAction::SymbolsMode:
        return selection.family == ImeKbLayoutFamily::Symbols ? labels.letters_label
                                                              : labels.symbols_label;
    default:
        return fallback_label;
    }
}

const char* GetEnterLabel(OrbisImeEnterLabel label, const ImeKbLanguageProfile& profile) {
    const auto& labels = profile.GetLabels().enter_labels;
    const auto index = static_cast<std::size_t>(label);
    return labels[index < labels.size() ? index : 0];
}

constexpr std::array<ImeTopPanelElementSpec, 2> kTopPanelDefaultElements = {{
    ImeTopPanelElementSpec{ImeTopPanelElementId::Prediction, 0, 9},
    ImeTopPanelElementSpec{ImeTopPanelElementId::Close, 9, 1},
}};

constexpr ImeTopPanelLayoutConfig kTopPanelDefaultLayout{
    kTopPanelDefaultElements.data(),
    kTopPanelDefaultElements.size(),
    10,
    static_cast<u8>(ImeSelectionGridIndex::DefaultTopPanelRow),
    static_cast<u8>(ImeSelectionGridIndex::DefaultTopPanelRows),
};

constexpr std::array<ImeTopPanelElementSpec, 1> kTopPanelNumericElements = {{
    ImeTopPanelElementSpec{ImeTopPanelElementId::Close, 2, 1},
}};
constexpr ImeTopPanelLayoutConfig kTopPanelNumericLayout{kTopPanelNumericElements.data(),
                                                         kTopPanelNumericElements.size(), 3, 0, 1};

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

ImVec2 ReprojectImePanelPosition(const ImVec2& position, const ImeViewportMetrics& previous,
                                 const ImeViewportMetrics& current) {
    if (previous.size.x <= 0.0f || previous.size.y <= 0.0f ||
        (previous.size.x == current.size.x && previous.size.y == current.size.y &&
         previous.offset.x == current.offset.x && previous.offset.y == current.offset.y)) {
        return position;
    }
    return {current.offset.x + (position.x - previous.offset.x) * current.size.x / previous.size.x,
            current.offset.y + (position.y - previous.offset.y) * current.size.y / previous.size.y};
}

float ComputeImeRawPanelHeight(float requested_height, bool numeric, bool multiline,
                               bool direct_input, bool hide_keyboard) {
    if (!direct_input) {
        return requested_height;
    }
    const float base_height =
        (numeric ? kRawNumericPanelBaseH : kRawPanelBaseH) - (hide_keyboard ? kKeysH : 0.0f);
    const float input_height = multiline ? kInputHMulti : kInputHSingle;
    return requested_height * (1.0f + input_height / base_height);
}

ImePanelMetrics ComputeImePanelMetrics(const ImePanelMetricsConfig& config) {
    ImePanelMetrics metrics{};
    metrics.panel_w = config.panel_w;
    metrics.panel_h = config.panel_h;
    const bool raw_panel = config.raw_panel;
    const float raw_base_h = config.numeric ? kRawNumericPanelBaseH : kRawPanelBaseH;
    const float input_base_h = config.multiline ? kInputHMulti : kInputHSingle;
    const float panel_base_h = raw_panel
                                   ? raw_base_h + (config.show_input ? input_base_h : 0.0f)
                                   : (config.multiline ? kPanelBaseHMulti : kPanelBaseHSingle);
    const float panel_base_w = config.numeric ? kNumericPanelBaseW : kPanelBaseW;
    metrics.padding_x = metrics.panel_w * (kPadX / panel_base_w);
    metrics.padding_bottom = metrics.panel_h * kPadBottom / panel_base_h;

    const float panel_scale = metrics.panel_w / panel_base_w;
    metrics.label_h = config.show_title ? (kLabelH * panel_scale) : metrics.padding_bottom;
    if (raw_panel) {
        metrics.label_h =
            metrics.panel_h * (raw_base_h - kPredictH - kKeysH - kPadBottom) / panel_base_h;
    }
    const float input_h = metrics.panel_h * input_base_h / panel_base_h;
    metrics.input_h = config.show_input ? input_h : 0.0f;
    metrics.predict_h = metrics.panel_h * kPredictH / panel_base_h;
    metrics.keys_h = metrics.panel_h * kKeysH / panel_base_h;
    metrics.key_gap = metrics.panel_w * (kKeyGap / panel_base_w);
    metrics.corner_radius = metrics.panel_w * kCornerRatio;

    if (config.hide_keyboard) {
        metrics.keys_h = 0.0f;
        metrics.predict_h = std::min(metrics.panel_h, kPredictH * panel_scale);
        metrics.input_h = config.show_input
                              ? std::min(input_base_h * panel_scale,
                                         std::max(0.0f, metrics.panel_h - metrics.predict_h))
                              : 0.0f;
        const float available =
            std::max(0.0f, metrics.panel_h - metrics.predict_h - metrics.input_h);
        metrics.label_h = std::min((raw_panel ? raw_base_h - kPredictH - kKeysH - kPadBottom
                                    : config.show_title ? kLabelH
                                                        : 0.0f) *
                                       panel_scale,
                                   available);
        metrics.padding_bottom = available - metrics.label_h;
    }

    const float keyboard_gap =
        config.hide_keyboard ? 0.0f : std::min(metrics.key_gap, metrics.predict_h);
    metrics.predict_h -= keyboard_gap;

    const float base_font_size = config.base_font_size > 0.0f ? config.base_font_size : 1.0f;
    metrics.label_font_scale = (metrics.label_h * 0.85f) / base_font_size;
    metrics.input_font_scale = (kInputFontSize * panel_scale) / base_font_size;
    metrics.key_font_scale = (kPanelBaseHSingle * kKeyFontRatio * panel_scale) / base_font_size;

    metrics.input_pos_local = {metrics.padding_x, metrics.label_h};
    metrics.input_size = {metrics.panel_w - metrics.padding_x * 2.0f, metrics.input_h};
    metrics.input_pos_screen = {config.window_pos.x + metrics.input_pos_local.x,
                                config.window_pos.y + metrics.input_pos_local.y};

    const float remaining_gap =
        metrics.panel_h - (metrics.label_h + metrics.input_h + metrics.predict_h + metrics.keys_h +
                           metrics.padding_bottom + keyboard_gap);
    metrics.predict_gap = std::max(0.0f, remaining_gap * 0.5f);

    metrics.predict_pos = {config.window_pos.x, config.window_pos.y + metrics.label_h +
                                                    metrics.input_h + metrics.predict_gap};
    metrics.close_w = metrics.predict_h;
    metrics.predict_size = {metrics.panel_w - metrics.close_w, metrics.predict_h};
    metrics.close_pos = {config.window_pos.x + metrics.panel_w - metrics.close_w,
                         metrics.predict_pos.y};
    metrics.close_size = {metrics.close_w, metrics.predict_h};

    metrics.kb_pos = {config.window_pos.x + metrics.padding_x,
                      metrics.predict_pos.y + metrics.predict_h + metrics.predict_gap +
                          keyboard_gap};
    metrics.kb_size = {metrics.panel_w - metrics.padding_x * 2.0f, metrics.keys_h};

    metrics.key_h =
        (metrics.kb_size.y - metrics.key_gap * static_cast<float>(kKeyRows - 1)) / kKeyRows;
    if (config.hide_keyboard) {
        metrics.key_h = 0.0f;
    } else if (metrics.key_h < 8.0f) {
        metrics.key_h = 8.0f;
    }

    if (config.accessibility && !config.hide_keyboard) {
        const float scale = std::min(config.panel_w / 1920.0f, config.panel_h / 1080.0f);
        const float keyboard_width = (config.numeric ? 510.0f : 1161.0f) * scale;
        metrics.key_gap = 9.0f * scale;
        metrics.key_h = 76.0f * scale;
        metrics.keys_h = 496.0f * scale;
        metrics.kb_size = {keyboard_width, metrics.keys_h};
        metrics.kb_pos = {config.window_pos.x + (config.panel_w - keyboard_width) * 0.5f,
                          config.window_pos.y + config.panel_h - 80.0f * scale - metrics.keys_h};
        metrics.key_font_scale = 32.0f * scale / base_font_size;
        metrics.corner_radius = 4.0f * scale;
    }
    return metrics;
}

const ImeTopPanelLayoutConfig& GetImeTopPanelLayoutConfig(const ImeKbLayoutSelection& selection) {
    return selection.family == ImeKbLayoutFamily::Numeric ? kTopPanelNumericLayout
                                                          : kTopPanelDefaultLayout;
}

ImU32 ImeColorToImU32(const OrbisImeColor& color) {
    return IM_COL32(color.r, color.g, color.b, color.a);
}

ImVec4 ImeColorToImVec4(const OrbisImeColor& color) {
    return ImGui::ColorConvertU32ToFloat4(ImeColorToImU32(color));
}

ImeStyleConfig GetDefaultImeStyleConfig() {
    return ImeStyleConfig{};
}

ImeStyleConfig ResolveImeStyleConfig(const OrbisImeParamExtended* extended) {
    ImeStyleConfig style = GetDefaultImeStyleConfig();
    if (!extended) {
        return style;
    }
    if (!True(extended->option & OrbisImeExtOption::SET_COLOR)) {
        return style;
    }

    style.color_base = extended->color_base;
    style.color_line = extended->color_line;
    style.color_text_field = extended->color_text_field;
    style.color_preedit = extended->color_preedit;
    style.color_button_default = extended->color_button_default;
    style.color_button_function = extended->color_button_function;
    style.color_button_symbol = extended->color_button_symbol;
    style.color_text = extended->color_text;
    style.color_special = extended->color_special;
    return style;
}

void ApplyImeStyleToKeyboardDrawParams(const ImeStyleConfig& style, ImeKbDrawParams& params) {
    params.key_bg_default = ImeColorToImU32(style.color_button_default);
    params.key_bg_function = ImeColorToImU32(style.color_button_function);
    params.key_bg_symbol = ImeColorToImU32(style.color_button_symbol);
    params.key_border = ImeColorToImU32(style.color_line);
    params.key_done = ImeColorToImU32(style.color_special);
    params.key_text = ImeColorToImU32(style.color_text);

    ImVec4 hotkey = ImeColorToImVec4(style.color_text);
    hotkey.x = std::clamp(hotkey.x * 0.92f, 0.0f, 1.0f);
    hotkey.y = std::clamp(hotkey.y * 0.92f, 0.0f, 1.0f);
    hotkey.z = std::clamp(hotkey.z * 0.92f, 0.0f, 1.0f);
    params.key_hotkey_text = ImGui::ColorConvertFloat4ToU32(hotkey);
}

void DrawImeKeyboardGrid(const ImeKbGridLayout& layout, const ImeKbDrawParams& params,
                         ImeKbDrawState& state) {
    const auto& language_profile =
        GetImeKeyboardLanguageProfile(params.selection.language, params.selection.variant);
    state.done_pressed = false;
    state.pressed_repeat = false;
    state.pressed_action = ImeKbKeyAction::None;
    state.pressed_text = nullptr;
    state.selected_row = -1;
    state.selected_col = -1;
    state.selected_center = {};
    state.hovered = false;
    state.clicked = false;

    auto* draw = ImGui::GetWindowDrawList();
    if (!draw || layout.cols <= 0 || layout.rows <= 0 || layout.size.x <= 0.0f ||
        layout.size.y <= 0.0f) {
        return;
    }

    const ImeKbLayoutModel* model = params.layout_model;
    if (!model) {
        model = &GetImeKeyboardLayout(params.selection);
    }
    if (!model->keys || model->key_count == 0) {
        model = &GetImeKeyboardLayout({});
    }

    const int grid_cols = layout.cols;
    const int grid_rows = layout.rows;
    const float key_gap_x = layout.key_gap_x;
    const float key_gap_y = layout.key_gap_y;
    const float key_h = layout.key_h;
    const float key_w = (layout.size.x - key_gap_x * (grid_cols - 1)) / grid_cols;
    const int fixed_bottom_rows = std::clamp(layout.fixed_bottom_rows, 0, grid_rows);

    thread_local std::vector<float> row_heights;
    row_heights.assign(static_cast<std::size_t>(grid_rows), key_h);
    if (fixed_bottom_rows > 0 && layout.bottom_row_h > 0.0f) {
        for (int row = grid_rows - fixed_bottom_rows; row < grid_rows; ++row) {
            row_heights[static_cast<std::size_t>(row)] = layout.bottom_row_h;
        }
    }

    thread_local std::vector<float> row_offsets;
    row_offsets.resize(static_cast<std::size_t>(grid_rows));
    float y_cursor = layout.pos.y;
    for (int row = 0; row < grid_rows; ++row) {
        row_offsets[static_cast<std::size_t>(row)] = y_cursor;
        y_cursor += row_heights[static_cast<std::size_t>(row)];
        if (row + 1 < grid_rows) {
            y_cursor += key_gap_y;
        }
    }

    const auto span_row_height = [&](int row, int span) {
        float total = 0.0f;
        for (int i = 0; i < span; ++i) {
            total += row_heights[static_cast<std::size_t>(row + i)];
        }
        if (span > 1) {
            total += key_gap_y * static_cast<float>(span - 1);
        }
        return total;
    };

    const auto idx = [grid_cols](int row, int col) { return row * grid_cols + col; };
    thread_local std::vector<const ImeKbKeySpec*> occupied;
    thread_local std::vector<bool> is_anchor;
    occupied.assign(static_cast<std::size_t>(grid_rows * grid_cols), nullptr);
    is_anchor.assign(static_cast<std::size_t>(grid_rows * grid_cols), false);

    for (std::size_t i = 0; i < model->key_count; ++i) {
        const ImeKbKeySpec& key = model->keys[i];
        if (key.col_span == 0 || key.row_span == 0) {
            continue;
        }
        if (key.row >= grid_rows || key.col >= grid_cols) {
            continue;
        }

        const int row_start = key.row;
        const int col_start = key.col;
        const int row_end = std::min(grid_rows, static_cast<int>(key.row + key.row_span));
        const int col_end = std::min(grid_cols, static_cast<int>(key.col + key.col_span));
        is_anchor[static_cast<std::size_t>(idx(row_start, col_start))] = true;
        for (int row = row_start; row < row_end; ++row) {
            for (int col = col_start; col < col_end; ++col) {
                occupied[static_cast<std::size_t>(idx(row, col))] = &key;
            }
        }
    }

    struct RenderedKey {
        const ImeKbKeySpec* key = nullptr;
        int row = 0;
        int col = 0;
        u8 col_span = 1;
        u8 row_span = 1;
        ImVec2 pos{};
        ImVec2 size{};
        ImVec2 center{};
        ImU32 bg = 0;
        const char* label = nullptr;
        const char* hotkey_label = nullptr;
        ImeKbKeyGlyph glyph = ImeKbKeyGlyph::None;
        bool underline_label = false;
        bool is_done = false;
        bool disabled_visual = false;
        bool selectable = false;
    };

    thread_local std::vector<RenderedKey> rendered_keys;
    rendered_keys.clear();
    rendered_keys.reserve(static_cast<std::size_t>(grid_cols * grid_rows));

    for (int row = 0; row < grid_rows; ++row) {
        for (int col = 0; col < grid_cols; ++col) {
            const std::size_t cell_index = static_cast<std::size_t>(idx(row, col));
            const ImeKbKeySpec* key = occupied[cell_index];
            if (key && !is_anchor[cell_index]) {
                continue;
            }

            u8 col_span = 1;
            u8 row_span = 1;
            if (key) {
                col_span =
                    static_cast<u8>(std::min(grid_cols - col, static_cast<int>(key->col_span)));
                row_span =
                    static_cast<u8>(std::min(grid_rows - row, static_cast<int>(key->row_span)));
            }

            const float x = layout.pos.x + col * (key_w + key_gap_x);
            const float y = row_offsets[static_cast<std::size_t>(row)];
            const float w = key_w * col_span + key_gap_x * (col_span - 1);
            const float h = span_row_height(row, row_span);
            ImVec2 pos{x, y};
            ImVec2 size{w, h};

            const auto slot_base_bg = [&](int slot_row) {
                const bool function_row =
                    fixed_bottom_rows > 0 && slot_row >= std::max(0, grid_rows - fixed_bottom_rows);
                if (function_row) {
                    return params.key_bg_function;
                }
                if (params.selection.family == ImeKbLayoutFamily::Symbols) {
                    return params.key_bg_symbol;
                }
                return params.key_bg_default;
            };

            ImU32 bg = slot_base_bg(row);
            bool is_done = false;
            const char* label = nullptr;
            const char* hotkey_label = nullptr;
            ImeKbKeyGlyph glyph = ImeKbKeyGlyph::None;
            bool underline_label = false;
            bool disabled_visual = false;
            if (key) {
                const bool symbols_layout = params.selection.family == ImeKbLayoutFamily::Symbols;
                const bool typing_key = key->action == ImeKbKeyAction::Character;
                const bool function_key =
                    key->action != ImeKbKeyAction::Character && key->action != ImeKbKeyAction::None;
                if (symbols_layout && typing_key) {
                    bg = params.key_bg_symbol;
                } else if (function_key) {
                    bg = params.key_bg_function;
                } else if (key->action == ImeKbKeyAction::None) {
                    bg = slot_base_bg(row);
                } else {
                    bg = params.key_bg_default;
                }
                is_done = key->action == ImeKbKeyAction::Done;
                if (is_done) {
                    bg = params.key_done;
                    label = GetEnterLabel(params.enter_label, language_profile);
                } else {
                    label = key->label;
                }
                hotkey_label = key->hotkey_label;
                glyph = key->glyph;

                if (!is_done) {
                    glyph = ResolveShiftOverrideGlyph(params.selection, *key, glyph);
                    label = ResolveLanguageOverrideLabel(params.selection, *key, label);
                }
            }

            rendered_keys.push_back(RenderedKey{
                .key = key,
                .row = row,
                .col = col,
                .col_span = col_span,
                .row_span = row_span,
                .pos = pos,
                .size = size,
                .center = {pos.x + size.x * 0.5f, pos.y + size.y * 0.5f},
                .bg = bg,
                .label = label,
                .hotkey_label = hotkey_label,
                .glyph = glyph,
                .underline_label = underline_label,
                .is_done = is_done,
                .disabled_visual = disabled_visual,
                .selectable = (key && key->action != ImeKbKeyAction::None),
            });
        }
    }

    struct ImeKbNavState {
        int cursor_row = -1;
        int cursor_col = -1;
        int fallback_prefer_col_dir = 0;
        ImeEdgeWrapNavState edge_wrap_nav{};
        int last_grid_rows = -1;
        int last_grid_cols = -1;
        int highlighted_row = -1;
        int highlighted_col = -1;
        int fade_row = -1;
        int fade_col = -1;
        double fade_started_at = 0.0;
        int press_pulse_row = -1;
        int press_pulse_col = -1;
        double press_pulse_started_at = -1.0;
        int mouse_hold_row = -1;
        int mouse_hold_col = -1;
        bool mouse_hold_prev_down = false;
        double mouse_hold_next_repeat_time = 0.0;
    };
    static std::unordered_map<ImGuiID, ImeKbNavState> s_nav_states;
    const ImGuiID nav_id = ImGui::GetID("##ImeKbGridNav");
    ImeKbNavState& nav_state = s_nav_states[nav_id];
    const auto clear_mouse_hold = [&]() {
        nav_state.mouse_hold_row = -1;
        nav_state.mouse_hold_col = -1;
        nav_state.mouse_hold_prev_down = false;
        nav_state.mouse_hold_next_repeat_time = 0.0;
    };
    const auto clear_selection_fade = [&]() {
        nav_state.highlighted_row = -1;
        nav_state.highlighted_col = -1;
        nav_state.fade_row = -1;
        nav_state.fade_col = -1;
        nav_state.fade_started_at = 0.0;
        nav_state.press_pulse_row = -1;
        nav_state.press_pulse_col = -1;
        nav_state.press_pulse_started_at = -1.0;
    };
    const bool grid_shape_changed =
        nav_state.last_grid_rows != grid_rows || nav_state.last_grid_cols != grid_cols;
    if (grid_shape_changed) {
        nav_state.last_grid_rows = grid_rows;
        nav_state.last_grid_cols = grid_cols;
        ResetImeEdgeWrapHold(nav_state.edge_wrap_nav);
        clear_mouse_hold();
        clear_selection_fade();
    }
    if (params.reset_nav_state) {
        ResetImeEdgeWrapHold(nav_state.edge_wrap_nav);
        clear_mouse_hold();
        clear_selection_fade();
    }

    const auto is_selectable_cell = [&](int row, int col) {
        if (row < 0 || row >= grid_rows || col < 0 || col >= grid_cols) {
            return false;
        }
        const ImeKbKeySpec* key = occupied[static_cast<std::size_t>(idx(row, col))];
        return key && key->action != ImeKbKeyAction::None;
    };

    const auto first_selectable_cell = [&]() -> std::pair<int, int> {
        for (int row = 0; row < grid_rows; ++row) {
            for (int col = 0; col < grid_cols; ++col) {
                if (is_selectable_cell(row, col)) {
                    return {row, col};
                }
            }
        }
        return {-1, -1};
    };

    const auto nearest_selectable_cell = [&](int from_row, int from_col) -> std::pair<int, int> {
        int best_row = -1;
        int best_col = -1;
        int best_distance = std::numeric_limits<int>::max();
        for (int row = 0; row < grid_rows; ++row) {
            for (int col = 0; col < grid_cols; ++col) {
                if (!is_selectable_cell(row, col)) {
                    continue;
                }
                const int distance = std::abs(row - from_row) + std::abs(col - from_col);
                if (distance < best_distance) {
                    best_distance = distance;
                    best_row = row;
                    best_col = col;
                }
            }
        }
        return {best_row, best_col};
    };

    const auto nearest_selectable_cell_on_row = [&](int from_row, int from_col,
                                                    int prefer_col_dir) -> std::pair<int, int> {
        if (from_row < 0 || from_row >= grid_rows) {
            return {-1, -1};
        }

        int best_col = -1;
        int best_distance = std::numeric_limits<int>::max();
        bool best_in_direction = false;
        for (int col = 0; col < grid_cols; ++col) {
            if (!is_selectable_cell(from_row, col)) {
                continue;
            }

            const int distance = std::abs(col - from_col);
            const bool in_direction =
                (prefer_col_dir > 0 && col > from_col) || (prefer_col_dir < 0 && col < from_col);
            const bool better_tie = (prefer_col_dir != 0 && in_direction != best_in_direction)
                                        ? in_direction
                                        : col < best_col;
            if (distance < best_distance ||
                (distance == best_distance && (best_col < 0 || better_tie))) {
                best_distance = distance;
                best_col = col;
                best_in_direction = in_direction;
            }
        }

        return best_col >= 0 ? std::pair<int, int>{from_row, best_col}
                             : std::pair<int, int>{-1, -1};
    };

    const auto visible_cell_for_cursor = [&]() -> std::pair<int, int> {
        if (is_selectable_cell(nav_state.cursor_row, nav_state.cursor_col)) {
            return {nav_state.cursor_row, nav_state.cursor_col};
        }
        auto row_fallback = nearest_selectable_cell_on_row(
            nav_state.cursor_row, nav_state.cursor_col, nav_state.fallback_prefer_col_dir);
        if (row_fallback.first >= 0 && row_fallback.second >= 0) {
            return row_fallback;
        }
        return nearest_selectable_cell(nav_state.cursor_row, nav_state.cursor_col);
    };

    if (params.requested_selected_row >= 0 && params.requested_selected_col >= 0) {
        const int row = std::clamp(params.requested_selected_row, 0, std::max(0, grid_rows - 1));
        const int col = std::clamp(params.requested_selected_col, 0, std::max(0, grid_cols - 1));
        auto requested = nearest_selectable_cell_on_row(row, col, 0);
        if (requested.first < 0 || requested.second < 0) {
            requested = nearest_selectable_cell(row, col);
        }
        if (requested.first >= 0 && requested.second >= 0) {
            nav_state.cursor_row = requested.first;
            nav_state.cursor_col = requested.second;
            nav_state.fallback_prefer_col_dir = 0;
            ResetImeEdgeWrapHold(nav_state.edge_wrap_nav);
        }
    }

    if (nav_state.cursor_row < 0 || nav_state.cursor_row >= grid_rows || nav_state.cursor_col < 0 ||
        nav_state.cursor_col >= grid_cols) {
        const int anchor_row = std::clamp(nav_state.cursor_row, 0, std::max(0, grid_rows - 1));
        const int anchor_col = std::clamp(nav_state.cursor_col, 0, std::max(0, grid_cols - 1));
        auto [row, col] = nearest_selectable_cell(anchor_row, anchor_col);
        if (row < 0 || col < 0) {
            const auto first = first_selectable_cell();
            row = first.first;
            col = first.second;
        }
        nav_state.cursor_row = row;
        nav_state.cursor_col = col;
        nav_state.fallback_prefer_col_dir = 0;
        ResetImeEdgeWrapHold(nav_state.edge_wrap_nav);
    }

    const auto move_cursor = [&](int step_row, int step_col, bool repeat_hint) {
        if (step_row == 0 && step_col == 0) {
            return;
        }

        int row = nav_state.cursor_row;
        int col = nav_state.cursor_col;
        const double now = ImGui::GetTime();
        if (row < 0 || col < 0) {
            const auto [first_row, first_col] = first_selectable_cell();
            nav_state.cursor_row = first_row;
            nav_state.cursor_col = first_col;
            nav_state.fallback_prefer_col_dir = 0;
            ResetImeEdgeWrapHold(nav_state.edge_wrap_nav);
            return;
        }

        const ImeKbKeySpec* origin_key =
            occupied[static_cast<std::size_t>(idx(nav_state.cursor_row, nav_state.cursor_col))];
        if (origin_key && origin_key->action == ImeKbKeyAction::None) {
            origin_key = nullptr;
        }
        const int max_steps = std::max(1, grid_rows * grid_cols);
        bool crossed_wrap = false;
        for (int i = 0; i < max_steps; ++i) {
            crossed_wrap = crossed_wrap || DoesImeKeyboardStepCrossGridEdge(
                                               row, col, step_row, step_col, grid_rows, grid_cols);
            const int next_row = row + step_row;
            const int next_col = col + step_col;
            row = (next_row + grid_rows) % grid_rows;
            col = (next_col + grid_cols) % grid_cols;

            const ImeKbKeySpec* candidate_key = occupied[static_cast<std::size_t>(idx(row, col))];
            if (!candidate_key || candidate_key->action == ImeKbKeyAction::None) {
                continue;
            }
            // Treat each spanned key as one navigation node in every direction.
            if (origin_key && candidate_key == origin_key) {
                continue;
            }

            if (ShouldDelayImeEdgeWrap(nav_state.edge_wrap_nav, step_row, step_col, repeat_hint,
                                       crossed_wrap, now, kPanelEdgeWrapHoldDelaySec)) {
                return;
            }

            nav_state.cursor_row = row;
            nav_state.cursor_col = col;
            nav_state.fallback_prefer_col_dir = step_col;
            ResetImeEdgeWrapHold(nav_state.edge_wrap_nav);
            return;
        }
    };

    if (params.allow_nav_input) {
        const bool use_imgui_lstick_nav = params.use_imgui_lstick_nav;
        const bool imgui_move_left_once =
            ImGui::IsKeyPressed(ImGuiKey_GamepadDpadLeft, false) ||
            (use_imgui_lstick_nav && ImGui::IsKeyPressed(ImGuiKey_GamepadLStickLeft, false));
        const bool imgui_move_right_once =
            ImGui::IsKeyPressed(ImGuiKey_GamepadDpadRight, false) ||
            (use_imgui_lstick_nav && ImGui::IsKeyPressed(ImGuiKey_GamepadLStickRight, false));
        const bool imgui_move_up_once =
            ImGui::IsKeyPressed(ImGuiKey_GamepadDpadUp, false) ||
            (use_imgui_lstick_nav && ImGui::IsKeyPressed(ImGuiKey_GamepadLStickUp, false));
        const bool imgui_move_down_once =
            ImGui::IsKeyPressed(ImGuiKey_GamepadDpadDown, false) ||
            (use_imgui_lstick_nav && ImGui::IsKeyPressed(ImGuiKey_GamepadLStickDown, false));
        const bool imgui_move_left_with_repeat =
            ImGui::IsKeyPressed(ImGuiKey_GamepadDpadLeft, true) ||
            (use_imgui_lstick_nav && ImGui::IsKeyPressed(ImGuiKey_GamepadLStickLeft, true));
        const bool imgui_move_right_with_repeat =
            ImGui::IsKeyPressed(ImGuiKey_GamepadDpadRight, true) ||
            (use_imgui_lstick_nav && ImGui::IsKeyPressed(ImGuiKey_GamepadLStickRight, true));
        const bool imgui_move_up_with_repeat =
            ImGui::IsKeyPressed(ImGuiKey_GamepadDpadUp, true) ||
            (use_imgui_lstick_nav && ImGui::IsKeyPressed(ImGuiKey_GamepadLStickUp, true));
        const bool imgui_move_down_with_repeat =
            ImGui::IsKeyPressed(ImGuiKey_GamepadDpadDown, true) ||
            (use_imgui_lstick_nav && ImGui::IsKeyPressed(ImGuiKey_GamepadLStickDown, true));
        const bool imgui_move_left_repeat = imgui_move_left_with_repeat && !imgui_move_left_once;
        const bool imgui_move_right_repeat = imgui_move_right_with_repeat && !imgui_move_right_once;
        const bool imgui_move_up_repeat = imgui_move_up_with_repeat && !imgui_move_up_once;
        const bool imgui_move_down_repeat = imgui_move_down_with_repeat && !imgui_move_down_once;
        const bool move_left =
            imgui_move_left_once || imgui_move_left_repeat || params.external_nav_left;
        const bool move_right =
            imgui_move_right_once || imgui_move_right_repeat || params.external_nav_right;
        const bool move_up = imgui_move_up_once || imgui_move_up_repeat || params.external_nav_up;
        const bool move_down =
            imgui_move_down_once || imgui_move_down_repeat || params.external_nav_down;
        const bool move_left_repeat =
            imgui_move_left_repeat || (params.external_nav_left && params.external_nav_left_repeat);
        const bool move_right_repeat =
            imgui_move_right_repeat ||
            (params.external_nav_right && params.external_nav_right_repeat);
        const bool move_up_repeat =
            imgui_move_up_repeat || (params.external_nav_up && params.external_nav_up_repeat);
        const bool move_down_repeat =
            imgui_move_down_repeat || (params.external_nav_down && params.external_nav_down_repeat);

        if (move_left) {
            move_cursor(0, -1, move_left_repeat);
        } else if (move_right) {
            move_cursor(0, 1, move_right_repeat);
        } else if (move_up) {
            move_cursor(-1, 0, move_up_repeat);
        } else if (move_down) {
            move_cursor(1, 0, move_down_repeat);
        }
    }

    const bool imgui_activate_selected_once = ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown, false);
    const bool imgui_activate_selected_repeat = ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown, true);

    const auto activate_key = [&](const RenderedKey& render_key, bool trigger_press_pulse) {
        if (!render_key.key || !render_key.selectable) {
            return;
        }
        if (trigger_press_pulse) {
            nav_state.press_pulse_row = render_key.row;
            nav_state.press_pulse_col = render_key.col;
            nav_state.press_pulse_started_at = ImGui::GetTime();
        }
        state.pressed_repeat = !trigger_press_pulse;
        state.pressed_action = render_key.key->action;
        state.pressed_text = render_key.key->text;
        if (render_key.is_done) {
            state.done_pressed = true;
        }
    };

    int selected_render_index = -1;
    const auto [visible_row, visible_col] = visible_cell_for_cursor();
    if (is_selectable_cell(visible_row, visible_col)) {
        const ImeKbKeySpec* selected_spec =
            occupied[static_cast<std::size_t>(idx(visible_row, visible_col))];
        for (int i = 0; i < static_cast<int>(rendered_keys.size()); ++i) {
            const auto& key = rendered_keys[static_cast<std::size_t>(i)];
            if (key.key == selected_spec && key.selectable) {
                selected_render_index = i;
                break;
            }
        }
    }
    state.selected_row = nav_state.cursor_row;
    state.selected_col = nav_state.cursor_col;
    if (selected_render_index >= 0) {
        state.selected_center =
            rendered_keys[static_cast<std::size_t>(selected_render_index)].center;
    }

    constexpr double kSelectionFadeOutDurationSec = 0.2;
    int highlighted_row = -1;
    int highlighted_col = -1;
    if (params.show_selection_highlight && selected_render_index >= 0) {
        const auto& highlighted_key =
            rendered_keys[static_cast<std::size_t>(selected_render_index)];
        highlighted_row = highlighted_key.row;
        highlighted_col = highlighted_key.col;
    }
    if (highlighted_row != nav_state.highlighted_row ||
        highlighted_col != nav_state.highlighted_col) {
        if (nav_state.highlighted_row >= 0 && nav_state.highlighted_col >= 0) {
            nav_state.fade_row = nav_state.highlighted_row;
            nav_state.fade_col = nav_state.highlighted_col;
            nav_state.fade_started_at = ImGui::GetTime();
        } else {
            nav_state.fade_row = -1;
            nav_state.fade_col = -1;
            nav_state.fade_started_at = 0.0;
        }
        nav_state.highlighted_row = highlighted_row;
        nav_state.highlighted_col = highlighted_col;
    }

    float faded_selection_alpha = 0.0f;
    if (nav_state.fade_row >= 0 && nav_state.fade_col >= 0 && kSelectionFadeOutDurationSec > 0.0) {
        const double elapsed = ImGui::GetTime() - nav_state.fade_started_at;
        if (elapsed >= 0.0 && elapsed < kSelectionFadeOutDurationSec) {
            faded_selection_alpha =
                1.0f - static_cast<float>(elapsed / kSelectionFadeOutDurationSec);
        } else {
            nav_state.fade_row = -1;
            nav_state.fade_col = -1;
            nav_state.fade_started_at = 0.0;
        }
    }

    const auto action_allows_repeat = [](ImeKbKeyAction action) {
        switch (action) {
        case ImeKbKeyAction::Character:
        case ImeKbKeyAction::Space:
        case ImeKbKeyAction::Backspace:
        case ImeKbKeyAction::ArrowLeft:
        case ImeKbKeyAction::ArrowRight:
        case ImeKbKeyAction::ArrowUp:
        case ImeKbKeyAction::ArrowDown:
        case ImeKbKeyAction::NewLine:
            return true;
        case ImeKbKeyAction::None:
        case ImeKbKeyAction::Shift:
        case ImeKbKeyAction::SymbolsMode:
        case ImeKbKeyAction::AlternateMode:
        case ImeKbKeyAction::PageNext:
        case ImeKbKeyAction::Keyboard:
        case ImeKbKeyAction::Menu:
        case ImeKbKeyAction::Settings:
        case ImeKbKeyAction::Done:
        default:
            return false;
        }
    };

    bool activate_selected = false;
    bool activate_selected_pulse = false;
    if (params.allow_activate_input) {
        bool activate_once = imgui_activate_selected_once;
        bool activate_repeat = imgui_activate_selected_repeat && !imgui_activate_selected_once;
        if (!activate_once && params.external_activate_pressed) {
            if (params.external_activate_repeat) {
                activate_repeat = true;
            } else {
                activate_once = true;
            }
        }

        if (selected_render_index >= 0) {
            const auto& selected_key =
                rendered_keys[static_cast<std::size_t>(selected_render_index)];
            const ImeKbKeyAction action = (selected_key.key && selected_key.selectable)
                                              ? selected_key.key->action
                                              : ImeKbKeyAction::None;
            if (!action_allows_repeat(action)) {
                activate_repeat = false;
            }
        }

        activate_selected = activate_once || activate_repeat;
        activate_selected_pulse = activate_once;
    }

    const auto draw_key_glyph = [&](ImVec2 pos, ImVec2 size, ImeKbKeyGlyph glyph) {
        if (glyph == ImeKbKeyGlyph::None) {
            return;
        }

        const float glyph_extent = std::min(size.x, size.y);
        const float thickness = std::max(1.2f, glyph_extent * 0.05f);
        const ImU32 color = params.key_text;
        const float cx = pos.x + size.x * 0.5f;
        const float cy = pos.y + size.y * 0.5f;
        const auto draw_rounded_path = [&](std::span<const ImVec2> points, bool closed,
                                           bool filled = false) {
            draw->PathClear();
            for (std::size_t i = 0; i < points.size(); ++i) {
                const auto& point = points[i];
                if (!closed && (i == 0 || i + 1 == points.size())) {
                    draw->PathLineTo(point);
                    continue;
                }
                const auto& previous = points[(i + points.size() - 1) % points.size()];
                const auto& next = points[(i + 1) % points.size()];
                const auto incoming = point - previous;
                const auto outgoing = next - point;
                const float previous_length = std::sqrt(ImLengthSqr(incoming));
                const float next_length = std::sqrt(ImLengthSqr(outgoing));
                const bool concave = closed && incoming.x * outgoing.y < incoming.y * outgoing.x;
                const float rounding = glyph_extent * (concave ? 0.04f : 0.14f);
                const auto before = ImLerp(
                    point, previous, std::min(rounding, previous_length * 0.48f) / previous_length);
                const auto after =
                    ImLerp(point, next, std::min(rounding, next_length * 0.48f) / next_length);
                draw->PathLineTo(before);
                draw->PathBezierCubicCurveTo(ImLerp(before, point, 0.55f),
                                             ImLerp(after, point, 0.55f), after, 6);
            }
            if (filled) {
                draw->PathFillConcave(color);
            } else {
                draw->PathStroke(color, thickness, closed ? ImDrawFlags_Closed : 0);
                if (!closed) {
                    draw->AddCircleFilled(points.front(), thickness * 0.5f, color);
                    draw->AddCircleFilled(points.back(), thickness * 0.5f, color);
                }
            }
        };
        const auto draw_arrow_glyph = [&](ImVec2 direction) {
            const ImVec2 center{cx, cy};
            const ImVec2 side{-direction.y, direction.x};
            const std::array<ImVec2, 3> points{
                center + direction * (glyph_extent * 0.22f),
                center - direction * (glyph_extent * 0.14f) + side * (glyph_extent * 0.19f),
                center - direction * (glyph_extent * 0.14f) - side * (glyph_extent * 0.19f),
            };
            draw_rounded_path(points, true, true);
        };
        const auto draw_shift_glyph = [&](bool filled, bool caps_locked) {
            const float extent = glyph_extent;
            const float center_y = cy - (caps_locked ? extent * 0.08f : 0.0f);
            const float tip_y = center_y - extent * 0.28f;
            const float shoulder_y = center_y - extent * 0.03f;
            const float base_y = center_y + extent * 0.25f;
            const float head_half_w = extent * 0.27f;
            const float stem_half_w = extent * 0.12f;

            const std::array<ImVec2, 7> shape{
                ImVec2{cx, tip_y},
                ImVec2{cx + head_half_w, shoulder_y},
                ImVec2{cx + stem_half_w, shoulder_y},
                ImVec2{cx + stem_half_w, base_y},
                ImVec2{cx - stem_half_w, base_y},
                ImVec2{cx - stem_half_w, shoulder_y},
                ImVec2{cx - head_half_w, shoulder_y},
            };

            draw_rounded_path(shape, true, filled);

            if (caps_locked) {
                const float marker_half_w = extent * 0.21f;
                const float marker_y = base_y + extent * 0.14f;
                draw->AddRectFilled({cx - marker_half_w, marker_y - thickness * 0.5f},
                                    {cx + marker_half_w, marker_y + thickness * 0.5f}, color,
                                    thickness * 0.5f);
            }
        };

        switch (glyph) {
        case ImeKbKeyGlyph::Backspace: {
            const float extent = glyph_extent;
            const float left = cx - extent * 0.12f;
            const float right = cx + extent * 0.34f;
            const float top = cy - extent * 0.20f;
            const float bottom = cy + extent * 0.20f;
            const ImVec2 tip{cx - extent * 0.34f, cy};
            const std::array<ImVec2, 5> frame{
                ImVec2{left, top},
                ImVec2{right, top},
                ImVec2{right, bottom},
                ImVec2{left, bottom},
                tip,
            };
            draw_rounded_path(frame, true);

            const float cross_half = extent * 0.10f;
            const float cross_cx = cx + extent * 0.095f;
            const std::array<ImVec2, 2> cross_first{{
                {cross_cx - cross_half, cy - cross_half},
                {cross_cx + cross_half, cy + cross_half},
            }};
            const std::array<ImVec2, 2> cross_second{{
                {cross_cx + cross_half, cy - cross_half},
                {cross_cx - cross_half, cy + cross_half},
            }};
            draw_rounded_path(cross_first, false);
            draw_rounded_path(cross_second, false);
            break;
        }
        case ImeKbKeyGlyph::Tab: {
            const float extent = glyph_extent;
            const std::array<ImVec2, 2> stem{{
                {cx - extent * 0.31f, cy},
                {cx + extent * 0.16f, cy},
            }};
            const std::array<ImVec2, 3> head{{
                {cx - extent * 0.03f, cy - extent * 0.19f},
                {cx + extent * 0.16f, cy},
                {cx - extent * 0.03f, cy + extent * 0.19f},
            }};
            const std::array<ImVec2, 2> stop{{
                {cx + extent * 0.31f, cy - extent * 0.23f},
                {cx + extent * 0.31f, cy + extent * 0.23f},
            }};
            draw_rounded_path(stem, false);
            draw_rounded_path(head, false);
            draw_rounded_path(stop, false);
            break;
        }
        case ImeKbKeyGlyph::Enter: {
            const float extent = std::min(size.x, size.y);
            const ImVec2 tip{cx - extent * 0.31f, cy + extent * 0.12f};
            const float right = cx + extent * 0.31f;
            const std::array<ImVec2, 3> stem{{
                {right, cy - extent * 0.25f},
                {right, tip.y},
                {tip.x + extent * 0.10f, tip.y},
            }};
            const std::array<ImVec2, 3> head{{
                {tip.x + extent * 0.17f, tip.y - extent * 0.17f},
                tip,
                {tip.x + extent * 0.17f, tip.y + extent * 0.17f},
            }};
            draw_rounded_path(stem, false);
            draw_rounded_path(head, false);
            break;
        }
        case ImeKbKeyGlyph::Keyboard: {
            const float x = glyph_extent * 0.34f;
            const float y = glyph_extent * 0.22f;
            draw->AddRect({cx - x, cy - y}, {cx + x, cy + y}, color, glyph_extent * 0.14f,
                          thickness);
            const float dot = std::max(0.65f, glyph_extent * 0.033f);
            for (int row = 0; row < 2; ++row) {
                for (int col = -1; col <= 1; ++col) {
                    draw->AddCircleFilled({cx + col * x * 0.53f, cy - y * (0.56f - row * 0.50f)},
                                          dot, color);
                }
            }
            const std::array<ImVec2, 2> space{{
                {cx - x * 0.52f, cy + y * 0.52f},
                {cx + x * 0.52f, cy + y * 0.52f},
            }};
            draw_rounded_path(space, false);
            break;
        }
        case ImeKbKeyGlyph::Options: {
            const float radius = std::max(1.0f, glyph_extent * 0.06f);
            for (int col = -1; col <= 1; ++col) {
                draw->AddCircleFilled({cx + col * glyph_extent * 0.19f, cy}, radius, color);
            }
            break;
        }
        case ImeKbKeyGlyph::Gyro: {
            const float e = glyph_extent;
            const std::array<ImVec2, 8> controller{{
                {cx - e * 0.29f, cy + e * 0.01f},
                {cx - e * 0.04f, cy + e * 0.01f},
                {cx + e * 0.03f, cy + e * 0.22f},
                {cx - e * 0.06f, cy + e * 0.22f},
                {cx - e * 0.12f, cy + e * 0.14f},
                {cx - e * 0.22f, cy + e * 0.14f},
                {cx - e * 0.28f, cy + e * 0.22f},
                {cx - e * 0.36f, cy + e * 0.22f},
            }};
            draw_rounded_path(controller, true);
            const ImVec2 target{cx + e * 0.18f, cy - e * 0.10f};
            draw->AddCircle(target, e * 0.12f, color, 0, thickness);
            draw->AddCircleFilled(target, thickness * 0.5f, color);
            break;
        }
        case ImeKbKeyGlyph::ArrowLeft: {
            draw_arrow_glyph({-1.0f, 0.0f});
            break;
        }
        case ImeKbKeyGlyph::ArrowRight: {
            draw_arrow_glyph({1.0f, 0.0f});
            break;
        }
        case ImeKbKeyGlyph::ArrowUp: {
            draw_arrow_glyph({0.0f, -1.0f});
            break;
        }
        case ImeKbKeyGlyph::ArrowDown: {
            draw_arrow_glyph({0.0f, 1.0f});
            break;
        }
        case ImeKbKeyGlyph::ShiftOutline:
            draw_shift_glyph(false, false);
            break;
        case ImeKbKeyGlyph::ShiftFilled:
            draw_shift_glyph(true, false);
            break;
        case ImeKbKeyGlyph::CapsLockFilled:
            draw_shift_glyph(true, true);
            break;
        case ImeKbKeyGlyph::None:
        default:
            break;
        }
    };

    const auto draw_key = [&](ImVec2 pos, ImVec2 size, ImU32 bg, const char* label,
                              const char* hotkey_label, ImeKbKeyGlyph glyph, float selection_alpha,
                              float pulse_expand_px, bool emphasize_main_label,
                              bool underline_main_label, bool disabled_visual, bool done_key) {
        const float clamped_selection_alpha = std::clamp(selection_alpha, 0.0f, 1.0f);
        if (clamped_selection_alpha > 0.0f) {
            const float selection_boost = 0.11f * clamped_selection_alpha;
            ImVec4 selected_bg = ImGui::ColorConvertU32ToFloat4(bg);
            selected_bg.x = std::min(1.0f, selected_bg.x + selection_boost);
            selected_bg.y = std::min(1.0f, selected_bg.y + selection_boost);
            selected_bg.z = std::min(1.0f, selected_bg.z + selection_boost);
            bg = ImGui::ColorConvertFloat4ToU32(selected_bg);
        }
        ImU32 key_text_color = params.key_text;
        ImU32 key_hotkey_color = params.key_hotkey_text;
        if (disabled_visual) {
            ImVec4 dim_bg = ImGui::ColorConvertU32ToFloat4(bg);
            dim_bg.x *= 0.72f;
            dim_bg.y *= 0.72f;
            dim_bg.z *= 0.72f;
            bg = ImGui::ColorConvertFloat4ToU32(dim_bg);
            key_text_color = IM_COL32(150, 150, 150, 255);
            key_hotkey_color = IM_COL32(122, 122, 122, 255);
        }
        const float expand = std::max(0.0f, pulse_expand_px);
        const ImVec2 draw_min{pos.x - expand, pos.y - expand};
        const ImVec2 draw_max{pos.x + size.x + expand, pos.y + size.y + expand};
        const float draw_corner_radius = layout.corner_radius + expand;
        draw->AddRectFilled(draw_min, draw_max, bg, draw_corner_radius);
        const bool selected = clamped_selection_alpha > 0.0f;
        ImU32 border_color = params.key_border;
        if (done_key) {
            ImVec4 done_border = ImGui::ColorConvertU32ToFloat4(bg);
            done_border.x = std::clamp(done_border.x * 0.72f, 0.0f, 1.0f);
            done_border.y = std::clamp(done_border.y * 0.72f, 0.0f, 1.0f);
            done_border.z = std::clamp(done_border.z * 0.72f, 0.0f, 1.0f);
            done_border.w = 1.0f;
            border_color = ImGui::ColorConvertFloat4ToU32(done_border);
        }
        if (selected) {
            border_color = IM_COL32(248, 248, 248, 255);
        }
        const float border_thickness = selected ? 2.0f : 1.0f;
        if (selected && clamped_selection_alpha < 1.0f) {
            ImVec4 border = ImGui::ColorConvertU32ToFloat4(border_color);
            border.w *= clamped_selection_alpha;
            border_color = ImGui::ColorConvertFloat4ToU32(border);
        }
        draw->AddRect(draw_min, draw_max, border_color, draw_corner_radius, 0, border_thickness);
        ImFont* font = ImGui::GetFont();
        const float base_font_size = ImGui::GetFontSize();
        const auto draw_hotkey_badge = [&]() {
            if (!hotkey_label || hotkey_label[0] == '\0') {
                return;
            }
            const float hotkey_padding_x = std::max(3.0f, size.y * 0.08f);
            const float hotkey_max_w = std::max(0.0f, size.x - hotkey_padding_x * 2.0f);
            float hotkey_font_size = std::max(6.0f, base_font_size * kHotkeyLabelScale);
            ImVec2 hotkey_size = font->CalcTextSizeA(
                hotkey_font_size, std::numeric_limits<float>::max(), -1.0f, hotkey_label);
            if (hotkey_max_w > 0.0f && hotkey_size.x > hotkey_max_w && hotkey_size.x > 0.0f) {
                hotkey_font_size *= hotkey_max_w / hotkey_size.x;
                hotkey_size = font->CalcTextSizeA(
                    hotkey_font_size, std::numeric_limits<float>::max(), -1.0f, hotkey_label);
            }
            ImVec2 hotkey_pos{pos.x + hotkey_padding_x, pos.y};
            const float hotkey_bg_pad_x = std::max(2.0f, hotkey_font_size * 0.28f);
            const float hotkey_bg_pad_bottom = std::max(1.0f, hotkey_font_size * 0.16f);
            const ImVec2 hotkey_bg_min{hotkey_pos.x - hotkey_bg_pad_x, pos.y};
            const ImVec2 hotkey_bg_max{hotkey_pos.x + hotkey_size.x + hotkey_bg_pad_x,
                                       hotkey_pos.y + hotkey_size.y + hotkey_bg_pad_bottom};
            const float hotkey_bg_rounding = std::max(2.0f, hotkey_font_size * 0.24f);
            const ImU32 hotkey_bg_color =
                disabled_visual ? IM_COL32(16, 16, 16, 120) : IM_COL32(10, 10, 10, 132);
            const ImU32 hotkey_bg_border =
                disabled_visual ? IM_COL32(70, 70, 70, 90) : IM_COL32(92, 92, 92, 105);
            draw->AddRectFilled(hotkey_bg_min, hotkey_bg_max, hotkey_bg_color, hotkey_bg_rounding);
            draw->AddRect(hotkey_bg_min, hotkey_bg_max, hotkey_bg_border, hotkey_bg_rounding, 0,
                          1.0f);
            draw->AddText(font, hotkey_font_size, hotkey_pos, key_hotkey_color, hotkey_label);
        };
        if ((!label || label[0] == '\0') && glyph != ImeKbKeyGlyph::None) {
            draw_key_glyph(pos, size, glyph);
        }
        if (label && label[0] != '\0') {
            float label_font_size =
                base_font_size *
                (emphasize_main_label ? kTypingKeyLabelScale : kFunctionKeyLabelScale);
            const float label_padding_x = std::max(4.0f, size.x * 0.08f);
            const float label_max_w = std::max(0.0f, size.x - label_padding_x * 2.0f);
            ImVec2 text_size = font->CalcTextSizeA(label_font_size,
                                                   std::numeric_limits<float>::max(), -1.0f, label);
            if (label_max_w > 0.0f && text_size.x > label_max_w && text_size.x > 0.0f) {
                label_font_size *= label_max_w / text_size.x;
                text_size = font->CalcTextSizeA(label_font_size, std::numeric_limits<float>::max(),
                                                -1.0f, label);
            }
            ImVec2 text_pos{pos.x + (size.x - text_size.x) * 0.5f,
                            pos.y + (size.y - text_size.y) * 0.5f};
            draw->AddText(font, label_font_size, text_pos, key_text_color, label);
            if (underline_main_label) {
                const float underline_pad = std::max(1.0f, label_font_size * 0.05f);
                const float underline_y =
                    std::min(pos.y + size.y - 3.0f, text_pos.y + text_size.y + underline_pad);
                const float underline_thickness = std::max(1.0f, label_font_size * 0.06f);
                draw->AddLine({text_pos.x, underline_y}, {text_pos.x + text_size.x, underline_y},
                              key_text_color, underline_thickness);
            }
        }
        // Draw hotkey tag last so it always stays over main key label/glyph.
        draw_hotkey_badge();
    };

    for (int i = 0; i < static_cast<int>(rendered_keys.size()); ++i) {
        const auto& key = rendered_keys[static_cast<std::size_t>(i)];
        const bool selected = params.show_selection_highlight && (i == selected_render_index);
        float selection_alpha = selected ? 1.0f : 0.0f;
        if (!selected && faded_selection_alpha > 0.0f && key.row == nav_state.fade_row &&
            key.col == nav_state.fade_col) {
            selection_alpha = faded_selection_alpha;
        }
        constexpr float kSelectedBorderThickness = 2.0f;
        float press_pulse_expand = 0.0f;
        if (selected && key.row == nav_state.press_pulse_row &&
            key.col == nav_state.press_pulse_col) {
            press_pulse_expand = ComputePressPulseExpand(
                nav_state.press_pulse_started_at, ImGui::GetTime(), kSelectorPressPulseDurationSec,
                kSelectedBorderThickness * kSelectorPressPulseExpandBorderFactor);
        }
        const bool emphasize_main_label = key.key && key.key->action == ImeKbKeyAction::Character;
        const bool underline_main_label = key.underline_label;
        draw_key(key.pos, key.size, key.bg, key.label, key.hotkey_label, key.glyph, selection_alpha,
                 press_pulse_expand, emphasize_main_label, underline_main_label,
                 key.disabled_visual, key.is_done);
        if (emphasize_main_label &&
            language_profile.preferred_characters.contains(Utf8FirstCodeUnit(key.label)) &&
            selection_alpha == 0.0f) {
            draw->AddRect({key.pos.x + 1.0f, key.pos.y + 1.0f},
                          {key.pos.x + key.size.x - 1.0f, key.pos.y + key.size.y - 1.0f},
                          ImGui::GetColorU32(params.key_text, 0.35f), layout.corner_radius);
        }

        if (!key.selectable) {
            continue;
        }

        if (!params.allow_pointer_input) {
            continue;
        }
        ImGui::PushID(idx(key.row, key.col));
        ImGui::SetCursorScreenPos(key.pos);
        ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
        (void)ImGui::InvisibleButton("##ImeGridKey", key.size);
        ImGui::PopItemFlag();
        if (ImGui::IsItemHovered()) {
            state.hovered = true;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            state.clicked = true;
            nav_state.cursor_row = key.row;
            nav_state.cursor_col = key.col;
            nav_state.fallback_prefer_col_dir = 0;
            activate_key(key, true);
            if (key.key && action_allows_repeat(key.key->action)) {
                nav_state.mouse_hold_row = key.row;
                nav_state.mouse_hold_col = key.col;
                nav_state.mouse_hold_prev_down = true;
                nav_state.mouse_hold_next_repeat_time =
                    ImGui::GetTime() + static_cast<double>(ImGui::GetIO().KeyRepeatDelay);
            } else {
                clear_mouse_hold();
            }
        }
        ImGui::PopID();
    }

    if (!params.allow_pointer_input) {
        clear_mouse_hold();
    }
    if (nav_state.mouse_hold_row >= 0 && nav_state.mouse_hold_col >= 0) {
        int hold_render_index = -1;
        for (int i = 0; i < static_cast<int>(rendered_keys.size()); ++i) {
            const auto& key = rendered_keys[static_cast<std::size_t>(i)];
            if (!key.selectable) {
                continue;
            }
            if (key.row == nav_state.mouse_hold_row && key.col == nav_state.mouse_hold_col) {
                hold_render_index = i;
                break;
            }
        }

        if (hold_render_index < 0 || !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            clear_mouse_hold();
        } else {
            const auto& hold_key = rendered_keys[static_cast<std::size_t>(hold_render_index)];
            const ImeKbKeyAction hold_action =
                hold_key.key ? hold_key.key->action : ImeKbKeyAction::None;
            if (!action_allows_repeat(hold_action)) {
                clear_mouse_hold();
            } else {
                const ImVec2 mouse_pos = ImGui::GetIO().MousePos;
                const bool mouse_over_hold_key =
                    mouse_pos.x >= hold_key.pos.x &&
                    mouse_pos.x <= (hold_key.pos.x + hold_key.size.x) &&
                    mouse_pos.y >= hold_key.pos.y &&
                    mouse_pos.y <= (hold_key.pos.y + hold_key.size.y);
                const bool down = ImGui::IsMouseDown(ImGuiMouseButton_Left) && mouse_over_hold_key;
                const double now = ImGui::GetTime();
                const double repeat_delay = static_cast<double>(ImGui::GetIO().KeyRepeatDelay);
                const double repeat_rate = static_cast<double>(ImGui::GetIO().KeyRepeatRate);
                bool trigger_repeat = false;
                if (!down) {
                    nav_state.mouse_hold_prev_down = false;
                    nav_state.mouse_hold_next_repeat_time = 0.0;
                } else if (!nav_state.mouse_hold_prev_down) {
                    nav_state.mouse_hold_prev_down = true;
                    nav_state.mouse_hold_next_repeat_time = now + repeat_delay;
                } else if (repeat_rate > 0.0 && now >= nav_state.mouse_hold_next_repeat_time) {
                    nav_state.mouse_hold_next_repeat_time = now + repeat_rate;
                    trigger_repeat = true;
                }

                if (trigger_repeat) {
                    activate_key(hold_key, false);
                }
            }
        }
    }

    if (activate_selected && selected_render_index >= 0) {
        activate_key(rendered_keys[static_cast<std::size_t>(selected_render_index)],
                     activate_selected_pulse);
    }
}

} // namespace Libraries::Ime
