// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "libc_internal.h"

namespace Libraries::LibcInternal {

void* PS4_SYSV_ABI internal_memset(void* s, int c, size_t n) {
    return std::memset(s, c, n);
}

void* PS4_SYSV_ABI internal_memcpy(void* dest, const void* src, size_t n) {
    return std::memcpy(dest, src, n);
}

int PS4_SYSV_ABI internal_memcpy_s(void* dest, size_t destsz, const void* src, size_t count) {
#ifdef _WIN64
    return memcpy_s(dest, destsz, src, count);
#else
    std::memcpy(dest, src, count);
    return 0; // ALL OK
#endif
}

int PS4_SYSV_ABI internal_strcpy_s(char* dest, size_t dest_size, const char* src) {
#ifdef _WIN64
    return strcpy_s(dest, dest_size, src);
#else
    std::strcpy(dest, src);
    return 0; // ALL OK
#endif
}

int PS4_SYSV_ABI internal_memcmp(const void* s1, const void* s2, size_t n) {
    return std::memcmp(s1, s2, n);
}

int PS4_SYSV_ABI internal_strncmp(const char* str1, const char* str2, size_t num) {
    return std::strncmp(str1, str2, num);
}

int PS4_SYSV_ABI internal_strlen(const char* str) {
    return std::strlen(str);
}

float PS4_SYSV_ABI internal_expf(float x) {
    return expf(x);
}

void* PS4_SYSV_ABI internal_malloc(size_t size) {
    return std::malloc(size);
}

char* PS4_SYSV_ABI internal_strncpy(char* dest, const char* src, std::size_t count) {
    return std::strncpy(dest, src, count);
}

void RegisterlibSceLibcInternal(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("NFLs+dRJGNg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memcpy_s);
    LIB_FUNCTION("Q3VBxCXhUHs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memcpy);
    LIB_FUNCTION("8zTFvBIAIN8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memset);
    LIB_FUNCTION("5Xa2ACNECdo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strcpy_s);
    LIB_FUNCTION("DfivPArhucg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memcmp);
    LIB_FUNCTION("8zsu04XNsZ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_expf);
    LIB_FUNCTION("aesyjrHVWy4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strncmp);
    LIB_FUNCTION("j4ViWNHEgww", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strlen);
    LIB_FUNCTION("6sJWiWSRuqk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strncpy);
    LIB_FUNCTION("gQX+4GDQjpM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_malloc);
};

} // namespace Libraries::LibcInternal
