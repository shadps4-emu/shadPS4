// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>

#include "common/assert.h"
#include "common/logging/log.h"
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

int PS4_SYSV_ABI internal_strcat_s(char* dest, size_t dest_size, const char* src) {
#ifdef _WIN64
    return strcat_s(dest, dest_size, src);
#else
    std::strcat(dest, src);
    return 0; // ALL OK
#endif
}

int PS4_SYSV_ABI internal_memcmp(const void* s1, const void* s2, size_t n) {
    return std::memcmp(s1, s2, n);
}

int PS4_SYSV_ABI internal_strcmp(const char* str1, const char* str2) {
    return std::strcmp(str1, str2);
}

int PS4_SYSV_ABI internal_strncmp(const char* str1, const char* str2, size_t num) {
    return std::strncmp(str1, str2, num);
}

size_t PS4_SYSV_ABI internal_strlen(const char* str) {
    return std::strlen(str);
}

char* PS4_SYSV_ABI internal_strncpy(char* dest, const char* src, std::size_t count) {
    return std::strncpy(dest, src, count);
}

char* PS4_SYSV_ABI internal_strcat(char* dest, const char* src) {
    return std::strcat(dest, src);
}

const char* PS4_SYSV_ABI internal_strchr(const char* str, int c) {
    return std::strchr(str, c);
}

double PS4_SYSV_ABI internal_sin(double x) {
    return std::sin(x);
}

float PS4_SYSV_ABI internal_sinf(float x) {
    return std::sinf(x);
}

double PS4_SYSV_ABI internal_cos(double x) {
    return std::cos(x);
}

float PS4_SYSV_ABI internal_cosf(float x) {
    return std::cosf(x);
}

void PS4_SYSV_ABI internal_sincos(double x, double* sinp, double* cosp) {
    *sinp = std::sin(x);
    *cosp = std::cos(x);
}

void PS4_SYSV_ABI internal_sincosf(float x, float* sinp, float* cosp) {
    *sinp = std::sinf(x);
    *cosp = std::cosf(x);
}

double PS4_SYSV_ABI internal_tan(double x) {
    return std::tan(x);
}

float PS4_SYSV_ABI internal_tanf(float x) {
    return std::tanf(x);
}

double PS4_SYSV_ABI internal_asin(double x) {
    return std::asin(x);
}

float PS4_SYSV_ABI internal_asinf(float x) {
    return std::asinf(x);
}

double PS4_SYSV_ABI internal_acos(double x) {
    return std::acos(x);
}

float PS4_SYSV_ABI internal_acosf(float x) {
    return std::acosf(x);
}

double PS4_SYSV_ABI internal_atan(double x) {
    return std::atan(x);
}

float PS4_SYSV_ABI internal_atanf(float x) {
    return std::atanf(x);
}

double PS4_SYSV_ABI internal_atan2(double y, double x) {
    return std::atan2(y, x);
}

float PS4_SYSV_ABI internal_atan2f(float y, float x) {
    return std::atan2f(y, x);
}

double PS4_SYSV_ABI internal_exp(double x) {
    return std::exp(x);
}

float PS4_SYSV_ABI internal_expf(float x) {
    return std::expf(x);
}

double PS4_SYSV_ABI internal_exp2(double x) {
    return std::exp2(x);
}

float PS4_SYSV_ABI internal_exp2f(float x) {
    return std::exp2f(x);
}

double PS4_SYSV_ABI internal_pow(double x, double y) {
    return std::pow(x, y);
}

float PS4_SYSV_ABI internal_powf(float x, float y) {
    return std::powf(x, y);
}

double PS4_SYSV_ABI internal_log(double x) {
    return std::log(x);
}

float PS4_SYSV_ABI internal_logf(float x) {
    return std::logf(x);
}

double PS4_SYSV_ABI internal_log10(double x) {
    return std::log10(x);
}

float PS4_SYSV_ABI internal_log10f(float x) {
    return std::log10f(x);
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

void RegisterlibSceLibcInternal(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("NFLs+dRJGNg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memcpy_s);
    LIB_FUNCTION("Q3VBxCXhUHs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memcpy);
    LIB_FUNCTION("8zTFvBIAIN8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memset);
    LIB_FUNCTION("5Xa2ACNECdo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strcpy_s);
    LIB_FUNCTION("K+gcnFFJKVc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strcat_s);
    LIB_FUNCTION("DfivPArhucg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memcmp);
    LIB_FUNCTION("aesyjrHVWy4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strcmp);
    LIB_FUNCTION("Ovb2dSJOAuE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strncmp);
    LIB_FUNCTION("j4ViWNHEgww", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strlen);
    LIB_FUNCTION("6sJWiWSRuqk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strncpy);
    LIB_FUNCTION("Ls4tzzhimqQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strcat);
    LIB_FUNCTION("ob5xAW4ln-0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strchr);
    LIB_FUNCTION("H8ya2H00jbI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_sin);
    LIB_FUNCTION("Q4rRL34CEeE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_sinf);
    LIB_FUNCTION("2WE3BTYVwKM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_cos);
    LIB_FUNCTION("-P6FNMzk2Kc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_cosf);
    LIB_FUNCTION("jMB7EFyu30Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sincos);
    LIB_FUNCTION("pztV4AF18iI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sincosf);
    LIB_FUNCTION("T7uyNqP7vQA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_tan);
    LIB_FUNCTION("ZE6RNL+eLbk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_tanf);
    LIB_FUNCTION("7Ly52zaL44Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_asin);
    LIB_FUNCTION("GZWjF-YIFFk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_asinf);
    LIB_FUNCTION("JBcgYuW8lPU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_acos);
    LIB_FUNCTION("QI-x0SL8jhw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_acosf);
    LIB_FUNCTION("OXmauLdQ8kY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_atan);
    LIB_FUNCTION("weDug8QD-lE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_atanf);
    LIB_FUNCTION("HUbZmOnT-Dg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_atan2);
    LIB_FUNCTION("EH-x713A99c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_atan2f);
    LIB_FUNCTION("NVadfnzQhHQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_exp);
    LIB_FUNCTION("8zsu04XNsZ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_expf);
    LIB_FUNCTION("dnaeGXbjP6E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_exp2);
    LIB_FUNCTION("wuAQt-j+p4o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_exp2f);
    LIB_FUNCTION("9LCjpWyQ5Zc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_pow);
    LIB_FUNCTION("1D0H2KNjshE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_powf);
    LIB_FUNCTION("rtV7-jWC6Yg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_log);
    LIB_FUNCTION("RQXLbdT2lc4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_logf);
    LIB_FUNCTION("WuMbPBKN1TU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_log10);
    LIB_FUNCTION("lhpd6Wk6ccs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_log10f);
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
};

} // namespace Libraries::LibcInternal
