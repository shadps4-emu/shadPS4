// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include "common/types.h"

namespace Libraries::LibC {

int PS4_SYSV_ABI ps4_memcmp(const void* s1, const void* s2, size_t n);
void* PS4_SYSV_ABI ps4_memcpy(void* dest, const void* src, size_t n);
void* PS4_SYSV_ABI ps4_memset(void* s, int c, size_t n);
int PS4_SYSV_ABI ps4_strcmp(const char* str1, const char* str2);
char* PS4_SYSV_ABI ps4_strncpy(char* dest, const char* src, size_t count);
void* PS4_SYSV_ABI ps4_memmove(void* dest, const void* src, std::size_t count);
char* PS4_SYSV_ABI ps4_strcpy(char* destination, const char* source);
char* PS4_SYSV_ABI ps4_strcat(char* dest, const char* src);
size_t PS4_SYSV_ABI ps4_strlen(const char* str);
char* PS4_SYSV_ABI ps4_strrchr(const char* s, int c);
int PS4_SYSV_ABI ps4_strncmp(const char* s1, const char* s2, size_t n);
char* PS4_SYSV_ABI ps4_strdup(const char* str1);

} // namespace Libraries::LibC
