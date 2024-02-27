// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/hle/libraries/libc/printf.h"

namespace Core::Libraries::LibC {

std::FILE* PS4_SYSV_ABI ps4_fopen(const char* filename, const char* mode);
int PS4_SYSV_ABI ps4_printf(VA_ARGS);
int PS4_SYSV_ABI ps4_vsnprintf(char* s, size_t n, const char* format, VaList* arg);
int PS4_SYSV_ABI ps4_puts(const char* s);
int PS4_SYSV_ABI ps4_fprintf(FILE* file, VA_ARGS);

} // namespace Core::Libraries::LibC
