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
int PS4_SYSV_ABI internal_snprintf(char* s, size_t n, VA_ARGS) {
    VA_CTX(ctx);
    return snprintf_ctx(s, n, &ctx);
}

static OrbisFILE* PS4_SYSV_ABI fopen(const char* path, const char* modes) {
    LOG_INFO(Lib_LibcInternal, "fopen path: {}, modes: {}", path, modes);

    s32 flags = 0;
    u16 mode = 0;

    if (!modes || !path)
        return nullptr;

    if (modes[0] == 'r') {
        flags = Libraries::Kernel::ORBIS_KERNEL_O_RDONLY;
        mode = 0555;
    } else if (modes[0] == 'w') {
        flags = Libraries::Kernel::ORBIS_KERNEL_O_WRONLY | Libraries::Kernel::ORBIS_KERNEL_O_CREAT |
                Libraries::Kernel::ORBIS_KERNEL_O_TRUNC;
        mode = 0666;
    } else if (modes[0] == 'a') {
        flags = Libraries::Kernel::ORBIS_KERNEL_O_WRONLY | Libraries::Kernel::ORBIS_KERNEL_O_CREAT |
                Libraries::Kernel::ORBIS_KERNEL_O_APPEND;
        mode = 0666;
    } else {
        return nullptr;
    }

    s32 fd = Libraries::Kernel::open(path, flags, mode);
    if (fd < 0)
        return nullptr;

    auto* f = new OrbisFILE{};
    f->fd = fd;
    return f;
}

int PS4_SYSV_ABI fseek(OrbisFILE* stream, long off, int whence) {
    if (!stream)
        return -1;

    s64 r = Libraries::Kernel::posix_lseek(stream->fd, off, whence);
    return (r < 0) ? -1 : 0;
}

size_t PS4_SYSV_ABI fread(void* ptr, size_t size, size_t nmemb, OrbisFILE* stream) {
    if (!stream || !ptr || size == 0 || nmemb == 0)
        return 0;

    size_t total = size * nmemb;
    s64 r = Libraries::Kernel::read(stream->fd, ptr, total);

    if (r < 0)
        return 0;

    return static_cast<size_t>(r) / size;
}

int PS4_SYSV_ABI fclose(OrbisFILE* stream) {
    if (!stream)
        return -1;

    Libraries::Kernel::close(stream->fd);
    delete stream;
    return 0;
}

void RegisterlibSceLibcInternalIo(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("eLdDw6l0-bU", "libSceLibcInternal", 1, "libSceLibcInternal", internal_snprintf);
    LIB_FUNCTION("xeYO4u7uyJ0", "libSceLibcInternal", 1, "libSceLibcInternal", fopen);
    LIB_FUNCTION("rQFVBXp-Cxg", "libSceLibcInternal", 1, "libSceLibcInternal", fseek);
    LIB_FUNCTION("lbB+UlZqVG0", "libSceLibcInternal", 1, "libSceLibcInternal", fread);
    LIB_FUNCTION("uodLYyUip20", "libSceLibcInternal", 1, "libSceLibcInternal", fclose);
}
} // namespace Libraries::LibcInternal