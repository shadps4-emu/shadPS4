// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/asm_x64/x64_utils.h"
#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"

namespace Shader::Backend::X64 {

namespace {

template <u32t N>
static const Operand& GetSuffleOperand(const Operands& comp1, const Operands& comp2, u32 index) {
    if (index < N) {
        return comp1[index];
    } else {
        return comp2[index - N];
    }
}
}

void EmitCompositeConstructU32x2(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2) {
    MovGP(ctx, dest[0], src1[0]);
    MovGP(ctx, dest[1], src2[0]);
}

void EmitCompositeConstructU32x3(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2, const Operands& src3) {
    MovGP(ctx, dest[0], src1[0]);
    MovGP(ctx, dest[1], src2[0]);
    MovGP(ctx, dest[2], src3[0]);
}

void EmitCompositeConstructU32x4(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2, const Operands& src3, const Operands& src4) {
    MovGP(ctx, dest[0], src1[0]);
    MovGP(ctx, dest[1], src2[0]);
    MovGP(ctx, dest[2], src3[0]);
    MovGP(ctx, dest[3], src4[0]);
}

void EmitCompositeConstructU32x2x2(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2) {
    MovGP(ctx, dest[0], src1[0]);
    MovGP(ctx, dest[1], src2[0]);
    MovGP(ctx, dest[2], src1[1]);
    MovGP(ctx, dest[3], src2[1]);
}

void EmitCompositeExtractU32x2(EmitContext& ctx, const Operands& dest, const Operands& composite, u32 index) {
    MovGP(ctx, dest[0], composite[index]);
}

void EmitCompositeExtractU32x3(EmitContext& ctx, const Operands& dest, const Operands& composite, u32 index) {
    MovGP(ctx, dest[0], composite[index]);
}

void EmitCompositeExtractU32x4(EmitContext& ctx, const Operands& dest, const Operands& composite, u32 index) {
    MovGP(ctx, dest[0], composite[index]);
}

void EmitCompositeInsertU32x2(EmitContext& ctx, const Operands& dest, const Operands& object, u32 index) {
    MovGP(ctx, dest[index], object[0]);
}

void EmitCompositeInsertU32x3(EmitContext& ctx, const Operands& dest, const Operands& object, u32 index) {
    MovGP(ctx, dest[index], object[0]);
}

void EmitCompositeInsertU32x4(EmitContext& ctx, const Operands& dest, const Operands& object, u32 index) {
    MovGP(ctx, dest[index], object[0]);
}

void EmitCompositeShuffleU32x2(EmitContext& ctx, const Operands& dest, const Operands& composite1, const Operands& composite2, u32 idx1, u32 idx2) {
    MovGP(ctx, dest[0], GetSuffleOperand<2>(composite1, composite2, idx1));
    MovGP(ctx, dest[1], GetSuffleOperand<2>(composite1, composite2, idx2));
}

void EmitCompositeShuffleU32x3(EmitContext& ctx, const Operands& dest, const Operands& composite1, const Operands& composite2, u32 idx1, u32 idx2, u32 idx3) {
    MovGP(ctx, dest[0], GetSuffleOperand<3>(composite1, composite2, idx1));
    MovGP(ctx, dest[1], GetSuffleOperand<3>(composite1, composite2, idx2));
    MovGP(ctx, dest[2], GetSuffleOperand<3>(composite1, composite2, idx3));
}

void EmitCompositeShuffleU32x4(EmitContext& ctx, const Operands& dest, const Operands& composite1, const Operands& composite2, u32 idx1, u32 idx2, u32 idx3, u32 idx4) {
    MovGP(ctx, dest[0], GetSuffleOperand<4>(composite1, composite2, idx1));
    MovGP(ctx, dest[1], GetSuffleOperand<4>(composite1, composite2, idx2));
    MovGP(ctx, dest[2], GetSuffleOperand<4>(composite1, composite2, idx3));
    MovGP(ctx, dest[3], GetSuffleOperand<4>(composite1, composite2, idx4));
}

Id EmitCompositeConstructF16x2(EmitContext& ctx, IR::Inst* inst, Id e1, Id e2) {
    return EmitCompositeConstruct(ctx, inst, ctx.F16[2], e1, e2);
}

Id EmitCompositeConstructF16x3(EmitContext& ctx, IR::Inst* inst, Id e1, Id e2, Id e3) {
    return EmitCompositeConstruct(ctx, inst, ctx.F16[3], e1, e2, e3);
}

Id EmitCompositeConstructF16x4(EmitContext& ctx, IR::Inst* inst, Id e1, Id e2, Id e3, Id e4) {
    return EmitCompositeConstruct(ctx, inst, ctx.F16[4], e1, e2, e3, e4);
}

Id EmitCompositeExtractF16x2(EmitContext& ctx, Id composite, u32 index) {
    return ctx.OpCompositeExtract(ctx.F16[1], composite, index);
}

Id EmitCompositeExtractF16x3(EmitContext& ctx, Id composite, u32 index) {
    return ctx.OpCompositeExtract(ctx.F16[1], composite, index);
}

Id EmitCompositeExtractF16x4(EmitContext& ctx, Id composite, u32 index) {
    return ctx.OpCompositeExtract(ctx.F16[1], composite, index);
}

Id EmitCompositeInsertF16x2(EmitContext& ctx, Id composite, Id object, u32 index) {
    return ctx.OpCompositeInsert(ctx.F16[2], object, composite, index);
}

Id EmitCompositeInsertF16x3(EmitContext& ctx, Id composite, Id object, u32 index) {
    return ctx.OpCompositeInsert(ctx.F16[3], object, composite, index);
}

Id EmitCompositeInsertF16x4(EmitContext& ctx, Id composite, Id object, u32 index) {
    return ctx.OpCompositeInsert(ctx.F16[4], object, composite, index);
}

Id EmitCompositeShuffleF16x2(EmitContext& ctx, Id composite1, Id composite2, u32 comp0, u32 comp1) {
    return ctx.OpVectorShuffle(ctx.F16[2], composite1, composite2, comp0, comp1);
}

Id EmitCompositeShuffleF16x3(EmitContext& ctx, Id composite1, Id composite2, u32 comp0, u32 comp1,
                             u32 comp2) {
    return ctx.OpVectorShuffle(ctx.F16[3], composite1, composite2, comp0, comp1, comp2);
}

Id EmitCompositeShuffleF16x4(EmitContext& ctx, Id composite1, Id composite2, u32 comp0, u32 comp1,
                             u32 comp2, u32 comp3) {
    return ctx.OpVectorShuffle(ctx.F16[4], composite1, composite2, comp0, comp1, comp2, comp3);
}

Id EmitCompositeConstructF32x2(EmitContext& ctx, IR::Inst* inst, Id e1, Id e2) {
    return EmitCompositeConstruct(ctx, inst, ctx.F32[2], e1, e2);
}

Id EmitCompositeConstructF32x3(EmitContext& ctx, IR::Inst* inst, Id e1, Id e2, Id e3) {
    return EmitCompositeConstruct(ctx, inst, ctx.F32[3], e1, e2, e3);
}

Id EmitCompositeConstructF32x4(EmitContext& ctx, IR::Inst* inst, Id e1, Id e2, Id e3, Id e4) {
    return EmitCompositeConstruct(ctx, inst, ctx.F32[4], e1, e2, e3, e4);
}

Id EmitCompositeConstructF32x2x2(EmitContext& ctx, IR::Inst* inst, Id e1, Id e2) {
    return EmitCompositeConstruct(ctx, inst, ctx.F32[4], e1, e2);
}

Id EmitCompositeExtractF32x2(EmitContext& ctx, Id composite, u32 index) {
    return ctx.OpCompositeExtract(ctx.F32[1], composite, index);
}

Id EmitCompositeExtractF32x3(EmitContext& ctx, Id composite, u32 index) {
    return ctx.OpCompositeExtract(ctx.F32[1], composite, index);
}

Id EmitCompositeExtractF32x4(EmitContext& ctx, Id composite, u32 index) {
    return ctx.OpCompositeExtract(ctx.F32[1], composite, index);
}

Id EmitCompositeInsertF32x2(EmitContext& ctx, Id composite, Id object, u32 index) {
    return ctx.OpCompositeInsert(ctx.F32[2], object, composite, index);
}

Id EmitCompositeInsertF32x3(EmitContext& ctx, Id composite, Id object, u32 index) {
    return ctx.OpCompositeInsert(ctx.F32[3], object, composite, index);
}

Id EmitCompositeInsertF32x4(EmitContext& ctx, Id composite, Id object, u32 index) {
    return ctx.OpCompositeInsert(ctx.F32[4], object, composite, index);
}

Id EmitCompositeShuffleF32x2(EmitContext& ctx, Id composite1, Id composite2, u32 comp0, u32 comp1) {
    return ctx.OpVectorShuffle(ctx.F32[2], composite1, composite2, comp0, comp1);
}

Id EmitCompositeShuffleF32x3(EmitContext& ctx, Id composite1, Id composite2, u32 comp0, u32 comp1,
                             u32 comp2) {
    return ctx.OpVectorShuffle(ctx.F32[3], composite1, composite2, comp0, comp1, comp2);
}

Id EmitCompositeShuffleF32x4(EmitContext& ctx, Id composite1, Id composite2, u32 comp0, u32 comp1,
                             u32 comp2, u32 comp3) {
    return ctx.OpVectorShuffle(ctx.F32[4], composite1, composite2, comp0, comp1, comp2, comp3);
}

void EmitCompositeConstructF64x2(EmitContext&) {
    UNREACHABLE_MSG("SPIR-V Instruction");
}

void EmitCompositeConstructF64x3(EmitContext&) {
    UNREACHABLE_MSG("SPIR-V Instruction");
}

void EmitCompositeConstructF64x4(EmitContext&) {
    UNREACHABLE_MSG("SPIR-V Instruction");
}

void EmitCompositeExtractF64x2(EmitContext&) {
    UNREACHABLE_MSG("SPIR-V Instruction");
}

void EmitCompositeExtractF64x3(EmitContext&) {
    UNREACHABLE_MSG("SPIR-V Instruction");
}

void EmitCompositeExtractF64x4(EmitContext&) {
    UNREACHABLE_MSG("SPIR-V Instruction");
}

Id EmitCompositeInsertF64x2(EmitContext& ctx, Id composite, Id object, u32 index) {
    return ctx.OpCompositeInsert(ctx.F64[2], object, composite, index);
}

Id EmitCompositeInsertF64x3(EmitContext& ctx, Id composite, Id object, u32 index) {
    return ctx.OpCompositeInsert(ctx.F64[3], object, composite, index);
}

Id EmitCompositeInsertF64x4(EmitContext& ctx, Id composite, Id object, u32 index) {
    return ctx.OpCompositeInsert(ctx.F64[4], object, composite, index);
}

Id EmitCompositeShuffleF64x2(EmitContext& ctx, Id composite1, Id composite2, u32 comp0, u32 comp1) {
    return ctx.OpVectorShuffle(ctx.F64[2], composite1, composite2, comp0, comp1);
}

Id EmitCompositeShuffleF64x3(EmitContext& ctx, Id composite1, Id composite2, u32 comp0, u32 comp1,
                             u32 comp2) {
    return ctx.OpVectorShuffle(ctx.F64[3], composite1, composite2, comp0, comp1, comp2);
}

Id EmitCompositeShuffleF64x4(EmitContext& ctx, Id composite1, Id composite2, u32 comp0, u32 comp1,
                             u32 comp2, u32 comp3) {
    return ctx.OpVectorShuffle(ctx.F64[4], composite1, composite2, comp0, comp1, comp2, comp3);
}

}