// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/arch.h"
#include "common/assert.h"
#include "core/signals.h"

#ifdef _WIN32
#include <intrin.h>
#include <windows.h>
#else
#include <csignal>
#include <cpuid.h>
#ifdef ARCH_X86_64
#include <Zydis/Decoder.h>
#include <Zydis/Formatter.h>
#endif
#endif

// CPUID constants from http://instlatx64.atw.hu/
// There aren't dumps for the PS4 itself, these are from a 7600x
// Optimally at some point we should dump the PS4 CPUID values
// For EAX=0 / EAX=0x80000000
constexpr static u32 maxCpuid = 0x10;
constexpr static const char* vendorId = "AuthenticAMD";

// For EAX=1
constexpr static u32 processorFamilyIDs = 0x00A60F12;
constexpr static u32 additionalInfo = 0x000C0800;
constexpr static u32 featureInfoEcx = 0x7EF8320B;
constexpr static u32 featureInfoEdx = 0x178BFBFF;

// For EAX=0x80000001
constexpr static u32 extendedFeatureInfoEax = 0x00A60F12;
constexpr static u32 extendedFeatureInfoEbx = 0x00000000;
constexpr static u32 extendedFeatureInfoEcx = 0x75C237FF;
constexpr static u32 extendedFeatureInfoEdx = 0x2FD3FBFF;

namespace Core {

#if defined(_WIN32)

static LONG WINAPI SignalHandler(EXCEPTION_POINTERS* pExp) noexcept {
    const auto* signals = Signals::Instance();

    auto* code_address = reinterpret_cast<void*>(pExp->ContextRecord->Rip);

    bool handled = false;
    switch (pExp->ExceptionRecord->ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
        handled = signals->DispatchAccessViolation(
            code_address, reinterpret_cast<void*>(pExp->ExceptionRecord->ExceptionInformation[1]),
            pExp->ExceptionRecord->ExceptionInformation[0] == 1);
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION: {
        u8 opcode = *reinterpret_cast<u8*>(code_address);
        if (opcode == 0x37) { // 0x37 is AAA instruction, we use that to patch CPUID
            int results[4];   // that's the type __cpuidex expects
            int rax = (int)(u32)pExp->ContextRecord->Rax;
            int rcx = (int)(u32)pExp->ContextRecord->Rcx;
            LOG_ERROR(Core, "CPUID - RAX BEFORE: 0x{:x}, RCX BEFORE: 0x{:x}", rax, rcx);
            if (rax == 1) {
                results[0] = processorFamilyIDs;
                results[1] = additionalInfo;
                results[2] = featureInfoEcx;
                results[3] = featureInfoEdx;
            } else if (rax == 0x80000001) {
                results[0] = extendedFeatureInfoEax;
                results[1] = extendedFeatureInfoEbx;
                results[2] = extendedFeatureInfoEcx;
                results[3] = extendedFeatureInfoEdx;
            } else {
                __cpuidex(reinterpret_cast<int*>(results), static_cast<int>(rax),
                      static_cast<int>(rcx));
                if (rax == 0 || rax == 0x80000000) {
                    // The order looks weird but is correct
                    // The string is saved in order of EBX, EDX, ECX
                    memcpy(&results[1], vendorId, 4);
                    memcpy(&results[3], vendorId + 4, 4);
                    memcpy(&results[2], vendorId + 8, 4);
                }
            }
            pExp->ContextRecord->Rax = results[0];
            pExp->ContextRecord->Rbx = results[1];
            pExp->ContextRecord->Rcx = results[2];
            pExp->ContextRecord->Rdx = results[3];
            pExp->ContextRecord->Rip += 1; // skip the illegal instruction
            LOG_ERROR(Core, "CPUID - RAX AFTER: 0x{:x}, RBX AFTER: 0x{:x}, RCX AFTER: 0x{:x}, RDX AFTER: 0x{:x}",
                      results[0], results[1], results[2], results[3]);
            handled = EXCEPTION_CONTINUE_EXECUTION;
        } else {
            handled = signals->DispatchIllegalInstruction(code_address);
        }
        break;
    }
    default:
        break;
    }

    return handled ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

#else

#ifdef __APPLE__
#if defined(ARCH_X86_64)
#define CODE_ADDRESS(ctx) reinterpret_cast<void*>((ctx)->uc_mcontext->__ss.__rip)
#define IS_WRITE_ERROR(ctx) ((ctx)->uc_mcontext->__es.__err & 0x2)
#elif defined(ARCH_ARM64)
#define CODE_ADDRESS(ctx) reinterpret_cast<void*>((ctx)->uc_mcontext->__ss.__pc)
#define IS_WRITE_ERROR(ctx) ((ctx)->uc_mcontext->__es.__esr & 0x40)
#endif
#else
#if defined(ARCH_X86_64)
#define CODE_ADDRESS(ctx) reinterpret_cast<void*>((ctx)->uc_mcontext.gregs[REG_RIP])
#define IS_WRITE_ERROR(ctx) ((ctx)->uc_mcontext.gregs[REG_ERR] & 0x2)
#endif
#endif

#ifndef IS_WRITE_ERROR
#error "Missing IS_WRITE_ERROR() implementation for target OS and CPU architecture."
#endif

static std::string DisassembleInstruction(void* code_address) {
    char buffer[256] = "<unable to decode>";

#ifdef ARCH_X86_64
    ZydisDecoder decoder;
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    static constexpr u64 max_length = 0x20;
    const auto status =
        ZydisDecoderDecodeFull(&decoder, code_address, max_length, &instruction, operands);
    if (ZYAN_SUCCESS(status)) {
        ZydisFormatter formatter;
        ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);
        ZydisFormatterFormatInstruction(&formatter, &instruction, operands,
                                        instruction.operand_count_visible, buffer, sizeof(buffer),
                                        reinterpret_cast<u64>(code_address), ZYAN_NULL);
    }
#endif

    return buffer;
}

static void SignalHandler(int sig, siginfo_t* info, void* raw_context) {
    auto* ctx = static_cast<ucontext_t*>(raw_context);
    const auto* signals = Signals::Instance();

    auto* code_address = CODE_ADDRESS(ctx);

    switch (sig) {
    case SIGSEGV:
    case SIGBUS:
        if (const bool is_write = IS_WRITE_ERROR(ctx);
            !signals->DispatchAccessViolation(code_address, info->si_addr, is_write)) {
            UNREACHABLE_MSG("Unhandled access violation at code address {}: {} address {}",
                            fmt::ptr(code_address), is_write ? "Write to" : "Read from",
                            fmt::ptr(info->si_addr));
        }
        break;
    case SIGILL: {
        u8 opcode = *reinterpret_cast<u8*>(code_address);
        if (opcode == 0x37) { // 0x37 is AAA instruction, we use that to patch CPUID
            u32 results[4];
            int rax = ctx->uc_mcontext.gregs[REG_RAX];
            int rcx = ctx->uc_mcontext.gregs[REG_RCX];
            LOG_ERROR(Core, "CPUID - RAX BEFORE: 0x{:x}, RCX BEFORE: 0x{:x}", rax, rcx);
            if (rax == 1) {
                results[0] = processorFamilyIDs;
                results[1] = additionalInfo;
                results[2] = featureInfoEcx;
                results[3] = featureInfoEdx;
            } else if (rax == 0x80000001) {
                results[0] = extendedFeatureInfoEax;
                results[1] = extendedFeatureInfoEbx;
                results[2] = extendedFeatureInfoEcx;
                results[3] = extendedFeatureInfoEdx;
            } else {
                __cpuid_count(rax, rcx, results[0], results[1], results[2], results[3]);
                if (rax == 0 || rax == 0x80000000) {
                    // The order looks weird but is correct
                    // The string is saved in order of EBX, EDX, ECX
                    memcpy(&results[1], vendorId, 4);
                    memcpy(&results[3], vendorId + 4, 4);
                    memcpy(&results[2], vendorId + 8, 4);
                }
            }
            ctx->uc_mcontext.gregs[REG_RAX] = results[0];
            ctx->uc_mcontext.gregs[REG_RBX] = results[1];
            ctx->uc_mcontext.gregs[REG_RCX] = results[2];
            ctx->uc_mcontext.gregs[REG_RDX] = results[3];
            ctx->uc_mcontext.gregs[REG_RIP] += 1; // skip the illegal instruction
            LOG_ERROR(Core, "CPUID - RAX AFTER: 0x{:x}, RBX AFTER: 0x{:x}, RCX AFTER: 0x{:x}, RDX AFTER: 0x{:x}",
                      results[0], results[1], results[2], results[3]);
            break;
        } else {
            if (!signals->DispatchIllegalInstruction(code_address)) {
                UNREACHABLE_MSG("Unhandled illegal instruction at code address {}: {}",
                                fmt::ptr(code_address), DisassembleInstruction(code_address));
            }
        }
        break;
    }
    default:
        break;
    }
}

#endif

SignalDispatch::SignalDispatch() {
#if defined(_WIN32)
    ASSERT_MSG(handle = AddVectoredExceptionHandler(0, SignalHandler),
               "Failed to register exception handler.");
#else
    struct sigaction action {};
    action.sa_sigaction = SignalHandler;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);

    ASSERT_MSG(sigaction(SIGSEGV, &action, nullptr) == 0 &&
                   sigaction(SIGBUS, &action, nullptr) == 0,
               "Failed to register access violation signal handler.");
    ASSERT_MSG(sigaction(SIGILL, &action, nullptr) == 0,
               "Failed to register illegal instruction signal handler.");
#endif
}

SignalDispatch::~SignalDispatch() {
#if defined(_WIN32)
    ASSERT_MSG(RemoveVectoredExceptionHandler(handle), "Failed to remove exception handler.");
#else
    struct sigaction action {};
    action.sa_handler = SIG_DFL;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);

    ASSERT_MSG(sigaction(SIGSEGV, &action, nullptr) == 0 &&
                   sigaction(SIGBUS, &action, nullptr) == 0,
               "Failed to remove access violation signal handler.");
    ASSERT_MSG(sigaction(SIGILL, &action, nullptr) == 0,
               "Failed to remove illegal instruction signal handler.");
#endif
}

bool SignalDispatch::DispatchAccessViolation(void* code_address, void* fault_address,
                                             bool is_write) const {
    for (const auto& [handler, _] : access_violation_handlers) {
        if (handler(code_address, fault_address, is_write)) {
            return true;
        }
    }
    return false;
}

bool SignalDispatch::DispatchIllegalInstruction(void* code_address) const {
    for (const auto& [handler, _] : illegal_instruction_handlers) {
        if (handler(code_address)) {
            return true;
        }
    }
    return false;
}

} // namespace Core