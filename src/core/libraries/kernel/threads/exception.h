// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

using SceKernelExceptionHandler = PS4_SYSV_ABI void (*)(int, void*);

constexpr int POSIX_SIGSEGV = 11;
constexpr int POSIX_SIGUSR1 = 30;

struct Mcontext {
    u64 mc_onstack;
    u64 mc_rdi;
    u64 mc_rsi;
    u64 mc_rdx;
    u64 mc_rcx;
    u64 mc_r8;
    u64 mc_r9;
    u64 mc_rax;
    u64 mc_rbx;
    u64 mc_rbp;
    u64 mc_r10;
    u64 mc_r11;
    u64 mc_r12;
    u64 mc_r13;
    u64 mc_r14;
    u64 mc_r15;
    int mc_trapno;
    u16 mc_fs;
    u16 mc_gs;
    u64 mc_addr;
    int mc_flags;
    u16 mc_es;
    u16 mc_ds;
    u64 mc_err;
    u64 mc_rip;
    u64 mc_cs;
    u64 mc_rflags;
    u64 mc_rsp;
    u64 mc_ss;
    u64 mc_len;
    u64 mc_fpformat;
    u64 mc_ownedfp;
    u64 mc_lbrfrom;
    u64 mc_lbrto;
    u64 mc_aux1;
    u64 mc_aux2;
    u64 mc_fpstate[104];
    u64 mc_fsbase;
    u64 mc_gsbase;
    u64 mc_spare[6];
};

struct ExStack {
    void* ss_sp;
    std::size_t ss_size;
    int ss_flags;
    int _align;
};

struct Sigset {
    u64 bits[2];
};

struct Ucontext {
    struct Sigset uc_sigmask;
    int field1_0x10[12];
    struct Mcontext uc_mcontext;
    struct Ucontext* uc_link;
    struct ExStack uc_stack;
    int uc_flags;
    int __spare[4];
    int field7_0x4f4[3];
};

void RegisterException(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
