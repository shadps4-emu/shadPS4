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

const std::vector<Key> kLayoutEnLettersUppercase = {
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
    {4, 2, 1, 1, "à", "L3", KeyType::AccentLettersLayout, {L3}},
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
    {5, 5, 1, 1, "...", "", KeyType::MoreOptions, {}},
    {5, 6, 1, 1, "+/⊗", "R3", KeyType::ControllerAction, {R3}},
    {5, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {5, 8, 2, 1, "Done", "R2", KeyType::Done, {R2}},
    //{5, 9, 2, 1, "Done", "R2", KeyType::Done,{R2}},
};

const std::vector<Key> kLayoutEnLettersLowercase = {
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
    {4, 2, 1, 1, "à", "L3", KeyType::AccentLettersLayout, {L3}},
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
// From PS5
const std::vector<Key> kLayoutEnSymbols1 = {
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
    {4, 1, 1, 1, "ABC", "L2+△", KeyType::LettersLayout, {L2, Triangle}},
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
const std::vector<Key> kLayoutEnSymbols2 = {
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
    {3, 3, 1, 1, "’", "", KeyType::Text, {}}, // not sure
    {3, 4, 1, 1, "‘", "", KeyType::Text, {}}, // not sure
    {3, 5, 1, 1, "‛", "", KeyType::Text, {}}, // not sure
    {3, 6, 1, 1, "‚", "", KeyType::Text, {}}, // not sure
    {3, 7, 1, 1, "№", "", KeyType::Text, {}},
    {3, 8, 1, 1, "", "", KeyType::Disabled, {}},
    {3, 9, 1, 1, "", "", KeyType::Disabled, {}},

    // Row 5
    {4, 0, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 1, 1, 1, "ABC", "L2+△", KeyType::LettersLayout, {L2, Triangle}},
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

const std::vector<Key> kLayoutEnAccentLettersUppercase = {
    // Row 0
    {0, 0, 1, 1, "À", "", KeyType::Text, {}},
    {0, 1, 1, 1, "Á", "", KeyType::Text, {}},
    {0, 2, 1, 1, "Â", "", KeyType::Text, {}},
    {0, 3, 1, 1, "Ã", "", KeyType::Text, {}},
    {0, 4, 1, 1, "Ä", "", KeyType::Text, {}},
    {0, 5, 1, 1, "Å", "", KeyType::Text, {}},
    {0, 6, 1, 1, "Ą", "", KeyType::Text, {}},
    {0, 7, 1, 1, "Æ", "", KeyType::Text, {}},
    {0, 8, 1, 1, "Ç", "", KeyType::Text, {}},
    {0, 9, 1, 1, "Ć", "", KeyType::Text, {}},

    // Row 1
    {1, 0, 1, 1, "È", "", KeyType::Text, {}},
    {1, 1, 1, 1, "É", "", KeyType::Text, {}},
    {1, 2, 1, 1, "Ê", "", KeyType::Text, {}},
    {1, 3, 1, 1, "Ë", "", KeyType::Text, {}},
    {1, 4, 1, 1, "Ę", "", KeyType::Text, {}},
    {1, 5, 1, 1, "Ğ", "", KeyType::Text, {}},
    {1, 6, 1, 1, "Ì", "", KeyType::Text, {}},
    {1, 7, 1, 1, "Í", "", KeyType::Text, {}},
    {1, 8, 1, 1, "Î", "", KeyType::Text, {}},
    {1, 9, 1, 1, "Ï", "", KeyType::Text, {}},

    // Row 2
    {2, 0, 1, 1, "İ", "", KeyType::Text, {}},
    {2, 1, 1, 1, "Ł", "", KeyType::Text, {}},
    {2, 2, 1, 1, "Ñ", "", KeyType::Text, {}},
    {2, 3, 1, 1, "Ń", "", KeyType::Text, {}},
    {2, 4, 1, 1, "Ò", "", KeyType::Text, {}},
    {2, 5, 1, 1, "Ó", "", KeyType::Text, {}},
    {2, 6, 1, 1, "Ô", "", KeyType::Text, {}},
    {2, 7, 1, 1, "Õ", "", KeyType::Text, {}},
    {2, 8, 1, 1, "Ö", "", KeyType::Text, {}},
    {2, 9, 1, 1, "Ø", "", KeyType::Text, {}},

    // Row 3
    {3, 0, 1, 1, "Œ", "", KeyType::Text, {}},
    {3, 1, 1, 1, "Ś", "", KeyType::Text, {}},
    {3, 2, 1, 1, "Ş", "", KeyType::Text, {}},
    {3, 3, 1, 1, "Š", "", KeyType::Text, {}},
    {3, 4, 1, 1, "ß", "", KeyType::Text, {}},
    {3, 5, 1, 1, "Ù", "", KeyType::Text, {}},
    {3, 6, 1, 1, "Ú", "", KeyType::Text, {}},
    {3, 7, 1, 1, "Û", "", KeyType::Text, {}},
    {3, 8, 1, 1, "Ü", "", KeyType::Text, {}},
    {3, 9, 1, 1, "Ý", "", KeyType::Text, {}},

    // Row 4
    {4, 0, 1, 1, "Ÿ", "", KeyType::Text, {}},
    {4, 1, 1, 1, "Ź", "", KeyType::Text, {}},
    {4, 2, 1, 1, "Ż", "", KeyType::Text, {}},
    {4, 3, 1, 1, "Ž", "", KeyType::Text, {}},
    {4, 4, 1, 1, "Ð", "", KeyType::Text, {}},
    {4, 5, 1, 1, "Þ", "", KeyType::Text, {}},
    {4, 6, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 8, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 9, 1, 1, "", "", KeyType::Disabled, {}},

    // Row 5
    {5, 0, 1, 1, "⬆", "L2", KeyType::Shift, {L2}},
    {5, 1, 1, 1, "@#:", "L2+△", KeyType::Symbols1Layout, {L3, Triangle}},
    {5, 2, 1, 1, "à", "L3", KeyType::AccentLettersLayout, {L3}},
    {5, 3, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    //{4, 4, 4, 1, "Space", "△", KeyType::Space,{Triangle}},
    //{4, 5, 4, 1, "Space", "△", KeyType::Space,{Triangle}},
    //{4, 6, 4, 1, "Space", "△", KeyType::Space,{Triangle}},
    {4, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {5, 8, 2, 1, "⇦", "□", KeyType::Backspace, {Square}},
    //{4, 9, 2, 1, "⇦", "□", KeyType::Backspace,{Square}},

    // Row 6
    {6, 0, 1, 1, "▲", "", KeyType::CursorUp, {Up}},
    {6, 1, 1, 1, "▼", "", KeyType::CursorDown, {Down}},
    {6, 2, 1, 1, "◀", "L1", KeyType::CursorLeft, {L1}},
    {6, 3, 1, 1, "▶", "R1", KeyType::CursorRight, {R1}},
    {6, 4, 1, 1, "KB", "", KeyType::ToggleKeyboard, {}},
    {6, 5, 1, 1, "...", "", KeyType::MoreOptions, {}},
    {6, 6, 1, 1, "+/⊗", "R3", KeyType::ControllerAction, {R3}},
    {5, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {6, 8, 2, 1, "Done", "R2", KeyType::Done, {R2}},
    //{5, 9, 2, 1, "Done", "R2", KeyType::Done,{R2}},
};

const std::vector<Key> kLayoutEnAccentLettersLowercase = {
    // Row 0
    {0, 0, 1, 1, "à", "", KeyType::Text, {}},
    {0, 1, 1, 1, "á", "", KeyType::Text, {}},
    {0, 2, 1, 1, "â", "", KeyType::Text, {}},
    {0, 3, 1, 1, "ã", "", KeyType::Text, {}},
    {0, 4, 1, 1, "ä", "", KeyType::Text, {}},
    {0, 5, 1, 1, "å", "", KeyType::Text, {}},
    {0, 6, 1, 1, "ą", "", KeyType::Text, {}},
    {0, 7, 1, 1, "æ", "", KeyType::Text, {}},
    {0, 8, 1, 1, "ç", "", KeyType::Text, {}},
    {0, 9, 1, 1, "ć", "", KeyType::Text, {}},

    // Row 1
    {1, 0, 1, 1, "è", "", KeyType::Text, {}},
    {1, 1, 1, 1, "é", "", KeyType::Text, {}},
    {1, 2, 1, 1, "ê", "", KeyType::Text, {}},
    {1, 3, 1, 1, "ë", "", KeyType::Text, {}},
    {1, 4, 1, 1, "ę", "", KeyType::Text, {}},
    {1, 5, 1, 1, "ğ", "", KeyType::Text, {}},
    {1, 6, 1, 1, "ì", "", KeyType::Text, {}},
    {1, 7, 1, 1, "í", "", KeyType::Text, {}},
    {1, 8, 1, 1, "î", "", KeyType::Text, {}},
    {1, 9, 1, 1, "ï", "", KeyType::Text, {}},

    // Row 2
    {2, 0, 1, 1, "ı", "", KeyType::Text, {}},
    {2, 1, 1, 1, "ł", "", KeyType::Text, {}},
    {2, 2, 1, 1, "ñ", "", KeyType::Text, {}},
    {2, 3, 1, 1, "ń", "", KeyType::Text, {}},
    {2, 4, 1, 1, "ò", "", KeyType::Text, {}},
    {2, 5, 1, 1, "ó", "", KeyType::Text, {}},
    {2, 6, 1, 1, "ô", "", KeyType::Text, {}},
    {2, 7, 1, 1, "õ", "", KeyType::Text, {}},
    {2, 8, 1, 1, "ö", "", KeyType::Text, {}},
    {2, 9, 1, 1, "ø", "", KeyType::Text, {}},

    // Row 3
    {3, 0, 1, 1, "œ", "", KeyType::Text, {}},
    {3, 1, 1, 1, "ś", "", KeyType::Text, {}},
    {3, 2, 1, 1, "ş", "", KeyType::Text, {}},
    {3, 3, 1, 1, "š", "", KeyType::Text, {}},
    {3, 4, 1, 1, "ß", "", KeyType::Text, {}},
    {3, 5, 1, 1, "ù", "", KeyType::Text, {}},
    {3, 6, 1, 1, "ú", "", KeyType::Text, {}},
    {3, 7, 1, 1, "û", "", KeyType::Text, {}},
    {3, 8, 1, 1, "ü", "", KeyType::Text, {}},
    {3, 9, 1, 1, "ý", "", KeyType::Text, {}},

    // Row 4
    {4, 0, 1, 1, "ÿ", "", KeyType::Text, {}},
    {4, 1, 1, 1, "ź", "", KeyType::Text, {}},
    {4, 2, 1, 1, "ż", "", KeyType::Text, {}},
    {4, 3, 1, 1, "ž", "", KeyType::Text, {}},
    {4, 4, 1, 1, "ð", "", KeyType::Text, {}},
    {4, 5, 1, 1, "þ", "", KeyType::Text, {}},
    {4, 6, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 8, 1, 1, "", "", KeyType::Disabled, {}},
    {4, 9, 1, 1, "", "", KeyType::Disabled, {}},

    // Row 5
    {5, 0, 1, 1, "⬆", "L2", KeyType::Shift, {L2}},
    {5, 1, 1, 1, "@#:", "L2+△", KeyType::Symbols1Layout, {L3, Triangle}},
    {5, 2, 1, 1, "à", "L3", KeyType::AccentLettersLayout, {L3}},
    {5, 3, 4, 1, "Space", "△", KeyType::Space, {Triangle}},
    //{4, 4, 4, 1, "Space", "△", KeyType::Space,{Triangle}},
    //{4, 5, 4, 1, "Space", "△", KeyType::Space,{Triangle}},
    //{4, 6, 4, 1, "Space", "△", KeyType::Space,{Triangle}},
    {4, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {5, 8, 2, 1, "⇦", "□", KeyType::Backspace, {Square}},
    //{4, 9, 2, 1, "⇦", "□", KeyType::Backspace,{Square}},

    // Row 6
    {6, 0, 1, 1, "▲", "", KeyType::CursorUp, {Up}},
    {6, 1, 1, 1, "▼", "", KeyType::CursorDown, {Down}},
    {6, 2, 1, 1, "◀", "L1", KeyType::CursorLeft, {L1}},
    {6, 3, 1, 1, "▶", "R1", KeyType::CursorRight, {R1}},
    {6, 4, 1, 1, "KB", "", KeyType::ToggleKeyboard, {}},
    {6, 5, 1, 1, "...", "", KeyType::MoreOptions, {}},
    //{5, 5, 1, 1, "…", "", KeyType::MoreOptions,{}},
    {6, 6, 1, 1, "+/⊗", "R3", KeyType::ControllerAction, {R3}},
    {5, 7, 1, 1, "", "", KeyType::Disabled, {}},
    {6, 8, 2, 1, "Done", "R2", KeyType::Done, {R2}},
    //{5, 9, 2, 1, "Done", "R2", KeyType::Done,{R2}},
};