// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>
#include <imgui.h>
#include "common/types.h"

/*
enum class OrbisImeKeyEntryType : u8 {
    ORBIS_IME_KEY_TYPE_CHARACTER = 0,
    ORBIS_IME_KEY_TYPE_FUNCTION = 1,
    ORBIS_IME_KEY_TYPE_DISABLED = 2
};
*/
/*
struct OrbisImeKeyEntry {
    u16 keycode;
    char16_t character;
    OrbisImeKeyEntryType type;
    u8 row;
    u8 col;
    u8 colspan;
    u8 rowspan;
    const char* label;
    const char* controller_hint;
    OrbisPadButtonDataOffset bound_buttons[2];
};
*/

enum class KeyType : u8 { Character = 0, Function = 1, Disabled = 2 };

struct KeyEntry {
    u16 keycode; // 0xF100+ unused, so can be used as temporary defined keys for unknown
    char16_t character;
    KeyType type;
    u8 row;
    u8 col;
    u8 colspan;
    u8 rowspan;
    const char* label;
    const char* controller_hint;
    ImGuiNavInput bound_buttons[2];
    bool allow_repeat{false};
};

int c16rtomb(char* out, char16_t ch);

extern const std::vector<KeyEntry> kLayoutEnLettersUppercase;
extern const std::vector<KeyEntry> kLayoutEnLettersLowercase;
extern const std::vector<KeyEntry> kLayoutEnAccentLettersUppercase;
extern const std::vector<KeyEntry> kLayoutEnAccentLettersLowercase;
extern const std::vector<KeyEntry> kLayoutEnSymbols1;
extern const std::vector<KeyEntry> kLayoutEnSymbols2;

constexpr ImGuiNavInput None = ImGuiNavInput_COUNT;
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

// Fake function keycodes
constexpr u16 KC_SYM1 = 0xF100;
constexpr u16 KC_SYM2 = 0xF101;
constexpr u16 KC_ACCENTS = 0xF102;
constexpr u16 KC_LETTERS = 0xF103;
constexpr u16 KC_KB = 0xF104;
constexpr u16 KC_GYRO = 0xF105;
constexpr u16 KC_OPT = 0xF106;