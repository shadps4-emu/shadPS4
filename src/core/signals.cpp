// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/arch.h"
#include "common/assert.h"
#include "common/decoder.h"
#include "common/signal_context.h"
#include "core/libraries/kernel/threads/exception.h"
#include "core/signals.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <csignal>
#include <pthread.h>
#ifdef ARCH_X86_64
#include <Zydis/Formatter.h>
#endif
#endif

#ifndef _WIN32
namespace Libraries::Kernel {
void SigactionHandler(int native_signum, siginfo_t* inf, ucontext_t* raw_context);
extern std::array<SceKernelExceptionHandler, 32> Handlers;
} // namespace Libraries::Kernel
#endif

namespace Core {

#if defined(_WIN32)

static LONG WINAPI SignalHandler(EXCEPTION_POINTERS* pExp) noexcept {
    const auto* signals = Signals::Instance();

    bool handled = false;
    switch (pExp->ExceptionRecord->ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
        handled = signals->DispatchAccessViolation(
            pExp, reinterpret_cast<void*>(pExp->ExceptionRecord->ExceptionInformation[1]));
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        handled = signals->DispatchIllegalInstruction(pExp);
        break;
    case DBG_PRINTEXCEPTION_C:
    case DBG_PRINTEXCEPTION_WIDE_C:
        // Used by OutputDebugString functions.
        return EXCEPTION_CONTINUE_EXECUTION;
    default:
        break;
    }

    return handled ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

#else

static std::string DisassembleInstruction(void* code_address) {
    char buffer[256] = "<unable to decode>";

#ifdef ARCH_X86_64
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    const auto status =
        Common::Decoder::Instance()->decodeInstruction(instruction, operands, code_address);
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

void SignalHandler(int sig, siginfo_t* info, void* raw_context) {
    const auto* signals = Signals::Instance();

    auto* code_address = Common::GetRip(raw_context);

    switch (sig) {
    case SIGSEGV:
    case SIGBUS: {
        const bool is_write = Common::IsWriteError(raw_context);
        if (!signals->DispatchAccessViolation(raw_context, info->si_addr)) {
            // If the guest has installed a custom signal handler, and the access violation didn't
            // come from HLE memory tracking, pass the signal on
            if (Libraries::Kernel::Handlers[sig]) {
                Libraries::Kernel::SigactionHandler(sig, info,
                                                    reinterpret_cast<ucontext_t*>(raw_context));
                return;
            }
            UNREACHABLE_MSG("Unhandled access violation at code address {}: {} address {}",
                            fmt::ptr(code_address), is_write ? "Write to" : "Read from",
                            fmt::ptr(info->si_addr));
        }
        break;
    }
    case SIGILL:
        if (!signals->DispatchIllegalInstruction(raw_context)) {
            if (Libraries::Kernel::Handlers[sig]) {
                Libraries::Kernel::SigactionHandler(sig, info,
                                                    reinterpret_cast<ucontext_t*>(raw_context));
                return;
            }
            UNREACHABLE_MSG("Unhandled illegal instruction at code address {}: {}",
                            fmt::ptr(code_address), DisassembleInstruction(code_address));
        }
        break;
    default:
        if (sig == SIGSLEEP) {
            // Sleep thread until signal is received again
            sigset_t sigset;
            sigemptyset(&sigset);
            sigaddset(&sigset, SIGSLEEP);
            sigwait(&sigset, &sig);
        }
        break;
    }
}

#endif

SignalDispatch::SignalDispatch() {
#if defined(_WIN32)
    ASSERT_MSG(handle = AddVectoredExceptionHandler(0, SignalHandler),
               "Failed to register exception handler.");
#else
    struct sigaction action{};
    action.sa_sigaction = SignalHandler;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);

    ASSERT_MSG(sigaction(SIGSEGV, &action, nullptr) == 0 &&
                   sigaction(SIGBUS, &action, nullptr) == 0,
               "Failed to register access violation signal handler.");
    ASSERT_MSG(sigaction(SIGILL, &action, nullptr) == 0,
               "Failed to register illegal instruction signal handler.");
    ASSERT_MSG(sigaction(SIGSLEEP, &action, nullptr) == 0,
               "Failed to register sleep signal handler.");
#endif
}

SignalDispatch::~SignalDispatch() {
#if defined(_WIN32)
    ASSERT_MSG(RemoveVectoredExceptionHandler(handle), "Failed to remove exception handler.");
#else
    struct sigaction action{};
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

bool SignalDispatch::DispatchAccessViolation(void* context, void* fault_address) const {
    for (const auto& [handler, _] : access_violation_handlers) {
        if (handler(context, fault_address)) {
            return true;
        }
    }
    return false;
}

bool SignalDispatch::DispatchIllegalInstruction(void* context) const {
    for (const auto& [handler, _] : illegal_instruction_handlers) {
        if (handler(context)) {
            return true;
        }
    }
    return false;
}

} // namespace Core