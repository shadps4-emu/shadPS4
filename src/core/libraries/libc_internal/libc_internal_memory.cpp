// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/linker.h"
#include "libc_internal_memory.h"

namespace Libraries::LibcInternal {

void* PS4_SYSV_ABI internal_memset(void* s, int c, size_t n) {
    return std::memset(s, c, n);
}

void* PS4_SYSV_ABI internal_memcpy(void* dest, const void* src, size_t n) {
    return std::memcpy(dest, src, n);
}

s32 PS4_SYSV_ABI internal_memcpy_s(void* dest, size_t destsz, const void* src, size_t count) {
#ifdef _WIN64
    return memcpy_s(dest, destsz, src, count);
#else
    std::memcpy(dest, src, count);
    return 0; // ALL OK
#endif
}

s32 PS4_SYSV_ABI internal_memcmp(const void* s1, const void* s2, size_t n) {
    return std::memcmp(s1, s2, n);
}

void* PS4_SYSV_ABI internal_malloc(u64 size) {
    // The guest may install its own heap via _sceKernelRtldSetApplicationHeapAPI when it has
    // allocations must go through it so that the matching free() sees memory it owns
    const auto heap_api = Common::Singleton<Core::Linker>::Instance()->HeapAPI();
    if (heap_api && heap_api->heap_malloc) {
        return heap_api->heap_malloc(size);
    }
    LOG_ERROR(Lib_LibcInternal, "(PARTIAL) called, no application heap installed");
    return std::malloc(size);
}

void PS4_SYSV_ABI internal_free(void* ptr) {
    if (ptr == nullptr) {
        return;
    }
    const auto heap_api = Common::Singleton<Core::Linker>::Instance()->HeapAPI();
    if (heap_api && heap_api->heap_free) {
        heap_api->heap_free(ptr);
        return;
    }
    LOG_ERROR(Lib_LibcInternal, "(PARTIAL) called, no application heap installed");
    std::free(ptr);
}

void RegisterlibSceLibcInternalMemory(Core::Loader::SymbolsResolver* sym) {

    LIB_FUNCTION("NFLs+dRJGNg", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memcpy_s);
    LIB_FUNCTION("Q3VBxCXhUHs", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memcpy);
    LIB_FUNCTION("8zTFvBIAIN8", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memset);
    LIB_FUNCTION("DfivPArhucg", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memcmp);
    LIB_FUNCTION("gQX+4GDQjpM", "libSceLibcInternal", 1, "libSceLibcInternal", internal_malloc);
    LIB_FUNCTION("tIhsqj0qsFE", "libSceLibcInternal", 1, "libSceLibcInternal", internal_free);
}

} // namespace Libraries::LibcInternal
