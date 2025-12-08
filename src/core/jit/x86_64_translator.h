// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <Zydis/Zydis.h>
#include "arm64_codegen.h"
#include "common/types.h"
#include "register_mapping.h"

namespace Core::Jit {

class X86_64Translator {
public:
    explicit X86_64Translator(Arm64CodeGenerator& codegen, RegisterMapper& reg_mapper);
    ~X86_64Translator() = default;

    bool TranslateInstruction(const ZydisDecodedInstruction& instruction,
                              const ZydisDecodedOperand* operands, VAddr address);

    bool TranslateMov(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands);
    bool TranslateAdd(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands);
    bool TranslateSub(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands);
    bool TranslateMul(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands);
    bool TranslateDiv(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands);
    bool TranslateAnd(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands);
    bool TranslateOr(const ZydisDecodedInstruction& instruction,
                     const ZydisDecodedOperand* operands);
    bool TranslateXor(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands);
    bool TranslateNot(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands);
    bool TranslateShl(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands);
    bool TranslateShr(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands);
    bool TranslateSar(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands);
    bool TranslatePush(const ZydisDecodedInstruction& instruction,
                       const ZydisDecodedOperand* operands);
    bool TranslatePop(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands);
    bool TranslateCall(const ZydisDecodedInstruction& instruction,
                       const ZydisDecodedOperand* operands, VAddr address);
    bool TranslateRet(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands);
    bool TranslateJmp(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands, VAddr address);
    bool TranslateCmp(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands);
    bool TranslateTest(const ZydisDecodedInstruction& instruction,
                       const ZydisDecodedOperand* operands);
    bool TranslateLea(const ZydisDecodedInstruction& instruction,
                      const ZydisDecodedOperand* operands);

    void UpdateFlagsForArithmetic(int result_reg, int src1_reg, int src2_reg, bool is_subtract);
    void UpdateFlagsForLogical(int result_reg);
    void UpdateFlagsForShift(int result_reg, int shift_amount);
    int GetConditionCode(ZydisMnemonic mnemonic);

private:
    int GetArm64Register(const ZydisDecodedOperand& operand);
    int GetArm64XmmRegister(const ZydisDecodedOperand& operand);
    void LoadMemoryOperand(int dst_reg, const ZydisDecodedOperand& mem_op, size_t size);
    void StoreMemoryOperand(int src_reg, const ZydisDecodedOperand& mem_op, size_t size);
    void LoadImmediate(int dst_reg, const ZydisDecodedOperand& imm_op);
    void CalculateMemoryAddress(int dst_reg, const ZydisDecodedOperand& mem_op);
    X86_64Register ZydisToX86_64Register(ZydisRegister reg);

    Arm64CodeGenerator& codegen;
    RegisterMapper& reg_mapper;
};

} // namespace Core::Jit
