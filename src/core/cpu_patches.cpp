// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <Zydis/Zydis.h>
#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>
#include "common/alignment.h"
#include "common/arch.h"
#include "common/assert.h"
#include "common/decoder.h"
#include "common/signal_context.h"
#include "common/types.h"
#include "core/signals.h"
#include "core/tls.h"
#include "cpu_patches.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#ifdef __APPLE__
#include <half.hpp>
#include <sys/sysctl.h>
#endif
#endif

using namespace Xbyak::util;

namespace Core {

static Xbyak::Reg ZydisToXbyakRegister(const ZydisRegister reg) {
    if (reg >= ZYDIS_REGISTER_EAX && reg <= ZYDIS_REGISTER_R15D) {
        return Xbyak::Reg32(reg - ZYDIS_REGISTER_EAX + Xbyak::Operand::EAX);
    }
    if (reg >= ZYDIS_REGISTER_RAX && reg <= ZYDIS_REGISTER_R15) {
        return Xbyak::Reg64(reg - ZYDIS_REGISTER_RAX + Xbyak::Operand::RAX);
    }
    if (reg >= ZYDIS_REGISTER_XMM0 && reg <= ZYDIS_REGISTER_XMM31) {
        return Xbyak::Xmm(reg - ZYDIS_REGISTER_XMM0 + xmm0.getIdx());
    }
    if (reg >= ZYDIS_REGISTER_YMM0 && reg <= ZYDIS_REGISTER_YMM31) {
        return Xbyak::Ymm(reg - ZYDIS_REGISTER_YMM0 + ymm0.getIdx());
    }
    UNREACHABLE_MSG("Unsupported register: {}", static_cast<u32>(reg));
}

static Xbyak::Reg ZydisToXbyakRegisterOperand(const ZydisDecodedOperand& operand) {
    ASSERT_MSG(operand.type == ZYDIS_OPERAND_TYPE_REGISTER,
               "Expected register operand, got type: {}", static_cast<u32>(operand.type));

    return ZydisToXbyakRegister(operand.reg.value);
}

static Xbyak::Address ZydisToXbyakMemoryOperand(const ZydisDecodedOperand& operand) {
    ASSERT_MSG(operand.type == ZYDIS_OPERAND_TYPE_MEMORY, "Expected memory operand, got type: {}",
               static_cast<u32>(operand.type));

    if (operand.mem.base == ZYDIS_REGISTER_RIP) {
        return ptr[rip + operand.mem.disp.value];
    }

    Xbyak::RegExp expression{};
    if (operand.mem.base != ZYDIS_REGISTER_NONE) {
        expression = expression + ZydisToXbyakRegister(operand.mem.base);
    }
    if (operand.mem.index != ZYDIS_REGISTER_NONE) {
        if (operand.mem.scale != 0) {
            expression = expression + ZydisToXbyakRegister(operand.mem.index) * operand.mem.scale;
        } else {
            expression = expression + ZydisToXbyakRegister(operand.mem.index);
        }
    }
    if (operand.mem.disp.size != 0 && operand.mem.disp.value != 0) {
        expression = expression + operand.mem.disp.value;
    }

    return ptr[expression];
}

static u64 ZydisToXbyakImmediateOperand(const ZydisDecodedOperand& operand) {
    ASSERT_MSG(operand.type == ZYDIS_OPERAND_TYPE_IMMEDIATE,
               "Expected immediate operand, got type: {}", static_cast<u32>(operand.type));
    return operand.imm.value.u;
}

static std::unique_ptr<Xbyak::Operand> ZydisToXbyakOperand(const ZydisDecodedOperand& operand) {
    switch (operand.type) {
    case ZYDIS_OPERAND_TYPE_REGISTER: {
        return std::make_unique<Xbyak::Reg>(ZydisToXbyakRegisterOperand(operand));
    }
    case ZYDIS_OPERAND_TYPE_MEMORY: {
        return std::make_unique<Xbyak::Address>(ZydisToXbyakMemoryOperand(operand));
    }
    default:
        UNREACHABLE_MSG("Unsupported operand type: {}", static_cast<u32>(operand.type));
    }
}

static bool OperandUsesRegister(const Xbyak::Operand* operand, int index) {
    if (operand->isREG()) {
        return operand->getIdx() == index;
    }
    if (operand->isMEM()) {
        const Xbyak::RegExp& reg_exp = operand->getAddress().getRegExp();
        return reg_exp.getBase().getIdx() == index || reg_exp.getIndex().getIdx() == index;
    }
    UNREACHABLE_MSG("Unsupported operand kind: {}", static_cast<u32>(operand->getKind()));
}

static bool IsRegisterAllocated(
    const std::initializer_list<const Xbyak::Operand*>& allocated_registers, const int index) {
    return std::ranges::find_if(allocated_registers.begin(), allocated_registers.end(),
                                [index](const Xbyak::Operand* operand) {
                                    return OperandUsesRegister(operand, index);
                                }) != allocated_registers.end();
}

static Xbyak::Reg AllocateScratchRegister(
    const std::initializer_list<const Xbyak::Operand*> allocated_registers, const u32 bits) {
    for (int index = Xbyak::Operand::R8; index <= Xbyak::Operand::R15; index++) {
        if (!IsRegisterAllocated(allocated_registers, index)) {
            return Xbyak::Reg32e(index, static_cast<int>(bits));
        }
    }
    UNREACHABLE_MSG("Out of scratch registers!");
}

#ifdef __APPLE__

static pthread_key_t stack_pointer_slot;
static pthread_key_t patch_stack_slot;
static std::once_flag patch_context_slots_init_flag;
static constexpr u32 patch_stack_size = 0x1000;

static_assert(sizeof(void*) == sizeof(u64),
              "Cannot fit a register inside a thread local storage slot.");

static void FreePatchStack(void* patch_stack) {
    // Subtract back to the bottom of the stack for free.
    std::free(static_cast<u8*>(patch_stack) - patch_stack_size);
}

static void InitializePatchContextSlots() {
    ASSERT_MSG(pthread_key_create(&stack_pointer_slot, nullptr) == 0,
               "Unable to allocate thread-local register for stack pointer.");
    ASSERT_MSG(pthread_key_create(&patch_stack_slot, FreePatchStack) == 0,
               "Unable to allocate thread-local register for patch stack.");
}

void InitializeThreadPatchStack() {
    std::call_once(patch_context_slots_init_flag, InitializePatchContextSlots);

    pthread_setspecific(patch_stack_slot,
                        static_cast<u8*>(std::malloc(patch_stack_size)) + patch_stack_size);
}

/// Saves the stack pointer to thread local storage and loads the patch stack.
static void SaveStack(Xbyak::CodeGenerator& c) {
    std::call_once(patch_context_slots_init_flag, InitializePatchContextSlots);

    // Save original stack pointer and load patch stack.
    c.putSeg(gs);
    c.mov(qword[reinterpret_cast<void*>(stack_pointer_slot * sizeof(void*))], rsp);
    c.putSeg(gs);
    c.mov(rsp, qword[reinterpret_cast<void*>(patch_stack_slot * sizeof(void*))]);
}

/// Restores the stack pointer from thread local storage.
static void RestoreStack(Xbyak::CodeGenerator& c) {
    std::call_once(patch_context_slots_init_flag, InitializePatchContextSlots);

    // Save patch stack pointer and load original stack.
    c.putSeg(gs);
    c.mov(qword[reinterpret_cast<void*>(patch_stack_slot * sizeof(void*))], rsp);
    c.putSeg(gs);
    c.mov(rsp, qword[reinterpret_cast<void*>(stack_pointer_slot * sizeof(void*))]);
}

#else

// These utilities are not implemented as we can't save anything to thread local storage without
// temporary registers.
void InitializeThreadPatchStack() {
    // No-op
}

/// Saves the stack pointer to thread local storage and loads the patch stack.
static void SaveStack(Xbyak::CodeGenerator& c) {
    UNIMPLEMENTED();
}

/// Restores the stack pointer from thread local storage.
static void RestoreStack(Xbyak::CodeGenerator& c) {
    UNIMPLEMENTED();
}

#endif

/// Switches to the patch stack, saves registers, and restores the original stack.
static void SaveRegisters(Xbyak::CodeGenerator& c, const std::initializer_list<Xbyak::Reg> regs) {
    SaveStack(c);
    for (const auto& reg : regs) {
        c.push(reg.cvt64());
    }
    RestoreStack(c);
}

/// Switches to the patch stack, restores registers, and restores the original stack.
static void RestoreRegisters(Xbyak::CodeGenerator& c,
                             const std::initializer_list<Xbyak::Reg> regs) {
    SaveStack(c);
    for (const auto& reg : regs) {
        c.pop(reg.cvt64());
    }
    RestoreStack(c);
}

/// Switches to the patch stack and stores all registers.
static void SaveContext(Xbyak::CodeGenerator& c, bool save_flags = false) {
    SaveStack(c);
    for (int reg = Xbyak::Operand::RAX; reg <= Xbyak::Operand::R15; reg++) {
        c.push(Xbyak::Reg64(reg));
    }
    c.lea(rsp, ptr[rsp - 32 * 16]);
    for (int reg = 0; reg <= 15; reg++) {
        c.vmovdqu(ptr[rsp + 32 * reg], Xbyak::Ymm(reg));
    }
    if (save_flags) {
        c.pushfq();
    }
}

/// Restores all registers and restores the original stack.
/// If the destination is a register, it is not restored to preserve the output.
static void RestoreContext(Xbyak::CodeGenerator& c, const Xbyak::Operand& dst,
                           bool restore_flags = false) {
    if (restore_flags) {
        c.popfq();
    }
    for (int reg = 15; reg >= 0; reg--) {
        if ((!dst.isXMM() && !dst.isYMM()) || dst.getIdx() != reg) {
            c.vmovdqu(Xbyak::Ymm(reg), ptr[rsp + 32 * reg]);
        }
    }
    c.lea(rsp, ptr[rsp + 32 * 16]);
    for (int reg = Xbyak::Operand::R15; reg >= Xbyak::Operand::RAX; reg--) {
        if (!dst.isREG() || dst.getIdx() != reg) {
            c.pop(Xbyak::Reg64(reg));
        } else {
            c.lea(rsp, ptr[rsp + 8]);
        }
    }
    RestoreStack(c);
}

#ifdef __APPLE__

static void GenerateANDN(const ZydisDecodedOperand* operands, Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);
    const auto src1 = ZydisToXbyakRegisterOperand(operands[1]);
    const auto src2 = ZydisToXbyakOperand(operands[2]);

    // Check if src2 is a memory operand or a register different to dst.
    // In those cases, we don't need to use a temporary register and are free to modify dst.
    // In cases where dst and src2 are the same register, a temporary needs to be used to avoid
    // modifying src2.
    bool src2_uses_dst = false;
    if (src2->isMEM()) {
        const auto base = src2->getAddress().getRegExp().getBase().getIdx();
        const auto index = src2->getAddress().getRegExp().getIndex().getIdx();
        src2_uses_dst = base == dst.getIdx() || index == dst.getIdx();
    } else {
        ASSERT(src2->isREG());
        src2_uses_dst = src2->getReg() == dst;
    }

    if (!src2_uses_dst) {
        if (dst != src1)
            c.mov(dst, src1);
        c.not_(dst);
        c.and_(dst, *src2);
    } else {
        const auto scratch = AllocateScratchRegister({&dst, &src1, src2.get()}, dst.getBit());

        SaveRegisters(c, {scratch});

        c.mov(scratch, src1);
        c.not_(scratch);
        c.and_(scratch, *src2);
        c.mov(dst, scratch);

        RestoreRegisters(c, {scratch});
    }
}

static void GenerateBEXTR(const ZydisDecodedOperand* operands, Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);
    const auto src = ZydisToXbyakOperand(operands[1]);
    const auto start_len = ZydisToXbyakRegisterOperand(operands[2]);

    const Xbyak::Reg32e shift(Xbyak::Operand::RCX, static_cast<int>(start_len.getBit()));
    const auto scratch1 =
        AllocateScratchRegister({&dst, src.get(), &start_len, &shift}, dst.getBit());
    const auto scratch2 =
        AllocateScratchRegister({&dst, src.get(), &start_len, &shift, &scratch1}, dst.getBit());

    if (dst.getIdx() == shift.getIdx()) {
        SaveRegisters(c, {scratch1, scratch2});
    } else {
        SaveRegisters(c, {scratch1, scratch2, shift});
    }

    c.mov(scratch1, *src);
    if (shift.getIdx() != start_len.getIdx()) {
        c.mov(shift, start_len);
    }

    c.shr(scratch1, shift.cvt8());
    c.shr(shift, 8);
    c.mov(scratch2, 1);
    c.shl(scratch2, shift.cvt8());
    c.dec(scratch2);

    c.mov(dst, scratch1);
    c.and_(dst, scratch2);

    if (dst.getIdx() == shift.getIdx()) {
        RestoreRegisters(c, {scratch2, scratch1});
    } else {
        RestoreRegisters(c, {shift, scratch2, scratch1});
    }
}

static void GenerateBLSI(const ZydisDecodedOperand* operands, Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);
    const auto src = ZydisToXbyakOperand(operands[1]);

    const auto scratch = AllocateScratchRegister({&dst, src.get()}, dst.getBit());

    SaveRegisters(c, {scratch});

    // BLSI sets CF to zero if source is zero, otherwise it sets CF to one.
    Xbyak::Label clear_carry, end;

    c.mov(scratch, *src);
    c.neg(scratch); // NEG, like BLSI, clears CF if the source is zero and sets it otherwise
    c.jnc(clear_carry);

    c.and_(scratch, *src);
    c.stc(); // setting/clearing carry needs to happen after the AND because that clears CF
    c.jmp(end);

    c.L(clear_carry);
    c.and_(scratch, *src);
    // We don't need to clear carry here since AND does that for us

    c.L(end);
    c.mov(dst, scratch);

    RestoreRegisters(c, {scratch});
}

static void GenerateBLSMSK(const ZydisDecodedOperand* operands, Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);
    const auto src = ZydisToXbyakOperand(operands[1]);

    const auto scratch = AllocateScratchRegister({&dst, src.get()}, dst.getBit());

    SaveRegisters(c, {scratch});

    Xbyak::Label clear_carry, end;

    // BLSMSK sets CF to zero if source is NOT zero, otherwise it sets CF to one.
    c.mov(scratch, *src);
    c.test(scratch, scratch);
    c.jnz(clear_carry);

    c.dec(scratch);
    c.xor_(scratch, *src);
    c.stc();
    c.jmp(end);

    c.L(clear_carry);
    c.dec(scratch);
    c.xor_(scratch, *src);
    // We don't need to clear carry here since XOR does that for us

    c.L(end);
    c.mov(dst, scratch);

    RestoreRegisters(c, {scratch});
}

static void GenerateBLSR(const ZydisDecodedOperand* operands, Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);
    const auto src = ZydisToXbyakOperand(operands[1]);

    const auto scratch = AllocateScratchRegister({&dst, src.get()}, dst.getBit());

    SaveRegisters(c, {scratch});

    Xbyak::Label clear_carry, end;

    // BLSR sets CF to zero if source is NOT zero, otherwise it sets CF to one.
    c.mov(scratch, *src);
    c.test(scratch, scratch);
    c.jnz(clear_carry);

    c.dec(scratch);
    c.and_(scratch, *src);
    c.stc();
    c.jmp(end);

    c.L(clear_carry);
    c.dec(scratch);
    c.and_(scratch, *src);
    // We don't need to clear carry here since AND does that for us

    c.L(end);
    c.mov(dst, scratch);

    RestoreRegisters(c, {scratch});
}

static __attribute__((sysv_abi)) void PerformVCVTPH2PS(float* out, const half_float::half* in,
                                                       const u32 count) {
    for (u32 i = 0; i < count; i++) {
        out[i] = half_float::half_cast<float>(in[i]);
    }
}

static void GenerateVCVTPH2PS(const ZydisDecodedOperand* operands, Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);
    const auto src = ZydisToXbyakOperand(operands[1]);

    const auto float_count = dst.getBit() / 32;
    const auto byte_count = float_count * 4;

    SaveContext(c, true);

    // Allocate stack space for outputs and load into first parameter.
    c.sub(rsp, byte_count);
    c.mov(rdi, rsp);

    if (src->isXMM()) {
        // Allocate stack space for inputs and load into second parameter.
        c.sub(rsp, byte_count);
        c.mov(rsi, rsp);

        // Move input to the allocated space.
        c.movdqu(ptr[rsp], *reinterpret_cast<Xbyak::Xmm*>(src.get()));
    } else {
        c.lea(rsi, src->getAddress());
    }

    // Load float count into third parameter.
    c.mov(rdx, float_count);

    c.mov(rax, reinterpret_cast<u64>(PerformVCVTPH2PS));
    c.call(rax);

    if (src->isXMM()) {
        // Clean up after inputs space.
        c.add(rsp, byte_count);
    }

    // Load outputs into destination register and clean up space.
    if (dst.isYMM()) {
        c.vmovdqu(*reinterpret_cast<const Xbyak::Ymm*>(&dst), ptr[rsp]);
    } else {
        c.movdqu(*reinterpret_cast<const Xbyak::Xmm*>(&dst), ptr[rsp]);
    }
    c.add(rsp, byte_count);

    RestoreContext(c, dst, true);
}

using SingleToHalfFloatConverter = half_float::half (*)(float);
static const SingleToHalfFloatConverter SingleToHalfFloatConverters[4] = {
    half_float::half_cast<half_float::half, std::round_to_nearest, float>,
    half_float::half_cast<half_float::half, std::round_toward_neg_infinity, float>,
    half_float::half_cast<half_float::half, std::round_toward_infinity, float>,
    half_float::half_cast<half_float::half, std::round_toward_zero, float>,
};

static __attribute__((sysv_abi)) void PerformVCVTPS2PH(half_float::half* out, const float* in,
                                                       const u32 count, const u8 rounding_mode) {
    const auto conversion_func = SingleToHalfFloatConverters[rounding_mode];

    for (u32 i = 0; i < count; i++) {
        out[i] = conversion_func(in[i]);
    }
}

static void GenerateVCVTPS2PH(const ZydisDecodedOperand* operands, Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakOperand(operands[0]);
    const auto src = ZydisToXbyakRegisterOperand(operands[1]);
    const auto ctrl = ZydisToXbyakImmediateOperand(operands[2]);

    const auto float_count = src.getBit() / 32;
    const auto byte_count = float_count * 4;

    SaveContext(c, true);

    if (dst->isXMM()) {
        // Allocate stack space for outputs and load into first parameter.
        c.sub(rsp, byte_count);
        c.mov(rdi, rsp);
    } else {
        c.lea(rdi, dst->getAddress());
    }

    // Allocate stack space for inputs and load into second parameter.
    c.sub(rsp, byte_count);
    c.mov(rsi, rsp);

    // Move input to the allocated space.
    if (src.isYMM()) {
        c.vmovdqu(ptr[rsp], *reinterpret_cast<const Xbyak::Ymm*>(&src));
    } else {
        c.movdqu(ptr[rsp], *reinterpret_cast<const Xbyak::Xmm*>(&src));
    }

    // Load float count into third parameter.
    c.mov(rdx, float_count);

    // Load rounding mode into fourth parameter.
    if (ctrl & 4) {
        // Load from MXCSR.RC.
        c.stmxcsr(ptr[rsp - 4]);
        c.mov(rcx, ptr[rsp - 4]);
        c.shr(rcx, 13);
        c.and_(rcx, 3);
    } else {
        c.mov(rcx, ctrl & 3);
    }

    c.mov(rax, reinterpret_cast<u64>(PerformVCVTPS2PH));
    c.call(rax);

    // Clean up after inputs space.
    c.add(rsp, byte_count);

    if (dst->isXMM()) {
        // Load outputs into destination register and clean up space.
        c.movdqu(*reinterpret_cast<Xbyak::Xmm*>(dst.get()), ptr[rsp]);
        c.add(rsp, byte_count);
    }

    RestoreContext(c, *dst, true);
}

static bool FilterRosetta2Only(const ZydisDecodedOperand*) {
    int ret = 0;
    size_t size = sizeof(ret);
    if (sysctlbyname("sysctl.proc_translated", &ret, &size, nullptr, 0) != 0) {
        return false;
    }
    return ret;
}

#else // __APPLE__

static bool FilterTcbAccess(const ZydisDecodedOperand* operands) {
    const auto& dst_op = operands[0];
    const auto& src_op = operands[1];

    // Patch only 'mov (64-bit register), fs:[0]'
    return src_op.type == ZYDIS_OPERAND_TYPE_MEMORY && src_op.mem.segment == ZYDIS_REGISTER_FS &&
           src_op.mem.base == ZYDIS_REGISTER_NONE && src_op.mem.index == ZYDIS_REGISTER_NONE &&
           src_op.mem.disp.value == 0 && dst_op.reg.value >= ZYDIS_REGISTER_RAX &&
           dst_op.reg.value <= ZYDIS_REGISTER_R15;
}

static void GenerateTcbAccess(const ZydisDecodedOperand* operands, Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);

#if defined(_WIN32)
    // The following logic is based on the Kernel32.dll asm of TlsGetValue
    static constexpr u32 TlsSlotsOffset = 0x1480;
    static constexpr u32 TlsExpansionSlotsOffset = 0x1780;
    static constexpr u32 TlsMinimumAvailable = 64;

    const auto slot = GetTcbKey();

    // Load the pointer to the table of TLS slots.
    c.putSeg(gs);
    if (slot < TlsMinimumAvailable) {
        // Load the pointer to TLS slots.
        c.mov(dst, ptr[reinterpret_cast<void*>(TlsSlotsOffset + slot * sizeof(LPVOID))]);
    } else {
        const u32 tls_index = slot - TlsMinimumAvailable;

        // Load the pointer to the table of TLS expansion slots.
        c.mov(dst, ptr[reinterpret_cast<void*>(TlsExpansionSlotsOffset)]);
        // Load the pointer to our buffer.
        c.mov(dst, qword[dst + tls_index * sizeof(LPVOID)]);
    }
#else
    const auto src = ZydisToXbyakMemoryOperand(operands[1]);

    // Replace fs read with gs read.
    c.putSeg(gs);
    c.mov(dst, src);
#endif
}

#endif // __APPLE__

static bool FilterNoSSE4a(const ZydisDecodedOperand*) {
    Cpu cpu;
    return !cpu.has(Cpu::tSSE4a);
}

static void GenerateEXTRQ(const ZydisDecodedOperand* operands, Xbyak::CodeGenerator& c) {
    bool immediateForm = operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                         operands[2].type == ZYDIS_OPERAND_TYPE_IMMEDIATE;

    ASSERT_MSG(operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER, "operand 0 must be a register");

    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);

    ASSERT_MSG(dst.isXMM(), "operand 0 must be an XMM register");

    Xbyak::Xmm xmm_dst = *reinterpret_cast<const Xbyak::Xmm*>(&dst);

    if (immediateForm) {
        u8 length = operands[1].imm.value.u & 0x3F;
        u8 index = operands[2].imm.value.u & 0x3F;

        LOG_DEBUG(Core, "Patching immediate form EXTRQ, length: {}, index: {}", length, index);

        const Xbyak::Reg64 scratch1 = rax;
        const Xbyak::Reg64 scratch2 = rcx;

        // Set rsp to before red zone and save scratch registers
        c.lea(rsp, ptr[rsp - 128]);
        c.pushfq();
        c.push(scratch1);
        c.push(scratch2);

        u64 mask;
        if (length == 0) {
            length = 64; // for the check below
            mask = 0xFFFF'FFFF'FFFF'FFFF;
        } else {
            mask = (1ULL << length) - 1;
        }

        ASSERT_MSG(length + index <= 64, "length + index must be less than or equal to 64.");

        // Get lower qword from xmm register
        c.vmovq(scratch1, xmm_dst);

        if (index != 0) {
            c.shr(scratch1, index);
        }

        // We need to move mask to a register because we can't use all the possible
        // immediate values with `and reg, imm32`
        c.mov(scratch2, mask);
        c.and_(scratch1, scratch2);

        // Writeback to xmm register, extrq instruction says top 64-bits are undefined so we don't
        // care to preserve them
        c.vmovq(xmm_dst, scratch1);

        c.pop(scratch2);
        c.pop(scratch1);
        c.popfq();
        c.lea(rsp, ptr[rsp + 128]);
    } else {
        ASSERT_MSG(operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                       operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                       operands[0].reg.value >= ZYDIS_REGISTER_XMM0 &&
                       operands[0].reg.value <= ZYDIS_REGISTER_XMM15 &&
                       operands[1].reg.value >= ZYDIS_REGISTER_XMM0 &&
                       operands[1].reg.value <= ZYDIS_REGISTER_XMM15,
                   "Unexpected operand types for EXTRQ instruction");

        const auto src = ZydisToXbyakRegisterOperand(operands[1]);

        ASSERT_MSG(src.isXMM(), "operand 1 must be an XMM register");

        Xbyak::Xmm xmm_src = *reinterpret_cast<const Xbyak::Xmm*>(&src);

        const Xbyak::Reg64 scratch1 = rax;
        const Xbyak::Reg64 scratch2 = rcx;
        const Xbyak::Reg64 mask = rdx;

        Xbyak::Label length_zero, end;

        c.lea(rsp, ptr[rsp - 128]);
        c.pushfq();
        c.push(scratch1);
        c.push(scratch2);
        c.push(mask);

        // Construct the mask out of the length that resides in bottom 6 bits of source xmm
        c.vmovq(scratch1, xmm_src);
        c.mov(scratch2, scratch1);
        c.and_(scratch2, 0x3F);
        c.jz(length_zero);

        // mask = (1ULL << length) - 1
        c.mov(mask, 1);
        c.shl(mask, cl);
        c.dec(mask);
        c.jmp(end);

        c.L(length_zero);
        c.mov(mask, 0xFFFF'FFFF'FFFF'FFFF);

        c.L(end);

        // Get the shift amount and store it in scratch2
        c.shr(scratch1, 8);
        c.and_(scratch1, 0x3F);
        c.mov(scratch2, scratch1); // cl now contains the shift amount

        c.vmovq(scratch1, xmm_dst);
        c.shr(scratch1, cl);
        c.and_(scratch1, mask);
        c.vmovq(xmm_dst, scratch1);

        c.pop(mask);
        c.pop(scratch2);
        c.pop(scratch1);
        c.popfq();
        c.lea(rsp, ptr[rsp + 128]);
    }
}

static void GenerateINSERTQ(const ZydisDecodedOperand* operands, Xbyak::CodeGenerator& c) {
    bool immediateForm = operands[2].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                         operands[3].type == ZYDIS_OPERAND_TYPE_IMMEDIATE;

    ASSERT_MSG(operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                   operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER,
               "operands 0 and 1 must be registers.");

    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);
    const auto src = ZydisToXbyakRegisterOperand(operands[1]);

    ASSERT_MSG(dst.isXMM() && src.isXMM(), "operands 0 and 1 must be xmm registers.");

    Xbyak::Xmm xmm_dst = *reinterpret_cast<const Xbyak::Xmm*>(&dst);
    Xbyak::Xmm xmm_src = *reinterpret_cast<const Xbyak::Xmm*>(&src);

    if (immediateForm) {
        u8 length = operands[2].imm.value.u & 0x3F;
        u8 index = operands[3].imm.value.u & 0x3F;

        const Xbyak::Reg64 scratch1 = rax;
        const Xbyak::Reg64 scratch2 = rcx;
        const Xbyak::Reg64 mask = rdx;

        // Set rsp to before red zone and save scratch registers
        c.lea(rsp, ptr[rsp - 128]);
        c.pushfq();
        c.push(scratch1);
        c.push(scratch2);
        c.push(mask);

        u64 mask_value;
        if (length == 0) {
            length = 64; // for the check below
            mask_value = 0xFFFF'FFFF'FFFF'FFFF;
        } else {
            mask_value = (1ULL << length) - 1;
        }

        ASSERT_MSG(length + index <= 64, "length + index must be less than or equal to 64.");

        c.vmovq(scratch1, xmm_src);
        c.vmovq(scratch2, xmm_dst);
        c.mov(mask, mask_value);

        // src &= mask
        c.and_(scratch1, mask);

        // src <<= index
        c.shl(scratch1, index);

        // dst &= ~(mask << index)
        mask_value = ~(mask_value << index);
        c.mov(mask, mask_value);
        c.and_(scratch2, mask);

        // dst |= src
        c.or_(scratch2, scratch1);

        // Insert scratch2 into low 64 bits of dst, upper 64 bits are unaffected
        c.vpinsrq(xmm_dst, xmm_dst, scratch2, 0);

        c.pop(mask);
        c.pop(scratch2);
        c.pop(scratch1);
        c.popfq();
        c.lea(rsp, ptr[rsp + 128]);
    } else {
        ASSERT_MSG(operands[2].type == ZYDIS_OPERAND_TYPE_UNUSED &&
                       operands[3].type == ZYDIS_OPERAND_TYPE_UNUSED,
                   "operands 2 and 3 must be unused for register form.");

        const Xbyak::Reg64 scratch1 = rax;
        const Xbyak::Reg64 scratch2 = rcx;
        const Xbyak::Reg64 index = rdx;
        const Xbyak::Reg64 mask = rbx;

        Xbyak::Label length_zero, end;

        c.lea(rsp, ptr[rsp - 128]);
        c.pushfq();
        c.push(scratch1);
        c.push(scratch2);
        c.push(index);
        c.push(mask);

        // Get upper 64 bits of src and copy it to mask and index
        c.vpextrq(index, xmm_src, 1);
        c.mov(mask, index);

        // When length is 0, set it to 64
        c.and_(mask, 0x3F); // mask now holds the length
        c.jz(length_zero);  // Check if length is 0 and set mask to all 1s if it is

        // Create a mask out of the length
        c.mov(cl, mask.cvt8());
        c.mov(mask, 1);
        c.shl(mask, cl);
        c.dec(mask);
        c.jmp(end);

        c.L(length_zero);
        c.mov(mask, 0xFFFF'FFFF'FFFF'FFFF);

        c.L(end);
        // Get index to insert at
        c.shr(index, 8);
        c.and_(index, 0x3F);

        // src &= mask
        c.vmovq(scratch1, xmm_src);
        c.and_(scratch1, mask);

        // mask = ~(mask << index)
        c.mov(cl, index.cvt8());
        c.shl(mask, cl);
        c.not_(mask);

        // src <<= index
        c.shl(scratch1, cl);

        // dst = (dst & mask) | src
        c.vmovq(scratch2, xmm_dst);
        c.and_(scratch2, mask);
        c.or_(scratch2, scratch1);

        // Upper 64 bits are undefined in insertq
        c.vmovq(xmm_dst, scratch2);

        c.pop(mask);
        c.pop(index);
        c.pop(scratch2);
        c.pop(scratch1);
        c.popfq();
        c.lea(rsp, ptr[rsp + 128]);
    }
}

using PatchFilter = bool (*)(const ZydisDecodedOperand*);
using InstructionGenerator = void (*)(const ZydisDecodedOperand*, Xbyak::CodeGenerator&);
struct PatchInfo {
    /// Filter for more granular patch conditions past just the instruction mnemonic.
    PatchFilter filter;

    /// Generator for the patch/trampoline.
    InstructionGenerator generator;

    /// Whether to use a trampoline for this patch.
    bool trampoline;
};

static const std::unordered_map<ZydisMnemonic, PatchInfo> Patches = {
#if defined(_WIN32)
    // Windows needs a trampoline.
    {ZYDIS_MNEMONIC_MOV, {FilterTcbAccess, GenerateTcbAccess, true}},
#elif !defined(__APPLE__)
    {ZYDIS_MNEMONIC_MOV, {FilterTcbAccess, GenerateTcbAccess, false}},
#endif

    {ZYDIS_MNEMONIC_EXTRQ, {FilterNoSSE4a, GenerateEXTRQ, true}},
    {ZYDIS_MNEMONIC_INSERTQ, {FilterNoSSE4a, GenerateINSERTQ, true}},

#ifdef __APPLE__
    // Patches for instruction sets not supported by Rosetta 2.
    // BMI1
    {ZYDIS_MNEMONIC_ANDN, {FilterRosetta2Only, GenerateANDN, true}},
    {ZYDIS_MNEMONIC_BEXTR, {FilterRosetta2Only, GenerateBEXTR, true}},
    {ZYDIS_MNEMONIC_BLSI, {FilterRosetta2Only, GenerateBLSI, true}},
    {ZYDIS_MNEMONIC_BLSMSK, {FilterRosetta2Only, GenerateBLSMSK, true}},
    {ZYDIS_MNEMONIC_BLSR, {FilterRosetta2Only, GenerateBLSR, true}},
    // F16C
    {ZYDIS_MNEMONIC_VCVTPH2PS, {FilterRosetta2Only, GenerateVCVTPH2PS, true}},
    {ZYDIS_MNEMONIC_VCVTPS2PH, {FilterRosetta2Only, GenerateVCVTPS2PH, true}},
#endif
};

static std::once_flag init_flag;

struct PatchModule {
    /// Mutex controlling access to module code regions.
    std::mutex mutex{};

    /// Start of the module.
    u8* start;

    /// End of the module.
    u8* end;

    /// Tracker for patched code locations.
    std::set<u8*> patched;

    /// Code generator for patching the module.
    Xbyak::CodeGenerator patch_gen;

    /// Code generator for writing trampoline patches.
    Xbyak::CodeGenerator trampoline_gen;

    PatchModule(u8* module_ptr, const u64 module_size, u8* trampoline_ptr,
                const u64 trampoline_size)
        : start(module_ptr), end(module_ptr + module_size), patch_gen(module_size, module_ptr),
          trampoline_gen(trampoline_size, trampoline_ptr) {}
};
static std::map<u64, PatchModule> modules;

static PatchModule* GetModule(const void* ptr) {
    auto upper_bound = modules.upper_bound(reinterpret_cast<u64>(ptr));
    if (upper_bound == modules.begin()) {
        return nullptr;
    }
    return &(std::prev(upper_bound)->second);
}

/// Returns a boolean indicating whether the instruction was patched, and the offset to advance past
/// whatever is at the current code pointer.
static std::pair<bool, u64> TryPatch(u8* code, PatchModule* module) {
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    const auto status = Common::Decoder::Instance()->decodeInstruction(instruction, operands, code,
                                                                       module->end - code);
    if (!ZYAN_SUCCESS(status)) {
        return std::make_pair(false, 1);
    }

    if (Patches.contains(instruction.mnemonic)) {
        const auto& patch_info = Patches.at(instruction.mnemonic);
        bool needs_trampoline = patch_info.trampoline;
        if (patch_info.filter(operands)) {
            auto& patch_gen = module->patch_gen;

            if (needs_trampoline && instruction.length < 5) {
                // Trampoline is needed but instruction is too short to patch.
                // Return false and length to fall back to the illegal instruction handler,
                // or to signal to AOT compilation that this instruction should be skipped and
                // handled at runtime.
                return std::make_pair(false, instruction.length);
            }

            // Reset state and move to current code position.
            patch_gen.reset();
            patch_gen.setSize(code - patch_gen.getCode());

            if (needs_trampoline) {
                auto& trampoline_gen = module->trampoline_gen;
                const auto trampoline_ptr = trampoline_gen.getCurr();

                patch_info.generator(operands, trampoline_gen);

                // Return to the following instruction at the end of the trampoline.
                trampoline_gen.jmp(code + instruction.length);

                // Replace instruction with near jump to the trampoline.
                patch_gen.jmp(trampoline_ptr, Xbyak::CodeGenerator::LabelType::T_NEAR);
            } else {
                patch_info.generator(operands, patch_gen);
            }

            const auto patch_size = patch_gen.getCurr() - code;
            if (patch_size > 0) {
                ASSERT_MSG(instruction.length >= patch_size,
                           "Instruction {} with length {} is too short to replace at: {}",
                           ZydisMnemonicGetString(instruction.mnemonic), instruction.length,
                           fmt::ptr(code));

                // Fill remaining space with nops.
                patch_gen.nop(instruction.length - patch_size);

                module->patched.insert(code);
                LOG_DEBUG(Core, "Patched instruction '{}' at: {}",
                          ZydisMnemonicGetString(instruction.mnemonic), fmt::ptr(code));
                return std::make_pair(true, instruction.length);
            }
        }
    }

    return std::make_pair(false, instruction.length);
}

#if defined(ARCH_X86_64)

static bool TryExecuteIllegalInstruction(void* ctx, void* code_address) {
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    const auto status =
        Common::Decoder::Instance()->decodeInstruction(instruction, operands, code_address);

    switch (instruction.mnemonic) {
    case ZYDIS_MNEMONIC_EXTRQ: {
        bool immediateForm = operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                             operands[2].type == ZYDIS_OPERAND_TYPE_IMMEDIATE;
        if (immediateForm) {
            LOG_CRITICAL(Core, "EXTRQ immediate form should have been patched at code address: {}",
                         fmt::ptr(code_address));
            return false;
        } else {
            ASSERT_MSG(operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                           operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                           operands[0].reg.value >= ZYDIS_REGISTER_XMM0 &&
                           operands[0].reg.value <= ZYDIS_REGISTER_XMM15 &&
                           operands[1].reg.value >= ZYDIS_REGISTER_XMM0 &&
                           operands[1].reg.value <= ZYDIS_REGISTER_XMM15,
                       "Unexpected operand types for EXTRQ instruction");

            const auto dstIndex = operands[0].reg.value - ZYDIS_REGISTER_XMM0;
            const auto srcIndex = operands[1].reg.value - ZYDIS_REGISTER_XMM0;

            const auto dst = Common::GetXmmPointer(ctx, dstIndex);
            const auto src = Common::GetXmmPointer(ctx, srcIndex);

            u64 lowQWordSrc;
            memcpy(&lowQWordSrc, src, sizeof(lowQWordSrc));

            u64 lowQWordDst;
            memcpy(&lowQWordDst, dst, sizeof(lowQWordDst));

            u64 length = lowQWordSrc & 0x3F;
            u64 mask;
            if (length == 0) {
                length = 64; // for the check below
                mask = 0xFFFF'FFFF'FFFF'FFFF;
            } else {
                mask = (1ULL << length) - 1;
            }

            u64 index = (lowQWordSrc >> 8) & 0x3F;
            if (length + index > 64) {
                // Undefined behavior if length + index is bigger than 64 according to the spec,
                // we'll warn and continue execution.
                LOG_TRACE(Core,
                          "extrq at {} with length {} and index {} is bigger than 64, "
                          "undefined behavior",
                          fmt::ptr(code_address), length, index);
            }

            lowQWordDst >>= index;
            lowQWordDst &= mask;

            memcpy(dst, &lowQWordDst, sizeof(lowQWordDst));

            Common::IncrementRip(ctx, instruction.length);

            return true;
        }
        break;
    }
    case ZYDIS_MNEMONIC_INSERTQ: {
        bool immediateForm = operands[2].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                             operands[3].type == ZYDIS_OPERAND_TYPE_IMMEDIATE;
        if (immediateForm) {
            LOG_CRITICAL(Core,
                         "INSERTQ immediate form should have been patched at code address: {}",
                         fmt::ptr(code_address));
            return false;
        } else {
            ASSERT_MSG(operands[2].type == ZYDIS_OPERAND_TYPE_UNUSED &&
                           operands[3].type == ZYDIS_OPERAND_TYPE_UNUSED,
                       "operands 2 and 3 must be unused for register form.");

            ASSERT_MSG(operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                           operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER,
                       "operands 0 and 1 must be registers.");

            const auto dstIndex = operands[0].reg.value - ZYDIS_REGISTER_XMM0;
            const auto srcIndex = operands[1].reg.value - ZYDIS_REGISTER_XMM0;

            const auto dst = Common::GetXmmPointer(ctx, dstIndex);
            const auto src = Common::GetXmmPointer(ctx, srcIndex);

            u64 lowQWordSrc, highQWordSrc;
            memcpy(&lowQWordSrc, src, sizeof(lowQWordSrc));
            memcpy(&highQWordSrc, (u8*)src + 8, sizeof(highQWordSrc));

            u64 lowQWordDst;
            memcpy(&lowQWordDst, dst, sizeof(lowQWordDst));

            u64 length = highQWordSrc & 0x3F;
            u64 mask;
            if (length == 0) {
                length = 64; // for the check below
                mask = 0xFFFF'FFFF'FFFF'FFFF;
            } else {
                mask = (1ULL << length) - 1;
            }

            u64 index = (highQWordSrc >> 8) & 0x3F;
            if (length + index > 64) {
                // Undefined behavior if length + index is bigger than 64 according to the spec,
                // we'll warn and continue execution.
                LOG_TRACE(Core,
                          "insertq at {} with length {} and index {} is bigger than 64, "
                          "undefined behavior",
                          fmt::ptr(code_address), length, index);
            }

            lowQWordSrc &= mask;
            lowQWordDst &= ~(mask << index);
            lowQWordDst |= lowQWordSrc << index;

            memcpy(dst, &lowQWordDst, sizeof(lowQWordDst));

            Common::IncrementRip(ctx, instruction.length);

            return true;
        }
        break;
    }
    default: {
        LOG_ERROR(Core, "Unhandled illegal instruction at code address {}: {}",
                  fmt::ptr(code_address), ZydisMnemonicGetString(instruction.mnemonic));
        return false;
    }
    }

    UNREACHABLE();
}
#elif defined(ARCH_ARM64)
// These functions shouldn't be needed for ARM as it will use a JIT so there's no need to patch
// instructions.
static bool TryExecuteIllegalInstruction(void*, void*) {
    return false;
}
#else
#error "Unsupported architecture"
#endif

static bool TryPatchJit(void* code_address) {
    auto* code = static_cast<u8*>(code_address);
    auto* module = GetModule(code);
    if (module == nullptr) {
        return false;
    }

    std::unique_lock lock{module->mutex};

    // Return early if already patched, in case multiple threads signaled at the same time.
    if (std::ranges::find(module->patched, code) != module->patched.end()) {
        return true;
    }

    return TryPatch(code, module).first;
}

static void TryPatchAot(void* code_address, u64 code_size) {
    auto* code = static_cast<u8*>(code_address);
    auto* module = GetModule(code);
    if (module == nullptr) {
        return;
    }

    std::unique_lock lock{module->mutex};

    const auto* end = code + code_size;
    while (code < end) {
        code += TryPatch(code, module).second;
    }
}

static bool PatchesAccessViolationHandler(void* context, void* /* fault_address */) {
    return TryPatchJit(Common::GetRip(context));
}

static bool PatchesIllegalInstructionHandler(void* context) {
    void* code_address = Common::GetRip(context);
    if (!TryPatchJit(code_address)) {
        return TryExecuteIllegalInstruction(context, code_address);
    }
    return true;
}

static void PatchesInit() {
    if (!Patches.empty()) {
        auto* signals = Signals::Instance();
        // Should be called last.
        constexpr auto priority = std::numeric_limits<u32>::max();
        signals->RegisterAccessViolationHandler(PatchesAccessViolationHandler, priority);
        signals->RegisterIllegalInstructionHandler(PatchesIllegalInstructionHandler, priority);
    }
}

void RegisterPatchModule(void* module_ptr, u64 module_size, void* trampoline_area_ptr,
                         u64 trampoline_area_size) {
    std::call_once(init_flag, PatchesInit);

    const auto module_addr = reinterpret_cast<u64>(module_ptr);
    modules.emplace(std::piecewise_construct, std::forward_as_tuple(module_addr),
                    std::forward_as_tuple(static_cast<u8*>(module_ptr), module_size,
                                          static_cast<u8*>(trampoline_area_ptr),
                                          trampoline_area_size));
}

void PrePatchInstructions(u64 segment_addr, u64 segment_size) {
#if defined(__APPLE__)
    // HACK: For some reason patching in the signal handler at the start of a page does not work
    // under Rosetta 2. Patch any instructions at the start of a page ahead of time.
    if (!Patches.empty()) {
        auto* code_page = reinterpret_cast<u8*>(Common::AlignUp(segment_addr, 0x1000));
        const auto* end_page = code_page + Common::AlignUp(segment_size, 0x1000);
        while (code_page < end_page) {
            TryPatchJit(code_page);
            code_page += 0x1000;
        }
    }
#elif !defined(_WIN32)
    // Linux and others have an FS segment pointing to valid memory, so continue to do full
    // ahead-of-time patching for now until a better solution is worked out.
    if (!Patches.empty()) {
        TryPatchAot(reinterpret_cast<void*>(segment_addr), segment_size);
    }
#endif
}

} // namespace Core
