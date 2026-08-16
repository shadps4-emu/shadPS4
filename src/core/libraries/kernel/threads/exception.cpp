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
#include <sys/ucontext.h>
#endif
#include <unordered_set>
#include "exception.h"

namespace Libraries::Kernel {

#ifndef _WIN32
Ucontext::Ucontext(siginfo_t const* inf, ucontext_t* raw_context) {
    if (!inf || !raw_context) {
        return;
    }
    host_context = raw_context;
#ifdef ARCH_X86_64
#ifdef __APPLE__
    const auto& regs = raw_context->uc_mcontext->__ss;
    uc_mcontext.mc_r8 = regs.__r8;
    uc_mcontext.mc_r9 = regs.__r9;
    uc_mcontext.mc_r10 = regs.__r10;
    uc_mcontext.mc_r11 = regs.__r11;
    uc_mcontext.mc_r12 = regs.__r12;
    uc_mcontext.mc_r13 = regs.__r13;
    uc_mcontext.mc_r14 = regs.__r14;
    uc_mcontext.mc_r15 = regs.__r15;
    uc_mcontext.mc_rdi = regs.__rdi;
    uc_mcontext.mc_rsi = regs.__rsi;
    uc_mcontext.mc_rbp = regs.__rbp;
    uc_mcontext.mc_rbx = regs.__rbx;
    uc_mcontext.mc_rdx = regs.__rdx;
    uc_mcontext.mc_rax = regs.__rax;
    uc_mcontext.mc_rcx = regs.__rcx;
    uc_mcontext.mc_rsp = regs.__rsp;
    uc_mcontext.mc_fs = regs.__fs;
    uc_mcontext.mc_gs = regs.__gs;
    uc_mcontext.mc_rip = regs.__rip;
    uc_mcontext.mc_addr = reinterpret_cast<uint64_t>(inf->si_addr);
#elif defined(__FreeBSD__)
    const auto& regs = raw_context->uc_mcontext;
    uc_mcontext.mc_r8 = regs.mc_r8;
    uc_mcontext.mc_r9 = regs.mc_r9;
    uc_mcontext.mc_r10 = regs.mc_r10;
    uc_mcontext.mc_r11 = regs.mc_r11;
    uc_mcontext.mc_r12 = regs.mc_r12;
    uc_mcontext.mc_r13 = regs.mc_r13;
    uc_mcontext.mc_r14 = regs.mc_r14;
    uc_mcontext.mc_r15 = regs.mc_r15;
    uc_mcontext.mc_rdi = regs.mc_rdi;
    uc_mcontext.mc_rsi = regs.mc_rsi;
    uc_mcontext.mc_rbp = regs.mc_rbp;
    uc_mcontext.mc_rbx = regs.mc_rbx;
    uc_mcontext.mc_rdx = regs.mc_rdx;
    uc_mcontext.mc_rax = regs.mc_rax;
    uc_mcontext.mc_rcx = regs.mc_rcx;
    uc_mcontext.mc_rsp = regs.mc_rsp;
    uc_mcontext.mc_fs = regs.mc_fs;
    uc_mcontext.mc_gs = regs.mc_gs;
    uc_mcontext.mc_rip = regs.mc_rip;
    uc_mcontext.mc_addr = uint64_t(regs.mc_addr);
#else
    const auto& regs = raw_context->uc_mcontext.gregs;
    uc_mcontext.mc_r8 = regs[REG_R8];
    uc_mcontext.mc_r9 = regs[REG_R9];
    uc_mcontext.mc_r10 = regs[REG_R10];
    uc_mcontext.mc_r11 = regs[REG_R11];
    uc_mcontext.mc_r12 = regs[REG_R12];
    uc_mcontext.mc_r13 = regs[REG_R13];
    uc_mcontext.mc_r14 = regs[REG_R14];
    uc_mcontext.mc_r15 = regs[REG_R15];
    uc_mcontext.mc_rdi = regs[REG_RDI];
    uc_mcontext.mc_rsi = regs[REG_RSI];
    uc_mcontext.mc_rbp = regs[REG_RBP];
    uc_mcontext.mc_rbx = regs[REG_RBX];
    uc_mcontext.mc_rdx = regs[REG_RDX];
    uc_mcontext.mc_rax = regs[REG_RAX];
    uc_mcontext.mc_rcx = regs[REG_RCX];
    uc_mcontext.mc_rsp = regs[REG_RSP];
    uc_mcontext.mc_fs = (regs[REG_CSGSFS] >> 32) & 0xFFFF;
    uc_mcontext.mc_gs = (regs[REG_CSGSFS] >> 16) & 0xFFFF;
    uc_mcontext.mc_rip = regs[REG_RIP];
    uc_mcontext.mc_addr = reinterpret_cast<uint64_t>(inf->si_addr);
#endif
#else
#error "ucontext_t conversion not implemented for current architecture."
#endif
}
#else
Ucontext::Ucontext(PCONTEXT context) {
    if (!context) {
        return;
    }
    host_context = context;
    uc_mcontext.mc_r8 = context->R8;
    uc_mcontext.mc_r9 = context->R9;
    uc_mcontext.mc_r10 = context->R10;
    uc_mcontext.mc_r11 = context->R11;
    uc_mcontext.mc_r12 = context->R12;
    uc_mcontext.mc_r13 = context->R13;
    uc_mcontext.mc_r14 = context->R14;
    uc_mcontext.mc_r15 = context->R15;
    uc_mcontext.mc_rdi = context->Rdi;
    uc_mcontext.mc_rsi = context->Rsi;
    uc_mcontext.mc_rbp = context->Rbp;
    uc_mcontext.mc_rbx = context->Rbx;
    uc_mcontext.mc_rdx = context->Rdx;
    uc_mcontext.mc_rax = context->Rax;
    uc_mcontext.mc_rcx = context->Rcx;
    uc_mcontext.mc_rsp = context->Rsp;
    uc_mcontext.mc_rip = context->Rip;
    uc_mcontext.mc_fs = context->SegFs;
    uc_mcontext.mc_gs = context->SegGs;
}
#endif

void Ucontext::SyncHostFromGuest() {
    if (!host_context) {
        return;
    }

#ifdef ARCH_X86_64
#ifdef _WIN32
    host_context->R8 = uc_mcontext.mc_r8;
    host_context->R9 = uc_mcontext.mc_r9;
    host_context->R10 = uc_mcontext.mc_r10;
    host_context->R11 = uc_mcontext.mc_r11;
    host_context->R12 = uc_mcontext.mc_r12;
    host_context->R13 = uc_mcontext.mc_r13;
    host_context->R14 = uc_mcontext.mc_r14;
    host_context->R15 = uc_mcontext.mc_r15;
    host_context->Rdi = uc_mcontext.mc_rdi;
    host_context->Rsi = uc_mcontext.mc_rsi;
    host_context->Rbp = uc_mcontext.mc_rbp;
    host_context->Rbx = uc_mcontext.mc_rbx;
    host_context->Rdx = uc_mcontext.mc_rdx;
    host_context->Rax = uc_mcontext.mc_rax;
    host_context->Rcx = uc_mcontext.mc_rcx;
    host_context->Rsp = uc_mcontext.mc_rsp;
    host_context->Rip = uc_mcontext.mc_rip;
    // host_context->SegFs = static_cast<DWORD>(uc_mcontext.mc_fs);
    // host_context->SegGs = static_cast<DWORD>(uc_mcontext.mc_gs);
#elif __APPLE__
    auto& regs = host_context->uc_mcontext->__ss;
    regs.__r8 = uc_mcontext.mc_r8;
    regs.__r9 = uc_mcontext.mc_r9;
    regs.__r10 = uc_mcontext.mc_r10;
    regs.__r11 = uc_mcontext.mc_r11;
    regs.__r12 = uc_mcontext.mc_r12;
    regs.__r13 = uc_mcontext.mc_r13;
    regs.__r14 = uc_mcontext.mc_r14;
    regs.__r15 = uc_mcontext.mc_r15;
    regs.__rdi = uc_mcontext.mc_rdi;
    regs.__rsi = uc_mcontext.mc_rsi;
    regs.__rbp = uc_mcontext.mc_rbp;
    regs.__rbx = uc_mcontext.mc_rbx;
    regs.__rdx = uc_mcontext.mc_rdx;
    regs.__rax = uc_mcontext.mc_rax;
    regs.__rcx = uc_mcontext.mc_rcx;
    regs.__rsp = uc_mcontext.mc_rsp;
    regs.__fs = uc_mcontext.mc_fs;
    regs.__gs = uc_mcontext.mc_gs;
    regs.__rip = uc_mcontext.mc_rip;
#elif defined(__FreeBSD__)
    auto& regs = host_context->uc_mcontext;
    regs.mc_r8 = uc_mcontext.mc_r8;
    regs.mc_r9 = uc_mcontext.mc_r9;
    regs.mc_r10 = uc_mcontext.mc_r10;
    regs.mc_r11 = uc_mcontext.mc_r11;
    regs.mc_r12 = uc_mcontext.mc_r12;
    regs.mc_r13 = uc_mcontext.mc_r13;
    regs.mc_r14 = uc_mcontext.mc_r14;
    regs.mc_r15 = uc_mcontext.mc_r15;
    regs.mc_rdi = uc_mcontext.mc_rdi;
    regs.mc_rsi = uc_mcontext.mc_rsi;
    regs.mc_rbp = uc_mcontext.mc_rbp;
    regs.mc_rbx = uc_mcontext.mc_rbx;
    regs.mc_rdx = uc_mcontext.mc_rdx;
    regs.mc_rax = uc_mcontext.mc_rax;
    regs.mc_rcx = uc_mcontext.mc_rcx;
    regs.mc_rsp = uc_mcontext.mc_rsp;
    regs.mc_fs = uc_mcontext.mc_fs;
    regs.mc_gs = uc_mcontext.mc_gs;
    regs.mc_rip = uc_mcontext.mc_rip;
#else
    auto& regs = host_context->uc_mcontext.gregs;
    regs[REG_R8] = uc_mcontext.mc_r8;
    regs[REG_R9] = uc_mcontext.mc_r9;
    regs[REG_R10] = uc_mcontext.mc_r10;
    regs[REG_R11] = uc_mcontext.mc_r11;
    regs[REG_R12] = uc_mcontext.mc_r12;
    regs[REG_R13] = uc_mcontext.mc_r13;
    regs[REG_R14] = uc_mcontext.mc_r14;
    regs[REG_R15] = uc_mcontext.mc_r15;
    regs[REG_RDI] = uc_mcontext.mc_rdi;
    regs[REG_RSI] = uc_mcontext.mc_rsi;
    regs[REG_RBP] = uc_mcontext.mc_rbp;
    regs[REG_RBX] = uc_mcontext.mc_rbx;
    regs[REG_RDX] = uc_mcontext.mc_rdx;
    regs[REG_RAX] = uc_mcontext.mc_rax;
    regs[REG_RCX] = uc_mcontext.mc_rcx;
    regs[REG_RSP] = uc_mcontext.mc_rsp;
    // regs[REG_CSGSFS] &= ~((greg_t{0xFFFF} << 32) | (greg_t{0xFFFF} << 16));
    // regs[REG_CSGSFS] |= (greg_t{uc_mcontext.mc_fs} << 32);
    // regs[REG_CSGSFS] |= (greg_t{uc_mcontext.mc_gs} << 16);
    regs[REG_RIP] = uc_mcontext.mc_rip;
#endif
#else
#error "ucontext_t conversion not implemented for current architecture."
#endif
}

std::array<Sigaction, 128> PosixActions{};
std::array<OrbisKernelExceptionHandler, 128> sceSigactionCallbacks{};
Sigset g_sigintr{};

#ifndef _WIN32
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
    default:
        // This is only needed for a few hardware signals now, so it needn't worry about about RT
        // ones anymore.
        UNREACHABLE_MSG("Signal {} has no job being here", s);
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
    s32 const val = sig - 1;
    if (val < 0 || val >= 0x80) {
        *Libraries::Kernel::__Error() = POSIX_EINVAL;
        return ORBIS_FAIL;
    }
    s->bits[val >> 5] |= 1 << (val & 0x1f);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_sigdelset(Sigset* s, s32 sig) {
    s32 const val = sig - 1;
    if (val < 0 || val >= 0x80) {
        *Libraries::Kernel::__Error() = POSIX_EINVAL;
        return ORBIS_FAIL;
    }
    s->bits[val >> 5] &= ~(1 << (val & 0x1f));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_sigismember(Sigset const* s, s32 sig) {
    s32 const val = sig - 1;
    if (val < 0 || val >= 0x80) {
        *Libraries::Kernel::__Error() = POSIX_EINVAL;
        return ORBIS_FAIL;
    }
    return ((s->bits[val >> 5] >> (val & 0x1f)) & 1) != 0;
}

bool PS4_SYSV_ABI posix_sigisemptyset(Sigset* s) {
    return s->bits[0] == 0 && s->bits[1] == 0 && s->bits[2] == 0 && s->bits[3] == 0;
}

s32 PS4_SYSV_ABI posix_sigaltstack(const OrbisKernelExceptionHandlerStack* ss,
                                   OrbisKernelExceptionHandlerStack* old_ss) {
    auto* thread = g_curthread;
    if (thread == nullptr) {
        SetPosixErrno(POSIX_EINVAL);
        return -1;
    }

    if (old_ss) {
        *old_ss = thread->sigaltstack;
    }

    if (ss == nullptr) {
        return 0;
    }

    if ((ss->ss_flags & ~(POSIX_SS_ONSTACK | POSIX_SS_DISABLE)) != 0) {
        SetPosixErrno(POSIX_EINVAL);
        return -1;
    }

    if ((ss->ss_flags & POSIX_SS_ONSTACK) && !(thread->sigaltstack.ss_flags & POSIX_SS_ONSTACK)) {
        // SS_ONSTACK is normally reported by the kernel rather than something
        // userspace explicitly sets when installing a stack.
        SetPosixErrno(POSIX_EINVAL);
        return -1;
    }

    constexpr s32 POSIX_MINSIGSTKSZ = (512 * 4); // from the freebsd source tree, todo validate

    if (!(ss->ss_flags & POSIX_SS_DISABLE) && ss->ss_size < POSIX_MINSIGSTKSZ) {
        SetPosixErrno(POSIX_ENOMEM);
        return -1;
    }

    thread->sigaltstack = *ss;

    if (!(ss->ss_flags & POSIX_SS_DISABLE)) {
        thread->sigaltstack.ss_flags &= ~POSIX_SS_ONSTACK;
    }

    return 0;
}

s32 PS4_SYSV_ABI posix_sigaction(s32 sig, Sigaction* act, Sigaction* oact) {
    if (sig < 1 || sig > 128 || sig == POSIX_SIGTHR || sig == POSIX_SIGKILL ||
        sig == POSIX_SIGSTOP) {
        *__Error() = POSIX_EINVAL;
        return ORBIS_FAIL;
    }
    if (oact != nullptr) {
        *oact = PosixActions[sig - 1];
    }

    if (act != nullptr) {
        PosixActions[sig - 1] = *act;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_pthread_sigmask(s32 how, const Sigset* set, Sigset* oset) {
    auto* thread = g_curthread;

    if (thread == nullptr) {
        return POSIX_EINVAL;
    }

    Sigset old_mask{};
    thread->GetGuestSigmask(old_mask);

    if (oset) {
        *oset = old_mask;
    }

    if (!set) {
        return ORBIS_OK;
    }

    Sigset new_mask = old_mask;

    switch (how) {
    case POSIX_SIG_BLOCK:
        for (size_t i = 0; i < 4; i++) {
            new_mask.bits[i] |= set->bits[i];
        }
        break;

    case POSIX_SIG_UNBLOCK:
        for (size_t i = 0; i < 4; i++) {
            new_mask.bits[i] &= ~set->bits[i];
        }
        break;

    case POSIX_SIG_SETMASK:
        new_mask = *set;
        break;

    default:
        return POSIX_EINVAL;
    }

    thread->SetGuestSigmask(new_mask);
    if (thread->HasDeliverableSignal()) {
        thread->WakeForSignal();
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_sigprocmask(s32 how, const Sigset* set, Sigset* oset) {
    return posix_pthread_sigmask(how, set, oset);
}

s32 PS4_SYSV_ABI posix_sigpending(Sigset* set) {
    posix_sigemptyset(set);

    for (s32 sig = 1; sig <= 128; sig++) {
        if (g_curthread->pending_signal_counts[sig - 1].load(std::memory_order_acquire) != 0) {
            posix_sigaddset(set, sig);
        }
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_sigsuspend(const Sigset* sigmask) {
    Sigset old_mask{};
    auto& thr = g_curthread;
    thr->GetGuestSigmask(old_mask);

    thr->sigsuspend_interrupted.store(false, std::memory_order_release);
    thr->in_sigsuspend.store(true, std::memory_order_release);

    thr->SetGuestSigmask(*sigmask);

    while (!thr->sigsuspend_interrupted.load(std::memory_order_acquire)) {
        thr->signal_sema.acquire();
    }

    thr->in_sigsuspend.store(false, std::memory_order_release);

    thr->SetGuestSigmask(old_mask);

    *__Error() = POSIX_EINTR;
    return ORBIS_FAIL;
}

s32 PS4_SYSV_ABI posix_sigwait(const Sigset* set, s32* sig) {
    if (set == nullptr || sig == nullptr) {
        return POSIX_EINVAL;
    }

    auto* thread = g_curthread;
    if (thread == nullptr) {
        return POSIX_EINVAL;
    }

    thread->sigwait_set = *set;
    thread->in_sigwait.store(true, std::memory_order_release);

    auto finish = [&] {
        thread->in_sigwait.store(false, std::memory_order_release);
        posix_sigemptyset(&thread->sigwait_set);
    };

    while (true) {
        const s32 pending = thread->FindPendingSignal(*set);

        if (pending != 0 && thread->ConsumeSignal(pending)) {
            *sig = pending;
            finish();
            return ORBIS_OK;
        }

        thread->signal_sema.acquire();
    }
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
    s32 const result = posix_sigaction(sig, &act, &oact);
    if (result >= ORBIS_OK) {
        return oact.__sigaction_handler.handler;
    }
    return reinterpret_cast<SigHandler>(-1);
}

s32 PS4_SYSV_ABI posix_pthread_kill(PthreadT thread, s32 sig) {
    if (sig < 1 || sig > 128) {
        return POSIX_EINVAL;
    }
    LOG_INFO(Lib_Kernel, "Raising signal {} on thread '{}'", sig, thread->name);
    thread->QueueSignal(sig);

    if (!thread->IsSignalBlocked(sig)) {
        thread->WakeForSignal();
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_raise(s32 sig) {
    return posix_pthread_kill(g_curthread, sig);
}

void PS4_SYSV_ABI sceCallbackHandler(int sig, Siginfo* info, Ucontext* context) {
    auto const cb = sceSigactionCallbacks[sig - 1];
    if (!cb) {
        UNREACHABLE(); // should be impossible unless race conditions perhaps
    }
    cb(sig, context);
}

// libkernel has a check in sceKernelInstallExceptionHandler and sceKernelRemoveExceptionHandler for
// validating if the application requested a handler for an allowed signal or not. However, that is
// just a wrapper for sigaction, which itself does not have any such restrictions, and therefore
// this check is ridiculously trivial to go around. This, however, means that we need to support all
// 127 - 3 possible signals, even if realistically, only homebrew will use most of them.
static std::unordered_set<s32> orbis_allowed_signals{
    POSIX_SIGHUP, POSIX_SIGILL, POSIX_SIGFPE, POSIX_SIGBUS, POSIX_SIGSEGV, POSIX_SIGUSR1,
};

s32 PS4_SYSV_ABI sceKernelInstallExceptionHandler(s32 signum, OrbisKernelExceptionHandler handler) {
    if (!orbis_allowed_signals.contains(signum)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (sceSigactionCallbacks[signum - 1] != nullptr) {
        return ORBIS_KERNEL_ERROR_EAGAIN;
    }
    sceSigactionCallbacks[signum - 1] = handler;
    LOG_INFO(Lib_Kernel, "Installing signal handler for {}", signum);
    Sigaction act = {};
    act.sa_flags = POSIX_SA_SIGINFO | POSIX_SA_RESTART;
    act.__sigaction_handler.sigaction = &sceCallbackHandler;
    posix_sigemptyset(&act.sa_mask);
    s32 const ret = posix_sigaction(signum, &act, nullptr);
    if (ret < 0) {
        LOG_ERROR(Lib_Kernel, "Failed to add handler for signal {}: {}", signum,
                  strerror(*__Error()));
        return ErrnoToSceKernelError(*__Error());
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelRemoveExceptionHandler(s32 signum) {
    if (!orbis_allowed_signals.contains(signum)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    sceSigactionCallbacks[signum - 1] = nullptr;
    Sigaction act = {};
    act.__sigaction_handler.handler = reinterpret_cast<SigHandler>(POSIX_SIG_DFL);
    posix_sigemptyset(&act.sa_mask);
    s32 const ret = posix_sigaction(signum, &act, nullptr);
    if (ret < 0) {
        LOG_ERROR(Lib_Kernel, "Failed to remove handler for signal {}: {}", signum,
                  strerror(*__Error()));
        return ErrnoToSceKernelError(*__Error());
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelRaiseException(PthreadT thread, s32 signum) {
    if (signum != POSIX_SIGUSR1) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    s32 const ret = posix_pthread_kill(thread, signum);
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
    LIB_FUNCTION("hpoDTzy9Yy0", "libkernel", 1, "libkernel", posix_sigpending);
    LIB_FUNCTION("0t0-MxQNwK4", "libkernel", 1, "libkernel", posix_raise);

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
    LIB_FUNCTION("hpoDTzy9Yy0", "libScePosix", 1, "libkernel", posix_sigpending);
    LIB_FUNCTION("0t0-MxQNwK4", "libScePosix", 1, "libkernel", posix_raise);
}

} // namespace Libraries::Kernel
