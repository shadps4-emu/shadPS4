// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <cstddef>
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
    // Base row height used for non-fixed rows.
    float key_h = 0.0f;
    // Optional fixed-size bottom rows (used by accents/specials to preserve function-row height).
    float bottom_row_h = 0.0f;
    int fixed_bottom_rows = 0;
    int cols = 10;
    int rows = 6;
    float corner_radius = 0.0f;
};

// Standardized OSK selection indexing:
// - Keyboard grid uses zero-based row/column indexing.
// - Top panel row placement is driven by ImeTopPanelLayoutConfig::row/row_span.
// - Keyboard rows follow active ImeKbLayoutModel::rows.
// - Function row count is driven by ImeKbLayoutModel::function_rows.
struct ImeSelectionGridIndex {
    static constexpr int DefaultKeyboardRows = 6;
    static constexpr int DefaultKeyboardCols = 10;
    static constexpr int DefaultTopPanelRow = 0;
    static constexpr int DefaultTopPanelRows = 1;
    static constexpr int DefaultFunctionRows = 2;

    static constexpr int PanelMinRow = 0;
    static constexpr int PanelMaxRow = 6; // Legacy default for 6-row keyboard layouts.
    static constexpr int PanelTopRow = 0;
    static constexpr int PanelKeyboardMinRow = 1;
    static constexpr int PanelKeyboardMaxRow = 6; // Legacy default for 6-row keyboard layouts.

    static constexpr int KeyboardMinRow = 0;
    static constexpr int KeyboardMaxRow = 5; // Legacy default for 6-row keyboard layouts.
    static constexpr int KeyboardMinCol = 0;
    static constexpr int KeyboardMaxCol = 9; // Legacy default for 10-column keyboard layouts.

    static constexpr int TopRowMinCol = 0;
    static constexpr int TopRowPredictionMaxCol = 8;
    static constexpr int TopRowCloseCol = 9;

    static constexpr int PanelTopRowFromConfig(int top_panel_row = DefaultTopPanelRow) {
        return std::max(PanelMinRow, top_panel_row);
    }

    static constexpr int PanelTopRowsFromConfig(int top_panel_rows = DefaultTopPanelRows) {
        return std::max(1, top_panel_rows);
    }

    static constexpr int KeyboardMaxRowForRows(int keyboard_rows) {
        return std::max(0, keyboard_rows - 1);
    }

    static constexpr int KeyboardMaxColForCols(int keyboard_cols) {
        return std::max(0, keyboard_cols - 1);
    }

    static constexpr int PanelKeyboardMinRowForTopPanel(int top_panel_row = DefaultTopPanelRow,
                                                        int top_panel_rows = DefaultTopPanelRows) {
        return PanelTopRowFromConfig(top_panel_row) + PanelTopRowsFromConfig(top_panel_rows);
    }

    static constexpr int PanelKeyboardMaxRowForKeyboardRows(
        int keyboard_rows, int top_panel_row = DefaultTopPanelRow,
        int top_panel_rows = DefaultTopPanelRows) {
        return PanelKeyboardMinRowForTopPanel(top_panel_row, top_panel_rows) +
               KeyboardMaxRowForRows(std::max(1, keyboard_rows));
    }

    static constexpr int PanelMaxRowForKeyboardRows(int keyboard_rows,
                                                    int top_panel_row = DefaultTopPanelRow,
                                                    int top_panel_rows = DefaultTopPanelRows) {
        return PanelKeyboardMaxRowForKeyboardRows(keyboard_rows, top_panel_row, top_panel_rows);
    }

    static constexpr int ResolveFunctionRows(int keyboard_rows,
                                             int configured_function_rows = DefaultFunctionRows) {
        return keyboard_rows > 2
                   ? std::min(std::max(0, configured_function_rows), keyboard_rows - 1)
                   : 0;
    }

    static constexpr int ResolveTypingRows(int keyboard_rows,
                                           int configured_function_rows = DefaultFunctionRows) {
        const int rows = std::max(1, keyboard_rows);
        return std::max(1, rows - ResolveFunctionRows(rows, configured_function_rows));
    }

    static constexpr int KeyboardFunctionMinRow(
        int keyboard_rows, int configured_function_rows = DefaultFunctionRows) {
        const int rows = std::max(1, keyboard_rows);
        const int function_rows = ResolveFunctionRows(rows, configured_function_rows);
        return function_rows > 0 ? (rows - function_rows) : rows;
    }

    static constexpr bool IsKeyboardFunctionRow(
        int keyboard_row, int keyboard_rows, int configured_function_rows = DefaultFunctionRows) {
        const int rows = std::max(1, keyboard_rows);
        return keyboard_row >= KeyboardFunctionMinRow(rows, configured_function_rows) &&
               keyboard_row <= KeyboardMaxRowForRows(rows);
    }

    static constexpr int ClampPanelRow(int row, int keyboard_rows = DefaultKeyboardRows,
                                       int top_panel_row = DefaultTopPanelRow,
                                       int top_panel_rows = DefaultTopPanelRows) {
        return std::clamp(row, PanelMinRow,
                          PanelMaxRowForKeyboardRows(keyboard_rows, top_panel_row, top_panel_rows));
    }

    static constexpr int ClampKeyboardRow(int row, int keyboard_rows = DefaultKeyboardRows) {
        return std::clamp(row, KeyboardMinRow, KeyboardMaxRowForRows(keyboard_rows));
    }

    static constexpr int ClampKeyboardCol(int col, int keyboard_cols = DefaultKeyboardCols) {
        return std::clamp(col, KeyboardMinCol, KeyboardMaxColForCols(keyboard_cols));
    }

    static constexpr int ClampTopRowCol(int col) {
        return std::clamp(col, TopRowMinCol, TopRowCloseCol);
    }

    static constexpr int ClampTopPredictionCol(int col) {
        return std::clamp(col, TopRowMinCol, TopRowPredictionMaxCol);
    }

    static constexpr int TopToKeyboardCol(int top_col, int keyboard_cols = DefaultKeyboardCols) {
        return ClampKeyboardCol(top_col, keyboard_cols);
    }

    static constexpr int KeyboardToTopCol(int keyboard_col) {
        return ClampTopRowCol(keyboard_col);
    }

    static constexpr int PanelToKeyboardRow(int panel_row, int keyboard_rows = DefaultKeyboardRows,
                                            int top_panel_row = DefaultTopPanelRow,
                                            int top_panel_rows = DefaultTopPanelRows) {
        return ClampKeyboardRow(panel_row -
                                    PanelKeyboardMinRowForTopPanel(top_panel_row, top_panel_rows),
                                keyboard_rows);
    }

    static constexpr int KeyboardToPanelRow(int keyboard_row,
                                            int keyboard_rows = DefaultKeyboardRows,
                                            int top_panel_row = DefaultTopPanelRow,
                                            int top_panel_rows = DefaultTopPanelRows) {
        return ClampPanelRow(keyboard_row +
                                 PanelKeyboardMinRowForTopPanel(top_panel_row, top_panel_rows),
                             keyboard_rows, top_panel_row, top_panel_rows);
    }

    static int GridColumnFromX(float x, float left, float width, int min_col, int max_col) {
        if (max_col < min_col || width <= 0.0f) {
            return min_col;
        }
        const float t = std::clamp((x - left) / width, 0.0f, 0.9999f);
        const int span = max_col - min_col + 1;
        const int offset = std::clamp(static_cast<int>(t * static_cast<float>(span)), 0, span - 1);
        return min_col + offset;
    }
};

enum class ImeKbLayoutFamily : u8 {
    Latin = 0,
    Symbols = 1,
    Specials = 2,
};

enum class ImeKbCaseState : u8 {
    Lower = 0,
    Upper = 1,
    CapsLock = 2,
};

enum class ImeKbLayoutId : u8 {
    LatinLower = 0,
    LatinUpper = 1,
    LatinCapsLock = 2,
    SymbolsPage1 = 3,
    SymbolsPage2 = 4,
    SpecialsPage1 = 5,
    SpecialsPage2 = 6,
};

struct ImeKbLayoutSelection {
    ImeKbLayoutFamily family = ImeKbLayoutFamily::Latin;
    ImeKbCaseState case_state = ImeKbCaseState::Lower;
    u8 page = 0;
};

enum class ImeKbKeyAction : u8 {
    // Disabled key slot. It is not selectable and should not expose any visible label.
    None = 0,
    Character = 1,
    Shift = 2,
    SymbolsMode = 3,
    SpecialsMode = 4,
    Space = 5,
    Backspace = 6,
    ArrowLeft = 7,
    ArrowRight = 8,
    ArrowUp = 9,
    ArrowDown = 10,
    Keyboard = 11,
    Menu = 12,
    Settings = 13,
    NewLine = 14,
    Done = 15,
    PagePrev = 16,
    PageNext = 17,
};

enum class ImeKbKeyGlyph : u8 {
    None = 0,
    Backspace = 1,
    ArrowLeft = 2,
    ArrowRight = 3,
    ArrowUp = 4,
    ArrowDown = 5,
};

struct ImeKbKeySpec {
    u8 row = 0;
    u8 col = 0;
    u8 col_span = 1;
    u8 row_span = 1;
    // Disabled key contract:
    // - action == ImeKbKeyAction::None
    // - label == nullptr
    // - hotkey_label == nullptr
    // - glyph == ImeKbKeyGlyph::None
    // This means the key loses both label visibility and functionality.
    const char* label = nullptr;
    const char* hotkey_label = nullptr;
    ImeKbKeyAction action = ImeKbKeyAction::None;
    ImeKbKeyGlyph glyph = ImeKbKeyGlyph::None;
};

struct ImeKbLayoutModel {
    const ImeKbKeySpec* keys = nullptr;
    std::size_t key_count = 0;
    u8 cols = 10;
    u8 rows = 6;
    u8 function_rows = static_cast<u8>(ImeSelectionGridIndex::DefaultFunctionRows);
};

enum class ImeTopPanelElementId : u8 {
    Prediction = 0,
    Close = 1,
};

struct ImeTopPanelElementSpec {
    ImeTopPanelElementId id = ImeTopPanelElementId::Prediction;
    u8 col = 0;
    u8 col_span = 1;
};

struct ImeTopPanelLayoutConfig {
    const ImeTopPanelElementSpec* elements = nullptr;
    std::size_t element_count = 0;
    u8 cols = 10;
    u8 row = static_cast<u8>(ImeSelectionGridIndex::DefaultTopPanelRow);
    u8 row_span = static_cast<u8>(ImeSelectionGridIndex::DefaultTopPanelRows);
};

// Mirrors OrbisImeParamExtended color buckets so UI styling can be themed
// through one config and optionally overridden by game-provided SET_COLOR.
struct ImeStyleConfig {
    OrbisImeColor color_base{18, 18, 18, 255};
    OrbisImeColor color_line{70, 70, 70, 255};
    OrbisImeColor color_text_field{22, 37, 60, 255};
    OrbisImeColor color_preedit{35, 35, 35, 255};
    OrbisImeColor color_button_default{35, 35, 35, 255};
    OrbisImeColor color_button_function{60, 60, 60, 255};
    OrbisImeColor color_button_symbol{78, 78, 78, 255};
    OrbisImeColor color_text{230, 230, 230, 255};
    OrbisImeColor color_special{30, 90, 170, 255};
};

// Shared edge-wrap hold state for controller navigation.
// It tracks last successful move direction/time and active wrap hold window.
struct ImeEdgeWrapNavState {
    int last_step_row = 0;
    int last_step_col = 0;
    double last_step_time = -1.0;
    int hold_step_row = 0;
    int hold_step_col = 0;
    double hold_release_time = 0.0;
    bool hold_active = false;
};

inline void ResetImeEdgeWrapHold(ImeEdgeWrapNavState& state) {
    state.hold_step_row = 0;
    state.hold_step_col = 0;
    state.hold_release_time = 0.0;
    state.hold_active = false;
}

inline void ResetImeEdgeWrapNav(ImeEdgeWrapNavState& state) {
    state.last_step_row = 0;
    state.last_step_col = 0;
    state.last_step_time = -1.0;
    ResetImeEdgeWrapHold(state);
}

inline bool ShouldDelayImeEdgeWrap(ImeEdgeWrapNavState& state, int step_row, int step_col,
                                   bool repeat_hint, bool wraps, double now, double hold_delay_sec,
                                   double repeat_window_sec) {
    (void)repeat_window_sec;
    if (step_row == 0 && step_col == 0) {
        return false;
    }
    if (state.hold_active && (state.hold_step_row != step_row || state.hold_step_col != step_col)) {
        ResetImeEdgeWrapHold(state);
    }

    if (!wraps) {
        return false;
    }

    const bool same_wrap_direction =
        state.hold_active && state.hold_step_row == step_row && state.hold_step_col == step_col;
    if (!(repeat_hint || same_wrap_direction)) {
        return false;
    }

    if (!same_wrap_direction) {
        state.hold_step_row = step_row;
        state.hold_step_col = step_col;
        state.hold_release_time = now + hold_delay_sec;
        state.hold_active = true;
    }
    return now < state.hold_release_time;
}

inline void CommitImeEdgeWrapStep(ImeEdgeWrapNavState& state, int step_row, int step_col,
                                  double now) {
    state.last_step_row = step_row;
    state.last_step_col = step_col;
    state.last_step_time = now;
    ResetImeEdgeWrapHold(state);
}

inline const ImeKbKeySpec* ResolveImeKeyboardKeyAt(const ImeKbLayoutModel& layout, int row,
                                                   int col) {
    const int grid_cols = std::max(1, static_cast<int>(layout.cols));
    const int grid_rows = std::max(1, static_cast<int>(layout.rows));
    if (row < 0 || row >= grid_rows || col < 0 || col >= grid_cols) {
        return nullptr;
    }
    if (!layout.keys || layout.key_count == 0) {
        return nullptr;
    }

    const ImeKbKeySpec* resolved = nullptr;
    for (std::size_t i = 0; i < layout.key_count; ++i) {
        const auto& key = layout.keys[i];
        if (key.col_span == 0 || key.row_span == 0) {
            continue;
        }
        if (key.row >= grid_rows || key.col >= grid_cols) {
            continue;
        }
        const int row_start = static_cast<int>(key.row);
        const int col_start = static_cast<int>(key.col);
        const int row_span = std::max(1, static_cast<int>(key.row_span));
        const int col_span = std::max(1, static_cast<int>(key.col_span));
        const int row_end = std::min(grid_rows, row_start + row_span);
        const int col_end = std::min(grid_cols, col_start + col_span);
        if (row >= row_start && row < row_end && col >= col_start && col < col_end) {
            // Match DrawImeKeyboardGrid occupancy behavior: later keys override earlier ones.
            resolved = &key;
        }
    }
    return resolved;
}

inline bool DoesImeKeyboardStepCrossGridEdge(int from_row, int from_col, int step_row, int step_col,
                                             int grid_rows, int grid_cols) {
    if (step_row == 0 && step_col == 0) {
        return false;
    }
    if (from_row < 0 || from_row >= grid_rows || from_col < 0 || from_col >= grid_cols) {
        return false;
    }

    const int next_row = from_row + step_row;
    const int next_col = from_col + step_col;
    return next_row < 0 || next_row >= grid_rows || next_col < 0 || next_col >= grid_cols;
}

inline bool DoesImeKeyboardNavigationWrap(const ImeKbLayoutModel& layout, int from_row,
                                          int from_col, int step_row, int step_col) {
    if (step_row == 0 && step_col == 0) {
        return false;
    }

    const int grid_cols = std::max(1, static_cast<int>(layout.cols));
    const int grid_rows = std::max(1, static_cast<int>(layout.rows));
    if (from_row < 0 || from_row >= grid_rows || from_col < 0 || from_col >= grid_cols) {
        return false;
    }

    const auto* origin_key = ResolveImeKeyboardKeyAt(layout, from_row, from_col);
    if (origin_key && origin_key->action == ImeKbKeyAction::None) {
        origin_key = nullptr;
    }

    int row = from_row;
    int col = from_col;
    bool crossed_wrap = false;
    const int max_steps = std::max(1, grid_rows * grid_cols);
    for (int i = 0; i < max_steps; ++i) {
        crossed_wrap = crossed_wrap || DoesImeKeyboardStepCrossGridEdge(
                                           row, col, step_row, step_col, grid_rows, grid_cols);

        const int next_row = row + step_row;
        const int next_col = col + step_col;
        row = (next_row + grid_rows) % grid_rows;
        col = (next_col + grid_cols) % grid_cols;

        const auto* candidate_key = ResolveImeKeyboardKeyAt(layout, row, col);
        if (origin_key && candidate_key == origin_key) {
            continue;
        }
        return crossed_wrap;
    }
    return false;
}

struct ImeKbDrawParams {
    ImeKbLayoutSelection selection{};
    const ImeKbLayoutModel* layout_model = nullptr;
    OrbisImeLanguage supported_languages = static_cast<OrbisImeLanguage>(0);
    OrbisImeEnterLabel enter_label = OrbisImeEnterLabel::Default;
    bool show_selection_highlight = true;
    bool allow_nav_input = true;
    bool use_imgui_lstick_nav = true;
    bool allow_activate_input = true;
    bool external_nav_left = false;
    bool external_nav_right = false;
    bool external_nav_up = false;
    bool external_nav_down = false;
    bool external_nav_left_repeat = false;
    bool external_nav_right_repeat = false;
    bool external_nav_up_repeat = false;
    bool external_nav_down_repeat = false;
    bool external_activate_pressed = false;
    bool external_activate_repeat = false;
    bool reset_nav_state = false;
    int requested_selected_row = -1;
    int requested_selected_col = -1;
    ImU32 key_bg_default = IM_COL32(35, 35, 35, 255);
    ImU32 key_bg_function = IM_COL32(60, 60, 60, 255);
    ImU32 key_bg_symbol = IM_COL32(78, 78, 78, 255);
    ImU32 key_border = IM_COL32(80, 80, 80, 255);
    ImU32 key_done = IM_COL32(30, 90, 170, 255);
    ImU32 key_text = IM_COL32(230, 230, 230, 255);
    ImU32 key_hotkey_text = IM_COL32(220, 220, 220, 255);
};

struct ImeKbDrawState {
    bool done_pressed = false;
    ImeKbKeyAction pressed_action = ImeKbKeyAction::None;
    const char* pressed_label = nullptr;
    u16 pressed_keycode = 0;
    char16_t pressed_character = u'\0';
    // Logical grid cursor. If the cursor cell is disabled, selected_center points to the
    // visible nearest enabled fallback key used for drawing and activation.
    int selected_row = -1;
    int selected_col = -1;
    ImVec2 selected_center{};
    bool hovered = false;
    bool clicked = false;
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
ImeKbLayoutId ResolveImeKeyboardLayoutId(const ImeKbLayoutSelection& selection);
const ImeKbLayoutModel& GetImeKeyboardLayout(ImeKbLayoutId id);
const ImeKbLayoutModel& GetImeKeyboardLayout(const ImeKbLayoutSelection& selection);
const ImeTopPanelLayoutConfig& GetImeTopPanelLayoutConfig();
const ImeTopPanelLayoutConfig& GetImeTopPanelLayoutConfig(ImeKbLayoutId id);
const ImeTopPanelLayoutConfig& GetImeTopPanelLayoutConfig(const ImeKbLayoutSelection& selection);
ImeStyleConfig GetDefaultImeStyleConfig();
ImeStyleConfig ResolveImeStyleConfig(const OrbisImeParamExtended* extended);
ImU32 ImeColorToImU32(const OrbisImeColor& color);
ImVec4 ImeColorToImVec4(const OrbisImeColor& color);
void ApplyImeStyleToKeyboardDrawParams(const ImeStyleConfig& style, ImeKbDrawParams& params);
void AddImeKeyboardGlyphsToFontRanges(ImFontGlyphRangesBuilder& builder);

void DrawImeKeyboardGrid(const ImeKbGridLayout& layout, const ImeKbDrawParams& params,
                         ImeKbDrawState& state);

} // namespace Libraries::Ime
