#pragma once

#include <cstddef>
#include "common/types.h"

namespace Core::Libraries::LibC {

void PS4_SYSV_ABI ps4_exit(int code);
int PS4_SYSV_ABI ps4_atexit(void (*func)());
void* PS4_SYSV_ABI ps4_malloc(size_t size);
void PS4_SYSV_ABI ps4_free(void* ptr);
void PS4_SYSV_ABI ps4_qsort(void* ptr, size_t count, size_t size,
                            int(PS4_SYSV_ABI* comp)(const void*, const void*));
int PS4_SYSV_ABI ps4_rand();

} // namespace Core::Libraries::LibC
