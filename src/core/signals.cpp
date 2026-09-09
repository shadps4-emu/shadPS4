// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/arch.h"
#include "common/assert.h"
#include "common/decoder.h"
#include "common/signal_context.h"
#include "core/cpu_patches.h" // Windows static guest red-zone protection
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/threads/exception.h"
#include "core/linker.h"
#include "core/module.h"
#include "core/signals.h"
#include "emulator.h"

#ifdef _WIN32
#include <windows.h>
static constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;
#else
#include <csignal>
#include <pthread.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <unistd.h>
#ifdef ARCH_X86_64
#include <Zydis/Formatter.h>
#endif
#endif

namespace Core {

namespace {

#if defined(_WIN32)
bool IsReadable(VAddr addr, u64 size) {
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(reinterpret_cast<LPCVOID>(addr), &info, sizeof(info))) {
        return false;
    }
    if (info.State != MEM_COMMIT || info.Protect == PAGE_NOACCESS ||
        (info.Protect & PAGE_GUARD)) {
        return false;
    }
    const auto region_end = reinterpret_cast<VAddr>(info.BaseAddress) + info.RegionSize;
    return addr + size <= region_end;
}
#else
bool IsReadable(VAddr addr, u64 size) {
#if defined(__APPLE__) || defined(__FreeBSD__)
    using MincoreVecT = char;
#else
    using MincoreVecT = unsigned char;
#endif
    const auto page_size = static_cast<u64>(sysconf(_SC_PAGESIZE));
    const VAddr page_start = addr & ~(page_size - 1);
    const VAddr page_end = (addr + size + page_size - 1) & ~(page_size - 1);
    std::vector<MincoreVecT> vec((page_end - page_start) / page_size);
    return mincore(reinterpret_cast<void*>(page_start), page_end - page_start, vec.data()) == 0;
}
#endif

} // namespace

std::vector<VAddr> StackTracer::Capture(u32 max_frames) {
    std::vector<VAddr> trace;
#if defined(ARCH_X86_64)
    // Self-capture the caller's context via the project's portable Ucontext, rather than
    // reading platform registers by hand.
#if defined(_WIN32)
    CONTEXT raw_ctx{};
    RtlCaptureContext(&raw_ctx);
    const Libraries::Kernel::Ucontext ctx{&raw_ctx};
#else
    ucontext_t raw_ctx{};
    getcontext(&raw_ctx);
    siginfo_t dummy_info{};
    const Libraries::Kernel::Ucontext ctx{&dummy_info, &raw_ctx};
#endif
    VAddr rbp = ctx.uc_mcontext.mc_rbp;
    if (ctx.uc_mcontext.mc_rip != 0) {
        trace.push_back(ctx.uc_mcontext.mc_rip);
    }

    VAddr prev_rbp = 0;
    for (u32 i = 0; i < max_frames && rbp != 0; i++) {
        // Standard SysV frame layout: [rbp+0] = saved rbp, [rbp+8] = return address.
        if (!IsReadable(rbp, sizeof(VAddr) * 2)) {
            break;
        }
        const VAddr saved_rbp = *reinterpret_cast<const VAddr*>(rbp);
        const VAddr return_addr = *reinterpret_cast<const VAddr*>(rbp + sizeof(VAddr));
        if (return_addr == 0) {
            break;
        }
        trace.push_back(return_addr);

        // The chain must move strictly upward; anything else means it's corrupt or we hit
        // the bottom of the stack.
        if (saved_rbp <= rbp || saved_rbp <= prev_rbp) {
            break;
        }
        prev_rbp = rbp;
        rbp = saved_rbp;
    }
#endif
    return trace;
}

std::vector<StackTracer::Frame> StackTracer::Resolve(const std::vector<VAddr>& return_addrs) {
    std::vector<Frame> frames;
    frames.reserve(return_addrs.size());

    for (const VAddr addr : return_addrs) {
        Frame frame{.return_addr = addr};
        Module* module = Common::Singleton<Linker>::Instance()->FindByAddress(addr);

        if (module) {
            frame.has_module = true;
            frame.module_name =
                (module->IsSystemLib() ? "/system_ex/common/lib/" : "/app0/") + module->name;

            const auto& info = module->info;
            bool found_segment = false;
            for (u32 s = 0; s < info.num_segments; s++) {
                const auto& seg = info.segments[s];
                if (addr >= seg.address && addr < seg.address + seg.size) {
                    frame.segment_index = s;
                    frame.segment_offset = addr - seg.address;
                    found_segment = true;
                    break;
                }
            }
            if (!found_segment) {
                frame.segment_offset = addr - module->GetBaseAddress();
            }

            VAddr best_addr = module->GetEntryAddress();
            std::string best_name = "Entry";
            for (const auto& sym : module->export_sym.GetSymbols()) {
                if (sym.virtual_address != 0 && sym.virtual_address <= addr &&
                    sym.virtual_address >= best_addr) {
                    best_addr = sym.virtual_address;
                    best_name = sym.nid_name;
                }
            }
            if (addr >= best_addr) {
                frame.has_symbol = true;
                frame.symbol_name = std::move(best_name);
                frame.symbol_offset = addr - best_addr;
            }
        }

        frames.push_back(std::move(frame));
    }

    return frames;
}

std::string StackTracer::Format(const std::vector<Frame>& frames) {
    std::string out;
    for (const auto& frame : frames) {
        if (!frame.has_module) {
            out += fmt::format("  offset $-----|{:016X}  <unmapped>\n", frame.return_addr);
            continue;
        }
        if (frame.has_symbol) {
            out += fmt::format("  offset ${:05X}|{:06X}  {}:{}+${:06X}\n", frame.segment_index,
                               frame.segment_offset, frame.module_name, frame.symbol_name,
                               frame.symbol_offset);
        } else {
            out += fmt::format("  offset ${:05X}|{:06X}  {}\n", frame.segment_index,
                               frame.segment_offset, frame.module_name);
        }
    }
    return out;
}

std::string StackTracer::Dump(u32 max_frames) {
    return Format(Resolve(Capture(max_frames)));
}

#if defined(_WIN32)

static LONG WINAPI SignalHandler(EXCEPTION_POINTERS* pExp) noexcept {
    using namespace Libraries::Kernel;
    const auto* signals = Signals::Instance();

    const bool use_static_windows_guest_red_zone_protection =
        WindowsGuestRedZoneProtection::IsStaticPatchingEnabled();
    DWORD code = 0;
    PVOID address = nullptr;

    if (pExp != nullptr && pExp->ExceptionRecord != nullptr) {
        code = pExp->ExceptionRecord->ExceptionCode;
        address = pExp->ExceptionRecord->ExceptionAddress;
    }

    Ucontext guest_context{pExp->ContextRecord};
    Siginfo guest_info{
        ._si_signo = 0,
        ._si_errno = 0,
        ._si_code = POSIX_SI_NOINFO,
        ._si_addr = (void*)guest_context.uc_mcontext.mc_rip,
    };

    bool handled = false;
    bool static_protection_exception = false; // Windows static guest red-zone protection
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
        guest_info._si_signo = POSIX_SIGSEGV;
        guest_info._si_code = POSIX_SEGV_MAPERR;
        static_protection_exception = true; // Windows static guest red-zone protection
        handled = signals->DispatchAccessViolation(
            pExp, reinterpret_cast<void*>(pExp->ExceptionRecord->ExceptionInformation[1]));
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        guest_info._si_signo = POSIX_SIGILL;
        guest_info._si_code = POSIX_ILL_ILLOPC;
        static_protection_exception = true; // Windows static guest red-zone protection
        handled = signals->DispatchIllegalInstruction(pExp);
        break;
    case EXCEPTION_PRIV_INSTRUCTION: // Windows static guest red-zone protection
        if (use_static_windows_guest_red_zone_protection) {
            static_protection_exception = true;
            handled = signals->DispatchIllegalInstruction(pExp);
        }
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        guest_info._si_signo = POSIX_SIGBUS;
        guest_info._si_code = POSIX_BUS_ADRALN;
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_INTDIV;
        break;
    case EXCEPTION_INT_OVERFLOW:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_INTOVF;
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_FLTDIV;
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_FLTINV;
        break;
    case EXCEPTION_FLT_OVERFLOW:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_FLTOVF;
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_FLTUND;
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_FLTSUB; // i am not sure about this one
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        guest_info._si_signo = POSIX_SIGFPE;
        guest_info._si_code = POSIX_FPE_FLTRES;
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        guest_info._si_signo = POSIX_SIGILL;
        guest_info._si_code = POSIX_ILL_BADSTK; // i am not sure about this one either
        break;
    case EXCEPTION_BREAKPOINT:
    case EXCEPTION_SINGLE_STEP:
        guest_info._si_signo = POSIX_SIGTRAP;
        guest_info._si_code = POSIX_TRAP_BRKPT;
        break;
    case DBG_PRINTEXCEPTION_C:
    case DBG_PRINTEXCEPTION_WIDE_C:
        // Used by OutputDebugString functions.
        return EXCEPTION_CONTINUE_EXECUTION;
    case MS_VC_EXCEPTION:
        LOG_DEBUG(Debug, "Pass MS_VC_EXCEPTION at {} to handler", address);
        return EXCEPTION_EXECUTE_HANDLER;
    default:
        break;
    }

    if (handled) {
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    if (guest_info._si_signo != 0) {
        if (g_curthread &&
            g_curthread->DispatchSignal(guest_info._si_signo, &guest_info, &guest_context)) {
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    const bool report_unhandled =
        use_static_windows_guest_red_zone_protection ? static_protection_exception : true;
    if (report_unhandled) {
        LOG_CRITICAL(Debug, "Unhandled Exception code {:#x} at {}", code, address);
        Common::Singleton<Core::Emulator>::Instance()->Shutdown();
    }

    return EXCEPTION_CONTINUE_SEARCH;
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

static s32 NativeSiCodeToGuest(s32 sig, s32 code) {
    using namespace Libraries::Kernel;
    switch (sig) {
    case SIGUSR1:
        return POSIX_SI_LWP;
    case SIGSEGV:
        switch (code) {
        case SEGV_MAPERR:
            return POSIX_SEGV_MAPERR;
        case SEGV_ACCERR:
            return POSIX_SEGV_ACCERR;
        }
    case SIGBUS:
        switch (code) {
        case BUS_ADRALN:
            return POSIX_BUS_ADRALN;
        case BUS_ADRERR:
            return POSIX_BUS_ADRERR;
        case BUS_OBJERR:
            return POSIX_BUS_OBJERR;
        }
    case SIGILL:
        switch (code) {
        case ILL_ILLOPC:
            return POSIX_ILL_ILLOPC;
        case ILL_ILLOPN:
            return POSIX_ILL_ILLOPN;
        case ILL_ILLADR:
            return POSIX_ILL_ILLADR;
        case ILL_ILLTRP:
            return POSIX_ILL_ILLTRP;
        case ILL_PRVOPC:
            return POSIX_ILL_PRVOPC;
        case ILL_PRVREG:
            return POSIX_ILL_PRVREG;
        case ILL_COPROC:
            return POSIX_ILL_COPROC;
        case ILL_BADSTK:
            return POSIX_ILL_BADSTK;
        }
    case SIGFPE:
        switch (code) {
        case FPE_INTOVF:
            return POSIX_FPE_INTOVF;
        case FPE_INTDIV:
            return POSIX_FPE_INTDIV;
        case FPE_FLTDIV:
            return POSIX_FPE_FLTDIV;
        case FPE_FLTOVF:
            return POSIX_FPE_FLTOVF;
        case FPE_FLTUND:
            return POSIX_FPE_FLTUND;
        case FPE_FLTRES:
            return POSIX_FPE_FLTRES;
        case FPE_FLTINV:
            return POSIX_FPE_FLTINV;
        case FPE_FLTSUB:
            return POSIX_FPE_FLTSUB;
        }
    case SIGTRAP:
        switch (code) {
        case TRAP_BRKPT:
            return POSIX_TRAP_BRKPT;
        case TRAP_TRACE:
            return POSIX_TRAP_TRACE;
#ifdef __FreeBSD__
        case TRAP_DTRACE:
            return POSIX_TRAP_DTRACE;
#endif
        }

    default:
        return POSIX_SI_NOINFO;
    }
}

void SignalHandler(int sig, siginfo_t* info, void* raw_context) {
    using namespace Libraries::Kernel;
    auto* thread = g_curthread;
    const auto* signals = Signals::Instance();

    auto* code_address = Common::GetRip(raw_context);

    Ucontext context{info, reinterpret_cast<ucontext_t*>(raw_context)};
    Siginfo guest_info{};
    if (info) {
        guest_info = *reinterpret_cast<Siginfo*>(info);
        guest_info._si_signo = sig == SIGUSR1 ? 0 : NativeToOrbisSignal(info->si_signo);
        guest_info._si_errno = NativeToPosixErrno(info->si_errno);
        guest_info._si_code = NativeSiCodeToGuest(sig, info->si_code);
        guest_info._si_addr = (void*)context.uc_mcontext.mc_rip;
    }
    Siginfo* info_p = info ? &guest_info : nullptr;
    Ucontext* context_p = raw_context ? &context : nullptr;

    switch (sig) {
    case SIGSEGV:
    case SIGBUS: {
        const bool is_write = Common::IsWriteError(raw_context);
        if (!signals->DispatchAccessViolation(raw_context, info->si_addr)) {
            if (thread && thread->DispatchSignal(NativeToOrbisSignal(sig), info_p, context_p)) {
                return;
            }
            UNREACHABLE_MSG("Unhandled access violation at code address {}: {} address {}",
                            fmt::ptr(code_address), is_write ? "Write to" : "Read from",
                            fmt::ptr(info->si_addr));
        }
        break;
    }
    case SIGILL:
        if (signals->DispatchIllegalInstruction(raw_context)) {
            return;
        }
    case SIGFPE:
    case SIGTRAP:
    case SIGSYS: {
        if (thread && thread->DispatchSignal(NativeToOrbisSignal(sig), info_p, context_p)) {
            return;
        }

        UNREACHABLE_MSG("Unhandled signal {} at code address {}", sig, fmt::ptr(code_address));
    }
    case SIGSLEEP: {
        // Sleep thread until signal is received again
        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGSLEEP);
        sigwait(&sigset, &sig);
        break;
    }
    case SIGUSR1:
        if (thread) {
            thread->DispatchPendingSignals(info_p, context_p);
        }
        break;
    default:
        UNREACHABLE_MSG("Unhandled signal {} at code address {}", sig, fmt::ptr(code_address));
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

    ASSERT_MSG(
        sigaction(SIGSEGV, &action, nullptr) == 0 && sigaction(SIGBUS, &action, nullptr) == 0 &&
            sigaction(SIGILL, &action, nullptr) == 0 && sigaction(SIGFPE, &action, nullptr) == 0 &&
            sigaction(SIGTRAP, &action, nullptr) == 0 && sigaction(SIGSYS, &action, nullptr) == 0 &&
            sigaction(SIGUSR1, &action, nullptr) == 0 && sigaction(SIGSLEEP, &action, nullptr) == 0,
        "Failed to register signal handlers.");
#endif
}

void SignalDispatch::RemoveHandlers() {
    // asserting here would get into an infinite loop until too
    // many nested exceptions makes the OS kill the process
#if defined(_WIN32)
    if (!(RemoveVectoredExceptionHandler(handle))) {
        LOG_CRITICAL(Core, "Failed to remove exception handler.");
        std::quick_exit(1);
    }
#else
    struct sigaction action{};
    action.sa_handler = SIG_DFL;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);

    if (!(sigaction(SIGSEGV, &action, nullptr) == 0 && sigaction(SIGBUS, &action, nullptr) == 0 &&
          sigaction(SIGILL, &action, nullptr) == 0 && sigaction(SIGFPE, &action, nullptr) == 0 &&
          sigaction(SIGTRAP, &action, nullptr) == 0 && sigaction(SIGSYS, &action, nullptr) == 0 &&
          sigaction(SIGUSR1, &action, nullptr) == 0 &&
          sigaction(SIGSLEEP, &action, nullptr) == 0)) {
        LOG_CRITICAL(Core, "Failed to remove signal handlers.");
        std::quick_exit(1);
    }
#endif
}

SignalDispatch::~SignalDispatch() {
    RemoveHandlers();
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
