// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/arch.h"
#include "common/assert.h"
#include "core/signals.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <csignal>
#ifdef ARCH_X86_64
#include <Zydis/Decoder.h>
#include <Zydis/Formatter.h>
#endif
#endif

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
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        handled = signals->DispatchIllegalInstruction(code_address);
        break;
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
#error "Missing IS_WRITE_ERROR() implementation for target OS and CPU architecture.
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
    const auto* ctx = static_cast<ucontext_t*>(raw_context);
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
    case SIGILL:
        if (!signals->DispatchIllegalInstruction(code_address)) {
            UNREACHABLE_MSG("Unhandled illegal instruction at code address {}: {}",
                            fmt::ptr(code_address), DisassembleInstruction(code_address));
        }
        break;
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
    constexpr struct sigaction action {
        .sa_flags = SA_SIGINFO | SA_ONSTACK, .sa_sigaction = SignalHandler, .sa_mask = 0,
    };
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
    constexpr struct sigaction action {
        .sa_flags = 0, .sa_handler = SIG_DFL, .sa_mask = 0,
    };
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