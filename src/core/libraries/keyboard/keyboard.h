// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Keyboard {

static constexpr s32 ORBIS_KEYBOARD_ERROR_INVALID_ARG = 0x80DA0001;
static constexpr s32 ORBIS_KEYBOARD_ERROR_INVALID_PORT = 0x80DA0002;
static constexpr s32 ORBIS_KEYBOARD_ERROR_INVALID_HANDLE = 0x80DA0003;
static constexpr s32 ORBIS_KEYBOARD_ERROR_ALREADY_OPENED = 0x80DA0004;
static constexpr s32 ORBIS_KEYBOARD_ERROR_NOT_INITIALIZED = 0x80DA0005;
static constexpr s32 ORBIS_KEYBOARD_ERROR_FATAL = 0x80DA00FF;

struct OrbisKeyboardData {
    s64 timestamp; // microseconds
    u8 padding[8];
    u32 unk1; // always 1
    u32 nkeys;
    u32 locks; // num lock, caps lock, scroll lock
    u32 mods;  // ctrl, shift, alt, meta
    u16 keycodes[32];
};

struct OrbisKeyboardKey2Char {
    s32 ok;
    s32 ok2; // wtf it is
    s32 keycode;
    char unk[8]; // zeros
};

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Keyboard