// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/backend.h"

#if defined(ARCHITECTURE_x86_64)
#define Crash() __asm__ __volatile__("int $3")
#elif defined(ARCHITECTURE_arm64)
#define Crash() __asm__ __volatile__("brk #0")
#else
#define Crash() exit(1)
#endif

void assert_fail_impl() {
    Common::Log::Stop();
    Crash();
}

[[noreturn]] void unreachable_impl() {
    Common::Log::Stop();
    Crash();
    throw std::runtime_error("Unreachable code");
}
