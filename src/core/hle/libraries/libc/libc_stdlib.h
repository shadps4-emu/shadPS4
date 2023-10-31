#pragma once

#include <types.h>

#include <cstddef>

namespace Core::Libraries::LibC::stdlib {
void PS4_SYSV_ABI exit(int code);
int PS4_SYSV_ABI atexit(void (*func)());
void* PS4_SYSV_ABI malloc(size_t size);
void PS4_SYSV_ABI free(void* ptr);
}  // namespace Core::Libraries::LibC::stdlib
