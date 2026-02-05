// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdarg>
#include <cstdio>

#include <common/va_ctx.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/libs.h"
#include "libc_internal_io.h"
#include "printf.h"

namespace Libraries::LibcInternal {

s32 PS4_SYSV_ABI internal_snprintf(char* s, size_t n, VA_ARGS) {
    VA_CTX(ctx);
    return snprintf_ctx(s, n, &ctx);
}

void RegisterlibSceLibcInternalIo(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("eLdDw6l0-bU", "libSceLibcInternal", 1, "libSceLibcInternal", internal_snprintf);
}

OrbisFILE* PS4_SYSV_ABI internal_fopen(const char* path, const char* mode) {
    std::scoped_lock lk{g_stream_mtx};

    

    return nullptr;
}

s32 PS4_SYSV_ABI internal_fseek(OrbisFILE* stream, s64 offset, s32 whence) {
    return 0;
}

u64 PS4_SYSV_ABI internal_fread(void* ptr, u64 size, u64 nmemb, OrbisFILE* stream) {
    return 0;
}

s32 PS4_SYSV_ABI internal_fclose(OrbisFILE* stream) {
    return 0;
}

void ForceRegisterlibSceLibcInternalIo(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("xeYO4u7uyJ0", "libSceLibcInternal", 1, "libSceLibcInternal", internal_fopen);
    LIB_FUNCTION("rQFVBXp-Cxg", "libSceLibcInternal", 1, "libSceLibcInternal", internal_fseek);
    LIB_FUNCTION("lbB+UlZqVG0", "libSceLibcInternal", 1, "libSceLibcInternal", internal_fread);
    LIB_FUNCTION("uodLYyUip20", "libSceLibcInternal", 1, "libSceLibcInternal", internal_fclose);
}

} // namespace Libraries::LibcInternal