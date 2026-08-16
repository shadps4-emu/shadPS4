// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

#ifndef _WIN32
#include <sys/signal.h>
#endif

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

using OrbisKernelExceptionHandler = PS4_SYSV_ABI void (*)(int, void*);
struct OrbisKernelExceptionHandlerStack {
    void* ss_sp;
    u64 ss_size;
    u32 ss_flags;
};

constexpr s32 POSIX_SS_ONSTACK = 0x0001; /* take signal on alternate stack */
constexpr s32 POSIX_SS_DISABLE = 0x0004; /* disable taking signals on alternate stack */

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

constexpr s32 POSIX_SA_ONSTACK = 0x0001;   /* take signal on signal stack */
constexpr s32 POSIX_SA_RESTART = 0x0002;   /* restart system call on signal return */
constexpr s32 POSIX_SA_RESETHAND = 0x0004; /* reset to SIG_DFL when taking signal */
constexpr s32 POSIX_SA_NODEFER = 0x0010;   /* don't mask the signal we're delivering */
constexpr s32 POSIX_SA_NOCLDWAIT = 0x0020; /* don't keep zombies around */
constexpr s32 POSIX_SA_SIGINFO = 0x0040;   /* signal handler with SA_SIGINFO args */

constexpr s32 POSIX_SIG_BLOCK = 1;   /* block specified signal set */
constexpr s32 POSIX_SIG_UNBLOCK = 2; /* unblock specified signal set */
constexpr s32 POSIX_SIG_SETMASK = 3; /* set specified signal set */

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
    u32 bits[4];
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
    s32 _si_signo; /* signal number */
    s32 _si_errno; /* errno association */
    /*
     * Cause of signal, one of the SI_ macros or signal-specific
     * values, i.e. one of the FPE_... values for SIGFPE.  This
     * value is equivalent to the second argument to an old-style
     * FreeBSD signal handler.
     */
    s32 _si_code;           /* signal code */
    s32 _si_pid;            /* sending process */
    u32 _si_uid;            /* sender's ruid */
    s32 _si_status;         /* exit value */
    void* _si_addr;         /* faulting instruction */
    union Sigval _si_value; /* signal value */
    union {
        struct {
            s32 _trapno; /* machine specific trap code */
        } _fault;
        struct {
            s32 _timerid;
            s32 _overrun;
        } _timer;
        struct {
            s32 _mqd;
        } _mesgq;
        struct {
            s64 _band; /* band event for SIGPOLL */
        } _poll;       /* was this ever used ? */
        struct {
            s64 __spare1__;
            s32 __spare2__[7];
        } __spare__;
    } _reason;
};

constexpr s32 POSIX_SI_NOINFO = 0;
constexpr s32 POSIX_SI_LWP = 0x10007;

struct Ucontext {
    struct Sigset uc_sigmask;
    int field1_0x10[12];
    Mcontext uc_mcontext;
    Ucontext* uc_link;
    ExStack uc_stack;
    int uc_flags;
    int __spare[4];
    int field7_0x4f4[3];

#ifndef _WIN32
    explicit Ucontext(siginfo_t const* inf, ucontext_t* raw_context);
    ucontext_t* host_context;
#else
    explicit Ucontext(PCONTEXT context);
    PCONTEXT host_context;
#endif
    void SyncHostFromGuest();
};

using SigHandler = void PS4_SYSV_ABI (*)(int);

struct Sigaction {
    union {
        void PS4_SYSV_ABI (*handler)(int);
        void PS4_SYSV_ABI (*sigaction)(int, struct Siginfo*, Ucontext*);
    } __sigaction_handler;
    int sa_flags;
    Sigset sa_mask;
};

constexpr uintptr_t POSIX_SIG_DFL = 0;
constexpr uintptr_t POSIX_SIG_IGN = 1;

s32 NativeToOrbisSignal(s32 s);

s32 PS4_SYSV_ABI posix_sigemptyset(Sigset* s);
s32 PS4_SYSV_ABI posix_sigfillset(Sigset* s);
s32 PS4_SYSV_ABI posix_sigaddset(Sigset* s, s32 sig);
s32 PS4_SYSV_ABI posix_sigdelset(Sigset* s, s32 sig);
s32 PS4_SYSV_ABI posix_sigismember(Sigset const* s, s32 sig);
bool PS4_SYSV_ABI posix_sigisemptyset(Sigset* s);

void RegisterException(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
