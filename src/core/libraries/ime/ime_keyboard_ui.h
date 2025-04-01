// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "ime_keyboard_layouts.h"

enum class KeyboardMode { Letters, Symbols };

void DrawVirtualKeyboard(char* buffer, std::size_t buffer_capacity, bool* input_changed,
                         KeyboardMode& kb_mode, bool& shift_enabled, bool* done_pressed);

void RenderKeyboardLayout(const std::vector<Key>& layout, char* buffer, std::size_t buffer_capacity,
                          bool* input_changed, KeyboardMode& kb_mode, bool& shift_enabled,
                          bool* done_pressed,
                          Libraries::Pad::OrbisPadButtonDataOffset current_pad_button);
