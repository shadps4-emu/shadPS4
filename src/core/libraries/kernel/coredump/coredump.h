// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

typedef s32 PS4_SYSV_ABI (*OrbisCoredumpHandler)(void* common);

struct OrbisCoredumpStopInfoCpu {
    void* thread;
    s32 reason_code;
};

struct OrbisCoredumpStopInfoGpu {
    u64 timestamp;
};

struct OrbisCoredumpThreadContextInfo {
    u64 rdi;
    u64 rsi;
    u64 rdx;
    u64 rcx;
    u64 r8;
    u64 r9;
    u64 rax;
    u64 rbx;
    u64 rbp;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;
    u64 rip;
    u64 rflags;
    u64 rsp;
};

void RegisterCoredump(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel