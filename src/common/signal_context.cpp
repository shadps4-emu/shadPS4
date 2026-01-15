// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/arch.h"
#include "common/assert.h"
#include "common/signal_context.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__FreeBSD__)
#include <sys/ucontext.h>
#include <machine/npx.h>
#else
#include <sys/ucontext.h>
#endif

namespace Common {

void* GetXmmPointer(void* ctx, u8 index) {
#if defined(_WIN32)
#define CASE(index)                                                                                \
    case index:                                                                                    \
        return (void*)(&((EXCEPTION_POINTERS*)ctx)->ContextRecord->Xmm##index.Low)
#elif defined(__APPLE__)
#define CASE(index)                                                                                \
    case index:                                                                                    \
        return (void*)(&((ucontext_t*)ctx)->uc_mcontext->__fs.__fpu_xmm##index);
#elif defined(__FreeBSD__)
    // In mc_fpstate
    // See <machine/npx.h> for the internals of mc_fpstate[].
#define CASE(index) \
    case index: { \
        auto& mctx = ((ucontext_t*)ctx)->uc_mcontext; \
        ASSERT(mctx.mc_fpformat == _MC_FPFMT_XMM); \
        auto* s_fpu = (struct savefpu*)(&mctx.mc_fpstate[0]); \
        return (void*)(&(s_fpu->sv_xmm[0])); \
    }
#else
#define CASE(index)                                                                                \
    case index:                                                                                    \
        return (void*)(&((ucontext_t*)ctx)->uc_mcontext.fpregs->_xmm[index].element[0])
#endif
    switch (index) {
        CASE(0);
        CASE(1);
        CASE(2);
        CASE(3);
        CASE(4);
        CASE(5);
        CASE(6);
        CASE(7);
        CASE(8);
        CASE(9);
        CASE(10);
        CASE(11);
        CASE(12);
        CASE(13);
        CASE(14);
        CASE(15);
    default: {
        UNREACHABLE_MSG("Invalid XMM register index: {}", index);
        return nullptr;
    }
    }
#undef CASE
}

void* GetRip(void* ctx) {
#if defined(_WIN32)
    return (void*)((EXCEPTION_POINTERS*)ctx)->ContextRecord->Rip;
#elif defined(__APPLE__)
    return (void*)((ucontext_t*)ctx)->uc_mcontext->__ss.__rip;
#elif defined(__FreeBSD__)
    return (void*)((ucontext_t*)ctx)->uc_mcontext.mc_rip;
#else
    return (void*)((ucontext_t*)ctx)->uc_mcontext.gregs[REG_RIP];
#endif
}

void IncrementRip(void* ctx, u64 length) {
#if defined(_WIN32)
    ((EXCEPTION_POINTERS*)ctx)->ContextRecord->Rip += length;
#elif defined(__APPLE__)
    ((ucontext_t*)ctx)->uc_mcontext->__ss.__rip += length;
#elif defined(__FreeBSD__)
    ((ucontext_t*)ctx)->uc_mcontext.mc_rip += length;
#else
    ((ucontext_t*)ctx)->uc_mcontext.gregs[REG_RIP] += length;
#endif
}

bool IsWriteError(void* ctx) {
#if defined(_WIN32)
    return ((EXCEPTION_POINTERS*)ctx)->ExceptionRecord->ExceptionInformation[0] == 1;
#elif defined(__APPLE__) && defined(ARCH_X86_64)
    return ((ucontext_t*)ctx)->uc_mcontext->__es.__err & 0x2;
#elif defined(__APPLE__) && defined(ARCH_ARM64)
    return ((ucontext_t*)ctx)->uc_mcontext->__es.__esr & 0x40;
#elif defined(__FreeBSD__) && defined(ARCH_X86_64)
    return ((ucontext_t*)ctx)->uc_mcontext.mc_err & 0x2;
#elif defined(ARCH_X86_64)
    return ((ucontext_t*)ctx)->uc_mcontext.gregs[REG_ERR] & 0x2;
#else
#error "Unsupported architecture"
#endif
}
} // namespace Common
