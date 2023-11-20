#pragma once

#include "common/types.h"
#include "core/hle/libraries/libc/printf.h"

namespace Core::Libraries::LibC {

int PS4_SYSV_ABI ps4_printf(VA_ARGS);
int PS4_SYSV_ABI ps4_vsnprintf(char* s, size_t n, const char* format, VaList* arg);
int PS4_SYSV_ABI ps4_puts(const char* s);
int PS4_SYSV_ABI ps4_fprintf(FILE* file, VA_ARGS);
FILE* PS4_SYSV_ABI ps4_fopen(const char* filename, const char* mode);
int PS4_SYSV_ABI ps4_fseek(FILE* stream, long int offset, int origin);
long PS4_SYSV_ABI ps4_ftell(FILE* stream);
size_t PS4_SYSV_ABI ps4_fread(void* ptr, size_t size, size_t count, FILE* stream);
int PS4_SYSV_ABI ps4_fclose(FILE* stream);

}  // namespace Core::Libraries::LibC
