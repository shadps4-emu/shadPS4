// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <ctime>

#include "core/libraries/libs.h"
#include "libc_internal_time.h"

namespace Libraries::LibcInternal {

s64 PS4_SYSV_ABI internal_time(s64* tloc) {
    const std::time_t result = std::time(nullptr);
    if (tloc != nullptr) {
        *tloc = static_cast<s64>(result);
    }
    return static_cast<s64>(result);
}

// NOTE: gmtime/gmtime_s/localtime/mktime are intentionally not implemented here. They all
// operate on a guest `struct tm`, and this codebase has no confirmed guest-facing definition of
// that layout to reuse (searched for OrbisTm / tm_sec / tm_year usage under
// src/core/libraries/, including the rtc library, and found none). Guessing the field order/
// padding would silently corrupt guest memory, so these remain unresolved (stub returns NULL/0)
// until the layout can be confirmed from an actual PS4 SDK header.

void RegisterlibSceLibcInternalTime(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("wLlFkwG9UcQ", "libSceLibcInternal", 1, "libSceLibcInternal", internal_time);
}

} // namespace Libraries::LibcInternal
