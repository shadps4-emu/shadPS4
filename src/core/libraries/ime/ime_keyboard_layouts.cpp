// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>
#include "ime_keyboard_layouts.h"

constexpr auto L1 = ImGuiNavInput_FocusPrev;
constexpr auto R1 = ImGuiNavInput_FocusNext;
constexpr auto L2 = ImGuiNavInput_TweakSlow;
constexpr auto R2 = ImGuiNavInput_TweakFast;
constexpr auto L3 = ImGuiNavInput_DpadLeft;  // adjust if needed
constexpr auto R3 = ImGuiNavInput_DpadRight; // adjust if needed
constexpr auto Up = ImGuiNavInput_DpadUp;
constexpr auto Down = ImGuiNavInput_DpadDown;
constexpr auto Left = ImGuiNavInput_DpadLeft;
constexpr auto Right = ImGuiNavInput_DpadRight;
constexpr auto Cross = ImGuiNavInput_Activate;
constexpr auto Circle = ImGuiNavInput_Menu;
constexpr auto Square = ImGuiNavInput_Cancel;
constexpr auto Triangle = ImGuiNavInput_Input;
constexpr auto TouchPad = ImGuiNavInput_Menu; // reuse if needed

const std::vector<Key> kUppercaseLayout = {
    // Row 1
    {0, 0, 1, 1, "1", "", KeyType::Text, {}},
    {0, 1, 1, 1, "2", "", KeyType::Text, {}},
    {0, 2, 1, 1, "3", "", KeyType::Text, {}},
    {0, 3, 1, 1, "4", "", KeyType::Text, {}},
    {0, 4, 1, 1, "5", "", KeyType::Text, {}},
    {0, 5, 1, 1, "6", "", KeyType::Text, {}},
    {0, 6, 1, 1, "7", "", KeyType::Text, {}},
    {0, 7, 1, 1, "8", "", KeyType::Text, {}},
    {0, 8, 1, 1, "9", "", KeyType::Text, {}},
    {0, 9, 1, 1, "0", "", KeyType::Text, {}},

    // Row 2
    {1, 0, 1, 1, "Q", "", KeyType::Text, {}},
    {1, 1, 1, 1, "W", "", KeyType::Text, {}},
    {1, 2, 1, 1, "E", "", KeyType::Text, {}},
    {1, 3, 1, 1, "R", "", KeyType::Text, {}},
    {1, 4, 1, 1, "T", "", KeyType::Text, {}},
    {1, 5, 1, 1, "Y", "", KeyType::Text, {}},
    {1, 6, 1, 1, "U", "", KeyType::Text, {}},
    {1, 7, 1, 1, "I", "", KeyType::Text, {}},
    {1, 8, 1, 1, "O", "", KeyType::Text, {}},
    {1, 9, 1, 1, "P", "", KeyType::Text, {}},

    // Row 3
    {2, 0, 1, 1, "A", "", KeyType::Text, {}},
    {2, 1, 1, 1, "S", "", KeyType::Text, {}},
    {2, 2, 1, 1, "D", "", KeyType::Text, {}},
    {2, 3, 1, 1, "F", "", KeyType::Text, {}},
    {2, 4, 1, 1, "G", "", KeyType::Text, {}},
    {2, 5, 1, 1, "H", "", KeyType::Text, {}},
    {2, 6, 1, 1, "J", "", KeyType::Text, {}},
    {2, 7, 1, 1, "K", "", KeyType::Text, {}},
    {2, 8, 1, 1, "L", "", KeyType::Text, {}},
    {2, 9, 1, 1, "\"", "", KeyType::Text, {}},

    // Row 4
    {3, 0, 1, 1, "Z", "", KeyType::Text, {}},
    {3, 1, 1, 1, "X", "", KeyType::Text, {}},
    {3, 2, 1, 1, "C", "", KeyType::Text, {}},
    {3, 3, 1, 1, "V", "", KeyType::Text, {}},
    {3, 4, 1, 1, "B", "", KeyType::Text, {}},
    {3, 5, 1, 1, "N", "", KeyType::Text, {}},
    {3, 6, 1, 1, "M", "", KeyType::Text, {}},
    {3, 7, 1, 1, "-", "", KeyType::Text, {}},
    {3, 8, 1, 1, "_", "", KeyType::Text, {}},
    {3, 9, 1, 1, "/", "", KeyType::Text, {}},

    // Row 5
    {4, 0, 1, 1, "⬆", "L2", KeyType::Shift, {L2}},

    {4, 1, 1, 1, "@#:", "L2+△", KeyType::Symbols1Layout, {L3, Triangle}},
    {4, 2, 1, 1, "à", "L3", KeyType::UnknownFunction, {L3}},
    {4, 3, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    //{4, 4, 4, 1, "Space", "△", KeyType::Space,{Triangle}},
    //{4, 5, 4, 1, "Space", "△", KeyType::Space,{Triangle}},
    //{4, 6, 4, 1, "Space", "△", KeyType::Space,{Triangle}},
    {4, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 8, 2, 1, "⇦", "□", KeyType::Backspace, {Square}},
    //{4, 9, 2, 1, "⇦", "□", KeyType::Backspace,{Square}},

    // Row 6
    {5, 0, 1, 1, "▲", "", KeyType::CursorUp, {Up}},
    {5, 1, 1, 1, "▼", "", KeyType::CursorDown, {Down}},
    {5, 2, 1, 1, "◀", "L1", KeyType::CursorLeft, {L1}},
    {5, 3, 1, 1, "▶", "R1", KeyType::CursorRight, {R1}},
    {5, 4, 1, 1, "KB", "", KeyType::ToggleKeyboard, {}},
    //{5, 4, 1, 1, "⌨", "", KeyType::ToggleKeyboard,{}},
    {5, 5, 1, 1, "...", "", KeyType::MoreOptions, {}},
    //{5, 5, 1, 1, "…", "", KeyType::MoreOptions,{}},
    {5, 6, 1, 1, "+/⊗", "R3", KeyType::ControllerAction, {R3}},
    {5, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {5, 8, 2, 1, "Done", "R2", KeyType::Done, {R2}},
    //{5, 9, 2, 1, "Done", "R2", KeyType::Done,{R2}},
};

const std::vector<Key> kLowercaseLayout = {
    // Row 1
    {0, 0, 1, 1, "1", "", KeyType::Text, {}},
    {0, 1, 1, 1, "2", "", KeyType::Text, {}},
    {0, 2, 1, 1, "3", "", KeyType::Text, {}},
    {0, 3, 1, 1, "4", "", KeyType::Text, {}},
    {0, 4, 1, 1, "5", "", KeyType::Text, {}},
    {0, 5, 1, 1, "6", "", KeyType::Text, {}},
    {0, 6, 1, 1, "7", "", KeyType::Text, {}},
    {0, 7, 1, 1, "8", "", KeyType::Text, {}},
    {0, 8, 1, 1, "9", "", KeyType::Text, {}},
    {0, 9, 1, 1, "0", "", KeyType::Text, {}},

    // Row 2
    {1, 0, 1, 1, "q", "", KeyType::Text, {}},
    {1, 1, 1, 1, "w", "", KeyType::Text, {}},
    {1, 2, 1, 1, "e", "", KeyType::Text, {}},
    {1, 3, 1, 1, "r", "", KeyType::Text, {}},
    {1, 4, 1, 1, "t", "", KeyType::Text, {}},
    {1, 5, 1, 1, "y", "", KeyType::Text, {}},
    {1, 6, 1, 1, "u", "", KeyType::Text, {}},
    {1, 7, 1, 1, "i", "", KeyType::Text, {}},
    {1, 8, 1, 1, "o", "", KeyType::Text, {}},
    {1, 9, 1, 1, "p", "", KeyType::Text, {}},

    // Row 3
    {2, 0, 1, 1, "a", "", KeyType::Text, {}},
    {2, 1, 1, 1, "s", "", KeyType::Text, {}},
    {2, 2, 1, 1, "d", "", KeyType::Text, {}},
    {2, 3, 1, 1, "f", "", KeyType::Text, {}},
    {2, 4, 1, 1, "g", "", KeyType::Text, {}},
    {2, 5, 1, 1, "h", "", KeyType::Text, {}},
    {2, 6, 1, 1, "j", "", KeyType::Text, {}},
    {2, 7, 1, 1, "k", "", KeyType::Text, {}},
    {2, 8, 1, 1, "l", "", KeyType::Text, {}},
    {2, 9, 1, 1, "-", "", KeyType::Text, {}},

    // Row 4
    {3, 0, 1, 1, "z", "", KeyType::Text, {}},
    {3, 1, 1, 1, "x", "", KeyType::Text, {}},
    {3, 2, 1, 1, "c", "", KeyType::Text, {}},
    {3, 3, 1, 1, "v", "", KeyType::Text, {}},
    {3, 4, 1, 1, "b", "", KeyType::Text, {}},
    {3, 5, 1, 1, "n", "", KeyType::Text, {}},
    {3, 6, 1, 1, "m", "", KeyType::Text, {}},
    {3, 7, 1, 1, "@", "", KeyType::Text, {}},
    {3, 8, 1, 1, ".", "", KeyType::Text, {}},
    {3, 9, 1, 1, "_", "", KeyType::Text, {}},

    // Row 5
    {4, 0, 1, 1, "⇧", "L2", KeyType::Shift, {L2}},
    {4, 1, 1, 1, "@#:", "L2+△", KeyType::Symbols1Layout, {L2, Triangle}},
    {4, 2, 1, 1, "à", "L3", KeyType::UnknownFunction, {L3}},
    {4, 3, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    //{4, 4, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    //{4, 5, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    //{4, 6, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    {4, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 8, 2, 1, "⇦", "□", KeyType::Backspace, {Square}},
    //{4, 8, 2, 1, "⇦", "□", KeyType::Backspace, {Square}},

    // Row 6
    {5, 0, 1, 1, "▲", "", KeyType::CursorUp, {Up}},
    {5, 1, 1, 1, "▼", "", KeyType::CursorDown, {Down}},
    {5, 2, 1, 1, "◀", "L1", KeyType::CursorLeft, {L1}},
    {5, 3, 1, 1, "▶", "R1", KeyType::CursorRight, {R1}},
    {5, 4, 1, 1, "KB", "", KeyType::ToggleKeyboard, {}},
    {5, 5, 1, 1, "...", "", KeyType::MoreOptions, {}},
    {5, 6, 1, 1, "+/⊗", "R3", KeyType::ControllerAction, {R3}},
    {5, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {5, 8, 2, 1, "Done", "R2", KeyType::Done, {R2}},
    //{5, 9, 2, 1, "Done", "R2", KeyType::Done, {R2}},
};
/*
const std::vector<Key> kSymbols1Layout = {
    // Row 1
    {0, 0, 1, 1, "!", "", KeyType::Text, {}},
    {0, 1, 1, 1, "?", "", KeyType::Text, {}},
    {0, 2, 1, 1, "\"", "", KeyType::Text, {}},
    {0, 3, 1, 1, "'", "", KeyType::Text, {}},
    {0, 4, 1, 1, "#", "", KeyType::Text, {}},
    {0, 5, 1, 1, "%", "", KeyType::Text, {}},
    {0, 6, 1, 1, "(", "", KeyType::Text, {}},
    {0, 7, 1, 1, ")", "", KeyType::Text, {}},
    {0, 8, 1, 1, "()", "", KeyType::Text, {}},
    {0, 9, 1, 1, "/", "", KeyType::Text, {}},

    // Row 2
    {1, 0, 1, 1, "-", "", KeyType::Text, {}},
    {1, 1, 1, 1, "_", "", KeyType::Text, {}},
    {1, 2, 1, 1, ",", "", KeyType::Text, {}},
    {1, 3, 1, 1, ".", "", KeyType::Text, {}},
    {1, 4, 1, 1, ":", "", KeyType::Text, {}},
    {1, 5, 1, 1, ";", "", KeyType::Text, {}},
    {1, 6, 1, 1, "*", "", KeyType::Text, {}},
    {1, 7, 1, 1, "+", "", KeyType::Text, {}},
    {1, 8, 1, 1, "=", "", KeyType::Text, {}},
    {1, 9, 1, 1, "&", "", KeyType::Text, {}},

    // Row 3
    {2, 0, 1, 1, "<", "", KeyType::Text, {}},
    {2, 1, 1, 1, ">", "", KeyType::Text, {}},
    {2, 2, 1, 1, "@", "", KeyType::Text, {}},
    {2, 3, 1, 1, "[", "", KeyType::Text, {}},
    {2, 4, 1, 1, "]", "", KeyType::Text, {}},
    {2, 5, 1, 1, "[]", "", KeyType::Text, {}},
    {2, 6, 1, 1, "{", "", KeyType::Text, {}},
    {2, 7, 1, 1, "}", "", KeyType::Text, {}},
    {2, 8, 1, 1, "{}", "", KeyType::Text, {}},
    {2, 9, 1, 2, "→", "", KeyType::UnknownFunction, {}},

    // Row 4
    {3, 0, 1, 1, "\\", "", KeyType::Text, {}},
    {3, 1, 1, 1, "|", "", KeyType::Text, {}},
    {3, 2, 1, 1, "^", "", KeyType::Text, {}},
    {3, 3, 1, 1, "`", "", KeyType::Text, {}},
    {3, 4, 1, 1, "$", "", KeyType::Text, {}},
    {3, 5, 1, 1, "€", "", KeyType::Text, {}},
    {3, 6, 1, 1, "´", "", KeyType::Text, {}},
    {3, 7, 1, 1, "ˊ", "", KeyType::Text, {}},
    {3, 8, 1, 1, "ˊ", "", KeyType::Text, {}},
    //{3, 9, 1, 2, "→", "", KeyType::UnknownFunction,{}},

    // Row 5
    {4, 0, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 1, 1, 1, "ABC", "L2+△", KeyType::TextLayout, {L2, Triangle}},
    {4, 2, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 3, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    //{4, 4, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    //{4, 5, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    //{4, 6, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    {4, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 8, 2, 1, "⇦", "□", KeyType::Backspace, {Square}},
    //{4, 9, 2, 1, "⇦", "□", KeyType::Backspace, {Square}},

    // Row 6
    {5, 0, 1, 1, "▲", "", KeyType::CursorUp, {Up}},
    {5, 1, 1, 1, "▼", "", KeyType::CursorDown, {Down}},
    {5, 2, 1, 1, "◀", "L1", KeyType::CursorLeft, {L1}},
    {5, 3, 1, 1, "▶", "R1", KeyType::CursorRight, {R1}},
    {5, 4, 1, 1, "KB", "", KeyType::ToggleKeyboard, {}},
    {5, 5, 1, 1, "...", "", KeyType::MoreOptions, {}},
    {5, 6, 1, 1, "+/⊗", "R3", KeyType::ControllerAction, {R3}},
    {5, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {5, 8, 2, 1, "Done", "R2", KeyType::Done, {R2}},
    //{5, 9, 2, 1, "Done", "R2", KeyType::Done, {R2}},

};
*/
// From PS5
const std::vector<Key> kSymbols1Layout = {
    // Row 1
    {0, 0, 1, 1, "!", "", KeyType::Text, {}},
    {0, 1, 1, 1, "?", "", KeyType::Text, {}},
    {0, 2, 1, 1, "\"", "", KeyType::Text, {}},
    {0, 3, 1, 1, "'", "", KeyType::Text, {}},
    {0, 4, 1, 1, "#", "", KeyType::Text, {}},
    {0, 5, 1, 1, "%", "", KeyType::Text, {}},
    {0, 6, 1, 1, "(", "", KeyType::Text, {}},
    {0, 7, 1, 1, ")", "", KeyType::Text, {}},
    {0, 8, 1, 1, "()", "", KeyType::Text, {}},
    {0, 9, 1, 1, "/", "", KeyType::Text, {}},

    // Row 2
    {1, 0, 1, 1, "-", "", KeyType::Text, {}},
    {1, 1, 1, 1, "_", "", KeyType::Text, {}},
    {1, 2, 1, 1, ",", "", KeyType::Text, {}},
    {1, 3, 1, 1, ".", "", KeyType::Text, {}},
    {1, 4, 1, 1, ":", "", KeyType::Text, {}},
    {1, 5, 1, 1, ";", "", KeyType::Text, {}},
    {1, 6, 1, 1, "*", "", KeyType::Text, {}},
    {1, 7, 1, 1, "&", "", KeyType::Text, {}},
    {1, 8, 1, 1, "+", "", KeyType::Text, {}},
    {1, 9, 1, 1, "=", "", KeyType::Text, {}},

    // Row 3
    {2, 0, 1, 1, "<", "", KeyType::Text, {}},
    {2, 1, 1, 1, ">", "", KeyType::Text, {}},
    {2, 2, 1, 1, "@", "", KeyType::Text, {}},
    {2, 3, 1, 1, "[", "", KeyType::Text, {}},
    {2, 4, 1, 1, "]", "", KeyType::Text, {}},
    {2, 5, 1, 1, "[]", "", KeyType::Text, {}},
    {2, 6, 1, 1, "{", "", KeyType::Text, {}},
    {2, 7, 1, 1, "}", "", KeyType::Text, {}},
    {2, 8, 1, 1, "{}", "", KeyType::Text, {}},
    {2, 9, 1, 2, "→", "", KeyType::Symbols2Layout, {}},

    // Row 4
    {3, 0, 1, 1, "\\", "", KeyType::Text, {}},
    {3, 1, 1, 1, "|", "", KeyType::Text, {}},
    {3, 2, 1, 1, "^", "", KeyType::Text, {}},
    {3, 3, 1, 1, "`", "", KeyType::Text, {}},
    {3, 4, 1, 1, "$", "", KeyType::Text, {}},
    {3, 5, 1, 1, "€", "", KeyType::Text, {}},
    {3, 6, 1, 1, "£", "", KeyType::Text, {}},
    {3, 7, 1, 1, "¥", "", KeyType::Text, {}},
    {3, 8, 1, 1, "₩", "", KeyType::Text, {}},
    //{3, 9, 1, 2, "→", "", KeyType::Symbols2Layout,{}},

    // Row 5
    {4, 0, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 1, 1, 1, "ABC", "L2+△", KeyType::TextLayout, {L2, Triangle}},
    {4, 2, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 3, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    //{4, 4, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    //{4, 5, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    //{4, 6, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    {4, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 8, 2, 1, "⇦", "□", KeyType::Backspace, {Square}},
    //{4, 9, 2, 1, "⇦", "□", KeyType::Backspace, {Square}},

    // Row 6
    {5, 0, 1, 1, "▲", "", KeyType::CursorUp, {Up}},
    {5, 1, 1, 1, "▼", "", KeyType::CursorDown, {Down}},
    {5, 2, 1, 1, "◀", "L1", KeyType::CursorLeft, {L1}},
    {5, 3, 1, 1, "▶", "R1", KeyType::CursorRight, {R1}},
    {5, 4, 1, 1, "KB", "", KeyType::ToggleKeyboard, {}},
    {5, 5, 1, 1, "...", "", KeyType::MoreOptions, {}},
    {5, 6, 1, 1, "+/⊗", "R3", KeyType::ControllerAction, {R3}},
    {5, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {5, 8, 2, 1, "Done", "R2", KeyType::Done, {R2}},
    //{5, 9, 2, 1, "Done", "R2", KeyType::Done, {R2}},

};

// From PS5
const std::vector<Key> kSymbols2Layout = {
    // Row 1
    {0, 0, 1, 1, "“", "", KeyType::Text, {}},
    {0, 1, 1, 1, "”", "", KeyType::Text, {}},
    {0, 2, 1, 1, "„", "", KeyType::Text, {}},
    {0, 3, 1, 1, "¡", "", KeyType::Text, {}},
    {0, 4, 1, 1, "‼", "", KeyType::Text, {}},
    {0, 5, 1, 1, "¿", "", KeyType::Text, {}},
    {0, 6, 1, 1, "⁇", "", KeyType::Text, {}},
    {0, 7, 1, 1, "~", "", KeyType::Text, {}},
    {0, 8, 1, 1, "·", "", KeyType::Text, {}},
    {0, 9, 1, 1, "", "", KeyType::Disabled, {}},

    // Row 2
    {1, 0, 1, 1, "×", "", KeyType::Text, {}},
    {1, 1, 1, 1, "÷", "", KeyType::Text, {}},
    {1, 2, 1, 1, "‹", "", KeyType::Text, {}},
    {1, 3, 1, 1, "›", "", KeyType::Text, {}},
    {1, 4, 1, 1, "«", "", KeyType::Text, {}},
    {1, 5, 1, 1, "»", "", KeyType::Text, {}},
    {1, 6, 1, 1, "º", "", KeyType::Text, {}},
    {1, 7, 1, 1, "ª", "", KeyType::Text, {}},
    {1, 8, 1, 1, "°", "", KeyType::Text, {}},
    {1, 9, 1, 1, "§", "", KeyType::Text, {}},

    // Row 3
    {2, 0, 1, 2, "←", "", KeyType::Symbols1Layout, {}},
    {2, 1, 1, 1, "¦", "", KeyType::Text, {}},
    {2, 2, 1, 1, "µ", "", KeyType::Text, {}},
    {2, 3, 1, 1, "¬", "", KeyType::Text, {}},
    {2, 4, 1, 1, "¹", "", KeyType::Text, {}},
    {2, 5, 1, 1, "²", "", KeyType::Text, {}},
    {2, 6, 1, 1, "³", "", KeyType::Text, {}},
    {2, 7, 1, 1, "¼", "", KeyType::Text, {}},
    {2, 8, 1, 1, "½", "", KeyType::Text, {}},
    {2, 9, 1, 1, "¾", "", KeyType::Text, {}},

    // Row 4
    //{3, 0, 1, 1, "←", "", KeyType::Symbols1Layout, {}},
    {3, 1, 1, 1, "¢", "", KeyType::Text, {}},
    {3, 2, 1, 1, "¤", "", KeyType::Text, {}},
    {3, 3, 1, 1, "’", "", KeyType::Text, {}},//not sure
    {3, 4, 1, 1, "‘", "", KeyType::Text, {}},//not sure
    {3, 5, 1, 1, "‛", "", KeyType::Text, {}},//not sure
    {3, 6, 1, 1, "‚", "", KeyType::Text, {}},//not sure
    {3, 7, 1, 1, "№", "", KeyType::Text, {}},
    {3, 8, 1, 1, "", "", KeyType::Disabled, {}},
    {3, 9, 1, 1, "", "", KeyType::Disabled,{}},

    // Row 5
    {4, 0, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 1, 1, 1, "ABC", "L2+△", KeyType::TextLayout, {L2, Triangle}},
    {4, 2, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 3, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    //{4, 4, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    //{4, 5, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    //{4, 6, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    {4, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 8, 2, 1, "⇦", "□", KeyType::Backspace, {Square}},
    //{4, 9, 2, 1, "⇦", "□", KeyType::Backspace, {Square}},

    // Row 6
    {5, 0, 1, 1, "▲", "", KeyType::CursorUp, {Up}},
    {5, 1, 1, 1, "▼", "", KeyType::CursorDown, {Down}},
    {5, 2, 1, 1, "◀", "L1", KeyType::CursorLeft, {L1}},
    {5, 3, 1, 1, "▶", "R1", KeyType::CursorRight, {R1}},
    {5, 4, 1, 1, "KB", "", KeyType::ToggleKeyboard, {}},
    {5, 5, 1, 1, "...", "", KeyType::MoreOptions, {}},
    {5, 6, 1, 1, "+/⊗", "R3", KeyType::ControllerAction, {R3}},
    {5, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {5, 8, 2, 1, "Done", "R2", KeyType::Done, {R2}},
    //{5, 9, 2, 1, "Done", "R2", KeyType::Done, {R2}},

};