#include "libc.h"

#include <debug.h>

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace Core::Libraries::LibC {

PS4_SYSV_ABI int printf(VA_ARGS) {
    VA_CTX(ctx);
    return printf_ctx(&ctx);
}

int PS4_SYSV_ABI vsnprintf(char* s, size_t n, const char* format, VaList* arg) { return vsnprintf_ctx(s, n, format, arg); }

PS4_SYSV_ABI void exit(int code) { std::exit(code); }

PS4_SYSV_ABI int atexit(void (*func)()) {
    int rt = std::atexit(func);
    if (rt != 0) {
        BREAKPOINT();
    }
    return rt;
}

int PS4_SYSV_ABI memcmp(const void* s1, const void* s2, size_t n) { return std::memcmp(s1, s2, n); }

void* PS4_SYSV_ABI memcpy(void* dest, const void* src, size_t n) { return std::memcpy(dest, src, n); }

void* PS4_SYSV_ABI memset(void* s, int c, size_t n) { return std::memset(s, c, n); }

void* PS4_SYSV_ABI malloc(size_t size) { return std::malloc(size); }

void PS4_SYSV_ABI free(void* ptr) { std::free(ptr); }

int PS4_SYSV_ABI strcmp(const char* str1, const char* str2) { return std::strcmp(str1, str2); }

size_t PS4_SYSV_ABI strlen(const char* str) { return std::strlen(str); }

char* PS4_SYSV_ABI strncpy(char* dest, const char* src, size_t count) { return std::strncpy(dest, src, count); }

void* PS4_SYSV_ABI memmove(void* dest, const void* src, std::size_t count) { return std::memmove(dest, src, count); }

char* PS4_SYSV_ABI strcpy(char* dest, const char* src) { return std::strcpy(dest, src); }

char* PS4_SYSV_ABI strcat(char* dest, const char* src) { return std::strcat(dest, src); }

// math
float PS4_SYSV_ABI atan2f(float y, float x) { return std::atan2f(y, x); }

float PS4_SYSV_ABI acosf(float num) { return std::acosf(num); }

float PS4_SYSV_ABI tanf(float num) { return std::tanf(num); }

float PS4_SYSV_ABI asinf(float num) { return std::asinf(num); }

double PS4_SYSV_ABI pow(double base, double exponent) { return std::pow(base, exponent); }

double PS4_SYSV_ABI _Sin(double x) { return std::sin(x); }

};  // namespace Core::Libraries::LibC