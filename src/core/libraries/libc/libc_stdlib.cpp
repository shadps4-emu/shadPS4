// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include "common/assert.h"
#include "core/libraries/libc/libc_stdlib.h"

namespace Libraries::LibC {

void PS4_SYSV_ABI ps4_exit(int code) {
    std::exit(code);
}

int PS4_SYSV_ABI ps4_atexit(void (*func)()) {
    int rt = std::atexit(func);
    ASSERT_MSG(rt == 0, "atexit returned {}", rt);
    return rt;
}

void* PS4_SYSV_ABI ps4_malloc(size_t size) {
    return std::malloc(size);
}

void PS4_SYSV_ABI ps4_free(void* ptr) {
    std::free(ptr);
}

typedef int(PS4_SYSV_ABI* pfunc_QsortCmp)(const void*, const void*);
thread_local static pfunc_QsortCmp compair_ps4;

int qsort_compair(const void* arg1, const void* arg2) {
    return compair_ps4(arg1, arg2);
}

void PS4_SYSV_ABI ps4_qsort(void* ptr, size_t count, size_t size,
                            int(PS4_SYSV_ABI* comp)(const void*, const void*)) {
    compair_ps4 = comp;
    std::qsort(ptr, count, size, qsort_compair);
}

int PS4_SYSV_ABI ps4_rand() {
    return std::rand();
}

} // namespace Libraries::LibC
