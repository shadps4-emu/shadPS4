#include "libc.h"

#include <debug.h>

#include <cstdlib>
#include <cstring>

namespace Core::Libraries::LibC {

void PS4_SYSV_ABI exit(int code) { std::exit(code); }

int PS4_SYSV_ABI atexit(void (*func)()) {
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

};  // namespace Core::Libraries::LibC