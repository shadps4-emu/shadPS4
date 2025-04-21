// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>
#include <imgui.h>
#include "common/types.h"

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
    ImGuiKey bound_buttons[2];
    bool allow_repeat{false};
};

int c16rtomb(char* out, char16_t ch);

extern const std::vector<KeyEntry> kLayoutEnLettersUppercase;
extern const std::vector<KeyEntry> kLayoutEnLettersLowercase;
extern const std::vector<KeyEntry> kLayoutEnAccentLettersUppercase;
extern const std::vector<KeyEntry> kLayoutEnAccentLettersLowercase;
extern const std::vector<KeyEntry> kLayoutEnSymbols1;
extern const std::vector<KeyEntry> kLayoutEnSymbols2;

constexpr ImGuiKey None = ImGuiKey::ImGuiKey_None;
constexpr ImGuiKey L1 = ImGuiKey::ImGuiKey_GamepadL1;
constexpr ImGuiKey R1 = ImGuiKey::ImGuiKey_GamepadR1;
constexpr ImGuiKey L2 = ImGuiKey::ImGuiKey_GamepadL2;
constexpr ImGuiKey R2 = ImGuiKey::ImGuiKey_GamepadR2;
constexpr ImGuiKey L3 = ImGuiKey::ImGuiKey_GamepadL3;
constexpr ImGuiKey R3 = ImGuiKey::ImGuiKey_GamepadR3;
constexpr ImGuiKey Up = ImGuiKey::ImGuiKey_GamepadDpadUp;
constexpr ImGuiKey Down = ImGuiKey::ImGuiKey_GamepadDpadDown;
constexpr ImGuiKey Left = ImGuiKey::ImGuiKey_GamepadDpadLeft;
constexpr ImGuiKey Right = ImGuiKey::ImGuiKey_GamepadDpadRight;
constexpr ImGuiKey Cross = ImGuiKey::ImGuiKey_GamepadFaceDown; // X button
constexpr ImGuiKey Circle = ImGuiKey::ImGuiKey_GamepadFaceRight; // O button
constexpr ImGuiKey Square = ImGuiKey::ImGuiKey_GamepadFaceLeft;  // [] button
constexpr ImGuiKey Triangle = ImGuiKey::ImGuiKey_GamepadFaceUp;  // /\ button
constexpr ImGuiKey Options = ImGuiKey::ImGuiKey_GraveAccent;   // Options button


// Fake function keycodes
constexpr u16 KC_SYM1 = 0xF100;
constexpr u16 KC_SYM2 = 0xF101;
constexpr u16 KC_ACCENTS = 0xF102;
constexpr u16 KC_LETTERS = 0xF103;
constexpr u16 KC_KB = 0xF104;
constexpr u16 KC_GYRO = 0xF105;
constexpr u16 KC_OPT = 0xF106;