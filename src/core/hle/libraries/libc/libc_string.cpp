#include "core/hle/libraries/libc/libc_string.h"

#include <cstring>
#include <common/log.h>

namespace Core::Libraries::LibC {

constexpr bool log_file_libc = true;  // disable it to disable logging

int PS4_SYSV_ABI memcmp(const void* s1, const void* s2, size_t n) { return std::memcmp(s1, s2, n); }

void* PS4_SYSV_ABI memcpy(void* dest, const void* src, size_t n) { return std::memcpy(dest, src, n); }

void* PS4_SYSV_ABI memset(void* s, int c, size_t n) { return std::memset(s, c, n); }

int PS4_SYSV_ABI strcmp(const char* str1, const char* str2) { return std::strcmp(str1, str2); }

char* PS4_SYSV_ABI strncpy(char* dest, const char* src, size_t count) { return std::strncpy(dest, src, count); }

void* PS4_SYSV_ABI memmove(void* dest, const void* src, std::size_t count) { return std::memmove(dest, src, count); }

char* PS4_SYSV_ABI strcpy(char* dest, const char* src) { 
    LOG_TRACE_IF(log_file_libc, "strcpy dest ={} src ={}", dest, src);
    return std::strcpy(dest, src); 
}

char* PS4_SYSV_ABI strcat(char* dest, const char* src) { 
    LOG_TRACE_IF(log_file_libc, "strcat dest ={} src ={}", dest, src);
    return std::strcat(dest, src); 
}

size_t PS4_SYSV_ABI strlen(const char* str) { return std::strlen(str); }
char* PS4_SYSV_ABI strstr(const char* haystack, const char* needle) {
    LOG_TRACE_IF(log_file_libc, "strstr haystack ={} needle ={}", haystack, needle);
    return (char*)std::strstr(haystack, needle);
}
}  // namespace Core::Libraries::LibC
