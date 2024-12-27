// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/spin_lock.h"

#if _MSC_VER
#include <intrin.h>
#if _M_AMD64
#define __x86_64__ 1
#endif
#if _M_ARM64
#define __aarch64__ 1
#endif
#else
#if __x86_64__
#include <xmmintrin.h>
#endif
#endif

namespace {

void ThreadPause() {
#if __x86_64__
    _mm_pause();
#elif __aarch64__ && _MSC_VER
    __yield();
#elif __aarch64__
    asm("yield");
#endif
}

} // Anonymous namespace

namespace Common {

void SpinLock::lock() {
    while (lck.test_and_set(std::memory_order_acquire)) {
        ThreadPause();
    }
}

void SpinLock::unlock() {
    lck.clear(std::memory_order_release);
}

bool SpinLock::try_lock() {
    if (lck.test_and_set(std::memory_order_acquire)) {
        return false;
    }
    return true;
}

} // namespace Common
