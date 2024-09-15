// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include "shader_recompiler/frontend/instruction.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/ir_emitter.h"

namespace Shader {
struct Info;
struct Profile;
} // namespace Shader

namespace Shader::Gcn {

enum class ConditionOp : u32 {
    F,
    EQ,
    LG,
    GT,
    GE,
    LT,
    LE,
    TRU,
    U,
};

enum class AtomicOp : u32 {
    Swap,
    CmpSwap,
    Add,
    Sub,
    Smin,
    Umin,
    Smax,
    Umax,
    And,
    Or,
    Xor,
    Inc,
    Dec,
    FCmpSwap,
    Fmin,
    Fmax,
};

enum class NegateMode : u32 {
    None,
    Src1,
    Result,
};

class Translator {
public:
    explicit Translator(IR::Block* block_, Info& info, const RuntimeInfo& runtime_info,
                        const Profile& profile);

    // Instruction categories
    void EmitPrologue();
    void EmitFetch(const GcnInst& inst);
    void EmitDataShare(const GcnInst& inst);
    void EmitVectorInterpolation(const GcnInst& inst);
    void EmitScalarMemory(const GcnInst& inst);
    void EmitVectorMemory(const GcnInst& inst);
    void EmitExport(const GcnInst& inst);
    void EmitFlowControl(u32 pc, const GcnInst& inst);
    void EmitScalarAlu(const GcnInst& inst);
    void EmitVectorAlu(const GcnInst& inst);

    // Instruction encodings
    void EmitSOPC(const GcnInst& inst);
    void EmitSOPK(const GcnInst& inst);

    // Scalar ALU
    void S_MOVK(const GcnInst& inst);
    void S_MOV(const GcnInst& inst);
    void S_MUL_I32(const GcnInst& inst);
    void S_CMP(ConditionOp cond, bool is_signed, const GcnInst& inst);
    void S_AND_SAVEEXEC_B64(const GcnInst& inst);
    void S_MOV_B64(const GcnInst& inst);
    void S_OR_B64(NegateMode negate, bool is_xor, const GcnInst& inst);
    void S_AND_B64(NegateMode negate, const GcnInst& inst);
    void S_ADD_I32(const GcnInst& inst);
    void S_AND_B32(NegateMode negate, const GcnInst& inst);
    void S_ASHR_I32(const GcnInst& inst);
    void S_OR_B32(const GcnInst& inst);
    void S_XOR_B32(const GcnInst& inst);
    void S_LSHR_B32(const GcnInst& inst);
    void S_CSELECT_B32(const GcnInst& inst);
    void S_CSELECT_B64(const GcnInst& inst);
    void S_BFE_U32(const GcnInst& inst);
    void S_LSHL_B32(const GcnInst& inst);
    void S_BFM_B32(const GcnInst& inst);
    void S_NOT_B64(const GcnInst& inst);
    void S_BREV_B32(const GcnInst& inst);
    void S_ADD_U32(const GcnInst& inst);
    void S_SUB_U32(const GcnInst& inst);
    void S_GETPC_B64(u32 pc, const GcnInst& inst);
    void S_ADDC_U32(const GcnInst& inst);
    void S_MULK_I32(const GcnInst& inst);
    void S_ADDK_I32(const GcnInst& inst);
    void S_MAX_U32(bool is_signed, const GcnInst& inst);
    void S_MIN_U32(bool is_signed, const GcnInst& inst);
    void S_ABSDIFF_I32(const GcnInst& inst);
    void S_CMPK(ConditionOp cond, bool is_signed, const GcnInst& inst);

    // Scalar Memory
    void S_LOAD_DWORD(int num_dwords, const GcnInst& inst);
    void S_BUFFER_LOAD_DWORD(int num_dwords, const GcnInst& inst);

    // Vector ALU
    void V_MOV(const GcnInst& inst);
    void V_SAD(const GcnInst& inst);
    void V_MAC_F32(const GcnInst& inst);
    void V_CVT_PKRTZ_F16_F32(const GcnInst& inst);
    void V_CVT_F32_F16(const GcnInst& inst);
    void V_CVT_F16_F32(const GcnInst& inst);
    void V_MUL_F32(const GcnInst& inst);
    void V_CNDMASK_B32(const GcnInst& inst);
    void V_OR_B32(bool is_xor, const GcnInst& inst);
    void V_AND_B32(const GcnInst& inst);
    void V_LSHLREV_B32(const GcnInst& inst);
    void V_LSHL_B32(const GcnInst& inst);
    void V_LSHL_B64(const GcnInst& inst);
    void V_ADD_I32(const GcnInst& inst);
    void V_ADDC_U32(const GcnInst& inst);
    void V_CVT_F32_I32(const GcnInst& inst);
    void V_CVT_F32_U32(const GcnInst& inst);
    void V_MAD_F32(const GcnInst& inst);
    void V_FRACT_F32(const GcnInst& inst);
    void V_ADD_F32(const GcnInst& inst);
    void V_CVT_OFF_F32_I4(const GcnInst& inst);
    void V_MED3_F32(const GcnInst& inst);
    void V_MED3_I32(const GcnInst& inst);
    void V_FLOOR_F32(const GcnInst& inst);
    void V_SUB_F32(const GcnInst& inst);
    void V_RCP_F32(const GcnInst& inst);
    void V_FMA_F32(const GcnInst& inst);
    void V_CMP_F32(ConditionOp op, bool set_exec, const GcnInst& inst);
    void V_MAX_F32(const GcnInst& inst, bool is_legacy = false);
    void V_MAX_F64(const GcnInst& inst);
    void V_MAX_U32(bool is_signed, const GcnInst& inst);
    void V_RSQ_F32(const GcnInst& inst);
    void V_SIN_F32(const GcnInst& inst);
    void V_LOG_F32(const GcnInst& inst);
    void V_EXP_F32(const GcnInst& inst);
    void V_SQRT_F32(const GcnInst& inst);
    void V_MIN_F32(const GcnInst& inst, bool is_legacy = false);
    void V_MIN3_F32(const GcnInst& inst);
    void V_MIN3_I32(const GcnInst& inst);
    void V_MADMK_F32(const GcnInst& inst);
    void V_CUBEMA_F32(const GcnInst& inst);
    void V_CUBESC_F32(const GcnInst& inst);
    void V_CUBETC_F32(const GcnInst& inst);
    void V_CUBEID_F32(const GcnInst& inst);
    void V_CVT_U32_F32(const GcnInst& inst);
    void V_SUBREV_F32(const GcnInst& inst);
    void V_SUBREV_I32(const GcnInst& inst);
    void V_MAD_U64_U32(const GcnInst& inst);
    void V_CMP_U32(ConditionOp op, bool is_signed, bool set_exec, const GcnInst& inst);
    void V_LSHRREV_B32(const GcnInst& inst);
    void V_MUL_HI_U32(bool is_signed, const GcnInst& inst);
    void V_SAD_U32(const GcnInst& inst);
    void V_BFE_U32(bool is_signed, const GcnInst& inst);
    void V_MAD_I32_I24(const GcnInst& inst, bool is_signed = true);
    void V_MUL_I32_I24(const GcnInst& inst);
    void V_SUB_I32(const GcnInst& inst);
    void V_LSHR_B32(const GcnInst& inst);
    void V_ASHRREV_I32(const GcnInst& inst);
    void V_ASHR_I32(const GcnInst& inst);
    void V_MAD_U32_U24(const GcnInst& inst);
    void V_RNDNE_F32(const GcnInst& inst);
    void V_BCNT_U32_B32(const GcnInst& inst);
    void V_COS_F32(const GcnInst& inst);
    void V_MAX3_F32(const GcnInst& inst);
    void V_MAX3_U32(bool is_signed, const GcnInst& inst);
    void V_CVT_I32_F32(const GcnInst& inst);
    void V_MIN_I32(const GcnInst& inst);
    void V_MUL_LO_U32(const GcnInst& inst);
    void V_TRUNC_F32(const GcnInst& inst);
    void V_CEIL_F32(const GcnInst& inst);
    void V_MIN_U32(const GcnInst& inst);
    void V_CMP_NE_U64(const GcnInst& inst);
    void V_BFI_B32(const GcnInst& inst);
    void V_NOT_B32(const GcnInst& inst);
    void V_CVT_F32_UBYTE(u32 index, const GcnInst& inst);
    void V_BFREV_B32(const GcnInst& inst);
    void V_LDEXP_F32(const GcnInst& inst);
    void V_CVT_FLR_I32_F32(const GcnInst& inst);
    void V_CMP_CLASS_F32(const GcnInst& inst);
    void V_FFBL_B32(const GcnInst& inst);
    void V_MBCNT_U32_B32(bool is_low, const GcnInst& inst);
    void V_BFM_B32(const GcnInst& inst);
    void V_FFBH_U32(const GcnInst& inst);
    void V_MOVRELS_B32(const GcnInst& inst);
    void V_MOVRELD_B32(const GcnInst& inst);
    void V_MOVRELSD_B32(const GcnInst& inst);

    // Vector Memory
    void BUFFER_LOAD(u32 num_dwords, bool is_typed, const GcnInst& inst);
    void BUFFER_LOAD_FORMAT(u32 num_dwords, const GcnInst& inst);
    void BUFFER_STORE(u32 num_dwords, bool is_typed, const GcnInst& inst);
    void BUFFER_STORE_FORMAT(u32 num_dwords, const GcnInst& inst);
    void BUFFER_ATOMIC(AtomicOp op, const GcnInst& inst);

    // Vector interpolation
    void V_INTERP_P2_F32(const GcnInst& inst);
    void V_INTERP_MOV_F32(const GcnInst& inst);

    // Data share
    void DS_SWIZZLE_B32(const GcnInst& inst);
    void DS_READ(int bit_size, bool is_signed, bool is_pair, bool stride64, const GcnInst& inst);
    void DS_WRITE(int bit_size, bool is_signed, bool is_pair, bool stride64, const GcnInst& inst);
    void DS_ADD_U32(const GcnInst& inst, bool rtn);
    void DS_MIN_U32(const GcnInst& inst, bool is_signed, bool rtn);
    void DS_MAX_U32(const GcnInst& inst, bool is_signed, bool rtn);
    void V_READFIRSTLANE_B32(const GcnInst& inst);
    void V_READLANE_B32(const GcnInst& inst);
    void V_WRITELANE_B32(const GcnInst& inst);
    void DS_APPEND(const GcnInst& inst);
    void DS_CONSUME(const GcnInst& inst);
    void S_BARRIER();

    // MIMG
    void IMAGE_GET_RESINFO(const GcnInst& inst);
    void IMAGE_SAMPLE(const GcnInst& inst);
    void IMAGE_GATHER(const GcnInst& inst);
    void IMAGE_STORE(const GcnInst& inst);
    void IMAGE_LOAD(bool has_mip, const GcnInst& inst);
    void IMAGE_GET_LOD(const GcnInst& inst);
    void IMAGE_ATOMIC(AtomicOp op, const GcnInst& inst);

private:
    template <typename T = IR::U32>
    [[nodiscard]] T GetSrc(const InstOperand& operand);
    template <typename T = IR::U64>
    [[nodiscard]] T GetSrc64(const InstOperand& operand);
    void SetDst(const InstOperand& operand, const IR::U32F32& value);
    void SetDst64(const InstOperand& operand, const IR::U64F64& value_raw);

    IR::U32 VMovRelSHelper(u32 src_vgprno, const IR::U32 m0);
    void VMovRelDHelper(u32 dst_vgprno, const IR::U32 src_val, const IR::U32 m0);

    void LogMissingOpcode(const GcnInst& inst);

private:
    IR::IREmitter ir;
    Info& info;
    const RuntimeInfo& runtime_info;
    const Profile& profile;
    bool opcode_missing = false;
};

void Translate(IR::Block* block, u32 block_base, std::span<const GcnInst> inst_list, Info& info,
               const RuntimeInfo& runtime_info, const Profile& profile);

} // namespace Shader::Gcn
