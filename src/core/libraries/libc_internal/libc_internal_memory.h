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

s32 PS4_SYSV_ABI internal_memcpy_s(void* dest, size_t destsz, const void* src, size_t count);

s32 PS4_SYSV_ABI internal_memcmp(const void* s1, const void* s2, size_t n);

void* PS4_SYSV_ABI internal_malloc(size_t size);

void PS4_SYSV_ABI internal_free(void* ptr);

void* PS4_SYSV_ABI internal_operator_new(size_t size);

void PS4_SYSV_ABI internal_operator_delete(void* ptr);

int PS4_SYSV_ABI internal_posix_memalign(void** ptr, size_t alignment, size_t size);

void RegisterlibSceLibcInternalMemory(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::LibcInternal