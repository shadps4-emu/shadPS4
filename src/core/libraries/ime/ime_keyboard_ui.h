// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/cstring.h"
#include "common/types.h"
#include "core/libraries/ime/ime_dialog.h"
#include "core/libraries/ime/ime_keyboard_layouts.h"

enum class KeyboardMode { Letters, Symbols };

// Renders the virtual keyboard and modifies buffer if a key is pressed.
// Flags:
// - `input_changed`: set to true if the text buffer changes
// - `done_pressed`: set to true if the Done/Enter key was pressed
void DrawVirtualKeyboard(char* buffer, std::size_t buffer_capacity, bool* input_changed,
                         KeyboardMode& kb_mode, bool& shift_enabled, bool* done_pressed);

// Renders a specific keyboard layout and processes key events.
void RenderKeyboardLayout(const std::vector<Key>& layout, char* buffer, std::size_t buffer_capacity,
                          bool* input_changed, KeyboardMode& kb_mode, bool& shift_enabled,
                          bool* done_pressed);
