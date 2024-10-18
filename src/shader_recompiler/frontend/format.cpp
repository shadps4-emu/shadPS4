// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "shader_recompiler/frontend/decode.h"

namespace Shader::Gcn {

constexpr std::array<InstFormat, 45> InstructionFormatSOP2 = {{
    // 0 = S_ADD_U32
    {InstClass::ScalarArith, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 1 = S_SUB_U32
    {InstClass::ScalarArith, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 2 = S_ADD_I32
    {InstClass::ScalarArith, InstCategory::ScalarALU, 2, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 3 = S_SUB_I32
    {InstClass::ScalarArith, InstCategory::ScalarALU, 2, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 4 = S_ADDC_U32
    {InstClass::ScalarArith, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 5 = S_SUBB_U32
    {InstClass::ScalarArith, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 6 = S_MIN_I32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 7 = S_MIN_U32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 8 = S_MAX_I32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 9 = S_MAX_U32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 10 = S_CSELECT_B32
    {InstClass::ScalarSelect, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 11 = S_CSELECT_B64
    {InstClass::ScalarSelect, InstCategory::ScalarALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    {},
    {},
    // 14 = S_AND_B32
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 15 = S_AND_B64
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 16 = S_OR_B32
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 17 = S_OR_B64
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 18 = S_XOR_B32
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 19 = S_XOR_B64
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 20 = S_ANDN2_B32
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 21 = S_ANDN2_B64
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 22 = S_ORN2_B32
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 23 = S_ORN2_B64
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 24 = S_NAND_B32
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 25 = S_NAND_B64
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 26 = S_NOR_B32
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 27 = S_NOR_B64
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 28 = S_XNOR_B32
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 29 = S_XNOR_B64
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 30 = S_LSHL_B32
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 31 = S_LSHL_B64
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 32 = S_LSHR_B32
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 33 = S_LSHR_B64
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 34 = S_ASHR_I32
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 35 = S_ASHR_I64
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 36 = S_BFM_B32
    {InstClass::ScalarBitField, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 37 = S_BFM_B64
    {InstClass::ScalarBitField, InstCategory::ScalarALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 38 = S_MUL_I32
    {InstClass::ScalarArith, InstCategory::ScalarALU, 2, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 39 = S_BFE_U32
    {InstClass::ScalarBitField, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 40 = S_BFE_I32
    {InstClass::ScalarBitField, InstCategory::ScalarALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 41 = S_BFE_U64
    {InstClass::ScalarBitField, InstCategory::ScalarALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 42 = S_BFE_I64
    {InstClass::ScalarBitField, InstCategory::ScalarALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 43 = S_CBRANCH_G_FORK
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 2, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 44 = S_ABSDIFF_I32
    {InstClass::ScalarAbs, InstCategory::ScalarALU, 2, 1, ScalarType::Sint32, ScalarType::Sint32},
}};

constexpr std::array<InstFormat, 22> InstructionFormatSOPK = {{
    // 0 = S_MOVK_I32
    {InstClass::ScalarMov, InstCategory::ScalarALU, 0, 1, ScalarType::Sint32, ScalarType::Sint32},
    {},
    // 2 = S_CMOVK_I32
    {InstClass::ScalarMov, InstCategory::ScalarALU, 0, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 3 = S_CMPK_EQ_I32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 0, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 4 = S_CMPK_LI32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 0, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 5 = S_CMPK_GT_I32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 0, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 6 = S_CMPK_GE_I32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 0, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 7 = S_CMPK_LT_I32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 0, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 8 = S_CMPK_LE_I32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 0, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 9 = S_CMPK_EQ_U32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 0, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 10 = S_CMPK_LG_U32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 0, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 11 = S_CMPK_GT_U32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 0, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 12 = S_CMPK_GE_U32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 0, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 13 = S_CMPK_LT_U32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 0, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 14 = S_CMPK_LE_U32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 0, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 15 = S_ADDK_I32
    {InstClass::ScalarArith, InstCategory::ScalarALU, 0, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 16 = S_MULK_I32
    {InstClass::ScalarArith, InstCategory::ScalarALU, 0, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 17 = S_CBRANCH_I_FORK
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 0, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 18 = S_GETREG_B32
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 0, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 19 = S_SETREG_B32
    {InstClass::ScalarRegAccess, InstCategory::FlowControl, 0, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    {},
    // 21 = S_SETREIMM32_B32
    {InstClass::ScalarRegAccess, InstCategory::FlowControl, 0, 1, ScalarType::Uint32,
     ScalarType::Uint32},
}};

constexpr std::array<InstFormat, 54> InstructionFormatSOP1 = {{
    {},
    {},
    {},
    // 3 = S_MOV_B32
    {InstClass::ScalarMov, InstCategory::ScalarALU, 1, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 4 = S_MOV_B64
    {InstClass::ScalarMov, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64, ScalarType::Uint64},
    // 5 = S_CMOV_B32
    {InstClass::ScalarMov, InstCategory::ScalarALU, 1, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 6 = S_CMOV_B64
    {InstClass::ScalarMov, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64, ScalarType::Uint64},
    // 7 = S_NOT_B32
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 8 = S_NOT_B64
    {InstClass::ScalarBitLogic, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 9 = S_WQM_B32
    {InstClass::ScalarQuadMask, InstCategory::ScalarALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 10 = S_WQM_B64
    {InstClass::ScalarQuadMask, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 11 = S_BREV_B32
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 12 = S_BREV_B64
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 13 = S_BCNT0_I32_B32
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint32,
     ScalarType::Sint32},
    // 14 = S_BCNT0_I32_B64
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Sint32},
    // 15 = S_BCNT1_I32_B32
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint32,
     ScalarType::Sint32},
    // 16 = S_BCNT1_I32_B64
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Sint32},
    // 17 = S_FF0_I32_B32
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint32,
     ScalarType::Sint32},
    // 18 = S_FF0_I32_B64
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Sint32},
    // 19 = S_FF1_I32_B32
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint32,
     ScalarType::Sint32},
    // 20 = S_FF1_I32_B64
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Sint32},
    // 21 = S_FLBIT_I32_B32
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint32,
     ScalarType::Sint32},
    // 22 = S_FLBIT_I32_B64
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Sint32},
    // 23 = S_FLBIT_I32
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 24 = S_FLBIT_I32_I64
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Sint64,
     ScalarType::Sint32},
    // 25 = S_SEXT_I32_I8
    {InstClass::ScalarConv, InstCategory::ScalarALU, 1, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 26 = S_SEXT_I32_I16
    {InstClass::ScalarConv, InstCategory::ScalarALU, 1, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 27 = S_BITSET0_B32
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 28 = S_BITSET0_B64
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 29 = S_BITSET1_B32
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 30 = S_BITSET1_B64
    {InstClass::ScalarBitManip, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 31 = S_GETPC_B64
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 0, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 32 = S_SETPC_B64
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 33 = S_SWAPPC_B64
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 34 = S_RFE_B64
    {InstClass::Undefined, InstCategory::Undefined, 1, 1, ScalarType::Uint64, ScalarType::Uint64},
    {},
    // 36 = S_AND_SAVEEXEC_B64
    {InstClass::ScalarExecMask, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 37 = S_OR_SAVEEXEC_B64
    {InstClass::ScalarExecMask, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 38 = S_XOR_SAVEEXEC_B64
    {InstClass::ScalarExecMask, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 39 = S_ANDN2_SAVEEXEC_B64
    {InstClass::ScalarExecMask, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 40 = S_ORN2_SAVEEXEC_B64
    {InstClass::ScalarExecMask, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 41 = S_NAND_SAVEEXEC_B64
    {InstClass::ScalarExecMask, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 42 = S_NOR_SAVEEXEC_B64
    {InstClass::ScalarExecMask, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 43 = S_XNOR_SAVEEXEC_B64
    {InstClass::ScalarExecMask, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 44 = S_QUADMASK_B32
    {InstClass::ScalarQuadMask, InstCategory::ScalarALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 45 = S_QUADMASK_B64
    {InstClass::ScalarQuadMask, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 46 = S_MOVRELS_B32
    {InstClass::ScalarMovRel, InstCategory::ScalarALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 47 = S_MOVRELS_B64
    {InstClass::ScalarMovRel, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 48 = S_MOVRELD_B32
    {InstClass::ScalarMovRel, InstCategory::ScalarALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 49 = S_MOVRELD_B64
    {InstClass::ScalarMovRel, InstCategory::ScalarALU, 1, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 50 = S_CBRANCH_JOIN
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 1, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    {},
    // 52 = S_ABS_I32
    {InstClass::ScalarAbs, InstCategory::ScalarALU, 1, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 53 = S_MOV_FED_B32
    {InstClass::Undefined, InstCategory::Undefined, 1, 1, ScalarType::Uint32, ScalarType::Uint32},
}};

constexpr std::array<InstFormat, 17> InstructionFormatSOPC = {{
    // 0 = S_CMP_EQ_I32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 1 = S_CMP_LI32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 2 = S_CMP_GT_I32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 3 = S_CMP_GE_I32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 4 = S_CMP_LT_I32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 5 = S_CMP_LE_I32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 6 = S_CMP_EQ_U32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 7 = S_CMP_LG_U32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 8 = S_CMP_GT_U32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 9 = S_CMP_GE_U32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 10 = S_CMP_LT_U32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 11 = S_CMP_LE_U32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 12 = S_BITCMP0_B32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 13 = S_BITCMP1_B32
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 14 = S_BITCMP0_B64
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Uint64, ScalarType::Uint64},
    // 15 = S_BITCMP1_B64
    {InstClass::ScalarCmp, InstCategory::ScalarALU, 2, 1, ScalarType::Uint64, ScalarType::Uint64},
    // 16 = S_SETVSKIP
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 2, 1, ScalarType::Undefined,
     ScalarType::Undefined},
}};

constexpr std::array<InstFormat, 27> InstructionFormatSOPP = {{
    // 0 = S_NOP
    {InstClass::ScalarWait, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    // 1 = S_ENDPGM
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    // 2 = S_BRANCH
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    {},
    // 4 = S_CBRANCH_SCC0
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    // 5 = S_CBRANCH_SCC1
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    // 6 = S_CBRANCH_VCCZ
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    // 7 = S_CBRANCH_VCCNZ
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    // 8 = S_CBRANCH_EXECZ
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    // 9 = S_CBRANCH_EXECNZ
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    // 10 = S_BARRIER
    {InstClass::ScalarSync, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    {},
    // 12 = S_WAITCNT
    {InstClass::ScalarSync, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    // 13 = S_SETHALT
    {InstClass::Undefined, InstCategory::Undefined, 0, 1, ScalarType::Any, ScalarType::Any},
    // 14 = S_SLEEP
    {InstClass::ScalarSync, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    // 15 = S_SETPRIO
    {InstClass::ScalarSync, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    // 16 = S_SENDMSG
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    // 17 = S_SENDMSGHALT
    {InstClass::ScalarProgFlow, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    // 18 = S_TRAP
    {InstClass::Undefined, InstCategory::Undefined, 0, 1, ScalarType::Any, ScalarType::Any},
    // 19 = S_ICACHE_INV
    {InstClass::ScalarCache, InstCategory::FlowControl, 0, 1, ScalarType::Any, ScalarType::Any},
    // 20 = S_INCPERFLEVEL
    {InstClass::DbgProf, InstCategory::DebugProfile, 0, 1, ScalarType::Any, ScalarType::Any},
    // 21 = S_DECPERFLEVEL
    {InstClass::DbgProf, InstCategory::DebugProfile, 0, 1, ScalarType::Any, ScalarType::Any},
    // 22 = S_TTRACEDATA
    {InstClass::DbgProf, InstCategory::DebugProfile, 0, 1, ScalarType::Any, ScalarType::Any},
    // 23 = S_CBRANCH_CDBGSYS
    {InstClass::Undefined, InstCategory::Undefined, 0, 1, ScalarType::Any, ScalarType::Any},
    // 24 = S_CBRANCH_CDBGUSER
    {InstClass::Undefined, InstCategory::Undefined, 0, 1, ScalarType::Any, ScalarType::Any},
    // 25 = S_CBRANCH_CDBGSYS_OR_USER
    {InstClass::Undefined, InstCategory::Undefined, 0, 1, ScalarType::Any, ScalarType::Any},
    // 26 = S_CBRANCH_CDBGSYS_AND_USER
    {InstClass::Undefined, InstCategory::Undefined, 0, 1, ScalarType::Any, ScalarType::Any},
}};

constexpr std::array<InstFormat, 32> InstructionFormatSMRD = {{
    // 0 = S_LOAD_DWORD
    {InstClass::ScalarMemRd, InstCategory::ScalarMemory, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 1 = S_LOAD_DWORDX2
    {InstClass::ScalarMemRd, InstCategory::ScalarMemory, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 2 = S_LOAD_DWORDX4
    {InstClass::ScalarMemRd, InstCategory::ScalarMemory, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 3 = S_LOAD_DWORDX8
    {InstClass::ScalarMemRd, InstCategory::ScalarMemory, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 4 = S_LOAD_DWORDX16
    {InstClass::ScalarMemRd, InstCategory::ScalarMemory, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    {},
    {},
    {},
    // 8 = S_BUFFER_LOAD_DWORD
    {InstClass::ScalarMemRd, InstCategory::ScalarMemory, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 9 = S_BUFFER_LOAD_DWORDX2
    {InstClass::ScalarMemRd, InstCategory::ScalarMemory, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 10 = S_BUFFER_LOAD_DWORDX4
    {InstClass::ScalarMemRd, InstCategory::ScalarMemory, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 11 = S_BUFFER_LOAD_DWORDX8
    {InstClass::ScalarMemRd, InstCategory::ScalarMemory, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 12 = S_BUFFER_LOAD_DWORDX16
    {InstClass::ScalarMemRd, InstCategory::ScalarMemory, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 29 = S_DCACHE_INV_VOL
    {InstClass::ScalarMemUt, InstCategory::ScalarMemory, 1, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 30 = S_MEMTIME
    {InstClass::ScalarMemUt, InstCategory::ScalarMemory, 1, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 31 = S_DCACHE_INV
    {InstClass::ScalarMemUt, InstCategory::ScalarMemory, 1, 1, ScalarType::Undefined,
     ScalarType::Undefined},
}};

constexpr std::array<InstFormat, 50> InstructionFormatVOP2 = {{
    // 0 = V_CNDMASK_B32
    {InstClass::VectorThreadMask, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 1 = V_READLANE_B32
    {InstClass::VectorLane, InstCategory::VectorALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 2 = V_WRITELANE_B32
    {InstClass::VectorLane, InstCategory::VectorALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 3 = V_ADD_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 4 = V_SUB_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 5 = V_SUBREV_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 6 = V_MAC_LEGACY_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 7 = V_MUL_LEGACY_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 8 = V_MUL_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 9 = V_MUL_I32_I24
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 10 = V_MUL_HI_I32_I24
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 11 = V_MUL_U32_U24
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 12 = V_MUL_HI_U32_U24
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 13 = V_MIN_LEGACY_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 14 = V_MAX_LEGACY_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 15 = V_MIN_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 16 = V_MAX_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 17 = V_MIN_I32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 18 = V_MAX_I32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 19 = V_MIN_U32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 20 = V_MAX_U32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 21 = V_LSHR_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 22 = V_LSHRREV_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 23 = V_ASHR_I32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 24 = V_ASHRREV_I32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 25 = V_LSHL_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 26 = V_LSHLREV_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 27 = V_AND_B32
    {InstClass::VectorBitLogic, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 28 = V_OR_B32
    {InstClass::VectorBitLogic, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 29 = V_XOR_B32
    {InstClass::VectorBitLogic, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 30 = V_BFM_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 31 = V_MAC_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 32 = V_MADMK_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 33 = V_MADAK_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 34 = V_BCNT_U32_B32
    {InstClass::VectorThreadMask, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 35 = V_MBCNT_LO_U32_B32
    {InstClass::VectorThreadMask, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 36 = V_MBCNT_HI_U32_B32
    {InstClass::VectorThreadMask, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 37 = V_ADD_I32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 38 = V_SUB_I32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 39 = V_SUBREV_I32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 40 = V_ADDC_U32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 41 = V_SUBB_U32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 42 = V_SUBBREV_U32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 43 = V_LDEXP_F32
    {InstClass::VectorFpField32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 44 = V_CVT_PKACCUM_U8_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 2, 1, ScalarType::Float32, ScalarType::Uint32},
    // 45 = V_CVT_PKNORM_I16_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 2, 1, ScalarType::Float32, ScalarType::Sint32},
    // 46 = V_CVT_PKNORM_U16_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 2, 1, ScalarType::Float32, ScalarType::Uint32},
    // 47 = V_CVT_PKRTZ_F16_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 2, 1, ScalarType::Float32, ScalarType::Uint32},
    // 48 = V_CVT_PK_U16_U32
    {InstClass::VectorConv, InstCategory::VectorALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 49 = V_CVT_PK_I16_I32
    {InstClass::VectorConv, InstCategory::VectorALU, 2, 1, ScalarType::Sint32, ScalarType::Sint32},
}};

constexpr std::array<InstFormat, 455> InstructionFormatVOP3 = {{
    // 0 = V_CMP_F_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 1 = V_CMP_LT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 2 = V_CMP_EQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 3 = V_CMP_LE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 4 = V_CMP_GT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 5 = V_CMP_LG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 6 = V_CMP_GE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 7 = V_CMP_O_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 8 = V_CMP_U_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 9 = V_CMP_NGE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 10 = V_CMP_NLG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 11 = V_CMP_NGT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 12 = V_CMP_NLE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 13 = V_CMP_NEQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 14 = V_CMP_NLT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 15 = V_CMP_TRU_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 16 = V_CMPX_F_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 17 = V_CMPX_LT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 18 = V_CMPX_EQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 19 = V_CMPX_LE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 20 = V_CMPX_GT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 21 = V_CMPX_LG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 22 = V_CMPX_GE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 23 = V_CMPX_O_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 24 = V_CMPX_U_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 25 = V_CMPX_NGE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 26 = V_CMPX_NLG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 27 = V_CMPX_NGT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 28 = V_CMPX_NLE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 29 = V_CMPX_NEQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 30 = V_CMPX_NLT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 31 = V_CMPX_TRU_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 32 = V_CMP_F_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 33 = V_CMP_LT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 34 = V_CMP_EQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 35 = V_CMP_LE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 36 = V_CMP_GT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 37 = V_CMP_LG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 38 = V_CMP_GE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 39 = V_CMP_O_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 40 = V_CMP_U_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 41 = V_CMP_NGE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 42 = V_CMP_NLG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 43 = V_CMP_NGT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 44 = V_CMP_NLE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 45 = V_CMP_NEQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 46 = V_CMP_NLT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 47 = V_CMP_TRU_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 48 = V_CMPX_F_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 49 = V_CMPX_LT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 50 = V_CMPX_EQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 51 = V_CMPX_LE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 52 = V_CMPX_GT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 53 = V_CMPX_LG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 54 = V_CMPX_GE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 55 = V_CMPX_O_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 56 = V_CMPX_U_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 57 = V_CMPX_NGE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 58 = V_CMPX_NLG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 59 = V_CMPX_NGT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 60 = V_CMPX_NLE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 61 = V_CMPX_NEQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 62 = V_CMPX_NLT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 63 = V_CMPX_TRU_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 64 = V_CMPS_F_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 65 = V_CMPS_LT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 66 = V_CMPS_EQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 67 = V_CMPS_LE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 68 = V_CMPS_GT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 69 = V_CMPS_LG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 70 = V_CMPS_GE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 71 = V_CMPS_O_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 72 = V_CMPS_U_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 73 = V_CMPS_NGE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 74 = V_CMPS_NLG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 75 = V_CMPS_NGT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 76 = V_CMPS_NLE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 77 = V_CMPS_NEQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 78 = V_CMPS_NLT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 79 = V_CMPS_TRU_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 80 = V_CMPSX_F_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 81 = V_CMPSX_LT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 82 = V_CMPSX_EQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 83 = V_CMPSX_LE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 84 = V_CMPSX_GT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 85 = V_CMPSX_LG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 86 = V_CMPSX_GE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 87 = V_CMPSX_O_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 88 = V_CMPSX_U_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 89 = V_CMPSX_NGE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 90 = V_CMPSX_NLG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 91 = V_CMPSX_NGT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 92 = V_CMPSX_NLE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 93 = V_CMPSX_NEQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 94 = V_CMPSX_NLT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 95 = V_CMPSX_TRU_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 96 = V_CMPS_F_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 97 = V_CMPS_LT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 98 = V_CMPS_EQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 99 = V_CMPS_LE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 100 = V_CMPS_GT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 101 = V_CMPS_LG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 102 = V_CMPS_GE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 103 = V_CMPS_O_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 104 = V_CMPS_U_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 105 = V_CMPS_NGE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 106 = V_CMPS_NLG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 107 = V_CMPS_NGT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 108 = V_CMPS_NLE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 109 = V_CMPS_NEQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 110 = V_CMPS_NLT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 111 = V_CMPS_TRU_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 112 = V_CMPSX_F_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 113 = V_CMPSX_LT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 114 = V_CMPSX_EQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 115 = V_CMPSX_LE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 116 = V_CMPSX_GT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 117 = V_CMPSX_LG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 118 = V_CMPSX_GE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 119 = V_CMPSX_O_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 120 = V_CMPSX_U_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 121 = V_CMPSX_NGE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 122 = V_CMPSX_NLG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 123 = V_CMPSX_NGT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 124 = V_CMPSX_NLE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 125 = V_CMPSX_NEQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 126 = V_CMPSX_NLT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 127 = V_CMPSX_TRU_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 128 = V_CMP_F_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 129 = V_CMP_LT_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 130 = V_CMP_EQ_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 131 = V_CMP_LE_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 132 = V_CMP_GT_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 133 = V_CMP_NE_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 134 = V_CMP_GE_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 135 = V_CMP_T_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 136 = V_CMP_CLASS_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 144 = V_CMPX_F_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 145 = V_CMPX_LT_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 146 = V_CMPX_EQ_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 147 = V_CMPX_LE_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 148 = V_CMPX_GT_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 149 = V_CMPX_NE_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 150 = V_CMPX_GE_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 151 = V_CMPX_T_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 152 = V_CMPX_CLASS_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 160 = V_CMP_F_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 161 = V_CMP_LT_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 162 = V_CMP_EQ_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 163 = V_CMP_LE_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 164 = V_CMP_GT_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 165 = V_CMP_NE_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 166 = V_CMP_GE_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 167 = V_CMP_T_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 168 = V_CMP_CLASS_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 176 = V_CMPX_F_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 177 = V_CMPX_LT_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 178 = V_CMPX_EQ_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 179 = V_CMPX_LE_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 180 = V_CMPX_GT_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 181 = V_CMPX_NE_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 182 = V_CMPX_GE_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 183 = V_CMPX_T_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 184 = V_CMPX_CLASS_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 192 = V_CMP_F_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 193 = V_CMP_LT_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 194 = V_CMP_EQ_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 195 = V_CMP_LE_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 196 = V_CMP_GT_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 197 = V_CMP_NE_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 198 = V_CMP_GE_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 199 = V_CMP_T_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 208 = V_CMPX_F_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 209 = V_CMPX_LT_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 210 = V_CMPX_EQ_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 211 = V_CMPX_LE_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 212 = V_CMPX_GT_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 213 = V_CMPX_NE_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 214 = V_CMPX_GE_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 215 = V_CMPX_T_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 224 = V_CMP_F_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 225 = V_CMP_LT_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 226 = V_CMP_EQ_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 227 = V_CMP_LE_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 228 = V_CMP_GT_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 229 = V_CMP_NE_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 230 = V_CMP_GE_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 231 = V_CMP_T_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 240 = V_CMPX_F_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 241 = V_CMPX_LT_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 242 = V_CMPX_EQ_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 243 = V_CMPX_LE_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 244 = V_CMPX_GT_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 245 = V_CMPX_NE_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 246 = V_CMPX_GE_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 247 = V_CMPX_T_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 256 = V_CNDMASK_B32
    {InstClass::VectorThreadMask, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 257 = V_READLANE_B32
    {InstClass::VectorLane, InstCategory::VectorALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 258 = V_WRITELANE_B32
    {InstClass::VectorLane, InstCategory::VectorALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 259 = V_ADD_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 260 = V_SUB_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 261 = V_SUBREV_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 262 = V_MAC_LEGACY_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 263 = V_MUL_LEGACY_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 264 = V_MUL_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 265 = V_MUL_I32_I24
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 266 = V_MUL_HI_I32_I24
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 267 = V_MUL_U32_U24
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 268 = V_MUL_HI_U32_U24
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 269 = V_MIN_LEGACY_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 270 = V_MAX_LEGACY_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 271 = V_MIN_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 272 = V_MAX_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 273 = V_MIN_I32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 274 = V_MAX_I32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 275 = V_MIN_U32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 276 = V_MAX_U32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 277 = V_LSHR_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 278 = V_LSHRREV_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 279 = V_ASHR_I32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 280 = V_ASHRREV_I32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 281 = V_LSHL_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 282 = V_LSHLREV_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 283 = V_AND_B32
    {InstClass::VectorBitLogic, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 284 = V_OR_B32
    {InstClass::VectorBitLogic, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 285 = V_XOR_B32
    {InstClass::VectorBitLogic, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 286 = V_BFM_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 287 = V_MAC_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 288 = V_MADMK_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 289 = V_MADAK_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 290 = V_BCNT_U32_B32
    {InstClass::VectorThreadMask, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 291 = V_MBCNT_LO_U32_B32
    {InstClass::VectorThreadMask, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 292 = V_MBCNT_HI_U32_B32
    {InstClass::VectorThreadMask, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 293 = V_ADD_I32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 3, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 294 = V_SUB_I32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 3, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 295 = V_SUBREV_I32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 3, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 296 = V_ADDC_U32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 297 = V_SUBB_U32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 298 = V_SUBBREV_U32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 299 = V_LDEXP_F32
    {InstClass::VectorFpField32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 300 = V_CVT_PKACCUM_U8_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 2, 1, ScalarType::Float32, ScalarType::Uint32},
    // 301 = V_CVT_PKNORM_I16_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 2, 1, ScalarType::Float32, ScalarType::Sint32},
    // 302 = V_CVT_PKNORM_U16_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 2, 1, ScalarType::Float32, ScalarType::Uint32},
    // 303 = V_CVT_PKRTZ_F16_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 2, 1, ScalarType::Float32, ScalarType::Uint32},
    // 304 = V_CVT_PK_U16_U32
    {InstClass::VectorConv, InstCategory::VectorALU, 2, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 305 = V_CVT_PK_I16_I32
    {InstClass::VectorConv, InstCategory::VectorALU, 2, 1, ScalarType::Sint32, ScalarType::Sint32},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 320 = V_MAD_LEGACY_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 321 = V_MAD_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 322 = V_MAD_I32_I24
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 3, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 323 = V_MAD_U32_U24
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 324 = V_CUBEID_F32
    {InstClass::VectorFpGraph32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 325 = V_CUBESC_F32
    {InstClass::VectorFpGraph32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 326 = V_CUBETC_F32
    {InstClass::VectorFpGraph32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 327 = V_CUBEMA_F32
    {InstClass::VectorFpGraph32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 328 = V_BFE_U32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 329 = V_BFE_I32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 3, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 330 = V_BFI_B32
    {InstClass::VectorBitLogic, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 331 = V_FMA_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 332 = V_FMA_F64
    {InstClass::VectorFpArith64, InstCategory::VectorALU, 3, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 333 = V_LERP_U8
    {InstClass::VectorIntGraph, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 334 = V_ALIGNBIT_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 335 = V_ALIGNBYTE_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 336 = V_MULLIT_F32
    {InstClass::VectorFpGraph32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 337 = V_MIN3_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 338 = V_MIN3_I32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 3, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 339 = V_MIN3_U32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 340 = V_MAX3_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 341 = V_MAX3_I32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 3, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 342 = V_MAX3_U32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 343 = V_MED3_F32
    {InstClass::VectorFpArith32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 344 = V_MED3_I32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 3, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 345 = V_MED3_U32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 346 = V_SAD_U8
    {InstClass::VectorIntGraph, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 347 = V_SAD_HI_U8
    {InstClass::VectorIntGraph, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 348 = V_SAD_U16
    {InstClass::VectorIntGraph, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 349 = V_SAD_U32
    {InstClass::VectorIntGraph, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 350 = V_CVT_PK_U8_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 3, 1, ScalarType::Float32, ScalarType::Uint32},
    // 351 = V_DIV_FIXUP_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 352 = V_DIV_FIXUP_F64
    {InstClass::VectorFpTran64, InstCategory::VectorALU, 3, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 353 = V_LSHL_B64
    {InstClass::VectorBitField64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 354 = V_LSHR_B64
    {InstClass::VectorBitField64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 355 = V_ASHR_I64
    {InstClass::VectorBitField64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 356 = V_ADD_F64
    {InstClass::VectorFpArith64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 357 = V_MUL_F64
    {InstClass::VectorFpArith64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 358 = V_MIN_F64
    {InstClass::VectorFpArith64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 359 = V_MAX_F64
    {InstClass::VectorFpArith64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 360 = V_LDEXP_F64
    {InstClass::VectorFpField64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 361 = V_MUL_LO_U32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 362 = V_MUL_HI_U32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 363 = V_MUL_LO_I32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 364 = V_MUL_HI_I32
    {InstClass::VectorIntArith32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 365 = V_DIV_SCALE_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 366 = V_DIV_SCALE_F64
    {InstClass::VectorFpTran64, InstCategory::VectorALU, 3, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 367 = V_DIV_FMAS_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 368 = V_DIV_FMAS_F64
    {InstClass::VectorFpTran64, InstCategory::VectorALU, 3, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 369 = V_MSAD_U8
    {InstClass::VectorIntGraph, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 370 = V_QSAD_U8
    {InstClass::Undefined, InstCategory::Undefined, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 371 = V_MQSAD_U8
    {InstClass::Undefined, InstCategory::Undefined, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 372 = V_TRIG_PREOP_F64
    {InstClass::VectorFpTran64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 373 = V_MQSAD_U32_U8
    {InstClass::VectorIntGraph, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 374 = V_MAD_U64_U32
    {InstClass::VectorIntArith64, InstCategory::VectorALU, 3, 1, ScalarType::Uint32,
     ScalarType::Uint64},
    // 375 = V_MAD_I64_I32
    {InstClass::VectorIntArith64, InstCategory::VectorALU, 3, 1, ScalarType::Sint32,
     ScalarType::Sint64},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 384 = V_NOP
    {InstClass::VectorMisc, InstCategory::VectorALU, 0, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 385 = V_MOV_B32
    {InstClass::VectorRegMov, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 386 = V_READFIRSTLANE_B32
    {InstClass::VectorLane, InstCategory::VectorALU, 1, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 387 = V_CVT_I32_F64
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float64, ScalarType::Sint32},
    // 388 = V_CVT_F64_I32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Sint32, ScalarType::Float64},
    // 389 = V_CVT_F32_I32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Sint32,
     ScalarType::Float32},
    // 390 = V_CVT_F32_U32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 391 = V_CVT_U32_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Uint32},
    // 392 = V_CVT_I32_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Sint32},
    // 393 = V_MOV_FED_B32
    {InstClass::Undefined, InstCategory::Undefined, 1, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 394 = V_CVT_F16_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float16},
    // 395 = V_CVT_F32_F16
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float16,
     ScalarType::Float32},
    // 396 = V_CVT_RPI_I32_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float32, ScalarType::Sint32},
    // 397 = V_CVT_FLR_I32_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float32, ScalarType::Sint32},
    // 398 = V_CVT_OFF_F32_I4
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Sint32, ScalarType::Float32},
    // 399 = V_CVT_F32_F64
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float32},
    // 400 = V_CVT_F64_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float64},
    // 401 = V_CVT_F32_UBYTE0
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 402 = V_CVT_F32_UBYTE1
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 403 = V_CVT_F32_UBYTE2
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 404 = V_CVT_F32_UBYTE3
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 405 = V_CVT_U32_F64
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float64, ScalarType::Uint32},
    // 406 = V_CVT_F64_U32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Uint32, ScalarType::Float64},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 416 = V_FRACT_F32
    {InstClass::VectorFpRound32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 417 = V_TRUNC_F32
    {InstClass::VectorFpRound32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 418 = V_CEIL_F32
    {InstClass::VectorFpRound32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 419 = V_RNDNE_F32
    {InstClass::VectorFpRound32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 420 = V_FLOOR_F32
    {InstClass::VectorFpRound32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 421 = V_EXP_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 422 = V_LOG_CLAMP_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 423 = V_LOG_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 424 = V_RCP_CLAMP_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 425 = V_RCP_LEGACY_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 426 = V_RCP_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 427 = V_RCP_IFLAG_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 428 = V_RSQ_CLAMP_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 429 = V_RSQ_LEGACY_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 430 = V_RSQ_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 431 = V_RCP_F64
    {InstClass::VectorFpTran64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 432 = V_RCP_CLAMP_F64
    {InstClass::VectorFpTran64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 433 = V_RSQ_F64
    {InstClass::VectorFpTran64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 434 = V_RSQ_CLAMP_F64
    {InstClass::VectorFpTran64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 435 = V_SQRT_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 436 = V_SQRT_F64
    {InstClass::VectorFpTran64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 437 = V_SIN_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 438 = V_COS_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 439 = V_NOT_B32
    {InstClass::VectorBitLogic, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 440 = V_BFREV_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 441 = V_FFBH_U32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 442 = V_FFBL_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 443 = V_FFBH_I32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 1, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 444 = V_FREXP_EXP_I32_F64
    {InstClass::VectorFpField64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Sint32},
    // 445 = V_FREXP_MANT_F64
    {InstClass::VectorFpField64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 446 = V_FRACT_F64
    {InstClass::VectorFpRound64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 447 = V_FREXP_EXP_I32_F32
    {InstClass::VectorFpField32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Sint32},
    // 448 = V_FREXP_MANT_F32
    {InstClass::VectorFpField32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 449 = V_CLREXCP
    {InstClass::Undefined, InstCategory::Undefined, 0, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 450 = V_MOVRELD_B32
    {InstClass::VectorMovRel, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 451 = V_MOVRELS_B32
    {InstClass::VectorMovRel, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 452 = V_MOVRELSD_B32
    {InstClass::VectorMovRel, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    {},
    {},
}};

constexpr std::array<InstFormat, 71> InstructionFormatVOP1 = {{
    // 0 = V_NOP
    {InstClass::VectorMisc, InstCategory::VectorALU, 0, 1, ScalarType::Any, ScalarType::Any},
    // 1 = V_MOV_B32
    {InstClass::VectorRegMov, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 2 = V_READFIRSTLANE_B32
    {InstClass::VectorLane, InstCategory::VectorALU, 1, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 3 = V_CVT_I32_F64
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float64, ScalarType::Sint32},
    // 4 = V_CVT_F64_I32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Sint32, ScalarType::Float64},
    // 5 = V_CVT_F32_I32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Sint32, ScalarType::Float32},
    // 6 = V_CVT_F32_U32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Uint32, ScalarType::Float32},
    // 7 = V_CVT_U32_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float32, ScalarType::Uint32},
    // 8 = V_CVT_I32_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float32, ScalarType::Sint32},
    // 9 = V_MOV_FED_B32
    {InstClass::Undefined, InstCategory::Undefined, 1, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 10 = V_CVT_F16_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float16},
    // 11 = V_CVT_F32_F16
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float16,
     ScalarType::Float32},
    // 12 = V_CVT_RPI_I32_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float32, ScalarType::Sint32},
    // 13 = V_CVT_FLR_I32_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float32, ScalarType::Sint32},
    // 14 = V_CVT_OFF_F32_I4
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Sint32, ScalarType::Float32},
    // 15 = V_CVT_F32_F64
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float32},
    // 16 = V_CVT_F64_F32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float64},
    // 17 = V_CVT_F32_UBYTE0
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Uint32, ScalarType::Float32},
    // 18 = V_CVT_F32_UBYTE1
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Uint32, ScalarType::Float32},
    // 19 = V_CVT_F32_UBYTE2
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Uint32, ScalarType::Float32},
    // 20 = V_CVT_F32_UBYTE3
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Uint32, ScalarType::Float32},
    // 21 = V_CVT_U32_F64
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Float64, ScalarType::Uint32},
    // 22 = V_CVT_F64_U32
    {InstClass::VectorConv, InstCategory::VectorALU, 1, 1, ScalarType::Uint32, ScalarType::Float64},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 32 = V_FRACT_F32
    {InstClass::VectorFpRound32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 33 = V_TRUNC_F32
    {InstClass::VectorFpRound32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 34 = V_CEIL_F32
    {InstClass::VectorFpRound32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 35 = V_RNDNE_F32
    {InstClass::VectorFpRound32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 36 = V_FLOOR_F32
    {InstClass::VectorFpRound32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 37 = V_EXP_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 38 = V_LOG_CLAMP_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 39 = V_LOG_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 40 = V_RCP_CLAMP_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 41 = V_RCP_LEGACY_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 42 = V_RCP_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 43 = V_RCP_IFLAG_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 44 = V_RSQ_CLAMP_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 45 = V_RSQ_LEGACY_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 46 = V_RSQ_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 47 = V_RCP_F64
    {InstClass::VectorFpTran64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 48 = V_RCP_CLAMP_F64
    {InstClass::VectorFpTran64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 49 = V_RSQ_F64
    {InstClass::VectorFpTran64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 50 = V_RSQ_CLAMP_F64
    {InstClass::VectorFpTran64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 51 = V_SQRT_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 52 = V_SQRT_F64
    {InstClass::VectorFpTran64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 53 = V_SIN_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 54 = V_COS_F32
    {InstClass::VectorFpTran32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 55 = V_NOT_B32
    {InstClass::VectorBitLogic, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 56 = V_BFREV_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 57 = V_FFBH_U32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 58 = V_FFBL_B32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 59 = V_FFBH_I32
    {InstClass::VectorBitField32, InstCategory::VectorALU, 1, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 60 = V_FREXP_EXP_I32_F64
    {InstClass::VectorFpField64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Sint32},
    // 61 = V_FREXP_MANT_F64
    {InstClass::VectorFpField64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 62 = V_FRACT_F64
    {InstClass::VectorFpRound64, InstCategory::VectorALU, 1, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 63 = V_FREXP_EXP_I32_F32
    {InstClass::VectorFpField32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Sint32},
    // 64 = V_FREXP_MANT_F32
    {InstClass::VectorFpField32, InstCategory::VectorALU, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 65 = V_CLREXCP
    {InstClass::Undefined, InstCategory::Undefined, 0, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 66 = V_MOVRELD_B32
    {InstClass::VectorMovRel, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 67 = V_MOVRELS_B32
    {InstClass::VectorMovRel, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 68 = V_MOVRELSD_B32
    {InstClass::VectorMovRel, InstCategory::VectorALU, 1, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    {},
    {},
}};

constexpr std::array<InstFormat, 248> InstructionFormatVOPC = {{
    // 0 = V_CMP_F_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 1 = V_CMP_LT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 2 = V_CMP_EQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 3 = V_CMP_LE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 4 = V_CMP_GT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 5 = V_CMP_LG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 6 = V_CMP_GE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 7 = V_CMP_O_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 8 = V_CMP_U_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 9 = V_CMP_NGE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 10 = V_CMP_NLG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 11 = V_CMP_NGT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 12 = V_CMP_NLE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 13 = V_CMP_NEQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 14 = V_CMP_NLT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 15 = V_CMP_TRU_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 16 = V_CMPX_F_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 17 = V_CMPX_LT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 18 = V_CMPX_EQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 19 = V_CMPX_LE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 20 = V_CMPX_GT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 21 = V_CMPX_LG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 22 = V_CMPX_GE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 23 = V_CMPX_O_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 24 = V_CMPX_U_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 25 = V_CMPX_NGE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 26 = V_CMPX_NLG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 27 = V_CMPX_NGT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 28 = V_CMPX_NLE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 29 = V_CMPX_NEQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 30 = V_CMPX_NLT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 31 = V_CMPX_TRU_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 32 = V_CMP_F_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 33 = V_CMP_LT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 34 = V_CMP_EQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 35 = V_CMP_LE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 36 = V_CMP_GT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 37 = V_CMP_LG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 38 = V_CMP_GE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 39 = V_CMP_O_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 40 = V_CMP_U_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 41 = V_CMP_NGE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 42 = V_CMP_NLG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 43 = V_CMP_NGT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 44 = V_CMP_NLE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 45 = V_CMP_NEQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 46 = V_CMP_NLT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 47 = V_CMP_TRU_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 48 = V_CMPX_F_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 49 = V_CMPX_LT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 50 = V_CMPX_EQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 51 = V_CMPX_LE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 52 = V_CMPX_GT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 53 = V_CMPX_LG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 54 = V_CMPX_GE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 55 = V_CMPX_O_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 56 = V_CMPX_U_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 57 = V_CMPX_NGE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 58 = V_CMPX_NLG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 59 = V_CMPX_NGT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 60 = V_CMPX_NLE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 61 = V_CMPX_NEQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 62 = V_CMPX_NLT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 63 = V_CMPX_TRU_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 64 = V_CMPS_F_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 65 = V_CMPS_LT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 66 = V_CMPS_EQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 67 = V_CMPS_LE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 68 = V_CMPS_GT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 69 = V_CMPS_LG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 70 = V_CMPS_GE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 71 = V_CMPS_O_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 72 = V_CMPS_U_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 73 = V_CMPS_NGE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 74 = V_CMPS_NLG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 75 = V_CMPS_NGT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 76 = V_CMPS_NLE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 77 = V_CMPS_NEQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 78 = V_CMPS_NLT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 79 = V_CMPS_TRU_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 80 = V_CMPSX_F_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 81 = V_CMPSX_LT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 82 = V_CMPSX_EQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 83 = V_CMPSX_LE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 84 = V_CMPSX_GT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 85 = V_CMPSX_LG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 86 = V_CMPSX_GE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 87 = V_CMPSX_O_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 88 = V_CMPSX_U_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 89 = V_CMPSX_NGE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 90 = V_CMPSX_NLG_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 91 = V_CMPSX_NGT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 92 = V_CMPSX_NLE_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 93 = V_CMPSX_NEQ_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 94 = V_CMPSX_NLT_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 95 = V_CMPSX_TRU_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 96 = V_CMPS_F_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 97 = V_CMPS_LT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 98 = V_CMPS_EQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 99 = V_CMPS_LE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 100 = V_CMPS_GT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 101 = V_CMPS_LG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 102 = V_CMPS_GE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 103 = V_CMPS_O_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 104 = V_CMPS_U_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 105 = V_CMPS_NGE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 106 = V_CMPS_NLG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 107 = V_CMPS_NGT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 108 = V_CMPS_NLE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 109 = V_CMPS_NEQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 110 = V_CMPS_NLT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 111 = V_CMPS_TRU_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 112 = V_CMPSX_F_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 113 = V_CMPSX_LT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 114 = V_CMPSX_EQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 115 = V_CMPSX_LE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 116 = V_CMPSX_GT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 117 = V_CMPSX_LG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 118 = V_CMPSX_GE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 119 = V_CMPSX_O_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 120 = V_CMPSX_U_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 121 = V_CMPSX_NGE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 122 = V_CMPSX_NLG_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 123 = V_CMPSX_NGT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 124 = V_CMPSX_NLE_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 125 = V_CMPSX_NEQ_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 126 = V_CMPSX_NLT_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 127 = V_CMPSX_TRU_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 128 = V_CMP_F_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 129 = V_CMP_LT_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 130 = V_CMP_EQ_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 131 = V_CMP_LE_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 132 = V_CMP_GT_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 133 = V_CMP_NE_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 134 = V_CMP_GE_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 135 = V_CMP_T_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 136 = V_CMP_CLASS_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 144 = V_CMPX_F_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 145 = V_CMPX_LT_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 146 = V_CMPX_EQ_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 147 = V_CMPX_LE_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 148 = V_CMPX_GT_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 149 = V_CMPX_NE_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 150 = V_CMPX_GE_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 151 = V_CMPX_T_I32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 152 = V_CMPX_CLASS_F32
    {InstClass::VectorFpCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Float32,
     ScalarType::Float32},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 160 = V_CMP_F_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 161 = V_CMP_LT_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 162 = V_CMP_EQ_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 163 = V_CMP_LE_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 164 = V_CMP_GT_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 165 = V_CMP_NE_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 166 = V_CMP_GE_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 167 = V_CMP_T_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 168 = V_CMP_CLASS_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 176 = V_CMPX_F_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 177 = V_CMPX_LT_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 178 = V_CMPX_EQ_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 179 = V_CMPX_LE_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 180 = V_CMPX_GT_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 181 = V_CMPX_NE_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 182 = V_CMPX_GE_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 183 = V_CMPX_T_I64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 184 = V_CMPX_CLASS_F64
    {InstClass::VectorFpCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Float64,
     ScalarType::Float64},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 192 = V_CMP_F_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 193 = V_CMP_LT_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 194 = V_CMP_EQ_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 195 = V_CMP_LE_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 196 = V_CMP_GT_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 197 = V_CMP_NE_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 198 = V_CMP_GE_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 199 = V_CMP_T_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 208 = V_CMPX_F_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 209 = V_CMPX_LT_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 210 = V_CMPX_EQ_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 211 = V_CMPX_LE_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 212 = V_CMPX_GT_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 213 = V_CMPX_NE_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 214 = V_CMPX_GE_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 215 = V_CMPX_T_U32
    {InstClass::VectorIntCmp32, InstCategory::VectorALU, 2, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 224 = V_CMP_F_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 225 = V_CMP_LT_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 226 = V_CMP_EQ_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 227 = V_CMP_LE_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 228 = V_CMP_GT_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 229 = V_CMP_NE_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 230 = V_CMP_GE_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 231 = V_CMP_T_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 240 = V_CMPX_F_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 241 = V_CMPX_LT_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 242 = V_CMPX_EQ_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 243 = V_CMPX_LE_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 244 = V_CMPX_GT_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 245 = V_CMPX_NE_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 246 = V_CMPX_GE_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 247 = V_CMPX_T_U64
    {InstClass::VectorIntCmp64, InstCategory::VectorALU, 2, 1, ScalarType::Uint64,
     ScalarType::Uint64},
}};

constexpr std::array<InstFormat, 3> InstructionFormatVINTRP = {{
    // 0 = V_INTERP_P1_F32
    {InstClass::VectorInterpFpCache, InstCategory::VectorInterpolation, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 1 = V_INTERP_P2_F32
    {InstClass::VectorInterpFpCache, InstCategory::VectorInterpolation, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 2 = V_INTERP_MOV_F32
    {InstClass::VectorInterpFpCache, InstCategory::VectorInterpolation, 1, 1, ScalarType::Float32,
     ScalarType::Float32},
}};

constexpr std::array<InstFormat, 256> InstructionFormatDS = {{
    // 0 = DS_ADD_U32
    {InstClass::DsAtomicArith32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 1 = DS_SUB_U32
    {InstClass::DsAtomicArith32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 2 = DS_RSUB_U32
    {InstClass::DsAtomicArith32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 3 = DS_INC_U32
    {InstClass::DsAtomicArith32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 4 = DS_DEC_U32
    {InstClass::DsAtomicArith32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 5 = DS_MIN_I32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 6 = DS_MAX_I32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 7 = DS_MIN_U32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 8 = DS_MAX_U32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 9 = DS_AND_B32
    {InstClass::DsAtomicLogic32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 10 = DS_OR_B32
    {InstClass::DsAtomicLogic32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 11 = DS_XOR_B32
    {InstClass::DsAtomicLogic32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 12 = DS_MSKOR_B32
    {InstClass::DsAtomicLogic32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 13 = DS_WRITE_B32
    {InstClass::DsIdxWr, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 14 = DS_WRITE2_B32
    {InstClass::DsIdxWr, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 15 = DS_WRITE2ST64_B32
    {InstClass::DsIdxWr, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 16 = DS_CMPST_B32
    {InstClass::DsAtomicCmpSt32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 17 = DS_CMPST_F32
    {InstClass::DsAtomicCmpSt32, InstCategory::DataShare, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 18 = DS_MIN_F32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 19 = DS_MAX_F32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 20 = DS_NOP
    {InstClass::DsDataShareMisc, InstCategory::DataShare, 3, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    {},
    {},
    {},
    // 24 = DS_GWS_SEMA_RELEASE_ALL
    {InstClass::GdsSync, InstCategory::DataShare, 3, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 25 = DS_GWS_INIT
    {InstClass::GdsSync, InstCategory::DataShare, 3, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 26 = DS_GWS_SEMA_V
    {InstClass::GdsSync, InstCategory::DataShare, 3, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 27 = DS_GWS_SEMA_BR
    {InstClass::GdsSync, InstCategory::DataShare, 3, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 28 = DS_GWS_SEMA_P
    {InstClass::GdsSync, InstCategory::DataShare, 3, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 29 = DS_GWS_BARRIER
    {InstClass::ScalarSync, InstCategory::FlowControl, 3, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 30 = DS_WRITE_B8
    {InstClass::DsIdxWr, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 31 = DS_WRITE_B16
    {InstClass::DsIdxWr, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 32 = DS_ADD_RTN_U32
    {InstClass::DsAtomicArith32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 33 = DS_SUB_RTN_U32
    {InstClass::DsAtomicArith32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 34 = DS_RSUB_RTN_U32
    {InstClass::DsAtomicArith32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 35 = DS_INC_RTN_U32
    {InstClass::DsAtomicArith32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 36 = DS_DEC_RTN_U32
    {InstClass::DsAtomicArith32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 37 = DS_MIN_RTN_I32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 38 = DS_MAX_RTN_I32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 39 = DS_MIN_RTN_U32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 40 = DS_MAX_RTN_U32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 41 = DS_AND_RTN_B32
    {InstClass::DsAtomicLogic32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 42 = DS_OR_RTN_B32
    {InstClass::DsAtomicLogic32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 43 = DS_XOR_RTN_B32
    {InstClass::DsAtomicLogic32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 44 = DS_MSKOR_RTN_B32
    {InstClass::DsAtomicLogic32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 45 = DS_WRXCHG_RTN_B32
    {InstClass::DsIdxWrXchg, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 46 = DS_WRXCHG2_RTN_B32
    {InstClass::DsIdxWrXchg, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 47 = DS_WRXCHG2ST64_RTN_B32
    {InstClass::DsIdxWrXchg, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 48 = DS_CMPST_RTN_B32
    {InstClass::DsAtomicCmpSt32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 49 = DS_CMPST_RTN_F32
    {InstClass::DsAtomicCmpSt32, InstCategory::DataShare, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 50 = DS_MIN_RTN_F32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 51 = DS_MAX_RTN_F32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 52 = DS_WRAP_RTN_B32
    {InstClass::DsIdxWrap, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 53 = DS_SWIZZLE_B32
    {InstClass::DsDataShareUt, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 54 = DS_READ_B32
    {InstClass::DsIdxRd, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 55 = DS_READ2_B32
    {InstClass::DsIdxRd, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 56 = DS_READ2ST64_B32
    {InstClass::DsIdxRd, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 57 = DS_READ_I8
    {InstClass::DsIdxRd, InstCategory::DataShare, 3, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 58 = DS_READ_U8
    {InstClass::DsIdxRd, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 59 = DS_READ_I16
    {InstClass::DsIdxRd, InstCategory::DataShare, 3, 1, ScalarType::Sint32, ScalarType::Sint32},
    // 60 = DS_READ_U16
    {InstClass::DsIdxRd, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 61 = DS_CONSUME
    {InstClass::DsAppendCon, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 62 = DS_APPEND
    {InstClass::DsAppendCon, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    // 63 = DS_ORDERED_COUNT
    {InstClass::GdsOrdCnt, InstCategory::DataShare, 3, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 64 = DS_ADD_U64
    {InstClass::DsAtomicArith64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 65 = DS_SUB_U64
    {InstClass::DsAtomicArith64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 66 = DS_RSUB_U64
    {InstClass::DsAtomicArith64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 67 = DS_INC_U64
    {InstClass::DsAtomicArith64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 68 = DS_DEC_U64
    {InstClass::DsAtomicArith64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 69 = DS_MIN_I64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 70 = DS_MAX_I64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 71 = DS_MIN_U64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 72 = DS_MAX_U64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 73 = DS_AND_B64
    {InstClass::DsAtomicLogic64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 74 = DS_OR_B64
    {InstClass::DsAtomicLogic64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 75 = DS_XOR_B64
    {InstClass::DsAtomicLogic64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 76 = DS_MSKOR_B64
    {InstClass::DsAtomicLogic64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 77 = DS_WRITE_B64
    {InstClass::DsIdxWr, InstCategory::DataShare, 3, 1, ScalarType::Uint64, ScalarType::Uint64},
    // 78 = DS_WRITE2_B64
    {InstClass::DsIdxWr, InstCategory::DataShare, 3, 1, ScalarType::Uint64, ScalarType::Uint64},
    // 79 = DS_WRITE2ST64_B64
    {InstClass::DsIdxWr, InstCategory::DataShare, 3, 1, ScalarType::Uint64, ScalarType::Uint64},
    // 80 = DS_CMPST_B64
    {InstClass::DsAtomicCmpSt64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 81 = DS_CMPST_F64
    {InstClass::DsAtomicCmpSt64, InstCategory::DataShare, 3, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 82 = DS_MIN_F64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 83 = DS_MAX_F64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Float64,
     ScalarType::Float64},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 96 = DS_ADD_RTN_U64
    {InstClass::DsAtomicArith64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 97 = DS_SUB_RTN_U64
    {InstClass::DsAtomicArith64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 98 = DS_RSUB_RTN_U64
    {InstClass::DsAtomicArith64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 99 = DS_INC_RTN_U64
    {InstClass::DsAtomicArith64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 100 = DS_DEC_RTN_U64
    {InstClass::DsAtomicArith64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 101 = DS_MIN_RTN_I64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 102 = DS_MAX_RTN_I64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 103 = DS_MIN_RTN_U64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 104 = DS_MAX_RTN_U64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 105 = DS_AND_RTN_B64
    {InstClass::DsAtomicLogic64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 106 = DS_OR_RTN_B64
    {InstClass::DsAtomicLogic64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 107 = DS_XOR_RTN_B64
    {InstClass::DsAtomicLogic64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 108 = DS_MSKOR_RTN_B64
    {InstClass::DsAtomicLogic64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 109 = DS_WRXCHG_RTN_B64
    {InstClass::DsIdxWrXchg, InstCategory::DataShare, 3, 1, ScalarType::Uint64, ScalarType::Uint64},
    // 110 = DS_WRXCHG2_RTN_B64
    {InstClass::DsIdxWrXchg, InstCategory::DataShare, 3, 1, ScalarType::Uint64, ScalarType::Uint64},
    // 111 = DS_WRXCHG2ST64_RTN_B64
    {InstClass::DsIdxWrXchg, InstCategory::DataShare, 3, 1, ScalarType::Uint64, ScalarType::Uint64},
    // 112 = DS_CMPST_RTN_B64
    {InstClass::DsAtomicCmpSt64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 113 = DS_CMPST_RTN_F64
    {InstClass::DsAtomicCmpSt64, InstCategory::DataShare, 3, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 114 = DS_MIN_RTN_F64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 115 = DS_MAX_RTN_F64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Float64,
     ScalarType::Float64},
    {},
    {},
    // 118 = DS_READ_B64
    {InstClass::DsIdxRd, InstCategory::DataShare, 3, 1, ScalarType::Uint64, ScalarType::Uint64},
    // 119 = DS_READ2_B64
    {InstClass::DsIdxRd, InstCategory::DataShare, 3, 1, ScalarType::Uint64, ScalarType::Uint64},
    // 120 = DS_READ2ST64_B64
    {InstClass::DsIdxRd, InstCategory::DataShare, 3, 1, ScalarType::Uint64, ScalarType::Uint64},
    {},
    {},
    {},
    {},
    {},
    // 126 = DS_CONDXCHG32_RTN_B64
    {InstClass::DsIdxCondXchg, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    {},
    // 128 = DS_ADD_SRC2_U32
    {InstClass::DsAtomicArith32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 129 = DS_SUB_SRC2_U32
    {InstClass::DsAtomicArith32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 130 = DS_RSUB_SRC2_U32
    {InstClass::DsAtomicArith32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 131 = DS_INC_SRC2_U32
    {InstClass::DsAtomicArith32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 132 = DS_DEC_SRC2_U32
    {InstClass::DsAtomicArith32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 133 = DS_MIN_SRC2_I32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 134 = DS_MAX_SRC2_I32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 135 = DS_MIN_SRC2_U32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 136 = DS_MAX_SRC2_U32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 137 = DS_AND_SRC2_B32
    {InstClass::DsAtomicLogic32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 138 = DS_OR_SRC2_B32
    {InstClass::DsAtomicLogic32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 139 = DS_XOR_SRC2_B32
    {InstClass::DsAtomicLogic32, InstCategory::DataShare, 3, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    {},
    // 141 = DS_WRITE_SRC2_B32
    {InstClass::DsIdxWr, InstCategory::DataShare, 3, 1, ScalarType::Uint32, ScalarType::Uint32},
    {},
    {},
    {},
    {},
    // 146 = DS_MIN_SRC2_F32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 147 = DS_MAX_SRC2_F32
    {InstClass::DsAtomicMinMax32, InstCategory::DataShare, 3, 1, ScalarType::Float32,
     ScalarType::Float32},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 192 = DS_ADD_SRC2_U64
    {InstClass::DsAtomicArith64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 193 = DS_SUB_SRC2_U64
    {InstClass::DsAtomicArith64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 194 = DS_RSUB_SRC2_U64
    {InstClass::DsAtomicArith64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 195 = DS_INC_SRC2_U64
    {InstClass::DsAtomicArith64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 196 = DS_DEC_SRC2_U64
    {InstClass::DsAtomicArith64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 197 = DS_MIN_SRC2_I64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 198 = DS_MAX_SRC2_I64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Sint64,
     ScalarType::Sint64},
    // 199 = DS_MIN_SRC2_U64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 200 = DS_MAX_SRC2_U64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 201 = DS_AND_SRC2_B64
    {InstClass::DsAtomicLogic64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 202 = DS_OR_SRC2_B64
    {InstClass::DsAtomicLogic64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    // 203 = DS_XOR_SRC2_B64
    {InstClass::DsAtomicLogic64, InstCategory::DataShare, 3, 1, ScalarType::Uint64,
     ScalarType::Uint64},
    {},
    // 205 = DS_WRITE_SRC2_B64
    {InstClass::DsIdxWr, InstCategory::DataShare, 3, 1, ScalarType::Uint64, ScalarType::Uint64},
    {},
    {},
    {},
    {},
    // 210 = DS_MIN_SRC2_F64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 211 = DS_MAX_SRC2_F64
    {InstClass::DsAtomicMinMax64, InstCategory::DataShare, 3, 1, ScalarType::Float64,
     ScalarType::Float64},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 222 = DS_WRITE_B96
    {InstClass::Undefined, InstCategory::Undefined, 3, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 223 = DS_WRITE_B128
    {InstClass::Undefined, InstCategory::Undefined, 3, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 253 = DS_CONDXCHG32_RTN_B128
    {InstClass::Undefined, InstCategory::Undefined, 3, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 254 = DS_READ_B96
    {InstClass::Undefined, InstCategory::Undefined, 3, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 255 = DS_READ_B128
    {InstClass::Undefined, InstCategory::Undefined, 3, 1, ScalarType::Undefined,
     ScalarType::Undefined},
}};

constexpr std::array<InstFormat, 114> InstructionFormatMUBUF = {{
    // 0 = BUFFER_LOAD_FORMAT_X
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 1 = BUFFER_LOAD_FORMAT_XY
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 2 = BUFFER_LOAD_FORMAT_XYZ
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 3 = BUFFER_LOAD_FORMAT_XYZW
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 4 = BUFFER_STORE_FORMAT_X
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 5 = BUFFER_STORE_FORMAT_XY
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 6 = BUFFER_STORE_FORMAT_XYZ
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 7 = BUFFER_STORE_FORMAT_XYZW
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 8 = BUFFER_LOAD_UBYTE
    {InstClass::VectorMemBufNoFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 9 = BUFFER_LOAD_SBYTE
    {InstClass::VectorMemBufNoFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 10 = BUFFER_LOAD_USHORT
    {InstClass::VectorMemBufNoFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 11 = BUFFER_LOAD_SSHORT
    {InstClass::VectorMemBufNoFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 12 = BUFFER_LOAD_DWORD
    {InstClass::VectorMemBufNoFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 13 = BUFFER_LOAD_DWORDX2
    {InstClass::VectorMemBufNoFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 14 = BUFFER_LOAD_DWORDX4
    {InstClass::VectorMemBufNoFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 15 = BUFFER_LOAD_DWORDX3
    {InstClass::VectorMemBufNoFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 24 = BUFFER_STORE_BYTE
    {InstClass::VectorMemBufNoFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    {},
    // 26 = BUFFER_STORE_SHORT
    {InstClass::VectorMemBufNoFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    {},
    // 28 = BUFFER_STORE_DWORD
    {InstClass::VectorMemBufNoFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 29 = BUFFER_STORE_DWORDX2
    {InstClass::VectorMemBufNoFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 30 = BUFFER_STORE_DWORDX4
    {InstClass::VectorMemBufNoFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 31 = BUFFER_STORE_DWORDX3
    {InstClass::VectorMemBufNoFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 48 = BUFFER_ATOMIC_SWAP
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 49 = BUFFER_ATOMIC_CMPSWAP
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 50 = BUFFER_ATOMIC_ADD
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 51 = BUFFER_ATOMIC_SUB
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    {},
    // 53 = BUFFER_ATOMIC_SMIN
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 54 = BUFFER_ATOMIC_UMIN
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 55 = BUFFER_ATOMIC_SMAX
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 56 = BUFFER_ATOMIC_UMAX
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 57 = BUFFER_ATOMIC_AND
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 58 = BUFFER_ATOMIC_OR
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 59 = BUFFER_ATOMIC_XOR
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 60 = BUFFER_ATOMIC_INC
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 61 = BUFFER_ATOMIC_DEC
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 62 = BUFFER_ATOMIC_FCMPSWAP
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 63 = BUFFER_ATOMIC_FMIN
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 64 = BUFFER_ATOMIC_FMAX
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 80 = BUFFER_ATOMIC_SWAP_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint64,
     ScalarType::Uint32},
    // 81 = BUFFER_ATOMIC_CMPSWAP_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint64,
     ScalarType::Uint32},
    // 82 = BUFFER_ATOMIC_ADD_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint64,
     ScalarType::Uint32},
    // 83 = BUFFER_ATOMIC_SUB_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint64,
     ScalarType::Uint32},
    {},
    // 85 = BUFFER_ATOMIC_SMIN_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint64,
     ScalarType::Uint32},
    // 86 = BUFFER_ATOMIC_UMIN_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint64,
     ScalarType::Uint32},
    // 87 = BUFFER_ATOMIC_SMAX_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint64,
     ScalarType::Uint32},
    // 88 = BUFFER_ATOMIC_UMAX_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint64,
     ScalarType::Uint32},
    // 89 = BUFFER_ATOMIC_AND_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint64,
     ScalarType::Uint32},
    // 90 = BUFFER_ATOMIC_OR_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint64,
     ScalarType::Uint32},
    // 91 = BUFFER_ATOMIC_XOR_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint64,
     ScalarType::Uint32},
    // 92 = BUFFER_ATOMIC_INC_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint64,
     ScalarType::Uint32},
    // 93 = BUFFER_ATOMIC_DEC_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Uint64,
     ScalarType::Uint32},
    // 94 = BUFFER_ATOMIC_FCMPSWAP_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 95 = BUFFER_ATOMIC_FMIN_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Float64,
     ScalarType::Float64},
    // 96 = BUFFER_ATOMIC_FMAX_X2
    {InstClass::VectorMemBufAtomic, InstCategory::VectorMemory, 4, 1, ScalarType::Float64,
     ScalarType::Float64},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 112 = BUFFER_WBINVL1_SC
    {InstClass::VectorMemL1Cache, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 113 = BUFFER_WBINVL1
    {InstClass::VectorMemL1Cache, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
}};

constexpr std::array<InstFormat, 8> InstructionFormatMTBUF = {{
    // 0 = TBUFFER_LOAD_FORMAT_X
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 1 = TBUFFER_LOAD_FORMAT_XY
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 2 = TBUFFER_LOAD_FORMAT_XYZ
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 3 = TBUFFER_LOAD_FORMAT_XYZW
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 4 = TBUFFER_STORE_FORMAT_X
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 5 = TBUFFER_STORE_FORMAT_XY
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 6 = TBUFFER_STORE_FORMAT_XYZ
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 7 = TBUFFER_STORE_FORMAT_XYZW
    {InstClass::VectorMemBufFmt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
}};

constexpr std::array<InstFormat, 112> InstructionFormatMIMG = {{
    // 0 = IMAGE_LOAD
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 1 = IMAGE_LOAD_MIP
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 2 = IMAGE_LOAD_PCK
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 3 = IMAGE_LOAD_PCK_SGN
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 4 = IMAGE_LOAD_MIP_PCK
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 5 = IMAGE_LOAD_MIP_PCK_SGN
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    {},
    {},
    // 8 = IMAGE_STORE
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 9 = IMAGE_STORE_MIP
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 10 = IMAGE_STORE_PCK
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 11 = IMAGE_STORE_MIP_PCK
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    {},
    {},
    // 14 = IMAGE_GET_RESINFO
    {InstClass::VectorMemImgUt, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 15 = IMAGE_ATOMIC_SWAP
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 16 = IMAGE_ATOMIC_CMPSWAP
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 17 = IMAGE_ATOMIC_ADD
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 18 = IMAGE_ATOMIC_SUB
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    {},
    // 20 = IMAGE_ATOMIC_SMIN
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 21 = IMAGE_ATOMIC_UMIN
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 22 = IMAGE_ATOMIC_SMAX
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Sint32,
     ScalarType::Sint32},
    // 23 = IMAGE_ATOMIC_UMAX
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 24 = IMAGE_ATOMIC_AND
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 25 = IMAGE_ATOMIC_OR
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 26 = IMAGE_ATOMIC_XOR
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 27 = IMAGE_ATOMIC_INC
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 28 = IMAGE_ATOMIC_DEC
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 29 = IMAGE_ATOMIC_FCMPSWAP
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 30 = IMAGE_ATOMIC_FMIN
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 31 = IMAGE_ATOMIC_FMAX
    {InstClass::VectorMemImgNoSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 32 = IMAGE_SAMPLE
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 33 = IMAGE_SAMPLE_CL
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 34 = IMAGE_SAMPLE_D
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 35 = IMAGE_SAMPLE_D_CL
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 36 = IMAGE_SAMPLE_L
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 37 = IMAGE_SAMPLE_B
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 38 = IMAGE_SAMPLE_B_CL
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 39 = IMAGE_SAMPLE_LZ
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 40 = IMAGE_SAMPLE_C
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 41 = IMAGE_SAMPLE_C_CL
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 42 = IMAGE_SAMPLE_C_D
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 43 = IMAGE_SAMPLE_C_D_CL
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 44 = IMAGE_SAMPLE_C_L
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 45 = IMAGE_SAMPLE_C_B
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 46 = IMAGE_SAMPLE_C_B_CL
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 47 = IMAGE_SAMPLE_C_LZ
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 48 = IMAGE_SAMPLE_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 49 = IMAGE_SAMPLE_CL_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 50 = IMAGE_SAMPLE_D_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 51 = IMAGE_SAMPLE_D_CL_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 52 = IMAGE_SAMPLE_L_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 53 = IMAGE_SAMPLE_B_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 54 = IMAGE_SAMPLE_B_CL_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 55 = IMAGE_SAMPLE_LZ_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 56 = IMAGE_SAMPLE_C_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 57 = IMAGE_SAMPLE_C_CL_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 58 = IMAGE_SAMPLE_C_D_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 59 = IMAGE_SAMPLE_C_D_CL_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 60 = IMAGE_SAMPLE_C_L_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 61 = IMAGE_SAMPLE_C_B_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 62 = IMAGE_SAMPLE_C_B_CL_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 63 = IMAGE_SAMPLE_C_LZ_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 64 = IMAGE_GATHER4
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 65 = IMAGE_GATHER4_CL
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    {},
    {},
    // 68 = IMAGE_GATHER4_L
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 69 = IMAGE_GATHER4_B
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 70 = IMAGE_GATHER4_B_CL
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 71 = IMAGE_GATHER4_LZ
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 72 = IMAGE_GATHER4_C
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 73 = IMAGE_GATHER4_C_CL
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    {},
    {},
    // 76 = IMAGE_GATHER4_C_L
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 77 = IMAGE_GATHER4_C_B
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 78 = IMAGE_GATHER4_C_B_CL
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 79 = IMAGE_GATHER4_C_LZ
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Uint32},
    // 80 = IMAGE_GATHER4_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 81 = IMAGE_GATHER4_CL_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    {},
    {},
    // 84 = IMAGE_GATHER4_L_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 85 = IMAGE_GATHER4_B_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 86 = IMAGE_GATHER4_B_CL_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 87 = IMAGE_GATHER4_LZ_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 88 = IMAGE_GATHER4_C_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 89 = IMAGE_GATHER4_C_CL_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    {},
    {},
    // 92 = IMAGE_GATHER4_C_L_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 93 = IMAGE_GATHER4_C_B_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 94 = IMAGE_GATHER4_C_B_CL_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 95 = IMAGE_GATHER4_C_LZ_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Uint32,
     ScalarType::Float32},
    // 96 = IMAGE_GET_LOD
    {InstClass::VectorMemImgUt, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    // 104 = IMAGE_SAMPLE_CD
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Float32,
     ScalarType::Float32},
    // 105 = IMAGE_SAMPLE_CD_CL
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 106 = IMAGE_SAMPLE_C_CD
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 107 = IMAGE_SAMPLE_C_CD_CL
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 108 = IMAGE_SAMPLE_CD_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 109 = IMAGE_SAMPLE_CD_CL_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 110 = IMAGE_SAMPLE_C_CD_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
    // 111 = IMAGE_SAMPLE_C_CD_CL_O
    {InstClass::VectorMemImgSmp, InstCategory::VectorMemory, 4, 1, ScalarType::Undefined,
     ScalarType::Undefined},
}};

constexpr std::array<InstFormat, 1> InstructionFormatEXP = {{
    {InstClass::Exp, InstCategory::Export, 4, 1, ScalarType::Float32, ScalarType::Any},
}};

InstFormat InstructionFormat(InstEncoding encoding, uint32_t opcode) {
    switch (encoding) {
    case InstEncoding::SOP1:
        return InstructionFormatSOP1[opcode];
    case InstEncoding::SOPP:
        return InstructionFormatSOPP[opcode];
    case InstEncoding::SOPC:
        return InstructionFormatSOPC[opcode];
    case InstEncoding::VOP1:
        return InstructionFormatVOP1[opcode];
    case InstEncoding::VOPC:
        return InstructionFormatVOPC[opcode];
    case InstEncoding::VOP3:
        return InstructionFormatVOP3[opcode];
    case InstEncoding::EXP:
        return InstructionFormatEXP[opcode];
    case InstEncoding::VINTRP:
        return InstructionFormatVINTRP[opcode];
    case InstEncoding::DS:
        return InstructionFormatDS[opcode];
    case InstEncoding::MUBUF:
        return InstructionFormatMUBUF[opcode];
    case InstEncoding::MTBUF:
        return InstructionFormatMTBUF[opcode];
    case InstEncoding::MIMG:
        return InstructionFormatMIMG[opcode];
    case InstEncoding::SMRD:
        return InstructionFormatSMRD[opcode];
    case InstEncoding::SOPK:
        return InstructionFormatSOPK[opcode];
    case InstEncoding::SOP2:
        return InstructionFormatSOP2[opcode];
    case InstEncoding::VOP2:
        return InstructionFormatVOP2[opcode];
    default:
        UNREACHABLE();
    }
    return {};
}

} // namespace Shader::Gcn
