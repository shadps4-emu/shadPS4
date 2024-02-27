// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/types.h"
#include "core/tls.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace Core {

thread_local u8 TLS[1024];

struct TLSPattern {
    uint8_t pattern[5];
    uint8_t pattern_size;
    uint8_t imm_size;
    uint8_t target_reg;
};

constexpr static TLSPattern TlsPatterns[] = {
    {{0x64, 0x48, 0xA1},
     3,
     8,
     0}, // 64 48 A1 | 00 00 00 00 00 00 00 00 # mov rax, qword ptr fs:[64b imm]

    {{0x64, 0x48, 0x8B, 0x4, 0x25},
     5,
     4,
     0}, // 64 48 8B 04 25 | 00 00 00 00 # mov rax,qword ptr fs:[0]
    {{0x64, 0x48, 0x8B, 0xC, 0x25}, 5, 4, 1},   // rcx
    {{0x64, 0x48, 0x8B, 0x14, 0x25}, 5, 4, 2},  // rdx
    {{0x64, 0x48, 0x8B, 0x1C, 0x25}, 5, 4, 3},  // rbx
    {{0x64, 0x48, 0x8B, 0x24, 0x25}, 5, 4, 4},  // rsp
    {{0x64, 0x48, 0x8B, 0x2C, 0x25}, 5, 4, 5},  // rbp
    {{0x64, 0x48, 0x8B, 0x34, 0x25}, 5, 4, 6},  // rsi
    {{0x64, 0x48, 0x8B, 0x3C, 0x25}, 5, 4, 7},  // rdi
    {{0x64, 0x4C, 0x8B, 0x4, 0x25}, 5, 4, 8},   // r8
    {{0x64, 0x4C, 0x8B, 0xC, 0x25}, 5, 4, 9},   // r9
    {{0x64, 0x4C, 0x8B, 0x14, 0x25}, 5, 4, 10}, // r10
    {{0x64, 0x4C, 0x8B, 0x1C, 0x25}, 5, 4, 11}, // r11
    {{0x64, 0x4C, 0x8B, 0x24, 0x25}, 5, 4, 12}, // r12
    {{0x64, 0x4C, 0x8B, 0x2C, 0x25}, 5, 4, 13}, // r13
    {{0x64, 0x4C, 0x8B, 0x34, 0x25}, 5, 4, 14}, // r14
    {{0x64, 0x4C, 0x8B, 0x3C, 0x25}, 5, 4, 15}, // r15
};

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

void PatchTLS(u64 segment_addr, u64 segment_size) {
    u8* code = reinterpret_cast<u8*>(segment_addr);
    auto remaining_size = segment_size;

    while (remaining_size) {
        for (const auto& tls_pattern : TlsPatterns) {
            const auto total_size = tls_pattern.pattern_size + tls_pattern.imm_size;
            if (remaining_size < total_size) {
                continue;
            }
            if (std::memcmp(code, tls_pattern.pattern, tls_pattern.pattern_size) != 0) {
                continue;
            }
            if (tls_pattern.imm_size == 4) {
                LOG_INFO(Core_Linker, "PATTERN32 FOUND at {}, reg: {} offset: {:#x}",
                         fmt::ptr(code), tls_pattern.target_reg,
                         *(u32*)(code + tls_pattern.pattern_size));
            } else {
                LOG_INFO(Core_Linker, "PATTERN64 FOUND at {}, reg: {} offset: {:#x}",
                         fmt::ptr(code), tls_pattern.target_reg,
                         *(u32*)(code + tls_pattern.pattern_size));
            }
            code[0] = 0xcd;
            code[1] = 0x80 + tls_pattern.target_reg;
            code[2] = tls_pattern.pattern_size | (tls_pattern.imm_size << 4);
            code += total_size - 1;
            remaining_size -= total_size - 1;
            break;
        }
        code++;
        remaining_size--;
    }
}

} // namespace Core
