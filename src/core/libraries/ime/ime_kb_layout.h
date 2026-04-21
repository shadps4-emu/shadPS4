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
    float key_h = 0.0f;
    int cols = 10;
    int rows = 6;
    float corner_radius = 0.0f;
};

// Standardized OSK selection indexing:
// - Keyboard grid uses zero-based row/column indexing.
// - Panel navigation uses a virtual 7x10 grid:
//   row 0 is top panel (prediction 0..8, close 9), rows 1..6 map to keyboard rows 0..5.
struct ImeSelectionGridIndex {
    static constexpr int PanelMinRow = 0;
    static constexpr int PanelMaxRow = 6;
    static constexpr int PanelTopRow = 0;
    static constexpr int PanelKeyboardMinRow = 1;
    static constexpr int PanelKeyboardMaxRow = 6;

    static constexpr int KeyboardMinRow = 0;
    static constexpr int KeyboardMaxRow = 5;
    static constexpr int KeyboardMinCol = 0;
    static constexpr int KeyboardMaxCol = 9;

    static constexpr int TopRowMinCol = 0;
    static constexpr int TopRowPredictionMaxCol = 8;
    static constexpr int TopRowCloseCol = 9;

    static constexpr int ClampPanelRow(int row) {
        return std::clamp(row, PanelMinRow, PanelMaxRow);
    }

    static constexpr int ClampKeyboardRow(int row) {
        return std::clamp(row, KeyboardMinRow, KeyboardMaxRow);
    }

    static constexpr int ClampKeyboardCol(int col) {
        return std::clamp(col, KeyboardMinCol, KeyboardMaxCol);
    }

    static constexpr int ClampTopRowCol(int col) {
        return std::clamp(col, TopRowMinCol, TopRowCloseCol);
    }

    static constexpr int ClampTopPredictionCol(int col) {
        return std::clamp(col, TopRowMinCol, TopRowPredictionMaxCol);
    }

    static constexpr int TopToKeyboardCol(int top_col) {
        return ClampKeyboardCol(top_col);
    }

    static constexpr int KeyboardToTopCol(int keyboard_col) {
        return ClampTopRowCol(keyboard_col);
    }

    static constexpr int PanelToKeyboardRow(int panel_row) {
        return ClampKeyboardRow(panel_row - PanelKeyboardMinRow);
    }

    static constexpr int KeyboardToPanelRow(int keyboard_row) {
        return ClampPanelRow(keyboard_row + PanelKeyboardMinRow);
    }

    static int GridColumnFromX(float x, float left, float width, int min_col, int max_col) {
        if (max_col < min_col || width <= 0.0f) {
            return min_col;
        }
        const float t = std::clamp((x - left) / width, 0.0f, 0.9999f);
        const int span = max_col - min_col + 1;
        const int offset =
            std::clamp(static_cast<int>(t * static_cast<float>(span)), 0, span - 1);
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
};

struct ImeKbDrawParams {
    ImeKbLayoutSelection selection{};
    const ImeKbLayoutModel* layout_model = nullptr;
    OrbisImeLanguage supported_languages = static_cast<OrbisImeLanguage>(0);
    OrbisImeEnterLabel enter_label = OrbisImeEnterLabel::Default;
    bool show_selection_highlight = true;
    bool allow_nav_input = true;
    bool allow_activate_input = true;
    bool external_activate_pressed = false;
    int requested_selected_row = -1;
    int requested_selected_col = -1;
    ImU32 key_bg = IM_COL32(35, 35, 35, 255);
    ImU32 key_bg_alt = IM_COL32(50, 50, 50, 255);
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

void DrawImeKeyboardGrid(const ImeKbGridLayout& layout, const ImeKbDrawParams& params,
                         ImeKbDrawState& state);

} // namespace Libraries::Ime
