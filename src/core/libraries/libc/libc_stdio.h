// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/libc/printf.h"

namespace Libraries::LibC {

std::FILE* PS4_SYSV_ABI ps4_fopen(const char* filename, const char* mode);
int PS4_SYSV_ABI ps4_printf(VA_ARGS);
int PS4_SYSV_ABI ps4_vsnprintf(char* s, size_t n, const char* format, VaList* arg);
int PS4_SYSV_ABI ps4_puts(const char* s);
int PS4_SYSV_ABI ps4_fprintf(FILE* file, VA_ARGS);
int PS4_SYSV_ABI ps4_setvbuf(FILE* stream, char* buf, int mode, size_t size);
int PS4_SYSV_ABI ps4_fclose(FILE* stream);
int PS4_SYSV_ABI ps4_fseek(FILE* stream, long offset, int whence);
int PS4_SYSV_ABI ps4_fgetpos(FILE* stream, fpos_t* pos);
std::size_t PS4_SYSV_ABI ps4_fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
long PS4_SYSV_ABI ps4_ftell(FILE* stream);

} // namespace Libraries::LibC
