#pragma once

#include <types.h>

#include "printf.h"

namespace Core::Libraries::LibC {
int PS4_SYSV_ABI printf(VA_ARGS);
int PS4_SYSV_ABI vsnprintf(char* s, size_t n, const char* format, VaList* arg);
int PS4_SYSV_ABI puts(const char* s);
int PS4_SYSV_ABI fprintf(FILE* file, VA_ARGS);
}  // namespace Core::Libraries::LibC