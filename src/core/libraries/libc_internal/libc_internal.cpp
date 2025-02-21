// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// #include <cmath>
// #include <csetjmp>
// #include <string>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "libc_internal.h"
#include "libc_internal_math.h"
#include "libc_internal_memory.h"
#include "libc_internal_str.h"

namespace Libraries::LibcInternal {

void RegisterlibSceLibcInternal(Core::Loader::SymbolsResolver* sym) {
    RegisterlibSceLibcInternalMath(sym);
    RegisterlibSceLibcInternalStr(sym);
    RegisterlibSceLibcInternalMemory(sym);
}
} // namespace Libraries::LibcInternal