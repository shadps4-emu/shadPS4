// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <memory>
#include <asm/prctl.h>
#include <sys/syscall.h>
#include <sys/ucontext.h>
#include <unistd.h>
#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>
#include "common/assert.h"
#include "core/cpu_id.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/signals.h"

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
std::unique_ptr<Xbyak::CodeGenerator> fault_trampoline;

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
        if (subleaf == 0) {
            ecx &= ~(1u << 22);
            edx &= ~(1u << 15);
        }
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
        ecx |= 1u << 22;
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

void PS4_SYSV_ABI ExecuteCpuId(u64* registers, CpuIdInstruction instruction, u32 destination) {
    auto* thread = Libraries::Kernel::g_curthread;
    ASSERT(thread != nullptr);
    const s32 cpu = thread->GetCurrentCpu();
    ASSERT_MSG(cpu >= 0 && cpu < GuestCpuCount, "Cannot resolve guest CPU identity: {}", cpu);
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

size_t CpuidLength(const u8* code) {
    for (size_t index = 0; index < 14; ++index) {
        const u8 byte = code[index];
        if (byte == 0x0f) {
            return code[index + 1] == 0xa2 ? index + 2 : 0;
        }
        if ((byte >= 0x40 && byte <= 0x4f) || byte == 0x66 || byte == 0x67 || byte == 0xf2 ||
            byte == 0xf3 || byte == 0x26 || byte == 0x2e || byte == 0x36 || byte == 0x3e ||
            byte == 0x64 || byte == 0x65) {
            continue;
        }
        return 0;
    }
    return 0;
}

bool HandleCpuidFault(void* context, void* fault_address) {
    auto& registers = static_cast<ucontext_t*>(context)->uc_mcontext.gregs;
    if (fault_address != nullptr || registers[REG_TRAPNO] != 13) {
        return false;
    }
    const auto address = static_cast<uintptr_t>(registers[REG_RIP]);
    const size_t length = CpuidLength(reinterpret_cast<const u8*>(address));
    if (length == 0) {
        return false;
    }
    if (address >= guest_begin.load(std::memory_order_relaxed) &&
        address < guest_end.load(std::memory_order_relaxed) &&
        Libraries::Kernel::g_curthread != nullptr) {
        const auto stack = static_cast<uintptr_t>(registers[REG_RSP]) - FaultStackSize;
        *reinterpret_cast<u64*>(stack) = address + length;
        registers[REG_RSP] = stack;
        registers[REG_RIP] = reinterpret_cast<uintptr_t>(fault_trampoline->getCode());
        return true;
    }

    const int saved_errno = errno;
    sigset_t blocked, previous;
    sigfillset(&blocked);
    if (sigprocmask(SIG_SETMASK, &blocked, &previous) != 0) {
        errno = saved_errno;
        return false;
    }
    const bool enabled = syscall(SYS_arch_prctl, ARCH_SET_CPUID, 1) == 0;
    if (enabled) {
        u32 result[4];
        Xbyak::util::Cpu::getCpuidEx(static_cast<u32>(registers[REG_RAX]),
                                     static_cast<u32>(registers[REG_RCX]), result);
        if (syscall(SYS_arch_prctl, ARCH_SET_CPUID, 0) != 0) {
            _exit(1);
        }
        registers[REG_RAX] = result[0];
        registers[REG_RBX] = result[1];
        registers[REG_RCX] = result[2];
        registers[REG_RDX] = result[3];
        registers[REG_RIP] += length;
    }
    sigprocmask(SIG_SETMASK, &previous, nullptr);
    errno = saved_errno;
    return enabled;
}

} // namespace

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
    fault_trampoline = std::make_unique<Xbyak::CodeGenerator>(4096);
    GenerateCpuIdInstruction(*fault_trampoline, CpuIdInstruction::Cpuid, 0, true);
    fault_trampoline->readyRE();
    Signals::Instance()->RegisterAccessViolationHandler(HandleCpuidFault, 2);
}

void EnableCpuIdFaulting() {
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
}

} // namespace Core
