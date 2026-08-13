// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/arch.h"
#include "common/assert.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/threads/exception.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/libs.h"
#include "core/signals.h"

#ifdef _WIN64
#include "common/ntapi.h"
#else
#include <csignal>
#endif
#include <unordered_set>

namespace Libraries::Kernel {

#ifdef _WIN32

// Windows doesn't have native versions of these, and we don't need to use them either.
s32 NativeToOrbisSignal(s32 s) {
    return s;
}

s32 OrbisToNativeSignal(s32 s) {
    return s;
}

#else

s32 NativeToOrbisSignal(s32 s) {
    switch (s) {
    case SIGHUP:
        return POSIX_SIGHUP;
    case SIGINT:
        return POSIX_SIGINT;
    case SIGQUIT:
        return POSIX_SIGQUIT;
    case SIGILL:
        return POSIX_SIGILL;
    case SIGTRAP:
        return POSIX_SIGTRAP;
    case SIGABRT:
        return POSIX_SIGABRT;
    case SIGFPE:
        return POSIX_SIGFPE;
    case SIGKILL:
        return POSIX_SIGKILL;
    case SIGBUS:
        return POSIX_SIGBUS;
    case SIGSEGV:
        return POSIX_SIGSEGV;
    case SIGSYS:
        return POSIX_SIGSYS;
    case SIGPIPE:
        return POSIX_SIGPIPE;
    case SIGALRM:
        return POSIX_SIGALRM;
    case SIGTERM:
        return POSIX_SIGTERM;
    case SIGURG:
        return POSIX_SIGURG;
    case SIGSTOP:
        return POSIX_SIGSTOP;
    case SIGTSTP:
        return POSIX_SIGTSTP;
    case SIGCONT:
        return POSIX_SIGCONT;
    case SIGCHLD:
        return POSIX_SIGCHLD;
    case SIGTTIN:
        return POSIX_SIGTTIN;
    case SIGTTOU:
        return POSIX_SIGTTOU;
    case SIGIO:
        return POSIX_SIGIO;
    case SIGXCPU:
        return POSIX_SIGXCPU;
    case SIGXFSZ:
        return POSIX_SIGXFSZ;
    case SIGVTALRM:
        return POSIX_SIGVTALRM;
    case SIGPROF:
        return POSIX_SIGPROF;
    case SIGWINCH:
        return POSIX_SIGWINCH;
    case SIGUSR1:
        return POSIX_SIGUSR1;
    case SIGUSR2:
        return POSIX_SIGUSR2;
    case _SIGEMT:
        return POSIX_SIGEMT;
    case _SIGINFO:
        return POSIX_SIGINFO;
    case 0:
        return 128;
    default:
        if (s > 0 && s < 128) {
            return s;
        }
        UNREACHABLE_MSG("Unknown signal {}", s);
    }
}

s32 OrbisToNativeSignal(s32 s) {
    switch (s) {
    case POSIX_SIGHUP:
        return SIGHUP;
    case POSIX_SIGINT:
        return SIGINT;
    case POSIX_SIGQUIT:
        return SIGQUIT;
    case POSIX_SIGILL:
        return SIGILL;
    case POSIX_SIGTRAP:
        return SIGTRAP;
    case POSIX_SIGABRT:
        return SIGABRT;
    case POSIX_SIGEMT:
        return _SIGEMT;
    case POSIX_SIGFPE:
        return SIGFPE;
    case POSIX_SIGKILL:
        return SIGKILL;
    case POSIX_SIGBUS:
        return SIGBUS;
    case POSIX_SIGSEGV:
        return SIGSEGV;
    case POSIX_SIGSYS:
        return SIGSYS;
    case POSIX_SIGPIPE:
        return SIGPIPE;
    case POSIX_SIGALRM:
        return SIGALRM;
    case POSIX_SIGTERM:
        return SIGTERM;
    case POSIX_SIGURG:
        return SIGURG;
    case POSIX_SIGSTOP:
        return SIGSTOP;
    case POSIX_SIGTSTP:
        return SIGTSTP;
    case POSIX_SIGCONT:
        return SIGCONT;
    case POSIX_SIGCHLD:
        return SIGCHLD;
    case POSIX_SIGTTIN:
        return SIGTTIN;
    case POSIX_SIGTTOU:
        return SIGTTOU;
    case POSIX_SIGIO:
        return SIGIO;
    case POSIX_SIGXCPU:
        return SIGXCPU;
    case POSIX_SIGXFSZ:
        return SIGXFSZ;
    case POSIX_SIGVTALRM:
        return SIGVTALRM;
    case POSIX_SIGPROF:
        return SIGPROF;
    case POSIX_SIGWINCH:
        return SIGWINCH;
    case POSIX_SIGINFO:
        return _SIGINFO;
    case POSIX_SIGUSR1:
        return SIGUSR1;
    case POSIX_SIGUSR2:
        return SIGUSR2;
    case 128:
        return 0;
    default:
        if (s > 0 && s < 128) {
            return s;
        }
        UNREACHABLE_MSG("Unknown signal {}", s);
    }
}

#endif

#ifdef __APPLE__
#define sigisemptyset(x) (*(x) == 0)
#endif

std::array<OrbisKernelExceptionHandler, 130> Handlers{};
Sigset g_sigintr{};

#ifndef _WIN64
void SigactionHandler(int native_signum, siginfo_t* inf, ucontext_t* raw_context) {
    const auto handler = Handlers[NativeToOrbisSignal(native_signum)];
    if (handler) {
        auto ctx = Ucontext{};
#ifdef ARCH_X86_64
#ifdef __APPLE__
        const auto& regs = raw_context->uc_mcontext->__ss;
        ctx.uc_mcontext.mc_r8 = regs.__r8;
        ctx.uc_mcontext.mc_r9 = regs.__r9;
        ctx.uc_mcontext.mc_r10 = regs.__r10;
        ctx.uc_mcontext.mc_r11 = regs.__r11;
        ctx.uc_mcontext.mc_r12 = regs.__r12;
        ctx.uc_mcontext.mc_r13 = regs.__r13;
        ctx.uc_mcontext.mc_r14 = regs.__r14;
        ctx.uc_mcontext.mc_r15 = regs.__r15;
        ctx.uc_mcontext.mc_rdi = regs.__rdi;
        ctx.uc_mcontext.mc_rsi = regs.__rsi;
        ctx.uc_mcontext.mc_rbp = regs.__rbp;
        ctx.uc_mcontext.mc_rbx = regs.__rbx;
        ctx.uc_mcontext.mc_rdx = regs.__rdx;
        ctx.uc_mcontext.mc_rax = regs.__rax;
        ctx.uc_mcontext.mc_rcx = regs.__rcx;
        ctx.uc_mcontext.mc_rsp = regs.__rsp;
        ctx.uc_mcontext.mc_fs = regs.__fs;
        ctx.uc_mcontext.mc_gs = regs.__gs;
        ctx.uc_mcontext.mc_rip = regs.__rip;
        ctx.uc_mcontext.mc_addr = reinterpret_cast<uint64_t>(inf->si_addr);
#elif defined(__FreeBSD__)
        const auto& regs = raw_context->uc_mcontext;
        ctx.uc_mcontext.mc_r8 = regs.mc_r8;
        ctx.uc_mcontext.mc_r9 = regs.mc_r9;
        ctx.uc_mcontext.mc_r10 = regs.mc_r10;
        ctx.uc_mcontext.mc_r11 = regs.mc_r11;
        ctx.uc_mcontext.mc_r12 = regs.mc_r12;
        ctx.uc_mcontext.mc_r13 = regs.mc_r13;
        ctx.uc_mcontext.mc_r14 = regs.mc_r14;
        ctx.uc_mcontext.mc_r15 = regs.mc_r15;
        ctx.uc_mcontext.mc_rdi = regs.mc_rdi;
        ctx.uc_mcontext.mc_rsi = regs.mc_rsi;
        ctx.uc_mcontext.mc_rbp = regs.mc_rbp;
        ctx.uc_mcontext.mc_rbx = regs.mc_rbx;
        ctx.uc_mcontext.mc_rdx = regs.mc_rdx;
        ctx.uc_mcontext.mc_rax = regs.mc_rax;
        ctx.uc_mcontext.mc_rcx = regs.mc_rcx;
        ctx.uc_mcontext.mc_rsp = regs.mc_rsp;
        ctx.uc_mcontext.mc_fs = regs.mc_fs;
        ctx.uc_mcontext.mc_gs = regs.mc_gs;
        ctx.uc_mcontext.mc_rip = regs.mc_rip;
        ctx.uc_mcontext.mc_addr = uint64_t(regs.mc_addr);
#else
        const auto& regs = raw_context->uc_mcontext.gregs;
        ctx.uc_mcontext.mc_r8 = regs[REG_R8];
        ctx.uc_mcontext.mc_r9 = regs[REG_R9];
        ctx.uc_mcontext.mc_r10 = regs[REG_R10];
        ctx.uc_mcontext.mc_r11 = regs[REG_R11];
        ctx.uc_mcontext.mc_r12 = regs[REG_R12];
        ctx.uc_mcontext.mc_r13 = regs[REG_R13];
        ctx.uc_mcontext.mc_r14 = regs[REG_R14];
        ctx.uc_mcontext.mc_r15 = regs[REG_R15];
        ctx.uc_mcontext.mc_rdi = regs[REG_RDI];
        ctx.uc_mcontext.mc_rsi = regs[REG_RSI];
        ctx.uc_mcontext.mc_rbp = regs[REG_RBP];
        ctx.uc_mcontext.mc_rbx = regs[REG_RBX];
        ctx.uc_mcontext.mc_rdx = regs[REG_RDX];
        ctx.uc_mcontext.mc_rax = regs[REG_RAX];
        ctx.uc_mcontext.mc_rcx = regs[REG_RCX];
        ctx.uc_mcontext.mc_rsp = regs[REG_RSP];
        ctx.uc_mcontext.mc_fs = (regs[REG_CSGSFS] >> 32) & 0xFFFF;
        ctx.uc_mcontext.mc_gs = (regs[REG_CSGSFS] >> 16) & 0xFFFF;
        ctx.uc_mcontext.mc_rip = (regs[REG_RIP]);
        ctx.uc_mcontext.mc_addr = reinterpret_cast<uint64_t>(inf->si_addr);
#endif
#else
        UNREACHABLE_MSG("SigactionHandler not implemented for current architecture.");
#endif
        handler(NativeToOrbisSignal(native_signum), &ctx);
    } else {
        UNREACHABLE_MSG("Unhandled exception");
    }
}
#else
void ExceptionHandler(void* arg1, void* arg2, void* arg3, PCONTEXT context) {
    const char* thrName = (char*)arg1;
    int native_signum = reinterpret_cast<uintptr_t>(arg2);
    LOG_INFO(Lib_Kernel, "Exception raised successfully on thread '{}'", thrName);
    const auto handler = Handlers[NativeToOrbisSignal(native_signum)];
    if (handler) {
        auto ctx = Ucontext{};
        ctx.uc_mcontext.mc_r8 = context->R8;
        ctx.uc_mcontext.mc_r9 = context->R9;
        ctx.uc_mcontext.mc_r10 = context->R10;
        ctx.uc_mcontext.mc_r11 = context->R11;
        ctx.uc_mcontext.mc_r12 = context->R12;
        ctx.uc_mcontext.mc_r13 = context->R13;
        ctx.uc_mcontext.mc_r14 = context->R14;
        ctx.uc_mcontext.mc_r15 = context->R15;
        ctx.uc_mcontext.mc_rdi = context->Rdi;
        ctx.uc_mcontext.mc_rsi = context->Rsi;
        ctx.uc_mcontext.mc_rbp = context->Rbp;
        ctx.uc_mcontext.mc_rbx = context->Rbx;
        ctx.uc_mcontext.mc_rdx = context->Rdx;
        ctx.uc_mcontext.mc_rax = context->Rax;
        ctx.uc_mcontext.mc_rcx = context->Rcx;
        ctx.uc_mcontext.mc_rsp = context->Rsp;
        ctx.uc_mcontext.mc_fs = context->SegFs;
        ctx.uc_mcontext.mc_gs = context->SegGs;
        handler(NativeToOrbisSignal(native_signum), &ctx);
    } else {
        UNREACHABLE_MSG("Unhandled exception");
    }
}
#endif

s32 PS4_SYSV_ABI posix_sigemptyset(Sigset* s) {
    s->bits[0] = 0;
    s->bits[1] = 0;
    s->bits[2] = 0;
    s->bits[3] = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_sigfillset(Sigset* s) {
    s->bits[0] = ~0U;
    s->bits[1] = ~0U;
    s->bits[2] = ~0U;
    s->bits[3] = ~0U;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_sigaddset(Sigset* s, s32 sig) {
    s32 val = sig - 1;
    if (val >= 0x80) {
        *Libraries::Kernel::__Error() = POSIX_EINVAL;
        return ORBIS_FAIL;
    }
    s->bits[val >> 5] |= 1 << (val & 0x1f);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_sigdelset(Sigset* s, s32 sig) {
    s32 val = sig - 1;
    if (val >= 0x80) {
        *Libraries::Kernel::__Error() = POSIX_EINVAL;
        return ORBIS_FAIL;
    }
    s->bits[val >> 5] &= ~(1 << (val & 0x1f));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_sigismember(Sigset* s, s32 sig) {
    s32 val = sig - 1;
    if (val >= 0x80) {
        *Libraries::Kernel::__Error() = POSIX_EINVAL;
        return ORBIS_FAIL;
    }
    return ((s->bits[val >> 5] >> (val & 0x1f)) & 1) != 0;
}

bool PS4_SYSV_ABI posix_sigisemptyset(Sigset* s) {
    return s->bits[0] == 0 && s->bits[1] == 0 && s->bits[2] == 0 && s->bits[3] == 0;
}

#ifndef _WIN32
static void GuestSigsetToNative(const Sigset& guest, sigset_t& native) {
    sigemptyset(&native);

    for (s32 sig = 1; sig <= 128; sig++) {
        if (posix_sigismember(const_cast<Sigset*>(&guest), sig) != 0) {
            const s32 native_sig = OrbisToNativeSignal(sig);
            if (native_sig > 0 && native_sig <= 128) {
                sigaddset(&native, native_sig);
            }
        }
    }
}

static void NativeSigsetToGuest(const sigset_t& native, Sigset& guest) {
    posix_sigemptyset(&guest);

    for (s32 sig = 1; sig <= 128; sig++) {
        const s32 native_sig = OrbisToNativeSignal(sig);
        if (native_sig > 0 && native_sig <= 128 && sigismember(&native, native_sig) == 1) {
            posix_sigaddset(&guest, sig);
        }
    }
}
#endif

s32 PS4_SYSV_ABI posix_sigprocmask(s32 how, const Sigset* set, Sigset* oset) {
    LOG_ERROR(Lib_Kernel, "(STUBBED) called, how = {}", how);
    return ORBIS_OK;
}

constexpr s32 POSIX_SS_ONSTACK = 0x0001; /* take signal on alternate stack */
constexpr s32 POSIX_SS_DISABLE = 0x0004; /* disable taking signals on alternate stack */

s32 PS4_SYSV_ABI posix_sigaltstack(const OrbisKernelExceptionHandlerStack* ss,
                                   OrbisKernelExceptionHandlerStack* old_ss) {
    s32 ret = 0;
#ifndef _WIN32
    stack_t native_ss{};
    if (ss) {
        LOG_INFO(Lib_Kernel, "called, ss.ss_size: {}, ss.ss_sp: {}, ss.ss_flags: {:#x}",
                 ss->ss_size, ss->ss_sp, ss->ss_flags);
        native_ss.ss_sp = ss->ss_sp;
        native_ss.ss_size = ss->ss_size == 0 ? 0 : std::max(ss->ss_size, (u64)MINSIGSTKSZ + 0x1000);
        u32 guest_ss_flags = ss->ss_flags;
        if ((guest_ss_flags & POSIX_SS_ONSTACK)) {
            native_ss.ss_flags |= SS_ONSTACK;
            guest_ss_flags &= ~POSIX_SS_ONSTACK;
        }
        if ((guest_ss_flags & POSIX_SS_DISABLE)) {
            native_ss.ss_flags |= SS_DISABLE;
            guest_ss_flags &= ~POSIX_SS_DISABLE;
        }
        if (guest_ss_flags != 0) {
            LOG_ERROR(Lib_Kernel, "Unrecognized guest flag(s): {:#x}", guest_ss_flags);
        }
    }
    stack_t native_old_ss{};
    ret = sigaltstack(ss ? &native_ss : nullptr, old_ss ? &native_old_ss : nullptr);
    if (ret < 0) {
        SetPosixErrno(errno);
        LOG_ERROR(Lib_Kernel, "sigaltstack returned {} {}", errno, strerror(errno));
    }
    if (old_ss) {
        old_ss->ss_sp = native_old_ss.ss_sp;
        old_ss->ss_size = native_old_ss.ss_size;
        u32 host_ss_flags = native_old_ss.ss_flags;
        if ((host_ss_flags & SS_ONSTACK)) {
            old_ss->ss_flags |= POSIX_SS_ONSTACK;
            host_ss_flags &= ~SS_ONSTACK;
        }
        if ((host_ss_flags & SS_DISABLE)) {
            old_ss->ss_flags |= POSIX_SS_DISABLE;
            host_ss_flags &= ~SS_DISABLE;
        }
        if (host_ss_flags != 0) {
            LOG_ERROR(Lib_Kernel, "Unrecognized host flag(s): {:#x}", host_ss_flags);
        }
    }
#else
    LOG_ERROR(Lib_Kernel, "(stubbed)");
#endif
    return ret;
}

s32 PS4_SYSV_ABI posix_sigaction(s32 sig, Sigaction* act, Sigaction* oact) {
    if (sig < 1 || sig > 128 || sig == POSIX_SIGTHR || sig == POSIX_SIGKILL ||
        sig == POSIX_SIGSTOP) {
        *__Error() = POSIX_EINVAL;
        return ORBIS_FAIL;
    }
#ifdef _WIN32
    LOG_ERROR(Lib_Kernel, "(STUBBED) called, sig: {}", sig);
    Handlers[sig] = reinterpret_cast<OrbisKernelExceptionHandler>(
        act ? act->__sigaction_handler.sigaction : nullptr);
    if (oact) {
        memset(oact, 0, sizeof(*oact));
    }
    if (act && oact) {
        oact->__sigaction_handler = act->__sigaction_handler;
        oact->sa_mask = act->sa_mask;
        oact->sa_flags = act->sa_flags;
    }
#else
    s32 native_sig = OrbisToNativeSignal(sig);
    if (native_sig == SIGVTALRM || IsPthreadCancelSignal(native_sig)) {
        LOG_ERROR(Lib_Kernel, "Guest is attempting to use the HLE-reserved signal {}!", sig);
        *__Error() = POSIX_EINVAL;
        return ORBIS_FAIL;
    }
#if !defined(__APPLE__) && !defined(__FreeBSD__)
    if (native_sig >= __SIGRTMIN && native_sig < SIGRTMIN) {
        LOG_ERROR(Lib_Kernel, "Guest is attempting to use the HLE libc-reserved signal {}!", sig);
        *__Error() = POSIX_EINVAL;
        return ORBIS_FAIL;
    }
#else
    if (native_sig > SIGUSR2) {
        LOG_ERROR(Lib_Kernel,
                  "Guest is attempting to use SIGRT signals, which aren't available on this "
                  "platform (signal: {})!",
                  sig);
    }
#endif
    LOG_INFO(Lib_Kernel, "called, sig: {}, native sig: {}", sig, native_sig);
    struct sigaction native_act{};
    if (act) {
        native_act.sa_flags = act->sa_flags;
        native_act.sa_sigaction =
            reinterpret_cast<decltype(native_act.sa_sigaction)>(SigactionHandler);
        GuestSigsetToNative(act->sa_mask, native_act.sa_mask);
    }

    const auto prev_handler = Handlers[sig];

    if (native_sig == SIGSEGV || native_sig == SIGBUS || native_sig == SIGILL) {
        Handlers[sig] = reinterpret_cast<OrbisKernelExceptionHandler>(
            act ? act->__sigaction_handler.sigaction : nullptr);

        if (oact) {
            oact->sa_flags = 0;
            oact->__sigaction_handler.sigaction =
                reinterpret_cast<decltype(oact->__sigaction_handler.sigaction)>(prev_handler);
            posix_sigemptyset(&oact->sa_mask);
        }

        return ORBIS_OK;
    }
    if (native_sig > 127) {
        LOG_WARNING(Lib_Kernel, "We can't install a handler for native signal {}!", native_sig);
        return ORBIS_OK;
    }
    struct sigaction native_oact{};
    const s32 ret =
        sigaction(native_sig, act ? &native_act : nullptr, oact ? &native_oact : nullptr);

    if (ret < 0) {
        LOG_ERROR(Lib_Kernel, "sigaction failed: {}", strerror(errno));
        SetPosixErrno(errno);
        return ORBIS_FAIL;
    }

    Handlers[sig] = reinterpret_cast<OrbisKernelExceptionHandler>(
        act ? act->__sigaction_handler.sigaction : nullptr);

    if (oact) {
        oact->sa_flags = native_oact.sa_flags;
        oact->__sigaction_handler.sigaction =
            reinterpret_cast<decltype(oact->__sigaction_handler.sigaction)>(prev_handler);
        NativeSigsetToGuest(native_oact.sa_mask, oact->sa_mask);
    }
#endif
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_pthread_sigmask(s32 how, const Sigset* set, Sigset* oset) {
#ifndef _WIN32
    sigset_t native_set{};
    sigset_t native_oset{};

    sigset_t* native_set_ptr = nullptr;
    if (set) {
        sigemptyset(&native_set);
        for (s32 sig = 1; sig <= 128; sig++) {
            if (posix_sigismember(const_cast<Sigset*>(set), sig) != 0) {
                const s32 native_sig = OrbisToNativeSignal(sig);
                if (native_sig > 0 && native_sig <= 128) {
                    sigaddset(&native_set, native_sig);
                }
            }
        }
        native_set_ptr = &native_set;
    }

    const int ret = pthread_sigmask(how, native_set_ptr, oset ? &native_oset : nullptr);
    if (ret != 0) {
        SetPosixErrno(errno);
        return ORBIS_FAIL;
    }

    if (oset) {
        posix_sigemptyset(oset);

        for (s32 sig = 1; sig <= 128; sig++) {
            const s32 native_sig = OrbisToNativeSignal(sig);
            if (native_sig > 0 && native_sig <= 128 && sigismember(&native_oset, native_sig) == 1) {
                posix_sigaddset(oset, sig);
            }
        }
    }

    return ORBIS_OK;
#else
    LOG_ERROR(Lib_Kernel, "(STUBBED) called");
    return ORBIS_OK;
#endif
}

s32 PS4_SYSV_ABI posix_sigsuspend(const Sigset* sigmask) {
#ifndef _WIN32
    sigset_t native_mask;
    sigemptyset(&native_mask);

    if (sigmask) {
        for (s32 sig = 1; sig <= 128; sig++) {
            if (posix_sigismember(const_cast<Sigset*>(sigmask), sig) != 0) {
                const s32 native_sig = OrbisToNativeSignal(sig);
                if (native_sig > 0 && native_sig <= 128) {
                    sigaddset(&native_mask, native_sig);
                }
            }
        }
    }

    const int ret = sigsuspend(&native_mask);
    ASSERT(ret == -1);

    SetPosixErrno(errno);
    return ORBIS_FAIL;
#else
    LOG_ERROR(Lib_Kernel, "(STUBBED) called");
    return ORBIS_OK;
#endif
}

s32 PS4_SYSV_ABI posix_sigwait(const Sigset* set, s32* sig) {
#ifndef _WIN32
    if (set == nullptr || sig == nullptr) {
        return POSIX_EINVAL;
    }

    sigset_t native_set;
    GuestSigsetToNative(*set, native_set);

    int native_sig;
    const int ret = sigwait(&native_set, &native_sig);
    if (ret != 0) {
        SetPosixErrno(errno);
    }

    s32 guest_sig = NativeToOrbisSignal(native_sig);

    return ORBIS_OK;
#else
    LOG_ERROR(Lib_Kernel, "(STUBBED) called");
    return ORBIS_OK;
#endif
}

SigHandler PS4_SYSV_ABI posix_signal(s32 sig, SigHandler func) {
    Sigaction act{};
    act.__sigaction_handler.handler = func;
    posix_sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (posix_sigismember(&g_sigintr, sig) == 0) {
        act.sa_flags |= POSIX_SA_RESTART;
    }
    Sigaction oact{};
    s32 result = posix_sigaction(sig, &act, &oact);
    if (result >= ORBIS_OK) {
        return oact.__sigaction_handler.handler;
    }
    return reinterpret_cast<SigHandler>(-1);
}

s32 PS4_SYSV_ABI posix_pthread_kill(PthreadT thread, s32 sig) {
    if (sig < 1 || sig > 128) { // off-by-one error?
        return POSIX_EINVAL;
    }
    LOG_WARNING(Lib_Kernel, "Raising signal {} on thread '{}'", sig, thread->name);
    int const native_signum = OrbisToNativeSignal(sig);
#ifndef _WIN64
    const auto pthr = reinterpret_cast<pthread_t>(thread->native_thr->GetHandle());
    const auto ret = pthread_kill(pthr, native_signum);
    if (ret != 0) {
        LOG_ERROR(Kernel, "Failed to send exception signal to thread '{}': {}", thread->name,
                  strerror(errno));
    }
#else
    USER_APC_OPTION option;
    option.UserApcFlags = QueueUserApcFlagsSpecialUserApc;

    u64 res = NtQueueApcThreadEx(reinterpret_cast<HANDLE>(thread->native_thr->GetHandle()), option,
                                 ExceptionHandler, (void*)thread->name.c_str(),
                                 (void*)(s64)native_signum, nullptr);
    ASSERT(res == 0);
#endif
    return ORBIS_OK;
}

// libkernel has a check in sceKernelInstallExceptionHandler and sceKernelRemoveExceptionHandler for
// validating if the application requested a handler for an allowed signal or not. However, that is
// just a wrapper for sigaction, which itself does not have any such restrictions, and therefore
// this check is ridiculously trivial to go around. This, however, means that we need to support all
// 127 - 3 possible signals, even if realistically, only homebrew will use most of them.
static std::unordered_set<s32> orbis_allowed_signals{
    POSIX_SIGHUP, POSIX_SIGILL, POSIX_SIGFPE, POSIX_SIGBUS, POSIX_SIGSEGV, POSIX_SIGUSR1,
};

int PS4_SYSV_ABI sceKernelInstallExceptionHandler(s32 signum, OrbisKernelExceptionHandler handler) {
    if (!orbis_allowed_signals.contains(signum)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (Handlers[signum] != nullptr) {
        return ORBIS_KERNEL_ERROR_EAGAIN;
    }
    LOG_INFO(Lib_Kernel, "Installing signal handler for {}", signum);
    Sigaction act = {};
    act.sa_flags = POSIX_SA_SIGINFO | POSIX_SA_RESTART;
    act.__sigaction_handler.sigaction =
        reinterpret_cast<decltype(act.__sigaction_handler.sigaction)>(handler);
    posix_sigemptyset(&act.sa_mask);
    s32 ret = posix_sigaction(signum, &act, nullptr);
    if (ret < 0) {
        LOG_ERROR(Lib_Kernel, "Failed to add handler for signal {}: {}", signum,
                  strerror(*__Error()));
        return ErrnoToSceKernelError(*__Error());
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelRemoveExceptionHandler(s32 signum) {
    if (!orbis_allowed_signals.contains(signum)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    int const native_signum = OrbisToNativeSignal(signum);
    Handlers[signum] = nullptr;
    Sigaction act = {};
    act.sa_flags = POSIX_SA_SIGINFO;
    act.__sigaction_handler.sigaction = nullptr;
    posix_sigemptyset(&act.sa_mask);
    s32 ret = posix_sigaction(signum, &act, nullptr);
    if (ret < 0) {
        LOG_ERROR(Lib_Kernel, "Failed to remove handler for signal {}: {}", signum,
                  strerror(*__Error()));
        return ErrnoToSceKernelError(*__Error());
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelRaiseException(PthreadT thread, int signum) {
    if (signum != POSIX_SIGUSR1) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    s32 ret = posix_pthread_kill(thread, signum);
    if (ret < 0) {
        return ErrnoToSceKernelError(ret);
    }
    return ret;
}

s32 PS4_SYSV_ABI sceKernelDebugRaiseException(u32 error, s64 unk) {
    if (unk != 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    UNREACHABLE_MSG("error {:#x}", error);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelDebugRaiseExceptionOnReleaseMode(u32 error, s64 unk) {
    if (unk != 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    UNREACHABLE_MSG("error {:#x}", error);
    return ORBIS_OK;
}

void RegisterException(Core::Loader::SymbolsResolver* sym) {
    LIB_OBJ("nQVWJEGHObc", "libkernel", 1, "libkernel", &g_sigintr);

    LIB_FUNCTION("il03nluKfMk", "libkernel_unity", 1, "libkernel", sceKernelRaiseException);
    LIB_FUNCTION("WkwEd3N7w0Y", "libkernel_unity", 1, "libkernel",
                 sceKernelInstallExceptionHandler);
    LIB_FUNCTION("Qhv5ARAoOEc", "libkernel_unity", 1, "libkernel", sceKernelRemoveExceptionHandler);
    LIB_FUNCTION("OMDRKKAZ8I4", "libkernel", 1, "libkernel", sceKernelDebugRaiseException);
    LIB_FUNCTION("zE-wXIZjLoM", "libkernel", 1, "libkernel",
                 sceKernelDebugRaiseExceptionOnReleaseMode);
    LIB_FUNCTION("WkwEd3N7w0Y", "libkernel", 1, "libkernel", sceKernelInstallExceptionHandler);
    LIB_FUNCTION("Qhv5ARAoOEc", "libkernel", 1, "libkernel", sceKernelRemoveExceptionHandler);

    LIB_FUNCTION("KiJEPEWRyUY", "libkernel", 1, "libkernel", posix_sigaction);
    LIB_FUNCTION("VADc3MNQ3cM", "libkernel", 1, "libkernel", posix_signal);
    LIB_FUNCTION("+F7C-hdk7+E", "libkernel", 1, "libkernel", posix_sigemptyset);
    LIB_FUNCTION("VkTAsrZDcJ0", "libkernel", 1, "libkernel", posix_sigfillset);
    LIB_FUNCTION("JUimFtKe0Kc", "libkernel", 1, "libkernel", posix_sigaddset);
    LIB_FUNCTION("Nd-u09VFSCA", "libkernel", 1, "libkernel", posix_sigdelset);
    LIB_FUNCTION("JnNl8Xr-z4Y", "libkernel", 1, "libkernel", posix_sigismember);
    LIB_FUNCTION("aPcyptbOiZs", "libkernel", 1, "libkernel", posix_sigprocmask);
    LIB_FUNCTION("yH-uQW3LbX0", "libkernel", 1, "libkernel", posix_pthread_kill);
    LIB_FUNCTION("sHziAegVp74", "libkernel", 1, "libkernel", posix_sigaltstack);
    LIB_FUNCTION("JZKw5+Wrnaw", "libkernel", 1, "libkernel", posix_pthread_sigmask);
    LIB_FUNCTION("KZ-4qlqlpmo", "libkernel", 1, "libkernel", posix_sigsuspend);
    LIB_FUNCTION("mrbHXqK8wkg", "libkernel", 1, "libkernel", posix_sigwait);

    LIB_FUNCTION("KiJEPEWRyUY", "libkernel_psmkit", 1, "libkernel", posix_sigaction);
    LIB_FUNCTION("VADc3MNQ3cM", "libkernel_psmkit", 1, "libkernel", posix_signal);
    LIB_FUNCTION("+F7C-hdk7+E", "libkernel_psmkit", 1, "libkernel", posix_sigemptyset);
    LIB_FUNCTION("VkTAsrZDcJ0", "libkernel_psmkit", 1, "libkernel", posix_sigfillset);
    LIB_FUNCTION("JUimFtKe0Kc", "libkernel_psmkit", 1, "libkernel", posix_sigaddset);
    LIB_FUNCTION("Nd-u09VFSCA", "libkernel_psmkit", 1, "libkernel", posix_sigdelset);
    LIB_FUNCTION("JnNl8Xr-z4Y", "libkernel_psmkit", 1, "libkernel", posix_sigismember);
    LIB_FUNCTION("aPcyptbOiZs", "libkernel_psmkit", 1, "libkernel", posix_sigprocmask);
    LIB_FUNCTION("yH-uQW3LbX0", "libkernel_psmkit", 1, "libkernel", posix_pthread_kill);
    LIB_FUNCTION("sHziAegVp74", "libkernel_psmkit", 1, "libkernel", posix_sigaltstack);

    LIB_FUNCTION("KiJEPEWRyUY", "libScePosix", 1, "libkernel", posix_sigaction);
    LIB_FUNCTION("VADc3MNQ3cM", "libScePosix", 1, "libkernel", posix_signal);
    LIB_FUNCTION("+F7C-hdk7+E", "libScePosix", 1, "libkernel", posix_sigemptyset);
    LIB_FUNCTION("VkTAsrZDcJ0", "libScePosix", 1, "libkernel", posix_sigfillset);
    LIB_FUNCTION("JUimFtKe0Kc", "libScePosix", 1, "libkernel", posix_sigaddset);
    LIB_FUNCTION("Nd-u09VFSCA", "libScePosix", 1, "libkernel", posix_sigdelset);
    LIB_FUNCTION("JnNl8Xr-z4Y", "libScePosix", 1, "libkernel", posix_sigismember);
    LIB_FUNCTION("aPcyptbOiZs", "libScePosix", 1, "libkernel", posix_sigprocmask);
    LIB_FUNCTION("yH-uQW3LbX0", "libScePosix", 1, "libkernel", posix_pthread_kill);
    LIB_FUNCTION("sHziAegVp74", "libScePosix", 1, "libkernel", posix_sigaltstack);
    LIB_FUNCTION("JZKw5+Wrnaw", "libScePosix", 1, "libkernel", posix_pthread_sigmask);
}

} // namespace Libraries::Kernel
