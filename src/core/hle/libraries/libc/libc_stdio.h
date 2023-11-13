#pragma once

#include "common/types.h"
#include "core/hle/libraries/libc/printf.h"

namespace Core::Libraries::LibC {

int PS4_SYSV_ABI printf(VA_ARGS);
int PS4_SYSV_ABI vsnprintf(char* s, size_t n, const char* format, VaList* arg);
int PS4_SYSV_ABI puts(const char* s);
int PS4_SYSV_ABI fprintf(FILE* file, VA_ARGS);
int PS4_SYSV_ABI snprintf(char* s, size_t n, VA_ARGS);
int PS4_SYSV_ABI sprintf(char* s, VA_ARGS);
FILE* PS4_SYSV_ABI fopen(const char* filename, const char* mode);
int PS4_SYSV_ABI fclose(FILE* stream);
}  // namespace Core::Libraries::LibC
