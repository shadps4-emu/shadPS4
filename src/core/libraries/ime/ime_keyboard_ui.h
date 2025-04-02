// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "core/libraries/pad/pad.h"
#include "ime_keyboard_layouts.h"
#include "core/libraries/ime/ime.h"
#include "core/libraries/ime/ime_error.h"
#include "core/libraries/ime/ime_ui.h"
#include "core/libraries/ime/ime_common.h"

enum class KeyboardMode { Letters1, Letters2, Symbols1, Symbols2 };

void DrawVirtualKeyboard(char* buffer, std::size_t buffer_capacity, bool* input_changed,
                         KeyboardMode& kb_mode, bool& shift_enabled, bool* done_pressed);


void RenderKeyboardLayout(const std::vector<Key>& layout, char* buffer, std::size_t buffer_capacity,
                          bool* input_changed, KeyboardMode& kb_mode, bool& shift_enabled,
                          bool* done_pressed);