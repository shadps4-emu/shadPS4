#pragma once

#include <types.h>

#include "printf.h"

namespace Emulator::HLE::Libraries::LibC {

// HLE functions
PS4_SYSV_ABI int printf(VA_ARGS);
PS4_SYSV_ABI void exit(int code);
PS4_SYSV_ABI int atexit(void (*func)());
int PS4_SYSV_ABI memcmp(const void* s1, const void* s2, size_t n);
void* PS4_SYSV_ABI memcpy(void* dest, const void* src, size_t n);
void* PS4_SYSV_ABI memset(void* s, int c, size_t n);
void* PS4_SYSV_ABI malloc(size_t size);
int PS4_SYSV_ABI strcmp(const char* str1, const char* str2);
size_t PS4_SYSV_ABI strlen(const char* str);
}  // namespace Emulator::HLE::Libraries::LibC