// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdarg>
#include <cstdio>

#include <common/va_ctx.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/libs.h"
#include "libc_internal_io.h"
#include "printf.h"

namespace Libraries::LibcInternal {

s32 PS4_SYSV_ABI internal_snprintf(char* s, u64 n, VA_ARGS) {
    VA_CTX(ctx);
    return snprintf_ctx(s, n, &ctx);
}

std::vector<OrbisFILE*> g_files{};
// Constants for tracking accurate file indexes.
// Since the file struct is exposed to the application, accuracy is important.
static constexpr s32 g_initial_files = 5;
static constexpr s32 g_max_files = 0x100 - g_initial_files;

OrbisFILE* PS4_SYSV_ABI internal__Fofind() {
    u64 index = 0;
    while (index != g_max_files) {
        OrbisFILE* file = g_files.at(index);
        // If file doesn't exist, create it.
        if (file == nullptr) {
            file = new OrbisFILE();
            if (file == nullptr) {
                return nullptr;
            }
            // Store new file in the array, initialize default values, and return it.
            g_files.at(index) = file;
            file->_Mode = 0x80;
            file->_Idx = index + g_initial_files;
            return file;
        }
        // Special case, files with mode 0 are returned?
        if (file->_Mode == 0) {
            file->_Mode = 0xff7f;
            return file;
        }
        index++;
    }
    return nullptr;
}

OrbisFILE* PS4_SYSV_ABI internal__Foprep(const char* path, const char* mode, OrbisFILE* file,
                                         s32 fd, s32 flag1, s32 flag2) {
    if (file == nullptr) {
        *Kernel::__Error() = POSIX_ENOMEM;
    }
    file->_Handle = -1;

    // TODO: The rest of this.

    return file;
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

void RegisterlibSceLibcInternalIo(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("eLdDw6l0-bU", "libSceLibcInternal", 1, "libSceLibcInternal", internal_snprintf);
    LIB_FUNCTION("xGT4Mc55ViQ", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Fofind);
    LIB_FUNCTION("dREVnZkAKRE", "libSceLibcInternal", 1, "libSceLibcInternal", internal__Foprep);
}

void ForceRegisterlibSceLibcInternalIo(Core::Loader::SymbolsResolver* sym) {
    // Goal is to be minimally intrusive here to allow LLE for printf/stdout writes.
    LIB_FUNCTION("xeYO4u7uyJ0", "libSceLibcInternal", 1, "libSceLibcInternal", internal_fopen);
    LIB_FUNCTION("rQFVBXp-Cxg", "libSceLibcInternal", 1, "libSceLibcInternal", internal_fseek);
    LIB_FUNCTION("lbB+UlZqVG0", "libSceLibcInternal", 1, "libSceLibcInternal", internal_fread);
    LIB_FUNCTION("uodLYyUip20", "libSceLibcInternal", 1, "libSceLibcInternal", internal_fclose);
}

} // namespace Libraries::LibcInternal