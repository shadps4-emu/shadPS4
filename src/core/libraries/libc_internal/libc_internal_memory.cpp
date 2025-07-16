// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
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

void* PS4_SYSV_ABI internal_malloc(size_t size) {
    return std::malloc(size);
}

void PS4_SYSV_ABI internal_free(void* ptr) {
    std::free(ptr);
}

void* PS4_SYSV_ABI internal_operator_new(size_t size) {
    if (size == 0) {
        // Size of 1 is used if 0 is provided.
        size = 1;
    }
    void* ptr = std::malloc(size);
    ASSERT_MSG(ptr, "Failed to allocate new object with size {}", size);
    return ptr;
}

void PS4_SYSV_ABI internal_operator_delete(void* ptr) {
    if (ptr) {
        std::free(ptr);
    }
}

int PS4_SYSV_ABI internal_posix_memalign(void** ptr, size_t alignment, size_t size) {
#ifdef _WIN64
    void* allocated = _aligned_malloc(size, alignment);
    if (!allocated) {
        return errno;
    }
    *ptr = allocated;
    return 0;
#else
    return posix_memalign(ptr, alignment, size);
#endif
}

void* PS4_SYSV_ABI internal_calloc(size_t num, size_t size) {
    return std::calloc(num, size);
}

void* PS4_SYSV_ABI internal_realloc(void* ptr, size_t new_size) {
    return std::realloc(ptr, new_size);
}

void RegisterlibSceLibcInternalMemory(Core::Loader::SymbolsResolver* sym) {

    LIB_FUNCTION("NFLs+dRJGNg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memcpy_s);
    LIB_FUNCTION("Q3VBxCXhUHs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memcpy);
    LIB_FUNCTION("8zTFvBIAIN8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memset);
    LIB_FUNCTION("DfivPArhucg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memcmp);
    LIB_FUNCTION("gQX+4GDQjpM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_malloc);
    LIB_FUNCTION("tIhsqj0qsFE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_free);
    LIB_FUNCTION("fJnpuVVBbKk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_operator_new);
    LIB_FUNCTION("hdm0YfMa7TQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_operator_new);
    LIB_FUNCTION("MLWl90SFWNE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_operator_delete);
    LIB_FUNCTION("z+P+xCnWLBk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_operator_delete);
    LIB_FUNCTION("cVSk9y8URbc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_memalign);
}

} // namespace Libraries::LibcInternal
