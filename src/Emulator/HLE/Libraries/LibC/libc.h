#pragma once

#include <types.h>

#include "printf.h"

namespace Emulator::HLE::Libraries::LibC {

// HLE functions
PS4_SYSV_ABI int printf(VA_ARGS);
int PS4_SYSV_ABI vsnprintf(char* s, size_t n, const char* format, VaList* arg);
PS4_SYSV_ABI void exit(int code);
PS4_SYSV_ABI int atexit(void (*func)());
int PS4_SYSV_ABI memcmp(const void* s1, const void* s2, size_t n);
void* PS4_SYSV_ABI memcpy(void* dest, const void* src, size_t n);
void* PS4_SYSV_ABI memset(void* s, int c, size_t n);
void* PS4_SYSV_ABI malloc(size_t size);
void PS4_SYSV_ABI free(void* ptr);
int PS4_SYSV_ABI strcmp(const char* str1, const char* str2);
size_t PS4_SYSV_ABI strlen(const char* str);
char* PS4_SYSV_ABI strncpy(char* dest, const char* src, size_t count);
void* PS4_SYSV_ABI memmove(void* dest, const void* src, std::size_t count);
}  // namespace Emulator::HLE::Libraries::LibC