#include <cstring>
#include <unordered_set>
#include <imgui.h>
#include <imgui_internal.h>

#include "ime_common.h"
#include "ime_dialog.h"
#include "ime_keyboard_layouts.h"
#include "ime_keyboard_ui.h"
using namespace ImGui;

/**
 * Removes one UTF-8 codepoint from the end of 'buffer', if present.
 */
void Utf8SafeBackspace(char* buffer) {
    size_t len = std::strlen(buffer);
    if (len == 0)
        return;

    // Move backward over any continuation bytes.
    while (len > 0 && (static_cast<unsigned char>(buffer[len]) & 0b11000000) == 0b10000000) {
        --len;
    }

    if (len > 0) {
        // Remove one codepoint.
        buffer[len - 1] = '\0';
        buffer[len] = '\0';
    }
}

/**
 * Picks which layout vector we want for OrbisImeType, kb_mode, shift_state, etc.
 */
const std::vector<KeyEntry>* GetKeyboardLayout(OrbisImeType type, KeyboardMode mode,
                                               ShiftState shift, u64 language) {
    switch (type) {
    case OrbisImeType::Number:
        // For numeric input, you might have a dedicated numeric layout,
        // but here we reuse kLayoutEnSymbols1.
        return &kLayoutEnSymbols1;

    case OrbisImeType::Url:
    case OrbisImeType::Mail:
        // Use letters; uppercase if SHIFT is on.
        if (shift == ShiftState::CapsLock || shift == ShiftState::Shift) {
            return &kLayoutEnLettersUppercase;
        } else {
            return &kLayoutEnLettersLowercase;
        }

    case OrbisImeType::BasicLatin:
    case OrbisImeType::Default:
    default:
        switch (mode) {
        case KeyboardMode::Symbols1:
            return &kLayoutEnSymbols1;
        case KeyboardMode::Symbols2:
            return &kLayoutEnSymbols2;
        case KeyboardMode::AccentLetters:
            if (shift == ShiftState::CapsLock || shift == ShiftState::Shift) {
                return &kLayoutEnAccentLettersUppercase;
            } else {
                return &kLayoutEnAccentLettersLowercase;
            }
        case KeyboardMode::Letters:
        default:
            if (shift == ShiftState::CapsLock || shift == ShiftState::Shift) {
                return &kLayoutEnLettersUppercase;
            } else {
                return &kLayoutEnLettersLowercase;
            }
        }
    }
}

/**
 * Renders the given layout using the style logic:
 *  - For symbols layout and if style.use_button_symbol_color is true,
 *    character keys get style.color_button_symbol.
 *  - Function keys get style.color_button_function.
 *  - The "Done"/"Enter" key (keycode 0x0D) gets style.color_special.
 *  - Otherwise, keys use style.color_button_default.
 *
 * This version retains all GUI layout details (positions, colors, sizes, etc.) exactly as in your
 * base files. The only change is in key event detection: after drawing each key with Button(), we
 * use IsItemActive() to determine the pressed state so that the backend key processing works
 * correctly.
 */
void RenderKeyboardLayout(const std::vector<KeyEntry>& layout, KeyboardMode mode,
                          void (*on_key_event)(const VirtualKeyEvent*),
                          const KeyboardStyle& style) {
    ImGui::BeginGroup();

    /* ─────────────── 1. grid size & cell metrics ─────────────── */
    int max_col = 0, max_row = 0;
    for (const KeyEntry& k : layout) {
        max_col = std::max(max_col, k.col + (int)k.colspan);
        max_row = std::max(max_row, k.row + (int)k.rowspan);
    }
    if (max_col == 0 || max_row == 0) {
        ImGui::EndGroup();
        return;
    }

    const float pad = 20.0f;
    const float spacing_w = (max_col - 1) * style.key_spacing;
    const float spacing_h = (max_row - 1) * style.key_spacing;
    const float cell_w = std::floor((style.layout_width - spacing_w - 2 * pad) / max_col + 0.5f);
    const float cell_h = std::floor((style.layout_height - spacing_h - 85.0f) / max_row + 0.5f);

    ImVec2 origin = ImGui::GetCursorScreenPos();
    origin.x += pad;

    ImGui::PushStyleColor(ImGuiCol_NavHighlight, style.color_line);
    ImGui::SetWindowFontScale(1.50f);

    const int function_rows_start = std::max(0, max_row - 2);

    /* ─────────────── 2. draw every key ───────────────────────── */
    for (const KeyEntry& key : layout) {
        /* position & size */
        float x = origin.x + key.col * (cell_w + style.key_spacing);
        float y = origin.y + key.row * (cell_h + style.key_spacing);
        float w = key.colspan * cell_w + (key.colspan - 1) * style.key_spacing;
        float h = key.rowspan * cell_h + (key.rowspan - 1) * style.key_spacing;
        ImVec2 pos(x, y), size(w, h);

        /* ------------ background colour decision --------------- */
        const bool in_function_rows = (key.row >= function_rows_start);
        const bool is_done_enter = (key.keycode == 0x0D);

        ImU32 bg_color;
        if (is_done_enter) {
            bg_color = style.color_special; // always wins
        } else if (in_function_rows) {
            bg_color = style.color_button_function; // bottom two rows
        } else if ((mode == KeyboardMode::Symbols1 || mode == KeyboardMode::Symbols2) &&
                   style.use_button_symbol_color) {
            bg_color = style.color_button_symbol; // symbol tint
        } else {
            bg_color = style.color_button_default; // normal default
        }

        /* label */
        std::string label = (key.label && key.label[0]) ? key.label : " ";

        /* ---------- ImGui button ---------- */
        ImGui::PushID(&key);
        ImGui::PushStyleColor(ImGuiCol_Text, style.color_text);
        ImGui::PushStyleColor(ImGuiCol_Button, bg_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              IM_COL32((bg_color >> IM_COL32_R_SHIFT & 0xFF) * 220 / 255,
                                       (bg_color >> IM_COL32_G_SHIFT & 0xFF) * 220 / 255,
                                       (bg_color >> IM_COL32_B_SHIFT & 0xFF) * 220 / 255,
                                       (bg_color >> IM_COL32_A_SHIFT & 0xFF)));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              IM_COL32((bg_color >> IM_COL32_R_SHIFT & 0xFF) * 180 / 255,
                                       (bg_color >> IM_COL32_G_SHIFT & 0xFF) * 180 / 255,
                                       (bg_color >> IM_COL32_B_SHIFT & 0xFF) * 180 / 255,
                                       (bg_color >> IM_COL32_A_SHIFT & 0xFF)));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

        if (key.allow_repeat)
            ImGui::PushButtonRepeat(true);

        ImGui::SetCursorScreenPos(pos);
        bool pressed = ImGui::Button(label.c_str(), size); // Down + repeats

        if (key.allow_repeat)
            ImGui::PopButtonRepeat();

        /* ---------- event generation ---------- */
        if (on_key_event) {
            if (ImGui::IsItemActivated()) {
                VirtualKeyEvent ev{VirtualKeyEventType::Down, &key};
                on_key_event(&ev);
            } else if (pressed && key.allow_repeat) {
                VirtualKeyEvent ev{VirtualKeyEventType::Repeat, &key};
                on_key_event(&ev);
            }

            if (ImGui::IsItemDeactivated()) {
                VirtualKeyEvent ev{VirtualKeyEventType::Up, &key};
                on_key_event(&ev);
            }
        }

        /* cleanup */
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);
        ImGui::PopID();
    }

    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor(); // NavHighlight
    ImGui::EndGroup();
}

/**
 * Selects the correct layout via GetKeyboardLayout() then calls RenderKeyboardLayout().
 */
void DrawVirtualKeyboard(KeyboardMode kb_mode, OrbisImeType ime_type, ShiftState shift_state,
                         u64 language, void (*on_key_event)(const VirtualKeyEvent*),
                         const KeyboardStyle& style) {
    const std::vector<KeyEntry>* layout =
        GetKeyboardLayout(ime_type, kb_mode, shift_state, language);
    if (!layout)
        return;

    RenderKeyboardLayout(*layout, kb_mode, on_key_event, style);
}
