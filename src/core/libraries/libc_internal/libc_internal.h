// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::LibcInternal {
void* PS4_SYSV_ABI internal_memset(void* s, int c, size_t n);
void* PS4_SYSV_ABI internal_memcpy(void* dest, const void* src, size_t n);
int PS4_SYSV_ABI internal_memcpy_s(void* dest, size_t destsz, const void* src, size_t count);
int PS4_SYSV_ABI internal_strcpy_s(char* dest, size_t dest_size, const char* src);
int PS4_SYSV_ABI internal_memcmp(const void* s1, const void* s2, size_t n);
float PS4_SYSV_ABI internal_expf(float x);

void RegisterlibSceLibcInternal(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::LibcInternal