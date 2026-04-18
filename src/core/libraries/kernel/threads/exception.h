// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

using OrbisKernelExceptionHandler = PS4_SYSV_ABI void (*)(int, void*);
struct OrbisKernelExceptionHandlerStack {
    void* ss_sp;
    int ss_flags;
    size_t ss_size;
};

constexpr s32 POSIX_SIGHUP = 1;
constexpr s32 POSIX_SIGINT = 2;
constexpr s32 POSIX_SIGQUIT = 3;
constexpr s32 POSIX_SIGILL = 4;
constexpr s32 POSIX_SIGTRAP = 5;
constexpr s32 POSIX_SIGABRT = 6;
constexpr s32 POSIX_SIGEMT = 7;
constexpr s32 POSIX_SIGFPE = 8;
constexpr s32 POSIX_SIGKILL = 9;
constexpr s32 POSIX_SIGBUS = 10;
constexpr s32 POSIX_SIGSEGV = 11;
constexpr s32 POSIX_SIGSYS = 12;
constexpr s32 POSIX_SIGPIPE = 13;
constexpr s32 POSIX_SIGALRM = 14;
constexpr s32 POSIX_SIGTERM = 15;
constexpr s32 POSIX_SIGURG = 16;
constexpr s32 POSIX_SIGSTOP = 17;
constexpr s32 POSIX_SIGTSTP = 18;
constexpr s32 POSIX_SIGCONT = 19;
constexpr s32 POSIX_SIGCHLD = 20;
constexpr s32 POSIX_SIGTTIN = 21;
constexpr s32 POSIX_SIGTTOU = 22;
constexpr s32 POSIX_SIGIO = 23;
constexpr s32 POSIX_SIGXCPU = 24;
constexpr s32 POSIX_SIGXFSZ = 25;
constexpr s32 POSIX_SIGVTALRM = 26;
constexpr s32 POSIX_SIGPROF = 27;
constexpr s32 POSIX_SIGWINCH = 28;
constexpr s32 POSIX_SIGINFO = 29;
constexpr s32 POSIX_SIGUSR1 = 30;
constexpr s32 POSIX_SIGUSR2 = 31;
constexpr s32 POSIX_SIGTHR = 32;
constexpr s32 POSIX_SIGLIBRT = 33;

#if defined(__linux__) || defined(__FreeBSD__)
constexpr s32 _SIGEMT = 128;
constexpr s32 _SIGINFO = 129;
#elif !defined(_WIN32)
constexpr s32 _SIGEMT = SIGEMT;
constexpr s32 _SIGINFO = SIGINFO;
#endif

constexpr s32 POSIX_SA_NOCLDSTOP = 1;
constexpr s32 POSIX_SA_NOCLDWAIT = 2;
constexpr s32 POSIX_SA_SIGINFO = 4;
constexpr s32 POSIX_SA_ONSTACK = 0x08000000;
constexpr s32 POSIX_SA_RESTART = 0x10000000;
constexpr s32 POSIX_SA_NODEFER = 0x40000000;
constexpr s32 POSIX_SA_RESETHAND = 0x80000000;
constexpr s32 POSIX_SA_RESTORER = 0x04000000;

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

union Sigval {
    /* Members as suggested by Annex C of POSIX 1003.1b. */
    int sival_int;
    void* sival_ptr;
    /* 6.0 compatibility */
    int sigval_int;
    void* sigval_ptr;
};

struct Siginfo {
    int _si_signo; /* signal number */
    int _si_errno; /* errno association */
    /*
     * Cause of signal, one of the SI_ macros or signal-specific
     * values, i.e. one of the FPE_... values for SIGFPE.  This
     * value is equivalent to the second argument to an old-style
     * FreeBSD signal handler.
     */
    int _si_code;           /* signal code */
    s32 _si_pid;            /* sending process */
    u32 _si_uid;            /* sender's ruid */
    int _si_status;         /* exit value */
    void* _si_addr;         /* faulting instruction */
    union Sigval _si_value; /* signal value */
    union {
        struct {
            int _trapno; /* machine specific trap code */
        } _fault;
        struct {
            int _timerid;
            int _overrun;
        } _timer;
        struct {
            int _mqd;
        } _mesgq;
        struct {
            long _band; /* band event for SIGPOLL */
        } _poll;        /* was this ever used ? */
        struct {
            long __spare1__;
            int __spare2__[7];
        } __spare__;
    } _reason;
};

struct Sigaction {
    union {
        void (*handler)(int);
        void (*sigaction)(int, struct Siginfo*, void*);
    } __sigaction_handler;
    int sa_flags;
    Sigset sa_mask;
};

struct Ucontext {
    struct Sigset uc_sigmask;
    int field1_0x10[12];
    Mcontext uc_mcontext;
    Ucontext* uc_link;
    ExStack uc_stack;
    int uc_flags;
    int __spare[4];
    int field7_0x4f4[3];
};

s32 NativeToOrbisSignal(s32 s);
s32 OrbisToNativeSignal(s32 s);

void RegisterException(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
