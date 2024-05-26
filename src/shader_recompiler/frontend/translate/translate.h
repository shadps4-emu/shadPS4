// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include "shader_recompiler/frontend/instruction.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/runtime_info.h"

namespace Shader {
struct Info;
}

namespace Shader::Gcn {

enum class ConditionOp : u32 {
    EQ,
    LG,
    GT,
    GE,
    LT,
    LE,
};

class Translator {
public:
    explicit Translator(IR::Block* block_, Info& info);

    void EmitFetch(const GcnInst& inst);

    // Scalar ALU
    void S_MOV(const GcnInst& inst);
    void S_MUL_I32(const GcnInst& inst);
    void S_CMP(ConditionOp cond, bool is_signed, const GcnInst& inst);

    // Scalar Memory
    void S_LOAD_DWORD(int num_dwords, const GcnInst& inst);
    void S_BUFFER_LOAD_DWORD(int num_dwords, const GcnInst& inst);

    // Vector ALU
    void V_MOV(const GcnInst& inst);
    void V_SAD(const GcnInst& inst);
    void V_MAC_F32(const GcnInst& inst);
    void V_CVT_PKRTZ_F16_F32(const GcnInst& inst);
    void V_MUL_F32(const GcnInst& inst);
    void V_CMP_EQ_U32(const GcnInst& inst);
    void V_CNDMASK_B32(const GcnInst& inst);

    // Vector Memory
    void TBUFFER_LOAD_FORMAT_XYZW(const GcnInst& inst);

    // Vector interpolation
    void V_INTERP_P2_F32(const GcnInst& inst);

    // Data share
    void DS_READ(int bit_size, bool is_signed, bool is_pair, const GcnInst& inst);
    void DS_WRITE(int bit_size, bool is_signed, bool is_pair, const GcnInst& inst);

    // MIMG
    void IMAGE_GET_RESINFO(const GcnInst& inst);
    void IMAGE_SAMPLE(const GcnInst& inst);

    // Export
    void EXP(const GcnInst& inst);

private:
    IR::U32F32 GetSrc(const InstOperand& operand, bool flt_zero = false);
    void SetDst(const InstOperand& operand, const IR::U32F32& value);

private:
    IR::Block* block;
    IR::IREmitter ir;
    Info& info;
};

void Translate(IR::Block* block, std::span<const GcnInst> inst_list, Info& info);

} // namespace Shader::Gcn
