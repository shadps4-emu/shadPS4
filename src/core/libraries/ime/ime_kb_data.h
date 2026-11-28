// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include "core/libraries/ime/ime_kb_layout.h"

namespace Libraries::Ime {

enum class ImeKbPage : u8 {
    Default,
    Shift,
    Symbols1,
    Symbols2,
    Accent,
    AccentShift,
    Alphabet,
    AlphabetShift,
    FullWidth,
    FullWidthShift,
    FullWidthSymbols,
};

struct ImeKbCharacter {
    const char* text;
    const char* label;
    ImeKbKeyAction action;
};

struct ImeKbPageData {
    std::span<const u16> keys;
    u8 cols;
    u8 rows;
};

const ImeKbPageData* GetImeKeyboardPageData(ImeKbVariant variant, OrbisImeType type,
                                            ImeKbPage page);
const ImeKbCharacter& GetImeKeyboardCharacter(u16 index);

} // namespace Libraries::Ime
