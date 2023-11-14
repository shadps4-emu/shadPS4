#include "core/hle/libraries/libc/libc_string.h"

#include <cstdlib>
#include <cstring>
#include "common/log.h"
#include <core/hle/libraries/libs.h>

namespace Core::Libraries::LibC {

int PS4_SYSV_ABI memcmp(const void* s1, const void* s2, size_t n) {
    PRINT_FUNCTION_NAME();
    return std::memcmp(s1, s2, n);
}

void* PS4_SYSV_ABI memcpy(void* dest, const void* src, size_t n) {
    PRINT_FUNCTION_NAME();
    return std::memcpy(dest, src, n);
}

void* PS4_SYSV_ABI memset(void* s, int c, size_t n) {
    PRINT_FUNCTION_NAME();
    return std::memset(s, c, n);
}

int PS4_SYSV_ABI strcmp(const char* str1, const char* str2) {
    PRINT_FUNCTION_NAME();
    return std::strcmp(str1, str2);
}

char* PS4_SYSV_ABI strncpy(char* dest, const char* src, size_t count) {
    PRINT_FUNCTION_NAME();
    return std::strncpy(dest, src, count);
}

void* PS4_SYSV_ABI memmove(void* dest, const void* src, std::size_t count) {
    PRINT_FUNCTION_NAME();
    return std::memmove(dest, src, count);
}

char* PS4_SYSV_ABI strcpy(char* dest, const char* src) {
    PRINT_FUNCTION_NAME();
    return std::strcpy(dest, src);
}

char* PS4_SYSV_ABI strcat(char* dest, const char* src) {
    PRINT_FUNCTION_NAME();
    return std::strcat(dest, src);
}

size_t PS4_SYSV_ABI strlen(const char* str) {
    PRINT_FUNCTION_NAME();
    return std::strlen(str);
}

int PS4_SYSV_ABI strncmp(const char* str1, const char* str2, size_t num) {
    PRINT_FUNCTION_NAME();
    return std::strncmp(str1, str2, num);
}

char* PS4_SYSV_ABI strrchr(char* str, int character) {
    PRINT_FUNCTION_NAME();
    return (char*)std::strrchr(str, character);
}

char* PS4_SYSV_ABI strdup(const char* str1) {
    PRINT_FUNCTION_NAME();
    char* dup = (char*)std::malloc(std::strlen(str1) + 1);
    if (dup != NULL) strcpy(dup, str1);
    return dup;
}

}  // namespace Core::Libraries::LibC
