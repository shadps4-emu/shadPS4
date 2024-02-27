// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/types.h"
#include "core/tls.h"

namespace Core {

thread_local u8 TLS[1024];

uintptr_t GetGuestTls(s64 tls_offset) {
    if (tls_offset == 0) {
        return reinterpret_cast<uintptr_t>(TLS);
    }
    UNREACHABLE_MSG("Unimplemented offset info tls");
}

#ifdef _WIN64
static LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS pExp) noexcept {
    auto orig_rip = pExp->ContextRecord->Rip;
    while (*(u8*)pExp->ContextRecord->Rip == 0x66) {
        pExp->ContextRecord->Rip++;
    }

    if (*(u8*)pExp->ContextRecord->Rip == 0xcd) {
        int reg = *(u8*)(pExp->ContextRecord->Rip + 1) - 0x80;
        int sizes = *(u8*)(pExp->ContextRecord->Rip + 2);
        int pattern_size = sizes & 0xF;
        int imm_size = sizes >> 4;

        int64_t tls_offset;
        if (imm_size == 4) {
            tls_offset = *(s32*)(pExp->ContextRecord->Rip + pattern_size);
        } else {
            tls_offset = *(s64*)(pExp->ContextRecord->Rip + pattern_size);
        }

        (&pExp->ContextRecord->Rax)[reg] = GetGuestTls(tls_offset); /* GetGuestTls */
        pExp->ContextRecord->Rip += pattern_size + imm_size;

        return EXCEPTION_CONTINUE_EXECUTION;
    }

    pExp->ContextRecord->Rip = orig_rip;
    const u32 ec = pExp->ExceptionRecord->ExceptionCode;
    switch (ec) {
    case EXCEPTION_ACCESS_VIOLATION: {
        LOG_CRITICAL(Core, "Exception EXCEPTION_ACCESS_VIOLATION ({:#x})", ec);
        const auto info = pExp->ExceptionRecord->ExceptionInformation;
        switch (info[0]) {
        case 0:
            LOG_CRITICAL(Core, "Read violation at address {:#x}", info[1]);
            break;
        case 1:
            LOG_CRITICAL(Core, "Write violation at address {:#x}", info[1]);
            break;
        case 8:
            LOG_CRITICAL(Core, "DEP violation at address {:#x}", info[1]);
            break;
        default:
            break;
        }
        break;
    }
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        LOG_CRITICAL(Core, "Exception EXCEPTION_ARRAY_BOUNDS_EXCEEDED ({:#x})", ec);
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        LOG_CRITICAL(Core, "Exception EXCEPTION_DATATYPE_MISALIGNMENT ({:#x})", ec);
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        LOG_CRITICAL(Core, "Exception EXCEPTION_FLT_DIVIDE_BY_ZERO ({:#x})", ec);
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        LOG_CRITICAL(Core, "Exception EXCEPTION_ILLEGAL_INSTRUCTION ({:#x})", ec);
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        LOG_CRITICAL(Core, "Exception EXCEPTION_IN_PAGE_ERROR ({:#x})", ec);
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        LOG_CRITICAL(Core, "Exception EXCEPTION_INT_DIVIDE_BY_ZERO ({:#x})", ec);
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        LOG_CRITICAL(Core, "Exception EXCEPTION_PRIV_INSTRUCTION ({:#x})", ec);
        break;
    case EXCEPTION_STACK_OVERFLOW:
        LOG_CRITICAL(Core, "Exception EXCEPTION_STACK_OVERFLOW ({:#x})", ec);
        break;
    default:
        return EXCEPTION_CONTINUE_SEARCH;
    }
    Flush();
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void InstallTlsHandler() {
#ifdef _WIN64
    if (!AddVectoredExceptionHandler(0, ExceptionHandler)) {
        LOG_CRITICAL(Core, "Failed to register an exception handler");
    }
#endif
}

} // namespace Core
