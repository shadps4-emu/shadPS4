// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "register_mapping.h"
#include "simd_translator.h"

namespace Core::Jit {

SimdTranslator::SimdTranslator(Arm64CodeGenerator& codegen, RegisterMapper& reg_mapper)
    : codegen(codegen), reg_mapper(reg_mapper) {}

int SimdTranslator::GetArm64NeonRegister(const ZydisDecodedOperand& operand) {
    if (operand.type != ZYDIS_OPERAND_TYPE_REGISTER) {
        return -1;
    }
    if (operand.reg.value < ZYDIS_REGISTER_XMM0 || operand.reg.value > ZYDIS_REGISTER_XMM15) {
        return -1;
    }
    X86_64Register xmm_reg =
        static_cast<X86_64Register>(static_cast<int>(X86_64Register::XMM0) +
                                    static_cast<int>(operand.reg.value - ZYDIS_REGISTER_XMM0));
    return reg_mapper.MapX86_64XmmToArm64Neon(xmm_reg);
}

void SimdTranslator::LoadMemoryOperandV(int vreg, const ZydisDecodedOperand& mem_op) {
    ASSERT_MSG(mem_op.type == ZYDIS_OPERAND_TYPE_MEMORY, "Expected memory operand");

    int addr_reg = RegisterMapper::SCRATCH_REG;
    codegen.mov(addr_reg, 0);

    if (mem_op.mem.base != ZYDIS_REGISTER_NONE && mem_op.mem.base != ZYDIS_REGISTER_RIP) {
        if (mem_op.mem.base >= ZYDIS_REGISTER_RAX && mem_op.mem.base <= ZYDIS_REGISTER_R15) {
            X86_64Register x86_base =
                static_cast<X86_64Register>(mem_op.mem.base - ZYDIS_REGISTER_RAX);
            if (x86_base < X86_64Register::COUNT) {
                int base_reg = reg_mapper.MapX86_64ToArm64(x86_base);
                codegen.mov(addr_reg, base_reg);
            }
        }
    }

    if (mem_op.mem.disp.value != 0) {
        codegen.add(addr_reg, addr_reg, static_cast<s32>(mem_op.mem.disp.value));
    }

    codegen.ldr_v(vreg, addr_reg, 0);
}

void SimdTranslator::StoreMemoryOperandV(int vreg, const ZydisDecodedOperand& mem_op) {
    ASSERT_MSG(mem_op.type == ZYDIS_OPERAND_TYPE_MEMORY, "Expected memory operand");

    int addr_reg = RegisterMapper::SCRATCH_REG;
    codegen.mov(addr_reg, 0);

    if (mem_op.mem.base != ZYDIS_REGISTER_NONE) {
        if (mem_op.mem.base >= ZYDIS_REGISTER_RAX && mem_op.mem.base <= ZYDIS_REGISTER_R15) {
            X86_64Register x86_base =
                static_cast<X86_64Register>(mem_op.mem.base - ZYDIS_REGISTER_RAX);
            if (x86_base < X86_64Register::COUNT) {
                int base_reg = reg_mapper.MapX86_64ToArm64(x86_base);
                codegen.mov(addr_reg, base_reg);
            }
        }
    }

    if (mem_op.mem.disp.value != 0) {
        codegen.add(addr_reg, addr_reg, static_cast<s32>(mem_op.mem.disp.value));
    }

    codegen.str_v(vreg, addr_reg, 0);
}

bool SimdTranslator::TranslateSseInstruction(const ZydisDecodedInstruction& instruction,
                                             const ZydisDecodedOperand* operands) {
    switch (instruction.mnemonic) {
    case ZYDIS_MNEMONIC_MOVAPS:
        return TranslateMovaps(instruction, operands);
    case ZYDIS_MNEMONIC_MOVUPS:
        return TranslateMovups(instruction, operands);
    case ZYDIS_MNEMONIC_ADDPS:
        return TranslateAddps(instruction, operands);
    case ZYDIS_MNEMONIC_SUBPS:
        return TranslateSubps(instruction, operands);
    case ZYDIS_MNEMONIC_MULPS:
        return TranslateMulps(instruction, operands);
    default:
        LOG_WARNING(Core, "Unsupported SSE instruction: {}",
                    ZydisMnemonicGetString(instruction.mnemonic));
        return false;
    }
}

bool SimdTranslator::TranslateMovaps(const ZydisDecodedInstruction& instruction,
                                     const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    int dst_vreg = GetArm64NeonRegister(dst);
    if (dst_vreg == -1) {
        return false;
    }

    if (src.type == ZYDIS_OPERAND_TYPE_REGISTER) {
        int src_vreg = GetArm64NeonRegister(src);
        if (src_vreg == -1) {
            return false;
        }
        codegen.mov_v(dst_vreg, src_vreg);
    } else if (src.type == ZYDIS_OPERAND_TYPE_MEMORY) {
        LoadMemoryOperandV(dst_vreg, src);
    } else {
        return false;
    }

    return true;
}

bool SimdTranslator::TranslateMovups(const ZydisDecodedInstruction& instruction,
                                     const ZydisDecodedOperand* operands) {
    return TranslateMovaps(instruction, operands);
}

bool SimdTranslator::TranslateAddps(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    int dst_vreg = GetArm64NeonRegister(dst);
    if (dst_vreg == -1) {
        return false;
    }

    if (src.type == ZYDIS_OPERAND_TYPE_REGISTER) {
        int src_vreg = GetArm64NeonRegister(src);
        if (src_vreg == -1) {
            return false;
        }
        codegen.add_v(dst_vreg, dst_vreg, src_vreg);
    } else if (src.type == ZYDIS_OPERAND_TYPE_MEMORY) {
        int scratch_vreg = 8;
        LoadMemoryOperandV(scratch_vreg, src);
        codegen.add_v(dst_vreg, dst_vreg, scratch_vreg);
    } else {
        return false;
    }

    return true;
}

bool SimdTranslator::TranslateSubps(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    int dst_vreg = GetArm64NeonRegister(dst);
    if (dst_vreg == -1) {
        return false;
    }

    if (src.type == ZYDIS_OPERAND_TYPE_REGISTER) {
        int src_vreg = GetArm64NeonRegister(src);
        if (src_vreg == -1) {
            return false;
        }
        codegen.sub_v(dst_vreg, dst_vreg, src_vreg);
    } else if (src.type == ZYDIS_OPERAND_TYPE_MEMORY) {
        int scratch_vreg = 8;
        LoadMemoryOperandV(scratch_vreg, src);
        codegen.sub_v(dst_vreg, dst_vreg, scratch_vreg);
    } else {
        return false;
    }

    return true;
}

bool SimdTranslator::TranslateMulps(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    int dst_vreg = GetArm64NeonRegister(dst);
    if (dst_vreg == -1) {
        return false;
    }

    if (src.type == ZYDIS_OPERAND_TYPE_REGISTER) {
        int src_vreg = GetArm64NeonRegister(src);
        if (src_vreg == -1) {
            return false;
        }
        codegen.mul_v(dst_vreg, dst_vreg, src_vreg);
    } else if (src.type == ZYDIS_OPERAND_TYPE_MEMORY) {
        int scratch_vreg = 8;
        LoadMemoryOperandV(scratch_vreg, src);
        codegen.mul_v(dst_vreg, dst_vreg, scratch_vreg);
    } else {
        return false;
    }

    return true;
}

} // namespace Core::Jit
