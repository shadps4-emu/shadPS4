// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/threads/exception.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/libs.h"

#ifdef _WIN64
#include "common/ntapi.h"
#else
#include <signal.h>
#endif

namespace Libraries::Kernel {

static std::array<SceKernelExceptionHandler, 32> Handlers{};

#ifndef _WIN64
void SigactionHandler(int signum, siginfo_t* inf, ucontext_t* raw_context) {
    const auto handler = Handlers[POSIX_SIGUSR1];
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
#endif
        handler(POSIX_SIGUSR1, &ctx);
    }
}
#else
void ExceptionHandler(void* arg1, void* arg2, void* arg3, PCONTEXT context) {
    const char* thrName = (char*)arg1;
    LOG_INFO(Lib_Kernel, "Exception raised successfully on thread '{}'", thrName);
    const auto handler = Handlers[POSIX_SIGUSR1];
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
        handler(POSIX_SIGUSR1, &ctx);
    }
}
#endif

int PS4_SYSV_ABI sceKernelInstallExceptionHandler(s32 signum, SceKernelExceptionHandler handler) {
    if (signum != POSIX_SIGUSR1) {
        LOG_ERROR(Lib_Kernel, "Installing non-supported exception handler for signal {}", signum);
        return 0;
    }
    ASSERT_MSG(!Handlers[POSIX_SIGUSR1], "Invalid parameters");
    Handlers[POSIX_SIGUSR1] = handler;
#ifndef _WIN64
    struct sigaction act = {};
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    act.sa_sigaction = reinterpret_cast<decltype(act.sa_sigaction)>(SigactionHandler);
    sigaction(SIGUSR2, &act, nullptr);
#endif
    return 0;
}

int PS4_SYSV_ABI sceKernelRemoveExceptionHandler(s32 signum) {
    if (signum != POSIX_SIGUSR1) {
        LOG_ERROR(Lib_Kernel, "Installing non-supported exception handler for signal {}", signum);
        return 0;
    }
    ASSERT_MSG(Handlers[POSIX_SIGUSR1], "Invalid parameters");
    Handlers[POSIX_SIGUSR1] = nullptr;
#ifndef _WIN64
    struct sigaction act = {};
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    act.sa_sigaction = nullptr;
    sigaction(SIGUSR2, &act, nullptr);
#endif
    return 0;
}

int PS4_SYSV_ABI sceKernelRaiseException(PthreadT thread, int signum) {
    LOG_WARNING(Lib_Kernel, "Raising exception on thread '{}'", thread->name);
    ASSERT_MSG(signum == POSIX_SIGUSR1, "Attempting to raise non user defined signal!");
#ifndef _WIN64
    const auto pthr = reinterpret_cast<pthread_t>(thread->native_thr.GetHandle());
    const auto ret = pthread_kill(pthr, SIGUSR2);
    if (ret != 0) {
        LOG_ERROR(Kernel, "Failed to send exception signal to thread '{}': {}", thread->name,
                  strerror(ret));
    }
#else
    USER_APC_OPTION option;
    option.UserApcFlags = QueueUserApcFlagsSpecialUserApc;

    u64 res = NtQueueApcThreadEx(reinterpret_cast<HANDLE>(thread->native_thr.GetHandle()), option,
                                 ExceptionHandler, (void*)thread->name.c_str(), nullptr, nullptr);
    ASSERT(res == 0);
#endif
    return 0;
}

s32 PS4_SYSV_ABI sceKernelDebugRaiseException(s32 error, s64 unk) {
    if (unk != 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    UNREACHABLE_MSG("error {:#x}", error);
    return 0;
}

s32 PS4_SYSV_ABI sceKernelDebugRaiseExceptionOnReleaseMode(s32 error, s64 unk) {
    if (unk != 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    UNREACHABLE_MSG("error {:#x}", error);
    return 0;
}

void RegisterException(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("il03nluKfMk", "libkernel_unity", 1, "libkernel", sceKernelRaiseException);
    LIB_FUNCTION("WkwEd3N7w0Y", "libkernel_unity", 1, "libkernel",
                 sceKernelInstallExceptionHandler);
    LIB_FUNCTION("Qhv5ARAoOEc", "libkernel_unity", 1, "libkernel", sceKernelRemoveExceptionHandler);
    LIB_FUNCTION("OMDRKKAZ8I4", "libkernel", 1, "libkernel", sceKernelDebugRaiseException);
    LIB_FUNCTION("zE-wXIZjLoM", "libkernel", 1, "libkernel",
                 sceKernelDebugRaiseExceptionOnReleaseMode);
}

} // namespace Libraries::Kernel
