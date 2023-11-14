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
int PS4_SYSV_ABI setvbuf(FILE* stream, char* buffer, int mode, size_t size);
int PS4_SYSV_ABI fseek(FILE* stream, long int offset, int origin);
int PS4_SYSV_ABI fgetpos(FILE* stream, fpos_t* pos);
size_t PS4_SYSV_ABI fread(void* ptr, size_t size, size_t count, FILE* stream);
int PS4_SYSV_ABI fputc(int character, FILE* stream);
long PS4_SYSV_ABI libc_ftell(FILE* stream);
}  // namespace Core::Libraries::LibC
