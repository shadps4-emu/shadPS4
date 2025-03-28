#pragma once

#include "common/cstring.h"
#include "common/types.h"

enum class KeyboardMode { Letters, Symbols };

void DrawVirtualKeyboard(char* buffer, std::size_t buffer_capacity, bool* input_changed,
                         KeyboardMode& kb_mode, bool& shift_enabled);
