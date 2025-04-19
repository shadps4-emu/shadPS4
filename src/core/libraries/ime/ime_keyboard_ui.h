#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/libraries/ime/ime_keyboard_layouts.h"

/**
 * KeyboardMode: which layout we show (letters, accents, symbols, etc.)
 */
enum class KeyboardMode { Letters, AccentLetters, Symbols1, Symbols2 };

/**
 * We handle raw key "Down" or "Up" events from an on-screen keyboard.
 */
enum class VirtualKeyEventType { Down, Up, Repeat };

struct VirtualKeyEvent {
    VirtualKeyEventType type;
    const KeyEntry* key;
};

enum class ShiftState : u8 {
    None = 0,    // lowercase
    Shift = 1,   // temporary uppercase
    CapsLock = 2 // full uppercase
};

/**
 * This struct holds all visual parameters for the on-screen keyboard,
 * including layout size, spacing, and button colors.
 *
 * If extended parameters are present, it override these defaults
 * in IME code. Then pass the result to DrawVirtualKeyboard(...).
 */
struct KeyboardStyle {
    float layout_width = 500.0f;
    float layout_height = 300.0f;
    float key_spacing = 5.0f;

    // For text, lines, etc.
    ImU32 color_text = IM_COL32(225, 225, 225, 255);
    ImU32 color_line = IM_COL32(88, 88, 88, 255);

    // Button colors
    ImU32 color_button_default = IM_COL32(35, 35, 35, 255);
    ImU32 color_button_function = IM_COL32(50, 50, 50, 255);
    ImU32 color_special = IM_COL32(0, 140, 200, 255);

    // If you're on a symbols layout, you may want to color them differently.
    bool use_button_symbol_color = false;
    ImU32 color_button_symbol = IM_COL32(60, 60, 60, 255);
};

/**
 * Safely remove one UTF-8 glyph from the end of 'buffer'.
 */
void Utf8SafeBackspace(char* buffer);

/**
 * Returns the appropriate layout (vector of KeyEntry) for the given
 * OrbisImeType, KeyboardMode, ShiftState, and language bitmask.
 */
const std::vector<KeyEntry>* GetKeyboardLayout(OrbisImeType type, KeyboardMode mode,
                                               ShiftState shift, u64 language);

/**
 * Renders a given layout using the style logic:
 * - If 'mode' is a symbols layout (Symbols1 or Symbols2) AND style.use_button_symbol_color == true,
 *   then normal character keys are drawn with style.color_button_symbol
 * - Function keys => style.color_button_function
 * - The "Done" or "Enter" key (keycode 0x0D) => style.color_special
 * - Otherwise => style.color_button_default
 *
 * We call on_key_event(...) with VirtualKeyEventType::Down/Up when the user clicks or releases a
 * key.
 */
void RenderKeyboardLayout(const std::vector<KeyEntry>& layout, KeyboardMode mode,
                          void (*on_key_event)(const VirtualKeyEvent*), const KeyboardStyle& style);

/**
 * Picks the correct layout from GetKeyboardLayout() for the given
 * kb_mode, shift_state, etc., then calls RenderKeyboardLayout().
 */
void DrawVirtualKeyboard(KeyboardMode kb_mode, OrbisImeType ime_type, ShiftState shift_state,
                         u64 language, void (*on_key_event)(const VirtualKeyEvent*),
                         const KeyboardStyle& style);
