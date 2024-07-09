// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>
#include <mutex>
#include <Zydis/Zydis.h>
#include <xbyak/xbyak.h>
#include "common/assert.h"
#include "common/types.h"
#include "instruction_emulator.h"

namespace Core {

static Xbyak::Reg ZydisToXbyakRegister(const ZydisRegister reg) {
    if (reg >= ZYDIS_REGISTER_EAX && reg <= ZYDIS_REGISTER_R15D) {
        return Xbyak::Reg32(reg - ZYDIS_REGISTER_EAX);
    } else if (reg >= ZYDIS_REGISTER_RAX && reg <= ZYDIS_REGISTER_R15) {
        return Xbyak::Reg64(reg - ZYDIS_REGISTER_RAX);
    } else {
        UNREACHABLE_MSG("Unsupported register: {}", static_cast<u32>(reg));
    }
}

static Xbyak::Reg ZydisToXbyakRegisterOperand(const ZydisDecodedOperand& operand) {
    ASSERT_MSG(operand.type == ZYDIS_OPERAND_TYPE_REGISTER, "Expected register operand, got type: {}", static_cast<u32>(operand.type));

    return ZydisToXbyakRegister(operand.reg.value);
}

static Xbyak::Address ZydisToXbyakMemoryOperand(const ZydisDecodedOperand& operand) {
    ASSERT_MSG(operand.type == ZYDIS_OPERAND_TYPE_MEMORY, "Expected memory operand, got type: {}", static_cast<u32>(operand.type));

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

    return Xbyak::util::ptr[expression];
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

#ifdef __APPLE__

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

static bool IsRegisterAllocated(const std::initializer_list<const Xbyak::Operand*>& allocated_registers, const int index) {
    return std::ranges::find_if(
        allocated_registers.begin(), allocated_registers.end(),
        [index](const Xbyak::Operand* operand) { return OperandUsesRegister(operand, index); }) != allocated_registers.end();
}

static Xbyak::Reg AllocateScratchRegister(const std::initializer_list<const Xbyak::Operand*> allocated_registers, const u32 bits) {
    for (int index = Xbyak::Operand::R8; index <= Xbyak::Operand::R15; index++) {
        if (!IsRegisterAllocated(allocated_registers, index)) {
            return Xbyak::Reg32e(index, static_cast<int>(bits));
        }
    }
    UNREACHABLE_MSG("Out of scratch registers!");
}

static constexpr u32 MaxSavedRegisters = 3;
static pthread_key_t register_save_slots[MaxSavedRegisters];
static std::once_flag register_save_init_flag;

static_assert(sizeof(void*) == sizeof(u64), "Cannot fit a register inside a thread local storage slot.");

static void InitializeRegisterSaveSlots() {
    for (u32 i = 0; i < MaxSavedRegisters; i++) {
        ASSERT_MSG(pthread_key_create(&register_save_slots[i], nullptr) == 0,
                   "Unable to allocate thread-local register save slot {}", i);
    }
}

static void SaveRegisters(Xbyak::CodeGenerator& c, const std::initializer_list<Xbyak::Reg> regs) {
    ASSERT_MSG(regs.size() <= MaxSavedRegisters, "Not enough space to save {} registers.", regs.size());

    std::call_once(register_save_init_flag, &InitializeRegisterSaveSlots);

    u32 index = 0;
    for (const auto& reg : regs) {
        const auto offset = reinterpret_cast<void*>(register_save_slots[index++] * sizeof(void*));

        c.putSeg(Xbyak::util::gs);
        c.mov(Xbyak::util::qword[offset], reg.cvt64());
    }
}

static void RestoreRegisters(Xbyak::CodeGenerator& c, const std::initializer_list<Xbyak::Reg> regs) {
    ASSERT_MSG(regs.size() <= MaxSavedRegisters, "Not enough space to restore {} registers.", regs.size());

    std::call_once(register_save_init_flag, &InitializeRegisterSaveSlots);

    u32 index = 0;
    for (const auto& reg : regs) {
        const auto offset = reinterpret_cast<void*>(register_save_slots[index++] * sizeof(void*));

        c.putSeg(Xbyak::util::gs);
        c.mov(reg.cvt64(), Xbyak::util::qword[offset]);
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
    const auto scratch1 = AllocateScratchRegister({&dst, src.get(), &start_len, &shift}, dst.getBit());
    const auto scratch2 = AllocateScratchRegister({&dst, src.get(), &start_len, &shift, &scratch1}, dst.getBit());

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

#endif

using InstructionGenerator = void(*)(const ZydisDecodedOperand*, Xbyak::CodeGenerator&);
static const std::unordered_map<ZydisMnemonic, InstructionGenerator> InstructionGenerators = {
#ifdef __APPLE__
    // BMI1 instructions that are not supported by Rosetta 2 on Apple Silicon.
    {ZYDIS_MNEMONIC_ANDN, &GenerateANDN},
    {ZYDIS_MNEMONIC_BEXTR, &GenerateBEXTR},
    {ZYDIS_MNEMONIC_BLSI, &GenerateBLSI},
    {ZYDIS_MNEMONIC_BLSMSK, &GenerateBLSMSK},
    {ZYDIS_MNEMONIC_BLSR, &GenerateBLSR},
#endif
};

void PatchInstructions(u64 segment_addr, u64 segment_size, Xbyak::CodeGenerator& c) {
    if (InstructionGenerators.empty()) {
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

        if (InstructionGenerators.contains(instruction.mnemonic)) {
            LOG_DEBUG(Core, "Replacing instruction '{}' at: {}", ZydisMnemonicGetString(instruction.mnemonic),
                      fmt::ptr(code));

            // Replace instruction with near jump to the trampoline.
            static constexpr u32 NearJmpSize = 5;
            ASSERT_MSG(instruction.length >= NearJmpSize, "Instruction {} with length {} is too short to replace at: {}",
                       ZydisMnemonicGetString(instruction.mnemonic), instruction.length, fmt::ptr(code));

            auto patch = Xbyak::CodeGenerator(instruction.length, code);
            patch.jmp(c.getCurr(), Xbyak::CodeGenerator::LabelType::T_NEAR);
            patch.nop(instruction.length - NearJmpSize);

            auto generator = InstructionGenerators.at(instruction.mnemonic);
            generator(operands, c);
            c.jmp(code + instruction.length); // Return to the following instruction.
        }

        code += instruction.length;
    }
}

} // namespace Loader
