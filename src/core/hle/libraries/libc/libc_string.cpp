#include <cstring>
#include "core/hle/libraries/libc/libc_string.h"

namespace Core::Libraries::LibC {

int PS4_SYSV_ABI ps4_memcmp(const void* s1, const void* s2, size_t n) {
    return std::memcmp(s1, s2, n);
}

void* PS4_SYSV_ABI ps4_memcpy(void* dest, const void* src, size_t n) {
    return std::memcpy(dest, src, n);
}

void* PS4_SYSV_ABI ps4_memset(void* s, int c, size_t n) {
    return std::memset(s, c, n);
}

int PS4_SYSV_ABI ps4_strcmp(const char* str1, const char* str2) {
    return std::strcmp(str1, str2);
}

char* PS4_SYSV_ABI ps4_strncpy(char* dest, const char* src, size_t count) {
    return std::strncpy(dest, src, count);
}

void* PS4_SYSV_ABI ps4_memmove(void* dest, const void* src, std::size_t count) {
    return std::memmove(dest, src, count);
}

char* PS4_SYSV_ABI ps4_strcpy(char* dest, const char* src) {
    return std::strcpy(dest, src);
}

char* PS4_SYSV_ABI ps4_strcat(char* dest, const char* src) {
    return std::strcat(dest, src);
}

size_t PS4_SYSV_ABI ps4_strlen(const char* str) {
    return std::strlen(str);
}

} // namespace Core::Libraries::LibC
