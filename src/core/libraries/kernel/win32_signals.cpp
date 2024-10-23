// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/orbis_signals.h"

#ifdef _WIN64
#include "common/assert.h"
#include "common/enum.h"
#include "common/error.h"
#include "common/types.h"
#include "core/libraries/error_codes.h"

#include "pthread.h"
extern "C" {
// Hack to get winpthreads internal state to get detached thread handle
#include "externals/winpthreads/src/thread.h"
}

#include <Windows.h>

#include <atomic>
#include <vector>

#define NTDLL_VERSION_WIN10_RS5_1809 17763
#define DR7_LAST_BRANCH 0x100

enum QUEUE_USER_APC_FLAGS {
    QUEUE_USER_APC_FLAGS_NONE = 0x00000000,
    QUEUE_USER_APC_FLAGS_SPECIAL_USER_APC = 0x00000001,
    QUEUE_USER_APC_CALLBACK_DATA_CONTEXT = 0x00010000,
};

using NTSTATUS = LONG;
using PPS_APC_ROUTINE = VOID(NTAPI*)(PVOID, PVOID, PVOID, PCONTEXT);
union USER_APC_OPTION {
    ULONG_PTR UserApcFlags;
    HANDLE MemoryReserveHandle;
};

using PNT_QUEUE_APC_THREAD_EX = NTSTATUS(NTAPI*)(HANDLE, USER_APC_OPTION, PPS_APC_ROUTINE, PVOID,
                                                 PVOID, PVOID);

static PNT_QUEUE_APC_THREAD_EX pfnNtQueueApcThreadEx;

struct Orbis::sigaction g_sigacts[ORBIS_SIG_MAXSIG32];

static FARPROC GetModuleProcAddr(const wchar_t* dll, u16 min_version, const char* proc_name) {
    // DWORD ver_handle = 0;
    // DWORD ver_size = GetFileVersionInfoSizeW(dll, &ver_handle);
    // if (ver_size == 0) {
    //     UNREACHABLE_MSG("Could not get {} version", dll);
    // }
    // std::vector<s8> ver_buf(ver_size, 0);
    // if (GetFileVersionInfoW(dll, ver_handle, ver_size, ver_buf.data()) == FALSE) {
    //     UNREACHABLE_MSG("Could not get {} version", dll);
    // }

    // UINT size = 0;
    // LPBYTE buffer = NULL;
    // if (VerQueryValueW(ver_buf.data(), L"\\", (VOID FAR * FAR*)&buffer, &size) == FALSE ||
    //     size == 0) {
    //     UNREACHABLE_MSG("Could not get {} version", dll);
    // }

    // const VS_FIXEDFILEINFO* p_ver = (VS_FIXEDFILEINFO*)buffer;
    // const u16 version = (p_ver->dwProductVersionLS >> 16 & 0xFFFF);
    // ASSERT_MSG(version >= min_version, "Your Windows version is too old, please update.");

    return GetProcAddress(GetModuleHandleW(dll), proc_name);
}

NTSTATUS NtQueueApcThreadEx(HANDLE ThreadHandle, USER_APC_OPTION UserApcOption,
                            PPS_APC_ROUTINE ApcRoutine, PVOID arg1, PVOID arg2, PVOID arg3) {
    LOG_INFO(Lib_Kernel, "called");
    static const auto pfn = reinterpret_cast<PNT_QUEUE_APC_THREAD_EX>(
        GetModuleProcAddr(L"ntdll.dll", NTDLL_VERSION_WIN10_RS5_1809, "NtQueueApcThreadEx"));
    return pfn(ThreadHandle, UserApcOption, ApcRoutine, arg1, arg2, arg3);
}

namespace Orbis {

static uintptr_t SigDfl(int sig) {
    switch (sig) {
    case ORBIS_SIGURG:
    case ORBIS_SIGCHLD:
    case ORBIS_SIGWINCH:
    case ORBIS_SIGINFO:
        return ORBIS_SIG_IGN;
    default:
        return ORBIS_SIG_ERR;
    }
}

static uintptr_t SigActType(s32 sig, const struct sigaction& act) {
    switch (reinterpret_cast<uintptr_t>(act.sa_sigaction)) {
    case ORBIS_SIG_DFL:
        return SigDfl(sig);
    case ORBIS_SIG_IGN:
        return ORBIS_SIG_IGN;
    case ORBIS_SIG_ERR:
        return ORBIS_SIG_ERR;
    case ORBIS_SIG_CATCH:
        return ORBIS_SIG_ERR;
    case ORBIS_SIG_HOLD:
        return ORBIS_SIG_ERR;
    default:
        return ORBIS_SIG_DFL;
    }
}

s32 sigaction(s32 sig, const struct sigaction* act, struct sigaction* oldact) {
    LOG_INFO(Lib_Kernel, "called. sig = {}", sig);
    if (sig > ORBIS_SIG_MAXSIG32) {
        return POSIX_EINVAL;
    }

    struct sigaction tmp = {};
    if (act != nullptr) {
        tmp = *act;
    }

    switch (sig) {
    case ORBIS_SIGKILL:
    case ORBIS_SIGSTOP: {
        if (tmp.sa_sigaction != ORBIS_SIG_DFL) {
            return POSIX_EINVAL;
        }
    }
    default:
        break;
    }

    auto& info = g_sigacts[ORBIS_SIG_IDX(sig)];
    if (oldact != nullptr) {
        *oldact = info;
    }
    info = tmp;

    return 0;
}

#pragma pack(push, 1)
struct YMMCONTEXT {
    M128A Ymm0;
    M128A Ymm1;
    M128A Ymm2;
    M128A Ymm3;
    M128A Ymm4;
    M128A Ymm5;
    M128A Ymm6;
    M128A Ymm7;
    M128A Ymm8;
    M128A Ymm9;
    M128A Ymm10;
    M128A Ymm11;
    M128A Ymm12;
    M128A Ymm13;
    M128A Ymm14;
    M128A Ymm15;
};

struct XSTATE {
    DWORD64 Mask;
    DWORD64 CompactionMask;
    DWORD64 Reserved[6];
    YMMCONTEXT YmmContext;
};
using PXSTATE = XSTATE*;
#pragma pack(pop)

// Taken from fpPS4
ucontext_t UContextFromWin64(CONTEXT& context) {
    ucontext_t ucontext{};

    const u32 flags = context.ContextFlags & (~CONTEXT_AMD64);
    if ((flags & CONTEXT_INTEGER) != 0) {
        ucontext.uc_flags |= ORBIS_UC_CPU;
        ucontext.uc_mcontext.mc_rax = context.Rax;
        ucontext.uc_mcontext.mc_rbx = context.Rbx;
        ucontext.uc_mcontext.mc_rcx = context.Rcx;
        ucontext.uc_mcontext.mc_rdx = context.Rdx;
        ucontext.uc_mcontext.mc_rsi = context.Rsi;
        ucontext.uc_mcontext.mc_rdi = context.Rdi;
        ucontext.uc_mcontext.mc_r8 = context.R8;
        ucontext.uc_mcontext.mc_r9 = context.R9;
        ucontext.uc_mcontext.mc_r10 = context.R10;
        ucontext.uc_mcontext.mc_r11 = context.R11;
        ucontext.uc_mcontext.mc_r12 = context.R12;
        ucontext.uc_mcontext.mc_r13 = context.R13;
        ucontext.uc_mcontext.mc_r14 = context.R14;
        ucontext.uc_mcontext.mc_r15 = context.R15;
    }
    if ((flags & CONTEXT_CONTROL) != 0) {
        ucontext.uc_flags |= ORBIS_UC_CPU;
        ucontext.uc_mcontext.mc_rsp = context.Rsp;
        ucontext.uc_mcontext.mc_rbp = context.Rbp;
        ucontext.uc_mcontext.mc_rip = context.Rip;
        ucontext.uc_mcontext.mc_rflags = context.EFlags;
        ucontext.uc_mcontext.mc_cs = context.SegCs;
        ucontext.uc_mcontext.mc_ss = context.SegSs;
    }
    if ((flags & CONTEXT_SEGMENTS) != 0) {
        ucontext.uc_flags |= ORBIS_UC_CPU;
        ucontext.uc_mcontext.mc_ds = context.SegDs;
        ucontext.uc_mcontext.mc_es = context.SegEs;
        ucontext.uc_mcontext.mc_fs = context.SegFs;
        ucontext.uc_mcontext.mc_gs = context.SegGs;
    }

    auto uc_xsave = reinterpret_cast<PXMM_SAVE_AREA32>(&ucontext.uc_mcontext.mc_fpstate[0]);
    auto uc_xstate = reinterpret_cast<PXSTATE>(uc_xsave + 1);
    if ((flags & CONTEXT_FLOATING_POINT) != 0) {
        ucontext.uc_flags |= ORBIS_UC_FPU;
        *uc_xsave = context.FltSave;
        uc_xstate->Mask = uc_xstate->Mask | XSTATE_MASK_LEGACY;
    }
    if ((flags & CONTEXT_XSTATE) != 0) {
        DWORD64 xs_mask{};
        ASSERT(GetXStateFeaturesMask(&context, &xs_mask) == TRUE);
        if ((xs_mask & XSTATE_MASK_AVX) != 0) {
            DWORD length;
            auto xs_ymm =
                reinterpret_cast<YMMCONTEXT*>(LocateXStateFeature(&context, XSTATE_AVX, &length));
            ASSERT(length >= sizeof(YMMCONTEXT));
            uc_xstate->YmmContext = *xs_ymm;
        }
    }
    if ((flags & CONTEXT_DEBUG_REGISTERS) != 0) {
        ucontext.uc_mcontext.mc_spare[0] = context.Dr0;
        ucontext.uc_mcontext.mc_spare[1] = context.Dr1;
        ucontext.uc_mcontext.mc_spare[2] = context.Dr2;
        ucontext.uc_mcontext.mc_spare[3] = context.Dr3;
        ucontext.uc_mcontext.mc_spare[4] = context.Dr6;
        ucontext.uc_mcontext.mc_spare[5] = context.Dr7;
    }

    // fix me
    if (context.Dr7 & DR7_LAST_BRANCH) {
        ucontext.uc_mcontext.mc_lbrfrom = context.LastBranchFromRip;
        ucontext.uc_mcontext.mc_lbrto = context.LastBranchToRip;
    } else {
        ucontext.uc_mcontext.mc_lbrfrom = 0; // context.Rsp;
        ucontext.uc_mcontext.mc_lbrto = 0;   // context.Rbp;
    }

    ucontext.uc_flags |= (flags << 8); // set as extended

    ucontext.uc_mcontext.mc_addr = ucontext.uc_mcontext.mc_rip;
    ucontext.uc_mcontext.mc_len = sizeof(mcontext_t);
    ucontext.uc_mcontext.mc_flags = ORBIS_MC_HASSEGS;
    ucontext.uc_mcontext.mc_fpformat = ORBIS_MC_FPFMT_XMM;
    ucontext.uc_mcontext.mc_ownedfp = ORBIS_MC_FPOWNED_FPU;

    return ucontext;
}

static thread_local sigset_t g_sc_mask;

VOID NTAPI ThreadApcProc(PVOID arg1, PVOID arg2, PVOID arg3, PCONTEXT context) {
    int sig = static_cast<int>(reinterpret_cast<intptr_t>(arg1));
    LOG_INFO(Lib_Kernel, "called. sig = {}", sig);
    const auto sact = g_sigacts[ORBIS_SIG_IDX(sig)];
    if (sact.sa_flags & ORBIS_SA_RESETHAND) {
        g_sigacts[ORBIS_SIG_IDX(sig)] = {};
    }
    switch (SigActType(sig, sact)) {
    case ORBIS_SIG_IGN:
    case ORBIS_SIG_ERR:
        return;
    }
    ucontext_t ucontext = UContextFromWin64(*context);
    ucontext.uc_mcontext.mc_err = errno;

    sigset_t save = g_sc_mask;
    if (sact.sa_flags & ORBIS_SA_NODEFER) {
        ucontext.sc_mask = sact.sa_mask;
    } else {
        g_sc_mask.__bits[0] |= sact.sa_mask.__bits[0];
        g_sc_mask.__bits[1] |= sact.sa_mask.__bits[1];
        g_sc_mask.__bits[2] |= sact.sa_mask.__bits[2];
        g_sc_mask.__bits[3] |= sact.sa_mask.__bits[3];
        ucontext.sc_mask = g_sc_mask;
    }

    if (sact.sa_flags & ORBIS_SA_SIGINFO) {
        siginfo_t info = {
            .si_signo = sig,
            .si_code = ORBIS_SI_USER,
            .si_pid = GetCurrentProcessId(),
        };
        sact.sa_sigaction(sig, &info, &ucontext);
    } else {
        sact.sa_handler(sig, ORBIS_SI_USER, &ucontext);
    }
    g_sc_mask = save;
}

s32 pthread_kill(pthread_t thread, s32 sig) {
    int result = 0;
    LOG_INFO(Lib_Kernel, "called. sig = {}", sig);
    if (sig == 0) {
        return result;
    }
    const auto tv = __pth_gpointer_locked(thread);
    const auto handle = OpenThread(THREAD_SET_CONTEXT, FALSE, tv->tid);
    USER_APC_OPTION option;
    option.UserApcFlags = QUEUE_USER_APC_FLAGS_SPECIAL_USER_APC;

    if (NtQueueApcThreadEx(handle, option, &ThreadApcProc,
                           reinterpret_cast<PVOID>(static_cast<intptr_t>(sig)), nullptr,
                           nullptr) != 0) {
        const auto error = GetLastError();
        LOG_ERROR(Lib_Kernel, "NtQueueApcThreadEx failed. error = {}: {}", error,
                  Common::NativeErrorToString(error));
        // TODO: error conversion
        // result = ???
    } else if (tv->evStart != nullptr) {
        SetEvent(tv->evStart);
    }
    CloseHandle(handle);
    return result;
}

} // namespace Orbis
#endif
