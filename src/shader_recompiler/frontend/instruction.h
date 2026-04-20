// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "shader_recompiler/frontend/opcodes.h"

namespace Shader::Gcn {

constexpr u32 GcnMaxSrcCount = 4;
constexpr u32 GcnMaxDstCount = 2;

enum OperandFieldRange {
    ScalarGPRMin = 0,
    ScalarGPRMax = 103,
    SignedConstIntPosMin = 129,
    SignedConstIntPosMax = 192,
    SignedConstIntNegMin = 193,
    SignedConstIntNegMax = 208,
    ConstFloatMin = 240,
    VectorGPRMin = 256,
    VectorGPRMax = 511
};

/// These are applied after loading an operand register.
struct InputModifiers {
    bool neg = false;
    bool neg_hi = false;
    bool abs = false;
    bool sext = false;
};

/// These are applied before storing an operand register.
struct OutputModifiers {
    bool clamp = false;
    float multiplier = 0.f;
};

struct OperandSelection {
    bool op_sel = false;
    bool op_sel_hi = false;
};

enum class SdwaSelector : u32 {
    Byte0 = 0,
    Byte1 = 1,
    Byte2 = 2,
    Byte3 = 3,
    Word0 = 4,
    Word1 = 5,
    Dword = 6,
    Invalid = 7,
};

enum class SdwaDstUnused : u32 {
    Pad = 0,
    Sext = 1,
    Preserve = 2,
    Invalid = 3,
};

struct InstOperand {
    OperandField field = OperandField::Undefined;
    ScalarType type = ScalarType::Undefined;
    InputModifiers input_modifier = {};
    OutputModifiers output_modifier = {};
    // only valid for packed 16bit operations
    OperandSelection op_sel = {};
    SdwaDstUnused sdwa_dst = SdwaDstUnused::Invalid;
    SdwaSelector sdwa_sel = SdwaSelector::Invalid;
    u32 code = 0xFFFFFFFF;
};

struct Operand {
    OperandField field = OperandField::Undefined;
    ScalarType type = ScalarType::Undefined;
    union {
        InputModifiers input_modifier = {};
        OutputModifiers output_modifier;
    };
    u32 code = 0xFFFFFFFF;
};

struct InstSOPK {
    u16 simm;
};

struct InstSOPP {
    u16 simm;
};

struct InstVOP3 {
    Operand vdst;
    Operand src0;
    Operand src1;
    Operand src2;
};

struct SMRD {
    u8 offset;
    bool imm;
    u8 sbase;
};

struct InstControlSOPK {
    s16 simm;
};

struct InstControlSOPP {
    s16 simm;
};

struct InstControlVOP3 {
    u64 : 8;
    u64 abs : 3;
    u64 clmp : 1;
    u64 op_sel : 4;
    u64 : 43;
    u64 omod : 2;
    u64 neg : 3;
};

struct InstControlVOP3P {
    u64 : 8;
    u64 neg_hi : 3;
    u64 op_sel : 3;
    u64 op_sel_hi_2 : 1;
    u64 clamp : 1;
    u64 : 43;
    u64 op_sel_hi_01 : 2;
    u64 neg : 3;

    bool get_op_sel_hi(int idx) const {
        switch (idx) {
        case 0:
            return (op_sel_hi_01 & 1) == 1;
        case 1:
            return ((op_sel_hi_01 >> 1) & 1) == 1;
        case 2:
            return (op_sel_hi_2 & 1) == 1;
        default:
            UNREACHABLE_MSG("get_op_sel_hi: {}", idx);
        }
    }
};

static_assert(sizeof(InstControlVOP3P) == 8);

struct InstControlSMRD {
    u32 offset : 8;
    u32 imm : 1;
    u32 count : 5;
    u32 : 18;
};

struct InstControlMUBUF {
    u64 offset : 12;
    u64 offen : 1;
    u64 idxen : 1;
    u64 glc : 1;
    u64 : 1;
    u64 lds : 1;
    u64 : 37;
    u64 slc : 1;
    u64 tfe : 1;
    u64 count : 3;
    u64 size : 5;
};

struct InstControlMTBUF {
    u64 offset : 12;
    u64 offen : 1;
    u64 idxen : 1;
    u64 glc : 1;
    u64 : 4;
    u64 dfmt : 4;
    u64 nfmt : 3;
    u64 : 28;
    u64 slc : 1;
    u64 tfe : 1;
    u64 count : 3;
    u64 size : 5;
};

struct InstControlMIMG {
    u64 : 8;
    u64 dmask : 4;
    u64 unrm : 1;
    u64 glc : 1;
    u64 da : 1;
    u64 r128 : 1;
    u64 tfe : 1;
    u64 lwe : 1;
    u64 : 7;
    u64 slc : 1;
    u64 mod : 32;
    u64 : 6;
};

struct InstControlDS {
    u64 offset0 : 8;
    u64 offset1 : 8;
    u64 : 1;
    u64 gds : 1;
    u64 dual : 1;
    u64 sign : 1;
    u64 relative : 1;
    u64 stride : 1;
    u64 size : 4;
    u64 : 38;
};

struct InstControlVINTRP {
    u32 : 8;
    u32 chan : 2;
    u32 attr : 6;
    u32 : 16;
};

struct InstControlEXP {
    u64 en : 4;
    u64 target : 6;
    u64 compr : 1;
    u64 done : 1;
    u64 vm : 1;
    u64 reserved : 51;
};

union InstControl {
    InstControlSOPK sopk;
    InstControlSOPP sopp;
    InstControlVOP3 vop3;
    InstControlVOP3P vop3p;
    InstControlSMRD smrd;
    InstControlMUBUF mubuf;
    InstControlMTBUF mtbuf;
    InstControlMIMG mimg;
    InstControlDS ds;
    InstControlVINTRP vintrp;
    InstControlEXP exp;
};

struct SdwaVop12 {
    u32 src0 : 8;
    u32 dst_sel : 3;
    u32 dst_u : 2;
    u32 clamp : 1;
    u32 omod : 2;
    u32 src0_sel : 3;
    u32 src0_sext : 1;
    u32 src0_neg : 1;
    u32 src0_abs : 1;
    u32 : 1;
    u32 s0 : 1;

    u32 src1_sel : 3;
    u32 src1_sext : 1;
    u32 src1_neg : 1;
    u32 src1_abs : 1;
    u32 : 1;
    u32 s1 : 1;
};

struct SdwaVopc {
    u32 src0 : 8;
    u32 sdst : 7;
    u32 sd : 1;
    u32 src0_sel : 3;
    u32 src0_sext : 1;
    u32 src0_neg : 1;
    u32 src0_abs : 1;
    u32 : 1;
    u32 s0 : 1;

    u32 src1_sel : 3;
    u32 src1_sext : 1;
    u32 src1_neg : 1;
    u32 src1_abs : 1;
    u32 : 1;
    u32 s1 : 1;
};

union Sdwa {
    SdwaVopc vopc;
    SdwaVop12 vop12;
};

struct GcnInst {
    Opcode opcode;
    InstEncoding encoding;
    InstClass inst_class;
    InstCategory category;
    InstControl control;
    u32 length;
    u32 src_count;
    u32 dst_count;
    std::array<InstOperand, GcnMaxSrcCount> src;
    std::array<InstOperand, GcnMaxDstCount> dst;

    u32 BranchTarget(u32 pc) const;

    bool IsTerminateInstruction() const;
    bool IsUnconditionalBranch() const;
    bool IsConditionalBranch() const;
    bool IsFork() const;
    bool IsCmpx() const;
};

} // namespace Shader::Gcn
