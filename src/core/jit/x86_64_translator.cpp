// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include "common/assert.h"
#include "common/logging/log.h"
#include "register_mapping.h"
#include "x86_64_translator.h"

namespace Core::Jit {

X86_64Translator::X86_64Translator(Arm64CodeGenerator& codegen, RegisterMapper& reg_mapper)
    : codegen(codegen), reg_mapper(reg_mapper) {}

bool X86_64Translator::TranslateInstruction(const ZydisDecodedInstruction& instruction,
                                            const ZydisDecodedOperand* operands, VAddr address) {
    switch (instruction.mnemonic) {
    case ZYDIS_MNEMONIC_MOV:
        return TranslateMov(instruction, operands);
    case ZYDIS_MNEMONIC_ADD:
        return TranslateAdd(instruction, operands);
    case ZYDIS_MNEMONIC_SUB:
        return TranslateSub(instruction, operands);
    case ZYDIS_MNEMONIC_MUL:
        return TranslateMul(instruction, operands);
    case ZYDIS_MNEMONIC_DIV:
    case ZYDIS_MNEMONIC_IDIV:
        return TranslateDiv(instruction, operands);
    case ZYDIS_MNEMONIC_AND:
        return TranslateAnd(instruction, operands);
    case ZYDIS_MNEMONIC_OR:
        return TranslateOr(instruction, operands);
    case ZYDIS_MNEMONIC_XOR:
        return TranslateXor(instruction, operands);
    case ZYDIS_MNEMONIC_NOT:
        return TranslateNot(instruction, operands);
    case ZYDIS_MNEMONIC_SHL:
        return TranslateShl(instruction, operands);
    case ZYDIS_MNEMONIC_SHR:
        return TranslateShr(instruction, operands);
    case ZYDIS_MNEMONIC_SAR:
        return TranslateSar(instruction, operands);
    case ZYDIS_MNEMONIC_PUSH:
        return TranslatePush(instruction, operands);
    case ZYDIS_MNEMONIC_POP:
        return TranslatePop(instruction, operands);
    case ZYDIS_MNEMONIC_CALL:
        return TranslateCall(instruction, operands, address);
    case ZYDIS_MNEMONIC_RET:
        return TranslateRet(instruction, operands);
    case ZYDIS_MNEMONIC_JMP:
        return TranslateJmp(instruction, operands, address);
    case ZYDIS_MNEMONIC_CMP:
        return TranslateCmp(instruction, operands);
    case ZYDIS_MNEMONIC_TEST:
        return TranslateTest(instruction, operands);
    case ZYDIS_MNEMONIC_LEA:
        return TranslateLea(instruction, operands);
    default:
        LOG_ERROR(Core, "Unsupported instruction: {}",
                  ZydisMnemonicGetString(instruction.mnemonic));
        return false;
    }
}

X86_64Register X86_64Translator::ZydisToX86_64Register(ZydisRegister reg) {
    if (reg >= ZYDIS_REGISTER_RAX && reg <= ZYDIS_REGISTER_R15) {
        return static_cast<X86_64Register>(static_cast<int>(reg - ZYDIS_REGISTER_RAX));
    } else if (reg >= ZYDIS_REGISTER_XMM0 && reg <= ZYDIS_REGISTER_XMM15) {
        return static_cast<X86_64Register>(static_cast<int>(X86_64Register::XMM0) +
                                           static_cast<int>(reg - ZYDIS_REGISTER_XMM0));
    }
    return X86_64Register::COUNT;
}

int X86_64Translator::GetArm64Register(const ZydisDecodedOperand& operand) {
    if (operand.type != ZYDIS_OPERAND_TYPE_REGISTER) {
        return -1;
    }
    X86_64Register x86_reg = ZydisToX86_64Register(operand.reg.value);
    if (x86_reg == X86_64Register::COUNT) {
        return -1;
    }
    return reg_mapper.MapX86_64ToArm64(x86_reg);
}

int X86_64Translator::GetArm64XmmRegister(const ZydisDecodedOperand& operand) {
    if (operand.type != ZYDIS_OPERAND_TYPE_REGISTER) {
        return -1;
    }
    X86_64Register x86_reg = ZydisToX86_64Register(operand.reg.value);
    if (!reg_mapper.IsXmmRegister(x86_reg)) {
        return -1;
    }
    return reg_mapper.MapX86_64XmmToArm64Neon(x86_reg);
}

void X86_64Translator::CalculateMemoryAddress(int dst_reg, const ZydisDecodedOperand& mem_op) {
    ASSERT_MSG(mem_op.type == ZYDIS_OPERAND_TYPE_MEMORY, "Expected memory operand");

    const auto& mem = mem_op.mem;
    int base_reg = -1;
    int index_reg = -1;

    if (mem.base != ZYDIS_REGISTER_NONE && mem.base != ZYDIS_REGISTER_RIP) {
        X86_64Register x86_base = ZydisToX86_64Register(mem.base);
        if (x86_base != X86_64Register::COUNT) {
            base_reg = reg_mapper.MapX86_64ToArm64(x86_base);
        }
    }

    if (mem.index != ZYDIS_REGISTER_NONE) {
        X86_64Register x86_index = ZydisToX86_64Register(mem.index);
        if (x86_index != X86_64Register::COUNT) {
            index_reg = reg_mapper.MapX86_64ToArm64(x86_index);
        }
    }

    if (base_reg == -1 && index_reg == -1 && mem.disp.value == 0) {
        codegen.mov(dst_reg, 0);
        return;
    }

    if (base_reg != -1) {
        codegen.mov(dst_reg, base_reg);
    } else {
        codegen.mov(dst_reg, 0);
    }

    if (index_reg != -1) {
        if (mem.scale > 0 && mem.scale <= 8) {
            codegen.mov(RegisterMapper::SCRATCH_REG, static_cast<s64>(mem.scale));
            codegen.mul(RegisterMapper::SCRATCH_REG, index_reg, RegisterMapper::SCRATCH_REG);
            codegen.add(dst_reg, dst_reg, RegisterMapper::SCRATCH_REG);
        } else {
            codegen.add(dst_reg, dst_reg, index_reg);
        }
    }

    if (mem.disp.value != 0) {
        codegen.add(dst_reg, dst_reg, static_cast<s32>(mem.disp.value));
    }
}

void X86_64Translator::LoadMemoryOperand(int dst_reg, const ZydisDecodedOperand& mem_op,
                                         size_t size) {
    CalculateMemoryAddress(RegisterMapper::SCRATCH_REG, mem_op);

    if (mem_op.mem.base == ZYDIS_REGISTER_RIP) {
        LOG_WARNING(Core, "RIP-relative addressing not fully supported in JIT");
    }

    switch (size) {
    case 1:
        codegen.ldrb(dst_reg, RegisterMapper::SCRATCH_REG, 0);
        break;
    case 2:
        codegen.ldrh(dst_reg, RegisterMapper::SCRATCH_REG, 0);
        break;
    case 4:
    case 8:
        codegen.ldr(dst_reg, RegisterMapper::SCRATCH_REG, 0);
        break;
    default:
        ASSERT_MSG(false, "Unsupported memory load size: {}", size);
    }
}

void X86_64Translator::StoreMemoryOperand(int src_reg, const ZydisDecodedOperand& mem_op,
                                          size_t size) {
    CalculateMemoryAddress(RegisterMapper::SCRATCH_REG, mem_op);

    if (mem_op.mem.base == ZYDIS_REGISTER_RIP) {
        LOG_WARNING(Core, "RIP-relative addressing not fully supported in JIT");
    }

    switch (size) {
    case 1:
        codegen.strb(src_reg, RegisterMapper::SCRATCH_REG, 0);
        break;
    case 2:
        codegen.strh(src_reg, RegisterMapper::SCRATCH_REG, 0);
        break;
    case 4:
    case 8:
        codegen.str(src_reg, RegisterMapper::SCRATCH_REG, 0);
        break;
    default:
        ASSERT_MSG(false, "Unsupported memory store size: {}", size);
    }
}

void X86_64Translator::LoadImmediate(int dst_reg, const ZydisDecodedOperand& imm_op) {
    ASSERT_MSG(imm_op.type == ZYDIS_OPERAND_TYPE_IMMEDIATE, "Expected immediate operand");
    s64 value = static_cast<s64>(imm_op.imm.value.s);
    codegen.mov(dst_reg, value);
}

bool X86_64Translator::TranslateMov(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    if (dst.type == ZYDIS_OPERAND_TYPE_REGISTER) {
        int dst_reg = GetArm64Register(dst);
        if (dst_reg == -1) {
            return false;
        }

        if (src.type == ZYDIS_OPERAND_TYPE_REGISTER) {
            int src_reg = GetArm64Register(src);
            if (src_reg == -1) {
                return false;
            }
            codegen.mov(dst_reg, src_reg);
        } else if (src.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
            LoadImmediate(dst_reg, src);
        } else if (src.type == ZYDIS_OPERAND_TYPE_MEMORY) {
            LoadMemoryOperand(dst_reg, src, instruction.operand_width / 8);
        } else {
            return false;
        }
    } else if (dst.type == ZYDIS_OPERAND_TYPE_MEMORY) {
        int src_reg = -1;
        if (src.type == ZYDIS_OPERAND_TYPE_REGISTER) {
            src_reg = GetArm64Register(src);
            if (src_reg == -1) {
                return false;
            }
        } else if (src.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
            LoadImmediate(RegisterMapper::SCRATCH_REG, src);
            src_reg = RegisterMapper::SCRATCH_REG;
        } else {
            return false;
        }
        StoreMemoryOperand(src_reg, dst, instruction.operand_width / 8);
    } else {
        return false;
    }

    return true;
}

bool X86_64Translator::TranslateAdd(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    int dst_reg = GetArm64Register(dst);
    if (dst_reg == -1) {
        return false;
    }

    if (src.type == ZYDIS_OPERAND_TYPE_REGISTER) {
        int src_reg = GetArm64Register(src);
        if (src_reg == -1) {
            return false;
        }
        codegen.add(dst_reg, dst_reg, src_reg);
    } else if (src.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
        s32 imm = static_cast<s32>(src.imm.value.s);
        codegen.add_imm(dst_reg, dst_reg, imm);
    } else if (src.type == ZYDIS_OPERAND_TYPE_MEMORY) {
        LoadMemoryOperand(RegisterMapper::SCRATCH_REG, src, instruction.operand_width / 8);
        codegen.add(dst_reg, dst_reg, RegisterMapper::SCRATCH_REG);
    } else {
        return false;
    }

    return true;
}

bool X86_64Translator::TranslateSub(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    int dst_reg = GetArm64Register(dst);
    if (dst_reg == -1) {
        return false;
    }

    if (src.type == ZYDIS_OPERAND_TYPE_REGISTER) {
        int src_reg = GetArm64Register(src);
        if (src_reg == -1) {
            return false;
        }
        codegen.sub(dst_reg, dst_reg, src_reg);
    } else if (src.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
        s32 imm = static_cast<s32>(src.imm.value.s);
        codegen.sub_imm(dst_reg, dst_reg, imm);
    } else if (src.type == ZYDIS_OPERAND_TYPE_MEMORY) {
        LoadMemoryOperand(RegisterMapper::SCRATCH_REG, src, instruction.operand_width / 8);
        codegen.sub(dst_reg, dst_reg, RegisterMapper::SCRATCH_REG);
    } else {
        return false;
    }

    return true;
}

bool X86_64Translator::TranslateMul(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];

    int dst_reg = GetArm64Register(dst);
    if (dst_reg == -1) {
        return false;
    }

    if (operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER) {
        int src_reg = GetArm64Register(operands[1]);
        if (src_reg == -1) {
            return false;
        }
        codegen.mul(dst_reg, dst_reg, src_reg);
    } else if (operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY) {
        LoadMemoryOperand(RegisterMapper::SCRATCH_REG, operands[1], instruction.operand_width / 8);
        codegen.mul(dst_reg, dst_reg, RegisterMapper::SCRATCH_REG);
    } else {
        return false;
    }

    return true;
}

bool X86_64Translator::TranslateDiv(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    LOG_WARNING(Core, "DIV instruction translation not fully implemented");
    return false;
}

bool X86_64Translator::TranslateAnd(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    int dst_reg = GetArm64Register(dst);
    if (dst_reg == -1) {
        return false;
    }

    if (src.type == ZYDIS_OPERAND_TYPE_REGISTER) {
        int src_reg = GetArm64Register(src);
        if (src_reg == -1) {
            return false;
        }
        codegen.and_(dst_reg, dst_reg, src_reg);
    } else if (src.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
        u64 imm = static_cast<u64>(src.imm.value.u);
        codegen.and_(dst_reg, dst_reg, imm);
    } else if (src.type == ZYDIS_OPERAND_TYPE_MEMORY) {
        LoadMemoryOperand(RegisterMapper::SCRATCH_REG, src, instruction.operand_width / 8);
        codegen.and_(dst_reg, dst_reg, RegisterMapper::SCRATCH_REG);
    } else {
        return false;
    }

    return true;
}

bool X86_64Translator::TranslateOr(const ZydisDecodedInstruction& instruction,
                                   const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    int dst_reg = GetArm64Register(dst);
    if (dst_reg == -1) {
        return false;
    }

    if (src.type == ZYDIS_OPERAND_TYPE_REGISTER) {
        int src_reg = GetArm64Register(src);
        if (src_reg == -1) {
            return false;
        }
        codegen.orr(dst_reg, dst_reg, src_reg);
    } else if (src.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
        u64 imm = static_cast<u64>(src.imm.value.u);
        codegen.orr(dst_reg, dst_reg, imm);
    } else if (src.type == ZYDIS_OPERAND_TYPE_MEMORY) {
        LoadMemoryOperand(RegisterMapper::SCRATCH_REG, src, instruction.operand_width / 8);
        codegen.orr(dst_reg, dst_reg, RegisterMapper::SCRATCH_REG);
    } else {
        return false;
    }

    return true;
}

bool X86_64Translator::TranslateXor(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    int dst_reg = GetArm64Register(dst);
    if (dst_reg == -1) {
        return false;
    }

    if (src.type == ZYDIS_OPERAND_TYPE_REGISTER) {
        int src_reg = GetArm64Register(src);
        if (src_reg == -1) {
            return false;
        }
        codegen.eor(dst_reg, dst_reg, src_reg);
    } else if (src.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
        u64 imm = static_cast<u64>(src.imm.value.u);
        codegen.eor(dst_reg, dst_reg, imm);
    } else if (src.type == ZYDIS_OPERAND_TYPE_MEMORY) {
        LoadMemoryOperand(RegisterMapper::SCRATCH_REG, src, instruction.operand_width / 8);
        codegen.eor(dst_reg, dst_reg, RegisterMapper::SCRATCH_REG);
    } else {
        return false;
    }

    return true;
}

bool X86_64Translator::TranslateNot(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];

    int dst_reg = GetArm64Register(dst);
    if (dst_reg == -1) {
        return false;
    }

    codegen.mvn(dst_reg, dst_reg);

    return true;
}

bool X86_64Translator::TranslateShl(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    int dst_reg = GetArm64Register(dst);
    if (dst_reg == -1) {
        return false;
    }

    if (src.type == ZYDIS_OPERAND_TYPE_REGISTER &&
        (src.reg.value == ZYDIS_REGISTER_CL || src.reg.value == ZYDIS_REGISTER_RCX)) {
        int cl_reg = reg_mapper.MapX86_64ToArm64(X86_64Register::RCX);
        codegen.lsl(dst_reg, dst_reg, cl_reg);
    } else if (src.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
        u64 shift_val = src.imm.value.u;
        if (shift_val < 64) {
            codegen.lsl(dst_reg, dst_reg, static_cast<u8>(shift_val));
        } else {
            codegen.mov(dst_reg, 0);
        }
    } else {
        return false;
    }

    return true;
}

bool X86_64Translator::TranslateShr(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    int dst_reg = GetArm64Register(dst);
    if (dst_reg == -1) {
        return false;
    }

    if (src.type == ZYDIS_OPERAND_TYPE_REGISTER &&
        (src.reg.value == ZYDIS_REGISTER_CL || src.reg.value == ZYDIS_REGISTER_RCX)) {
        int cl_reg = reg_mapper.MapX86_64ToArm64(X86_64Register::RCX);
        codegen.lsr(dst_reg, dst_reg, cl_reg);
    } else if (src.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
        u64 shift_val = src.imm.value.u;
        if (shift_val < 64) {
            codegen.lsr(dst_reg, dst_reg, static_cast<u8>(shift_val));
        } else {
            codegen.mov(dst_reg, 0);
        }
    } else {
        return false;
    }

    return true;
}

bool X86_64Translator::TranslateSar(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    int dst_reg = GetArm64Register(dst);
    if (dst_reg == -1) {
        return false;
    }

    if (src.type == ZYDIS_OPERAND_TYPE_REGISTER &&
        (src.reg.value == ZYDIS_REGISTER_CL || src.reg.value == ZYDIS_REGISTER_RCX)) {
        int cl_reg = reg_mapper.MapX86_64ToArm64(X86_64Register::RCX);
        codegen.asr(dst_reg, dst_reg, cl_reg);
    } else if (src.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
        u64 shift_val = src.imm.value.u;
        if (shift_val < 64) {
            codegen.asr(dst_reg, dst_reg, static_cast<u8>(shift_val));
        } else {
            codegen.mov(dst_reg, 0);
        }
    } else {
        return false;
    }

    return true;
}

bool X86_64Translator::TranslatePush(const ZydisDecodedInstruction& instruction,
                                     const ZydisDecodedOperand* operands) {
    const auto& src = operands[0];

    int sp_reg = reg_mapper.MapX86_64ToArm64(X86_64Register::RSP);
    codegen.sub(sp_reg, sp_reg, 8);

    if (src.type == ZYDIS_OPERAND_TYPE_REGISTER) {
        int src_reg = GetArm64Register(src);
        if (src_reg == -1) {
            return false;
        }
        codegen.str(src_reg, sp_reg, 0);
    } else if (src.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
        LoadImmediate(RegisterMapper::SCRATCH_REG, src);
        codegen.str(RegisterMapper::SCRATCH_REG, sp_reg, 0);
    } else if (src.type == ZYDIS_OPERAND_TYPE_MEMORY) {
        LoadMemoryOperand(RegisterMapper::SCRATCH_REG, src, instruction.operand_width / 8);
        codegen.str(RegisterMapper::SCRATCH_REG, sp_reg, 0);
    } else {
        return false;
    }

    return true;
}

bool X86_64Translator::TranslatePop(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];

    int dst_reg = GetArm64Register(dst);
    if (dst_reg == -1) {
        return false;
    }

    int sp_reg = reg_mapper.MapX86_64ToArm64(X86_64Register::RSP);
    codegen.ldr(dst_reg, sp_reg, 0);
    codegen.add(sp_reg, sp_reg, 8);

    return true;
}

bool X86_64Translator::TranslateCall(const ZydisDecodedInstruction& instruction,
                                     const ZydisDecodedOperand* operands, VAddr address) {
    LOG_WARNING(Core, "CALL instruction translation needs execution engine integration");
    return false;
}

bool X86_64Translator::TranslateRet(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    codegen.ret();
    return true;
}

bool X86_64Translator::TranslateJmp(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands, VAddr address) {
    const auto& target = operands[0];
    VAddr target_address = 0;

    // Calculate target address based on operand type
    if (target.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
        // Direct relative jump: JMP rel32
        // Target = current_address + instruction.length + offset
        s64 offset = static_cast<s64>(target.imm.value.s);
        target_address = address + instruction.length + offset;
    } else if (target.type == ZYDIS_OPERAND_TYPE_MEMORY) {
        // Indirect jump: JMP [mem]
        // Load address from memory into scratch register
        LoadMemoryOperand(RegisterMapper::SCRATCH_REG, target, 8);
        // TODO: don't use a dispatcher
        codegen.br(RegisterMapper::SCRATCH_REG);
        return true;
    } else if (target.type == ZYDIS_OPERAND_TYPE_REGISTER) {
        // Indirect jump: JMP reg
        int reg = GetArm64Register(target);
        if (reg == -1) {
            LOG_ERROR(Core, "Invalid register for JMP");
            return false;
        }
        codegen.br(reg);
        return true;
    } else {
        LOG_ERROR(Core, "Unsupported JMP operand type");
        return false;
    }

    // For direct jumps, we need to branch to the target address
    // Since the target block may not be translated yet, we'll generate
    // a placeholder that can be patched later during block linking
    // For now, generate a branch to a dispatcher function
    // TODO: Implement proper block linking to patch this with direct branch

    // Calculate offset from current code position
    void* placeholder_target = reinterpret_cast<void*>(target_address);
    codegen.b(placeholder_target);

    return true;
}

bool X86_64Translator::TranslateCmp(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    int dst_reg = GetArm64Register(dst);
    if (dst_reg == -1) {
        return false;
    }

    if (src.type == ZYDIS_OPERAND_TYPE_REGISTER) {
        int src_reg = GetArm64Register(src);
        if (src_reg == -1) {
            return false;
        }
        codegen.cmp(dst_reg, src_reg);
    } else if (src.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
        s32 imm = static_cast<s32>(src.imm.value.s);
        codegen.cmp_imm(dst_reg, imm);
    } else if (src.type == ZYDIS_OPERAND_TYPE_MEMORY) {
        LoadMemoryOperand(RegisterMapper::SCRATCH_REG, src, instruction.operand_width / 8);
        codegen.cmp(dst_reg, RegisterMapper::SCRATCH_REG);
    } else {
        return false;
    }

    return true;
}

bool X86_64Translator::TranslateTest(const ZydisDecodedInstruction& instruction,
                                     const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    int dst_reg = GetArm64Register(dst);
    if (dst_reg == -1) {
        return false;
    }

    if (src.type == ZYDIS_OPERAND_TYPE_REGISTER) {
        int src_reg = GetArm64Register(src);
        if (src_reg == -1) {
            return false;
        }
        codegen.tst(dst_reg, src_reg);
    } else if (src.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
        u64 imm = static_cast<u64>(src.imm.value.u);
        codegen.tst(dst_reg, imm);
    } else if (src.type == ZYDIS_OPERAND_TYPE_MEMORY) {
        LoadMemoryOperand(RegisterMapper::SCRATCH_REG, src, instruction.operand_width / 8);
        codegen.tst(dst_reg, RegisterMapper::SCRATCH_REG);
    } else {
        return false;
    }

    return true;
}

bool X86_64Translator::TranslateLea(const ZydisDecodedInstruction& instruction,
                                    const ZydisDecodedOperand* operands) {
    const auto& dst = operands[0];
    const auto& src = operands[1];

    ASSERT_MSG(src.type == ZYDIS_OPERAND_TYPE_MEMORY, "LEA source must be memory");

    int dst_reg = GetArm64Register(dst);
    if (dst_reg == -1) {
        return false;
    }

    CalculateMemoryAddress(dst_reg, src);

    return true;
}

void X86_64Translator::UpdateFlagsForArithmetic(int result_reg, int src1_reg, int src2_reg,
                                                bool is_subtract) {
    int flags_reg = reg_mapper.MapX86_64ToArm64(X86_64Register::FLAGS);

    codegen.cmp(result_reg, 0);

    codegen.mov(RegisterMapper::SCRATCH_REG, 0);

    codegen.b_eq(codegen.getCurr());
    codegen.mov(RegisterMapper::SCRATCH_REG, 1 << 6);
    codegen.b(codegen.getCurr());
}

void X86_64Translator::UpdateFlagsForLogical(int result_reg) {
    codegen.cmp(result_reg, 0);
}

void X86_64Translator::UpdateFlagsForShift(int result_reg, int shift_amount) {
    codegen.cmp(result_reg, 0);
}

int X86_64Translator::GetConditionCode(ZydisMnemonic mnemonic) {
    switch (mnemonic) {
    case ZYDIS_MNEMONIC_JZ:
        return 0;
    case ZYDIS_MNEMONIC_JNZ:
        return 1;
    case ZYDIS_MNEMONIC_JL:
        return 11;
    case ZYDIS_MNEMONIC_JLE:
        return 13;
    case ZYDIS_MNEMONIC_JNLE:
        return 12;
    case ZYDIS_MNEMONIC_JNL:
        return 10;
    case ZYDIS_MNEMONIC_JB:
        return 3;
    case ZYDIS_MNEMONIC_JBE:
        return 9;
    case ZYDIS_MNEMONIC_JNBE:
        return 8;
    case ZYDIS_MNEMONIC_JNB:
        return 2;
    default:
        return -1;
    }
}

} // namespace Core::Jit
