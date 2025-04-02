// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>
#include <imgui.h>

enum class KeyType {
    Text,          // Inserts character(s) into input buffer
    Backspace,     // Deletes last character
    Space,         // Adds space
    Enter,         // Submits input
    Shift,         // Toggle uppercase/lowercase
    Symbols1Layout, // Switch to symbols layout
    Symbols2Layout, // Switch to symbols layout
    TextLayout,    // Switch to text layout
    Done,          // Finish and close keyboard
    CursorLeft,
    CursorRight,
    CursorUp,
    CursorDown,
    ToggleKeyboard,   // Toggles keyboard layout
    MoreOptions,      // "..." button
    ControllerAction, // e.g. R3 🎮⊕
    Disabled,         // Filler or placeholder
    UnknownFunction,  // now same as disabled
};

struct Key {
    int row;
    int col;
    int colspan = 1;
    int rowspan = 1;
    std::string label;
    std::string controller_hint;
    KeyType type = KeyType::Text;
    std::vector<ImGuiNavInput> bound_buttons = {}; // Now using ImGui navigation inputs
};

extern const std::vector<Key> kUppercaseLayout;
extern const std::vector<Key> kLowercaseLayout;
extern const std::vector<Key> kSymbols1Layout;
extern const std::vector<Key> kSymbols2Layout;
