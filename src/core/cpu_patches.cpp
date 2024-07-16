// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>
#include <mutex>
#include <Zydis/Zydis.h>
#include <xbyak/xbyak.h>
#include "common/assert.h"
#include "common/types.h"
#include "core/tls.h"
#include "cpu_patches.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#ifdef __APPLE__
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

static constexpr u32 MaxSavedRegisters = 3;
static pthread_key_t register_save_slots[MaxSavedRegisters];
static std::once_flag register_save_init_flag;

static_assert(sizeof(void*) == sizeof(u64),
              "Cannot fit a register inside a thread local storage slot.");

static void InitializeRegisterSaveSlots() {
    for (u32 i = 0; i < MaxSavedRegisters; i++) {
        ASSERT_MSG(pthread_key_create(&register_save_slots[i], nullptr) == 0,
                   "Unable to allocate thread-local register save slot {}", i);
    }
}

static void SaveRegisters(Xbyak::CodeGenerator& c, const std::initializer_list<Xbyak::Reg> regs) {
    ASSERT_MSG(regs.size() <= MaxSavedRegisters, "Not enough space to save {} registers.",
               regs.size());

    std::call_once(register_save_init_flag, &InitializeRegisterSaveSlots);

    u32 index = 0;
    for (const auto& reg : regs) {
        const auto offset = reinterpret_cast<void*>(register_save_slots[index++] * sizeof(void*));

        c.putSeg(gs);
        c.mov(qword[offset], reg.cvt64());
    }
}

static void RestoreRegisters(Xbyak::CodeGenerator& c,
                             const std::initializer_list<Xbyak::Reg> regs) {
    ASSERT_MSG(regs.size() <= MaxSavedRegisters, "Not enough space to restore {} registers.",
               regs.size());

    std::call_once(register_save_init_flag, &InitializeRegisterSaveSlots);

    u32 index = 0;
    for (const auto& reg : regs) {
        const auto offset = reinterpret_cast<void*>(register_save_slots[index++] * sizeof(void*));

        c.putSeg(gs);
        c.mov(reg.cvt64(), qword[offset]);
    }
}

static void GenerateANDN(const ZydisDecodedOperand* operands, Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);
    const auto src1 = ZydisToXbyakRegisterOperand(operands[1]);
    const auto src2 = ZydisToXbyakOperand(operands[2]);

    const auto scratch = AllocateScratchRegister({&dst, &src1, src2.get()}, dst.getBit());

    SaveRegisters(c, {scratch});

    c.mov(scratch, src1);
    c.not_(scratch);
    c.and_(scratch, *src2);
    c.mov(dst, scratch);

    RestoreRegisters(c, {scratch});
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
        RestoreRegisters(c, {scratch1, scratch2});
    } else {
        RestoreRegisters(c, {scratch1, scratch2, shift});
    }
}

static void GenerateBLSI(const ZydisDecodedOperand* operands, Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);
    const auto src = ZydisToXbyakOperand(operands[1]);

    const auto scratch = AllocateScratchRegister({&dst, src.get()}, dst.getBit());

    SaveRegisters(c, {scratch});

    c.mov(scratch, *src);
    c.neg(scratch);
    c.and_(scratch, *src);
    c.mov(dst, scratch);

    RestoreRegisters(c, {scratch});
}

static void GenerateBLSMSK(const ZydisDecodedOperand* operands, Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);
    const auto src = ZydisToXbyakOperand(operands[1]);

    const auto scratch = AllocateScratchRegister({&dst, src.get()}, dst.getBit());

    SaveRegisters(c, {scratch});

    c.mov(scratch, *src);
    c.dec(scratch);
    c.xor_(scratch, *src);
    c.mov(dst, scratch);

    RestoreRegisters(c, {scratch});
}

static void GenerateBLSR(const ZydisDecodedOperand* operands, Xbyak::CodeGenerator& c) {
    const auto dst = ZydisToXbyakRegisterOperand(operands[0]);
    const auto src = ZydisToXbyakOperand(operands[1]);

    const auto scratch = AllocateScratchRegister({&dst, src.get()}, dst.getBit());

    SaveRegisters(c, {scratch});

    c.mov(scratch, *src);
    c.dec(scratch);
    c.and_(scratch, *src);
    c.mov(dst, scratch);

    RestoreRegisters(c, {scratch});
}

bool FilterRosetta2Only(const ZydisDecodedOperand*) {
    int ret = 0;
    size_t size = sizeof(ret);
    if (sysctlbyname("sysctl.proc_translated", &ret, &size, NULL, 0) != 0) {
        return false;
    }
    return ret;
}

#endif // __APPLE__

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
    const auto slot = GetTcbKey();

#if defined(_WIN32)
    // The following logic is based on the wine implementation of TlsGetValue
    // https://github.com/wine-mirror/wine/blob/a27b9551/dlls/kernelbase/thread.c#L719
    static constexpr u32 TlsSlotsOffset = 0x1480;
    static constexpr u32 TlsExpansionSlotsOffset = 0x1780;
    static constexpr u32 TlsMinimumAvailable = 64;

    const u32 teb_offset = slot < TlsMinimumAvailable ? TlsSlotsOffset : TlsExpansionSlotsOffset;
    const u32 tls_index = slot < TlsMinimumAvailable ? slot : slot - TlsMinimumAvailable;

    // Load the pointer to the table of TLS slots.
    c.putSeg(gs);
    c.mov(dst, ptr[reinterpret_cast<void*>(teb_offset)]);
    // Load the pointer to our buffer.
    c.mov(dst, qword[dst + tls_index * sizeof(LPVOID)]);
#elif defined(__APPLE__)
    // The following logic is based on the Darwin implementation of _os_tsd_get_direct, used by
    // pthread_getspecific https://github.com/apple/darwin-xnu/blob/main/libsyscall/os/tsd.h#L89-L96
    c.putSeg(gs);
    c.mov(dst, qword[reinterpret_cast<void*>(slot * sizeof(void*))]);
#else
    const auto src = ZydisToXbyakMemoryOperand(operands[1]);

    // Replace fs read with gs read.
    c.putSeg(gs);
    c.mov(dst, src);
#endif
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
#if defined(_WIN32) || defined(__APPLE__)
    // Windows and Apple need a trampoline.
    {ZYDIS_MNEMONIC_MOV, {FilterTcbAccess, GenerateTcbAccess, true}},
#else
    {ZYDIS_MNEMONIC_MOV, {FilterTcbAccess, GenerateTcbAccess, false}},
#endif

#ifdef __APPLE__
    // BMI1 instructions that are not supported by Rosetta 2 on Apple Silicon.
    {ZYDIS_MNEMONIC_ANDN, {FilterRosetta2Only, GenerateANDN, true}},
    {ZYDIS_MNEMONIC_BEXTR, {FilterRosetta2Only, GenerateBEXTR, true}},
    {ZYDIS_MNEMONIC_BLSI, {FilterRosetta2Only, GenerateBLSI, true}},
    {ZYDIS_MNEMONIC_BLSMSK, {FilterRosetta2Only, GenerateBLSMSK, true}},
    {ZYDIS_MNEMONIC_BLSR, {FilterRosetta2Only, GenerateBLSR, true}},
#endif
};

void PatchInstructions(u64 segment_addr, u64 segment_size, Xbyak::CodeGenerator& c) {
    if (Patches.empty()) {
        // Nothing to patch on this platform.
        return;
    }

    ZydisDecoder instr_decoder;
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    ZydisDecoderInit(&instr_decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

    u8* code = reinterpret_cast<u8*>(segment_addr);
    u8* end = code + segment_size;
    while (code < end) {
        ZyanStatus status =
            ZydisDecoderDecodeFull(&instr_decoder, code, end - code, &instruction, operands);
        if (!ZYAN_SUCCESS(status)) {
            code++;
            continue;
        }

        if (Patches.contains(instruction.mnemonic)) {
            auto patch_info = Patches.at(instruction.mnemonic);
            if (patch_info.filter(operands)) {
                auto patch_gen = Xbyak::CodeGenerator(instruction.length, code);

                if (patch_info.trampoline) {
                    const auto trampoline_ptr = c.getCurr();

                    patch_info.generator(operands, c);

                    // Return to the following instruction at the end of the trampoline.
                    c.jmp(code + instruction.length);

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

                    LOG_DEBUG(Core, "Patched instruction '{}' at: {}",
                              ZydisMnemonicGetString(instruction.mnemonic), fmt::ptr(code));
                }
            }
        }

        code += instruction.length;
    }
}

} // namespace Core
