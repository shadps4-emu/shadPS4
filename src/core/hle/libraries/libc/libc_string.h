#pragma once

#include <types.h>
#include <cstddef>

namespace Core::Libraries::LibC::string {
int PS4_SYSV_ABI memcmp(const void* s1, const void* s2, size_t n);
void* PS4_SYSV_ABI memcpy(void* dest, const void* src, size_t n);
void* PS4_SYSV_ABI memset(void* s, int c, size_t n);
int PS4_SYSV_ABI strcmp(const char* str1, const char* str2);
char* PS4_SYSV_ABI strncpy(char* dest, const char* src, size_t count);
void* PS4_SYSV_ABI memmove(void* dest, const void* src, std::size_t count);
char* PS4_SYSV_ABI strcpy(char* destination, const char* source);
char* PS4_SYSV_ABI strcat(char* dest, const char* src);
size_t PS4_SYSV_ABI strlen(const char* str);
}  // namespace Core::Libraries::LibC::string