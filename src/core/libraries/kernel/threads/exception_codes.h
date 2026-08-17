// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

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

constexpr VAddr POSIX_SIG_DFL = 0;
constexpr VAddr POSIX_SIG_IGN = 1;

/** si_code **/
/* codes for SIGILL */
constexpr s32 POSIX_ILL_ILLOPC = 1; /* Illegal opcode. */
constexpr s32 POSIX_ILL_ILLOPN = 2; /* Illegal operand. */
constexpr s32 POSIX_ILL_ILLADR = 3; /* Illegal addressing mode. */
constexpr s32 POSIX_ILL_ILLTRP = 4; /* Illegal trap. */
constexpr s32 POSIX_ILL_PRVOPC = 5; /* Privileged opcode. */
constexpr s32 POSIX_ILL_PRVREG = 6; /* Privileged register. */
constexpr s32 POSIX_ILL_COPROC = 7; /* Coprocessor error. */
constexpr s32 POSIX_ILL_BADSTK = 8; /* Internal stack error. */

/* codes for SIGBUS */
constexpr s32 POSIX_BUS_ADRALN = 1; /* Invalid address alignment. */
constexpr s32 POSIX_BUS_ADRERR = 2; /* Nonexistent physical address. */
constexpr s32 POSIX_BUS_OBJERR = 3; /* Object-specific hardware error. */

/* codes for SIGSEGV */
constexpr s32 POSIX_SEGV_MAPERR = 1; /* Address not mapped to object. */
constexpr s32 POSIX_SEGV_ACCERR = 2; /* Invalid permissions for mapped*/
/* object. */

/* codes for SIGFPE */
constexpr s32 POSIX_FPE_INTOVF = 1; /* Integer overflow. */
constexpr s32 POSIX_FPE_INTDIV = 2; /* Integer divide by zero. */
constexpr s32 POSIX_FPE_FLTDIV = 3; /* Floating point divide by zero. */
constexpr s32 POSIX_FPE_FLTOVF = 4; /* Floating point overflow. */
constexpr s32 POSIX_FPE_FLTUND = 5; /* Floating point underflow. */
constexpr s32 POSIX_FPE_FLTRES = 6; /* Floating point inexact result. */
constexpr s32 POSIX_FPE_FLTINV = 7; /* Invalid floating point operation. */
constexpr s32 POSIX_FPE_FLTSUB = 8; /* Subscript out of range. */

/* codes for SIGTRAP */
constexpr s32 POSIX_TRAP_BRKPT = 1;  /* Process breakpoint. */
constexpr s32 POSIX_TRAP_TRACE = 2;  /* Process trace trap. */
constexpr s32 POSIX_TRAP_DTRACE = 3; /* DTrace induced trap. */

/* codes for SIGCHLD */
constexpr s32 POSIX_CLD_EXITED = 1; /* Child has exited*/
constexpr s32 POSIX_CLD_KILLED =
    2; /* Child has terminated abnormally but did not create a core file*/
constexpr s32 POSIX_CLD_DUMPED = 3;    /* Child has terminated abnormally and created a core file */
constexpr s32 POSIX_CLD_TRAPPED = 4;   /* Traced child has trapped */
constexpr s32 POSIX_CLD_STOPPED = 5;   /* Child has stopped */
constexpr s32 POSIX_CLD_CONTINUED = 6; /* Stopped child has continued */

/* codes for SIGPOLL */
constexpr s32 POSIX_POLL_IN = 1;  /* Data input available */
constexpr s32 POSIX_POLL_OUT = 2; /* Output buffers available */
constexpr s32 POSIX_POLL_MSG = 3; /* Input message available */
constexpr s32 POSIX_POLL_ERR = 4; /* I/O Error */
constexpr s32 POSIX_POLL_PRI = 5; /* High priority input available */
constexpr s32 POSIX_POLL_HUP = 6; /* Device disconnected */

constexpr s32 POSIX_SI_NOINFO = 0;
constexpr s32 POSIX_SI_LWP = 0x10007;