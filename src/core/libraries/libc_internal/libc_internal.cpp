// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/va_ctx.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "libc_internal.h"
#include "libc_internal_io.h"
#include "libc_internal_math.h"
#include "libc_internal_memory.h"
#include "libc_internal_str.h"
#include "printf.h"

namespace Libraries::LibcInternal {

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    RegisterlibSceLibcInternalMath(sym);
    RegisterlibSceLibcInternalStr(sym);
    RegisterlibSceLibcInternalMemory(sym);
    RegisterlibSceLibcInternalIo(sym);
}
} // namespace Libraries::LibcInternal