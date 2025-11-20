// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdarg>
#include <cstdio>

#include <common/va_ctx.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "libc_internal_io.h"
#include "printf.h"

namespace Libraries::LibcInternal {
int PS4_SYSV_ABI internal_snprintf(char* s, size_t n, VA_ARGS) {
    VA_CTX(ctx);
    return snprintf_ctx(s, n, &ctx);
}
void RegisterlibSceLibcInternalIo(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("eLdDw6l0-bU", "libSceLibcInternal", 1, "libSceLibcInternal", internal_snprintf);
}
} // namespace Libraries::LibcInternal