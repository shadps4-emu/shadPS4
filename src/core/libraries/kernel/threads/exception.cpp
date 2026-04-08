// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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

#ifndef _WIN64
void SigactionHandler(int native_signum, siginfo_t* inf, ucontext_t* raw_context) {
    const auto handler = Handlers[NativeToOrbisSignal(native_signum)];
    if (handler) {
        auto ctx = Ucontext{};
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
    return 0;
}

bool PS4_SYSV_ABI posix_sigisemptyset(Sigset* s) {
    return s->bits[0] == 0 && s->bits[1] == 0;
}

s32 PS4_SYSV_ABI posix_sigalstack(const OrbisKernelExceptionHandlerStack* ss,
                                  OrbisKernelExceptionHandlerStack* old_ss) {
#ifdef __unix__
    stack_t native_ss{};
    if (ss) {
        native_ss.ss_sp = ss->ss_sp;
        native_ss.ss_flags = ss->ss_flags;
        native_ss.ss_size = ss->ss_size;
    }
    stack_t native_old_ss{};
    sigaltstack(&native_ss, &native_old_ss);
    if (old_ss) {
        old_ss->ss_sp = native_old_ss.ss_sp;
        old_ss->ss_flags = native_old_ss.ss_flags;
        old_ss->ss_size = native_old_ss.ss_size;
    }
#else
    LOG_ERROR(Lib_Kernel, "(stubbed)");
#endif
    return ORBIS_OK;
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
#else
    s32 native_sig = OrbisToNativeSignal(sig);
    if (native_sig == SIGVTALRM) {
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
        native_act.sa_flags = act->sa_flags; // todo check compatibility, on Linux it seems fine
        native_act.sa_sigaction =
            reinterpret_cast<decltype(native_act.sa_sigaction)>(SigactionHandler);
        if (!posix_sigisemptyset(&act->sa_mask)) {
            LOG_ERROR(Lib_Kernel, "Unhandled sa_mask: {:x}", act->sa_mask.bits[0]);
        }
    }
    auto const prev_handler = Handlers[sig];
    Handlers[sig] = reinterpret_cast<OrbisKernelExceptionHandler>(
        act ? act->__sigaction_handler.sigaction : nullptr);

    if (native_sig == SIGSEGV || native_sig == SIGBUS || native_sig == SIGILL) {
        return ORBIS_OK; // These are handled in Core::SignalHandler
    }
    if (native_sig > 127) {
        LOG_WARNING(Lib_Kernel, "We can't install a handler for native signal {}!", native_sig);
        return ORBIS_OK;
    }
    struct sigaction native_oact{};
    s32 ret = sigaction(native_sig, act ? &native_act : nullptr, oact ? &native_oact : nullptr);
    if (oact) {
        oact->sa_flags = native_oact.sa_flags;
        oact->__sigaction_handler.sigaction =
            reinterpret_cast<decltype(oact->__sigaction_handler.sigaction)>(prev_handler);
        if (!sigisemptyset(&native_oact.sa_mask)) {
            LOG_ERROR(Lib_Kernel, "Unhandled sa_mask");
        }
    }
    if (ret < 0) {
        LOG_ERROR(Lib_Kernel, "sigaction failed: {}", strerror(errno));
        *__Error() = ErrnoToSceKernelError(errno);
        return ORBIS_FAIL;
    }
#endif
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_pthread_kill(PthreadT thread, s32 sig) {
    if (sig < 1 || sig > 128) { // off-by-one error?
        return POSIX_EINVAL;
    }
    LOG_WARNING(Lib_Kernel, "Raising signal {} on thread '{}'", sig, thread->name);
    int const native_signum = OrbisToNativeSignal(sig);
#ifndef _WIN64
    const auto pthr = reinterpret_cast<pthread_t>(thread->native_thr.GetHandle());
    const auto ret = pthread_kill(pthr, native_signum);
    if (ret != 0) {
        LOG_ERROR(Kernel, "Failed to send exception signal to thread '{}': {}", thread->name,
                  strerror(errno));
    }
#else
    USER_APC_OPTION option;
    option.UserApcFlags = QueueUserApcFlagsSpecialUserApc;

    u64 res = NtQueueApcThreadEx(reinterpret_cast<HANDLE>(thread->native_thr.GetHandle()), option,
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

s32 PS4_SYSV_ABI sceKernelDebugRaiseException(s32 error, s64 unk) {
    if (unk != 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    UNREACHABLE_MSG("error {:#x}", error);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelDebugRaiseExceptionOnReleaseMode(s32 error, s64 unk) {
    if (unk != 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    UNREACHABLE_MSG("error {:#x}", error);
    return ORBIS_OK;
}

void RegisterException(Core::Loader::SymbolsResolver* sym) {
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
    LIB_FUNCTION("+F7C-hdk7+E", "libkernel", 1, "libkernel", posix_sigemptyset);
    LIB_FUNCTION("yH-uQW3LbX0", "libkernel", 1, "libkernel", posix_pthread_kill);
    LIB_FUNCTION("sHziAegVp74", "libkernel", 1, "libkernel", posix_sigalstack);

    LIB_FUNCTION("KiJEPEWRyUY", "libScePosix", 1, "libkernel", posix_sigaction);
    LIB_FUNCTION("+F7C-hdk7+E", "libScePosix", 1, "libkernel", posix_sigemptyset);
    LIB_FUNCTION("yH-uQW3LbX0", "libScePosix", 1, "libkernel", posix_pthread_kill);
    LIB_FUNCTION("sHziAegVp74", "libScePosix", 1, "libkernel", posix_sigalstack);
}

} // namespace Libraries::Kernel
