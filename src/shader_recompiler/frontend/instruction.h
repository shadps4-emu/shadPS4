// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
    bool abs = false;
};

/// These are applied before storing an operand register.
struct OutputModifiers {
    bool clamp = false;
    float multiplier = 0.f;
};

struct InstOperand {
    OperandField field = OperandField::Undefined;
    ScalarType type = ScalarType::Undefined;
    InputModifiers input_modifier = {};
    OutputModifiers output_modifier = {};
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
    u64 : 47;
    u64 omod : 2;
    u64 neg : 3;
};

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
    InstControlSMRD smrd;
    InstControlMUBUF mubuf;
    InstControlMTBUF mtbuf;
    InstControlMIMG mimg;
    InstControlDS ds;
    InstControlVINTRP vintrp;
    InstControlEXP exp;
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
