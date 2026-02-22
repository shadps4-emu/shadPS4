// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/threads/exception.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/libs.h"
#include "core/signals.h"

#ifdef _WIN64
#include "common/ntapi.h"
#else
#include <csignal>
#endif

namespace Libraries::Kernel {

#ifdef _WIN32

// Windows doesn't have native versions of these, and we don't need to use them either.
static s32 NativeToOrbisSignal(s32 s) {
    return s;
}

static s32 OrbisToNativeSignal(s32 s) {
    return s;
}

#else

static s32 NativeToOrbisSignal(s32 s) {
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
        UNREACHABLE_MSG("Unknown signal {}", s);
    }
}

static s32 OrbisToNativeSignal(s32 s) {
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
    case POSIX_SIGUSR1:
        return SIGUSR1;
    case POSIX_SIGUSR2:
        return SIGUSR2;
    default:
        UNREACHABLE_MSG("Unknown signal {}", s);
    }
}

#endif

std::array<SceKernelExceptionHandler, 32> Handlers{};

#ifndef _WIN64
void SigactionHandler(int native_signum, siginfo_t* inf, ucontext_t* raw_context) {
    const auto handler = Handlers[native_signum];
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
        ctx.uc_mcontext.mc_gs = regs.__rip;
        ctx.uc_mcontext.mc_addr = reinterpret_cast<uint64_t>(inf->si_addr);
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
    }
}
#else
void ExceptionHandler(void* arg1, void* arg2, void* arg3, PCONTEXT context) {
    const char* thrName = (char*)arg1;
    int native_signum = reinterpret_cast<uintptr_t>(arg2);
    LOG_INFO(Lib_Kernel, "Exception raised successfully on thread '{}'", thrName);
    const auto handler = Handlers[native_signum];
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
    }
}
#endif

int PS4_SYSV_ABI sceKernelInstallExceptionHandler(s32 signum, SceKernelExceptionHandler handler) {
    if (signum > POSIX_SIGUSR2) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    LOG_INFO(Lib_Kernel, "Installing signal handler for {}", signum);
    int const native_signum = OrbisToNativeSignal(signum);
#ifdef __APPLE__
    ASSERT_MSG(native_signum != SIGVTALRM, "SIGVTALRM is HLE-reserved on macOS!");
#endif
    ASSERT_MSG(!Handlers[native_signum], "Invalid parameters");
    Handlers[native_signum] = handler;
#ifndef _WIN64
    if (native_signum == SIGSEGV || native_signum == SIGBUS || native_signum == SIGILL) {
        return ORBIS_OK; // These are handled in Core::SignalHandler
    }
    struct sigaction act = {};
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    act.sa_sigaction = reinterpret_cast<decltype(act.sa_sigaction)>(SigactionHandler);
    sigemptyset(&act.sa_mask);
    sigaction(native_signum, &act, nullptr);
#endif
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelRemoveExceptionHandler(s32 signum) {
    if (signum > POSIX_SIGUSR2) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    int const native_signum = OrbisToNativeSignal(signum);
    ASSERT_MSG(Handlers[native_signum], "Invalid parameters");
    Handlers[native_signum] = nullptr;
#ifndef _WIN64
    if (native_signum == SIGSEGV || native_signum == SIGBUS || native_signum == SIGILL) {
        struct sigaction action{};
        action.sa_sigaction = Core::SignalHandler;
        action.sa_flags = SA_SIGINFO | SA_ONSTACK;
        sigemptyset(&action.sa_mask);

        ASSERT_MSG(sigaction(native_signum, &action, nullptr) == 0,
                   "Failed to reinstate original signal handler for signal {}", native_signum);
    } else {
        struct sigaction act = {};
        act.sa_flags = SA_SIGINFO | SA_RESTART;
        act.sa_sigaction = nullptr;
        sigemptyset(&act.sa_mask);
        sigaction(native_signum, &act, nullptr);
    }
#endif
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelRaiseException(PthreadT thread, int signum) {
    if (signum != POSIX_SIGUSR1) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    LOG_WARNING(Lib_Kernel, "Raising exception on thread '{}'", thread->name);
    int const native_signum = OrbisToNativeSignal(signum);
#ifndef _WIN64
    const auto pthr = reinterpret_cast<pthread_t>(thread->native_thr.GetHandle());
    const auto ret = pthread_kill(pthr, native_signum);
    if (ret != 0) {
        LOG_ERROR(Kernel, "Failed to send exception signal to thread '{}': {}", thread->name,
                  strerror(ret));
    }
#else
    USER_APC_OPTION option;
    option.UserApcFlags = QueueUserApcFlagsSpecialUserApc;

    u64 res = NtQueueApcThreadEx(reinterpret_cast<HANDLE>(thread->native_thr.GetHandle()), option,
                                 ExceptionHandler, (void*)thread->name.c_str(),
                                 (void*)native_signum, nullptr);
    ASSERT(res == 0);
#endif
    return ORBIS_OK;
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
}

} // namespace Libraries::Kernel
