// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <imgui.h>
#include "ime_keyboard_layouts.h"
#include "ime_keyboard_ui.h"

using namespace ImGui;

void DrawVirtualKeyboard(char* buffer, std::size_t buffer_capacity, bool* input_changed,
                         KeyboardMode& kb_mode, bool& shift_enabled, bool* done_pressed) {
    const std::vector<Key>* layout = nullptr;

    if (kb_mode == KeyboardMode::Symbols) {
        layout = &kSymbols1Layout;
    } else {
        layout = shift_enabled ? &kUppercaseLayout : &kLowercaseLayout;
    }

    RenderKeyboardLayout(*layout, buffer, buffer_capacity, input_changed, kb_mode, shift_enabled,
                         done_pressed);
}

void RenderKeyboardLayout(const std::vector<Key>& layout, char* buffer, std::size_t buffer_capacity,
                          bool* input_changed, KeyboardMode& kb_mode, bool& shift_enabled,
                          bool* done_pressed) {
    // Define desired total layout size (in pixels)
    const float layout_width = 485.0f;
    const float layout_height = 200.0f;
    const float cell_spacing = 4.0f;
    const float hint_padding = 2.0f;

    // Find max rows and columns
    int max_col = 0;
    int max_row = 0;
    for (const Key& key : layout) {
        max_col = std::max(max_col, key.col + static_cast<int>(key.colspan));
        max_row = std::max(max_row, key.row + static_cast<int>(key.rowspan));
    }

    // Calculate cell size dynamically
    const float cell_width = (layout_width - (max_col - 1) * cell_spacing) / max_col;
    const float cell_height = (layout_height - (max_row - 1) * cell_spacing) / max_row;

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (const Key& key : layout) {
        float x = origin.x + key.col * (cell_width + cell_spacing);
        float y = origin.y + key.row * (cell_height + cell_spacing);

        ImVec2 pos(x, y);
        ImVec2 size(key.colspan * cell_width + (key.colspan - 1) * cell_spacing,
                    key.rowspan * cell_height + (key.rowspan - 1) * cell_spacing);

        std::string button_id =
            key.label.empty() ? "##empty_" + std::to_string(key.row) + "_" + std::to_string(key.col)
                              : key.label;

        ImGui::SetCursorScreenPos(pos);

        if (ImGui::Button(button_id.c_str(), size)) {
            switch (key.type) {
            case KeyType::Text:
                if (!key.label.empty()) {
                    size_t len = std::strlen(buffer);
                    if (len + key.label.size() < buffer_capacity) {
                        std::strcat(buffer, key.label.c_str());
                        if (input_changed)
                            *input_changed = true;
                    }
                }
                break;
            case KeyType::Backspace:
                if (buffer[0] != '\0') {
                    size_t len = std::strlen(buffer);
                    buffer[len - 1] = '\0';
                    if (input_changed)
                        *input_changed = true;
                }
                break;
            case KeyType::Space:
                if (std::strlen(buffer) + 1 < buffer_capacity) {
                    std::strcat(buffer, " ");
                    if (input_changed)
                        *input_changed = true;
                }
                break;
            case KeyType::Enter:
            case KeyType::Done:
                if (done_pressed)
                    *done_pressed = true;
                break;
            case KeyType::Shift:
                shift_enabled = !shift_enabled;
                break;
            case KeyType::SymbolsLayout:
                kb_mode = KeyboardMode::Symbols;
                break;
            case KeyType::TextLayout:
                kb_mode = KeyboardMode::Letters;
                break;
            case KeyType::ToggleKeyboard:
                kb_mode = (kb_mode == KeyboardMode::Letters) ? KeyboardMode::Symbols
                                                             : KeyboardMode::Letters;
                break;
            default:
                break;
            }
        }

        // Controller hint
        if (!key.controller_hint.empty()) {
            float original_font_size = ImGui::GetFontSize();
            float small_font_size = original_font_size * 0.5f;

            ImVec2 text_size =
                ImGui::CalcTextSize(key.controller_hint.c_str(), nullptr, false, -1.0f) *
                (small_font_size / original_font_size);

            ImVec2 text_pos = pos + ImVec2(hint_padding, hint_padding);
            ImVec2 bg_min = text_pos - ImVec2(1.0f, 1.0f);
            ImVec2 bg_max = text_pos + text_size + ImVec2(2.0f, 1.0f);

            ImU32 bg_color = IM_COL32(0, 0, 0, 160);
            ImU32 fg_color = IM_COL32(255, 255, 255, 200);

            draw_list->AddRectFilled(bg_min, bg_max, bg_color, 2.0f);
            draw_list->AddText(nullptr, small_font_size, text_pos, fg_color,
                               key.controller_hint.c_str());
        }
    }
}
