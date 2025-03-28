// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cctype>
#include <cstring>
#include <string>
#include <imgui.h>
#include "ime_keyboard_ui.h"

void DrawVirtualKeyboard(char* buffer, std::size_t buffer_capacity, bool* input_changed,
                         KeyboardMode& kb_mode, bool& shift_enabled) {
    const char* row1_letters = "QWERTYUIOP";
    const char* row2_letters = "ASDFGHJKL";
    const char* row3_letters = "ZXCVBNM";

    const char* row1_symbols = "1234567890";
    const char* row2_symbols = "!@#$%^&*()";
    const char* row3_symbols = "-_=+[]{}";

    auto draw_row = [&](const char* row, float offset_x) {
        ImGui::SetCursorPosX(offset_x);
        for (int i = 0; row[i] != '\0'; ++i) {
            char ch = shift_enabled ? row[i] : static_cast<char>(tolower(row[i]));
            std::string key(1, ch);
            if (ImGui::Button(key.c_str(), ImVec2(35, 35))) {
                size_t len = std::strlen(buffer);
                if (len + 1 < buffer_capacity) {
                    buffer[len] = ch;
                    buffer[len + 1] = '\0';
                    if (input_changed) {
                        *input_changed = true;
                    }
                }
            }
            ImGui::SameLine();
        }
        ImGui::NewLine();
    };

    // SHIFT status label
    if (shift_enabled) {
        ImGui::SetCursorPosX(20.0f);
        ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "SHIFT ENABLED");
    }

    draw_row(kb_mode == KeyboardMode::Letters ? row1_letters : row1_symbols, 20.0f);
    draw_row(kb_mode == KeyboardMode::Letters ? row2_letters : row2_symbols, 35.0f);
    draw_row(kb_mode == KeyboardMode::Letters ? row3_letters : row3_symbols, 80.0f);

    ImGui::SetCursorPosX(20.0f);

    bool highlight = shift_enabled;
    if (highlight) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
    }

    if (ImGui::Button("SHIFT", ImVec2(75, 35))) {
        shift_enabled = !shift_enabled;
    }

    if (highlight) {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    if (ImGui::Button("SPACE", ImVec2(100, 35))) {
        size_t len = std::strlen(buffer);
        if (len + 1 < buffer_capacity) {
            buffer[len] = ' ';
            buffer[len + 1] = '\0';
            if (input_changed) {
                *input_changed = true;
            }
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("DELETE", ImVec2(75, 35))) {
        size_t len = std::strlen(buffer);
        if (len > 0) {
            buffer[len - 1] = '\0';
            if (input_changed) {
                *input_changed = true;
            }
        }
    }

    ImGui::SameLine();

    if (ImGui::Button(kb_mode == KeyboardMode::Letters ? "123" : "ABC", ImVec2(60, 35))) {
        kb_mode =
            (kb_mode == KeyboardMode::Letters) ? KeyboardMode::Symbols : KeyboardMode::Letters;
    }
}
