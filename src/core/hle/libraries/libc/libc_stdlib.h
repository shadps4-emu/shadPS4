#pragma once

#include <cstddef>
#include "common/types.h"

namespace Core::Libraries::LibC {

void PS4_SYSV_ABI exit(int code);
int PS4_SYSV_ABI atexit(void (*func)());
void* PS4_SYSV_ABI malloc(size_t size);
void PS4_SYSV_ABI free(void* ptr);
void PS4_SYSV_ABI qsort(void* ptr, size_t count, size_t size, int(PS4_SYSV_ABI* comp)(const void*, const void*));
int PS4_SYSV_ABI rand();
unsigned long int PS4_SYSV_ABI _Stoul(const char* str, char** endptr, int base);
void PS4_SYSV_ABI srand(unsigned int seed);
s64 PS4_SYSV_ABI time(s64* pt);
} // namespace Core::Libraries::LibC
