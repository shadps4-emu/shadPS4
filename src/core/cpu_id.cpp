// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <memory>
#include <asm/prctl.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/ucontext.h>
#include <unistd.h>
#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>
#include "common/assert.h"
#include "core/cpu_id.h"
#include "core/libraries/kernel/threads/pthread.h"

#ifdef ENABLE_CPU_ID_TRANSLATION
extern "C" __attribute__((noinline, visibility("default"))) int ShadCpuIdTranslationActive() {
    volatile int active = 0;
    return active;
}

extern "C" __attribute__((noinline, visibility("default"))) void ShadCpuIdTranslationRange(
    uintptr_t begin, uintptr_t end) {
    asm volatile("" : : "r"(begin), "r"(end) : "memory");
}
#endif

namespace Core {
namespace {

constexpr u32 GuestCpuCount = 8;
constexpr size_t RedZoneSize = 128;
constexpr size_t RegisterFrameSize = 17 * sizeof(u64);
constexpr size_t FaultStackSize = RedZoneSize + sizeof(u64);
u32 host_max_basic{};
u32 host_max_extended{};
u32 host_cache_leaf{};
static_assert(std::atomic<uintptr_t>::is_always_lock_free);
std::atomic<uintptr_t> guest_begin{};
std::atomic<uintptr_t> guest_end{};
std::array<std::unique_ptr<Xbyak::CodeGenerator>, 2> fault_trampolines;

std::array<u32, 4> GuestCpuid(u32 leaf, u32 subleaf, u32 cpu) {
    std::array<u32, 4> result{};
    Xbyak::util::Cpu::getCpuidEx(leaf, subleaf, result.data());
    if (leaf > host_max_basic && (leaf < 0x80000000 || leaf > host_max_extended)) {
        result = {};
    }
    auto& [eax, ebx, ecx, edx] = result;
    switch (leaf) {
    case 0:
        eax = std::max(host_max_basic, 0x1fu);
        break;
    case 1:
        ebx = (ebx & 0xffff) | (GuestCpuCount << 16) | (cpu << 24);
        // Jaguar has neither FMA nor RDRAND (AMD BKDG 48751, CPUID Fn0000_0001_ECX).
        ecx &= ~((1u << 12) | (1u << 30));
        edx |= 1u << 28;
        break;
    case 4:
    case 0x8000001d:
        if (host_cache_leaf != 0) {
            Xbyak::util::Cpu::getCpuidEx(host_cache_leaf, subleaf, result.data());
        }
        if ((eax & 0x1f) != 0) {
            const u32 sharing = ((eax >> 5) & 7) >= 3 ? GuestCpuCount : 1;
            const u32 cores = leaf == 4 ? (GuestCpuCount - 1) << 26 : 0;
            eax = (eax & 0x3fff) | ((sharing - 1) << 14) | cores;
        }
        break;
    case 7:
        // BMI1 is the only Jaguar structured extended feature (AMD BKDG 48751).
        result = {0, subleaf == 0 ? ebx & (1u << 3) : 0, 0, 0};
        break;
    case 0xb:
    case 0x1f:
        eax = subleaf == 1 ? 3 : 0;
        ebx = subleaf == 0 ? 1 : subleaf == 1 ? GuestCpuCount : 0;
        ecx = (subleaf & 0xff) | (subleaf < 2 ? (subleaf + 1) << 8 : 0);
        edx = cpu;
        break;
    case 0x1a:
        result = {};
        break;
    case 0x80000000:
        eax = std::max(host_max_extended, 0x8000001eu);
        break;
    case 0x80000001:
        // XOP, LWP, FMA4, TBM and MONITORX/MWAITX are not Jaguar instructions.
        ecx &= ~((1u << 11) | (1u << 15) | (1u << 16) | (1u << 21) | (1u << 29));
        ecx |= (1u << 6) | (1u << 22);
        break;
    case 0x80000008:
        ecx = (ecx & ~0xf0ffu) | (3u << 12) | (GuestCpuCount - 1);
        break;
    case 0x8000001e:
        result = {cpu, cpu, 0, 0};
        break;
    case 0x80000026:
        result = {};
        break;
    default:
        break;
    }
    return result;
}

u32 CurrentGuestCpu() {
    auto* thread = Libraries::Kernel::g_curthread;
    ASSERT(thread != nullptr);
    const s32 cpu = thread->GetCurrentCpu();
    ASSERT_MSG(cpu >= 0 && cpu < GuestCpuCount, "Cannot resolve guest CPU identity: {}", cpu);
    return cpu;
}

void PS4_SYSV_ABI ExecuteCpuId(u64* registers, CpuIdInstruction instruction, u32 destination) {
    const u32 cpu = CurrentGuestCpu();
    switch (instruction) {
    case CpuIdInstruction::Cpuid: {
        const auto result = GuestCpuid(static_cast<u32>(registers[0]),
                                       static_cast<u32>(registers[1]), static_cast<u32>(cpu));
        registers[0] = result[0];
        registers[3] = result[1];
        registers[1] = result[2];
        registers[2] = result[3];
        break;
    }
    case CpuIdInstruction::Rdtscp: {
        u32 low, high, auxiliary;
        asm volatile("rdtscp" : "=a"(low), "=d"(high), "=c"(auxiliary) : : "memory");
        registers[0] = low;
        registers[2] = high;
        registers[1] = cpu;
        break;
    }
    case CpuIdInstruction::Rdpid:
        registers[destination] = cpu;
        break;
    }
}

enum class FaultInstruction { Cpuid, Rdtscp, Rdpid, Rdtsc };

size_t FaultInstructionLength(const u8* code, FaultInstruction& instruction, u32& destination) {
    u8 rex = 0;
    u8 repeat = 0;
    for (size_t index = 0; index < 14; ++index) {
        const u8 byte = code[index];
        if (byte == 0x0f) {
            if (code[index + 1] == 0xa2) {
                instruction = FaultInstruction::Cpuid;
                return index + 2;
            }
            if (code[index + 1] == 0x31) {
                instruction = FaultInstruction::Rdtsc;
                return index + 2;
            }
            if (index < 13 && code[index + 1] == 0x01 && code[index + 2] == 0xf9) {
                instruction = FaultInstruction::Rdtscp;
                return index + 3;
            }
            if (index < 13 && repeat == 0xf3 && code[index + 1] == 0xc7 &&
                (code[index + 2] & 0xf8) == 0xf8) {
                instruction = FaultInstruction::Rdpid;
                destination = (code[index + 2] & 7) | ((rex & 1) << 3);
                return index + 3;
            }
            return 0;
        }
        if (byte >= 0x40 && byte <= 0x4f) {
            rex = byte;
            continue;
        }
        rex = 0;
        if (byte == 0xf2 || byte == 0xf3) {
            repeat = byte;
            continue;
        }
        if (byte == 0x66 || byte == 0x67 || byte == 0x26 || byte == 0x2e || byte == 0x36 ||
            byte == 0x3e || byte == 0x64 || byte == 0x65) {
            continue;
        }
        return 0;
    }
    return 0;
}

} // namespace

bool HandleCpuIdFault(void* context, void* fault_address) {
    auto& registers = static_cast<ucontext_t*>(context)->uc_mcontext.gregs;
    const bool illegal = registers[REG_TRAPNO] == 6;
    if (!illegal && (fault_address != nullptr || registers[REG_TRAPNO] != 13)) {
        return false;
    }
    const auto address = static_cast<uintptr_t>(registers[REG_RIP]);
    FaultInstruction instruction{};
    u32 destination = 0;
    const size_t length =
        FaultInstructionLength(reinterpret_cast<const u8*>(address), instruction, destination);
    if (length == 0) {
        return false;
    }
    if (instruction != FaultInstruction::Rdtsc &&
        address >= guest_begin.load(std::memory_order_relaxed) &&
        address < guest_end.load(std::memory_order_relaxed) &&
        Libraries::Kernel::g_curthread != nullptr) {
        if (instruction == FaultInstruction::Rdpid) {
            static constexpr std::array HostRegisters{
                REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP, REG_RSI, REG_RDI,
                REG_R8,  REG_R9,  REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15};
            registers[HostRegisters[destination]] = CurrentGuestCpu();
            registers[REG_RIP] += length;
            return true;
        }
        const auto stack = static_cast<uintptr_t>(registers[REG_RSP]) - FaultStackSize;
        *reinterpret_cast<u64*>(stack) = address + length;
        registers[REG_RSP] = stack;
        const auto index = static_cast<size_t>(instruction);
        registers[REG_RIP] = reinterpret_cast<uintptr_t>(fault_trampolines[index]->getCode());
        return true;
    }

    if (illegal || instruction == FaultInstruction::Rdpid) {
        return false;
    }

    const int saved_errno = errno;
    sigset_t blocked, previous;
    sigfillset(&blocked);
    if (sigprocmask(SIG_SETMASK, &blocked, &previous) != 0) {
        errno = saved_errno;
        return false;
    }
    const bool is_cpuid = instruction == FaultInstruction::Cpuid;
    const bool enabled = is_cpuid ? syscall(SYS_arch_prctl, ARCH_SET_CPUID, 1) == 0
                                  : prctl(PR_SET_TSC, PR_TSC_ENABLE) == 0;
    if (enabled) {
        u32 result[4]{};
        if (is_cpuid) {
            Xbyak::util::Cpu::getCpuidEx(static_cast<u32>(registers[REG_RAX]),
                                         static_cast<u32>(registers[REG_RCX]), result);
        } else if (instruction == FaultInstruction::Rdtscp) {
            asm volatile("rdtscp" : "=a"(result[0]), "=d"(result[3]), "=c"(result[2]) : : "memory");
        } else {
            asm volatile("rdtsc" : "=a"(result[0]), "=d"(result[3]) : : "memory");
        }
        const int restored = is_cpuid ? syscall(SYS_arch_prctl, ARCH_SET_CPUID, 0)
                                      : prctl(PR_SET_TSC, PR_TSC_SIGSEGV);
        if (restored != 0) {
            _exit(1);
        }
        registers[REG_RAX] = result[0];
        if (is_cpuid) {
            registers[REG_RBX] = result[1];
        }
        if (instruction != FaultInstruction::Rdtsc) {
            registers[REG_RCX] = result[2];
        }
        registers[REG_RDX] = result[3];
        registers[REG_RIP] += length;
    }
    sigprocmask(SIG_SETMASK, &previous, nullptr);
    errno = saved_errno;
    return enabled;
}

void GenerateCpuIdInstruction(Xbyak::CodeGenerator& c, CpuIdInstruction instruction,
                              u32 destination, bool fault_entry) {
    using namespace Xbyak::util;
    u32 xstate[4];
    Xbyak::util::Cpu::getCpuidEx(0xd, 0, xstate);
    const u32 state_size = std::max(xstate[2], 576u);
    if (!fault_entry) {
        c.lea(rsp, ptr[rsp - RedZoneSize]);
    }
    c.pushfq();
    for (int index = 15; index >= 0; --index) {
        if (index == 4) {
            c.push(0);
        } else {
            c.push(Xbyak::Reg64(index));
        }
    }
    c.mov(r12, rsp);
    c.lea(rax, ptr[rsp + RegisterFrameSize + (fault_entry ? FaultStackSize : RedZoneSize)]);
    c.mov(ptr[r12 + 4 * sizeof(u64)], rax);
    c.sub(rsp, state_size + 63);
    c.and_(rsp, -64);
    c.mov(r14, rsp);
    for (size_t offset = 512; offset < 576; offset += sizeof(u64)) {
        c.mov(qword[r14 + offset], 0);
    }
    c.xor_(ecx, ecx);
    c.xgetbv();
    c.mov(r13d, eax);
    c.mov(r15d, edx);
    c.db(0x48);
    c.db(0x0f);
    c.db(0xae);
    c.db(0x24);
    c.db(0x24); // XSAVE64 [rsp]
    c.cld();
    c.mov(rdi, r12);
    c.mov(esi, static_cast<u32>(instruction));
    c.mov(edx, destination);
    c.mov(rax, reinterpret_cast<uintptr_t>(&ExecuteCpuId));
    c.call(rax);
    c.mov(rsp, r14);
    c.mov(eax, r13d);
    c.mov(edx, r15d);
    c.db(0x48);
    c.db(0x0f);
    c.db(0xae);
    c.db(0x2c);
    c.db(0x24); // XRSTOR64 [rsp]
    c.mov(rsp, r12);
    for (int index = 0; index < 16; ++index) {
        if (index == 4) {
            c.lea(rsp, ptr[rsp + sizeof(u64)]);
        } else {
            c.pop(Xbyak::Reg64(index));
        }
    }
    c.popfq();
    c.mov(rsp, ptr[rsp - (RegisterFrameSize - 4 * sizeof(u64))]);
    if (fault_entry) {
        c.jmp(ptr[rsp - FaultStackSize]);
    }
}

void InitializeCpuId() {
    u32 data[4];
    Xbyak::util::Cpu::getCpuid(0, data);
    host_max_basic = data[0];
    Xbyak::util::Cpu::getCpuid(0x80000000, data);
    host_max_extended = data[0];
    Xbyak::util::Cpu::getCpuidEx(4, 0, data);
    if (host_max_basic >= 4 && (data[0] & 0x1f) != 0) {
        host_cache_leaf = 4;
    } else if (host_max_extended >= 0x8000001d) {
        host_cache_leaf = 0x8000001d;
    }
    for (const auto instruction : {CpuIdInstruction::Cpuid, CpuIdInstruction::Rdtscp}) {
        auto& trampoline = fault_trampolines[static_cast<size_t>(instruction)];
        trampoline = std::make_unique<Xbyak::CodeGenerator>(4096);
        GenerateCpuIdInstruction(*trampoline, instruction, 0, true);
        trampoline->readyRE();
    }
}

void EnableCpuIdFaulting() {
#ifdef ENABLE_CPU_ID_TRANSLATION
    if (ShadCpuIdTranslationActive()) {
        static std::atomic announced{false};
        if (!announced.exchange(true)) {
            LOG_INFO(Core, "CPU identity translation active");
        }
        return;
    }
#endif
    if (prctl(PR_SET_TSC, PR_TSC_SIGSEGV) != 0) {
        static std::atomic warned{false};
        if (!warned.exchange(true)) {
            LOG_WARNING(Core, "TSC faulting unavailable; RDTSCP identity requires static patches");
        }
    }
    if (syscall(SYS_arch_prctl, ARCH_SET_CPUID, 0) != 0) {
        static std::atomic warned{false};
        if (!warned.exchange(true)) {
            LOG_WARNING(Core, "CPUID faulting unavailable; CPU identity requires static patches");
        }
    }
}

void SetCpuIdGuestAddressRange(uintptr_t begin, uintptr_t end) {
    guest_begin.store(begin, std::memory_order_relaxed);
    guest_end.store(end, std::memory_order_relaxed);
#ifdef ENABLE_CPU_ID_TRANSLATION
    ShadCpuIdTranslationRange(begin, end);
#endif
}

} // namespace Core
