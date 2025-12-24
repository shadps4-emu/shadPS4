// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <Zydis/Zydis.h>
#include "arm64_codegen.h"
#include "register_mapping.h"

namespace Core::Jit {

class SimdTranslator {
public:
    explicit SimdTranslator(Arm64CodeGenerator& codegen, RegisterMapper& reg_mapper);

    bool TranslateSseInstruction(const ZydisDecodedInstruction& instruction,
                                 const ZydisDecodedOperand* operands);

    bool TranslateMovaps(const ZydisDecodedInstruction& instruction,
                         const ZydisDecodedOperand* operands);
    bool TranslateMovups(const ZydisDecodedInstruction& instruction,
                         const ZydisDecodedOperand* operands);
    bool TranslateAddps(const ZydisDecodedInstruction& instruction,
                        const ZydisDecodedOperand* operands);
    bool TranslateSubps(const ZydisDecodedInstruction& instruction,
                        const ZydisDecodedOperand* operands);
    bool TranslateMulps(const ZydisDecodedInstruction& instruction,
                        const ZydisDecodedOperand* operands);

private:
    int GetArm64NeonRegister(const ZydisDecodedOperand& operand);
    void LoadMemoryOperandV(int vreg, const ZydisDecodedOperand& mem_op);
    void StoreMemoryOperandV(int vreg, const ZydisDecodedOperand& mem_op);

    Arm64CodeGenerator& codegen;
    RegisterMapper& reg_mapper;
};

} // namespace Core::Jit
