// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

#include <cstdint>

#define ORBIS_SA_RESTART 0x0002
#define ORBIS_SA_RESETHAND 0x0004
#define ORBIS_SA_NODEFER 0x0010
#define ORBIS_SA_SIGINFO 0x0040

#define ORBIS_SIGKILL 9
#define ORBIS_SIGURG 16
#define ORBIS_SIGSTOP 17
#define ORBIS_SIGCHLD 20
#define ORBIS_SIGWINCH 28
#define ORBIS_SIGINFO 29
#define ORBIS_SIGUSR1 30

#define ORBIS_SI_USER 0x10001

#define ORBIS_SIG_WORDS 4
#define ORBIS_SIG_MAXSIG 128
#define ORBIS_SIG_MAXSIG32 32
#define ORBIS_SIG_IDX(sig) ((sig)-1)
#define ORBIS_SIG_WORD(sig) (_SIG_IDX(sig) >> 5)
#define ORBIS_SIG_BIT(sig) (1 << (_SIG_IDX(sig) & 31))
#define ORBIS_SIG_VALID(sig) ((sig) <= _SIG_MAXSIG && (sig) > 0)

#define ORBIS_SIG_ERR (uintptr_t(-1))
#define ORBIS_SIG_DFL (uintptr_t(0))
#define ORBIS_SIG_IGN (uintptr_t(1))
#define ORBIS_SIG_CATCH (uintptr_t(2))
#define ORBIS_SIG_HOLD (uintptr_t(3))

#define ORBIS_SS_ONSTACK 0x0001;               // take signal on alternate stack
#define ORBIS_SS_DISABLE 0x0004;               // disable taking signals on alternate stack
#define ORBIS_MINSIGSTKSZ (512 * 4);           // minimum stack size
#define ORBIS_SIGSTKSZ (MINSIGSTKSZ + 0x8000); // recommended stack size

#define ORBIS_UC_SIGMASK 0x01; // valid uc_sigmask
#define ORBIS_UC_STACK 0x02;   // valid uc_stack
#define ORBIS_UC_CPU 0x04;     // valid GPR context in uc_mcontext
#define ORBIS_UC_FPU 0x08;     // valid FPU context in uc_mcontext

#define ORBIS_MC_HASSEGS 0x01
#define ORBIS_MC_FPFMT_XMM 0x10002
#define ORBIS_MC_FPOWNED_FPU 0x20001 // FP state came from FPU

namespace Orbis {
using pid_t = u32;
using uid_t = u32;

union sigval {
    /* Members as suggested by Annex C of POSIX 1003.1b. */
    s32 sival_int;
    void* sival_ptr;
    /* 6.0 compatibility */
    s32 sigval_int;
    void* sigval_ptr;
};

struct siginfo_t {
    s32 si_signo; /* signal number */
    s32 si_errno; /* errno association */
    /*
     * Cause of signal, one of the SI_ macros or signal-specific
     * values, i.e. one of the FPE_... values for SIGFPE.  This
     * value is equivalent to the second argument to an old-style
     * FreeBSD signal handler.
     */
    s32 si_code;     /* signal code */
    pid_t si_pid;    /* sending process */
    uid_t si_uid;    /* sender's ruid */
    s32 si_status;   /* exit value */
    void* si_addr;   /* faulting instruction */
    sigval si_value; /* signal value */
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

#pragma pack(push, 4)
struct sigset_t {
    u32 __bits[ORBIS_SIG_WORDS];
};

struct mcontext_t {
    s64 mc_onstack; /* sigstack state to restore */
    s64 mc_rdi;     /* machine state (struct trapframe) */
    s64 mc_rsi;
    s64 mc_rdx;
    s64 mc_rcx;
    s64 mc_r8;
    s64 mc_r9;
    s64 mc_rax;
    s64 mc_rbx;
    s64 mc_rbp;
    s64 mc_r10;
    s64 mc_r11;
    s64 mc_r12;
    s64 mc_r13;
    s64 mc_r14;
    s64 mc_r15;
    s32 mc_trapno;
    s16 mc_fs;
    s16 mc_gs;
    s64 mc_addr;
    s32 mc_flags;
    s16 mc_es;
    s16 mc_ds;
    s64 mc_err;
    s64 mc_rip;
    s64 mc_cs;
    s64 mc_rflags;
    s64 mc_rsp;
    s64 mc_ss;
    s64 mc_len; /* sizeof(mcontext_t) */
    s64 mc_fpformat;
    s64 mc_ownedfp;
    s64 mc_lbrfrom;
    s64 mc_lbrto;
    s64 mc_aux1;
    s64 mc_aux2;
    s64 mc_fpstate[104];
    s64 mc_fsbase;
    s64 mc_gsbase;
    s64 mc_spare[6];
};

struct stack_t {
    void* ss_sp;    // signal stack base
    size_t ss_size; // signal stack length SIGSTKSZ
    s32 ss_flags;   // SS_DISABLE and/or SS_ONSTACK
    s32 _align;
};

struct ucontext_t {
    sigset_t sc_mask; /* signal mask to restore */
    s32 field1_0x10[12];
    struct mcontext_t uc_mcontext;
    struct ucontext_t* uc_link;
    struct stack_t uc_stack;
    s32 uc_flags;
    s32 __spare[4];
    s32 field7_0x4f4[3];
};

struct sigaction {
    union {
        void (*sa_handler)(s32, s32, void*);
        void (*sa_sigaction)(s32, siginfo_t*, void*);
    };
    s32 sa_flags;
    sigset_t sa_mask;
};
#pragma pack(pop)

static_assert(sizeof(ucontext_t) == 0x500);

using pthread_t = uintptr_t;

s32 sigaction(s32 sig, const struct sigaction* act, struct sigaction* oldact);
s32 pthread_kill(pthread_t thread, s32 sig);

} // namespace Orbis
