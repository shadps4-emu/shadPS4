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
char* PS4_SYSV_ABI strcpy(char* destination, const char* source);
char* PS4_SYSV_ABI strcat(char* dest, const char* src);
float PS4_SYSV_ABI atan2f(float y, float x);
float PS4_SYSV_ABI acosf(float num);
float PS4_SYSV_ABI tanf(float num);
float PS4_SYSV_ABI asinf(float num);

}  // namespace Emulator::HLE::Libraries::LibC