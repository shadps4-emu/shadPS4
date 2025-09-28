// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {

template <typename... Args>
Id EmitCompositeConstruct(EmitContext& ctx, IR::Inst* inst, Args&&... args) {
    return inst->AreAllArgsImmediates() ? ctx.ConstantComposite(args...)
                                        : ctx.OpCompositeConstruct(args...);
}

Id EmitCompositeConstructU32x2(EmitContext& ctx, IR::Inst* inst, Id e1, Id e2) {
    return EmitCompositeConstruct(ctx, inst, ctx.U32[2], e1, e2);
}

Id EmitCompositeConstructU32x3(EmitContext& ctx, IR::Inst* inst, Id e1, Id e2, Id e3) {
    return EmitCompositeConstruct(ctx, inst, ctx.U32[3], e1, e2, e3);
}

Id EmitCompositeConstructU32x4(EmitContext& ctx, IR::Inst* inst, Id e1, Id e2, Id e3, Id e4) {
    return EmitCompositeConstruct(ctx, inst, ctx.U32[4], e1, e2, e3, e4);
}

Id EmitCompositeConstructU32x2x2(EmitContext& ctx, IR::Inst* inst, Id e1, Id e2) {
    return EmitCompositeConstruct(ctx, inst, ctx.U32[4], e1, e2);
}

Id EmitCompositeExtractU32x2(EmitContext& ctx, Id composite, u32 index) {
    return ctx.OpCompositeExtract(ctx.U32[1], composite, index);
}

Id EmitCompositeExtractU32x3(EmitContext& ctx, Id composite, u32 index) {
    return ctx.OpCompositeExtract(ctx.U32[1], composite, index);
}

Id EmitCompositeExtractU32x4(EmitContext& ctx, Id composite, u32 index) {
    return ctx.OpCompositeExtract(ctx.U32[1], composite, index);
}

Id EmitCompositeInsertU32x2(EmitContext& ctx, Id composite, Id object, u32 index) {
    return ctx.OpCompositeInsert(ctx.U32[2], object, composite, index);
}

Id EmitCompositeInsertU32x3(EmitContext& ctx, Id composite, Id object, u32 index) {
    return ctx.OpCompositeInsert(ctx.U32[3], object, composite, index);
}

Id EmitCompositeInsertU32x4(EmitContext& ctx, Id composite, Id object, u32 index) {
    return ctx.OpCompositeInsert(ctx.U32[4], object, composite, index);
}

Id EmitCompositeShuffleU32x2(EmitContext& ctx, Id composite1, Id composite2, u32 comp0, u32 comp1) {
    return ctx.OpVectorShuffle(ctx.U32[2], composite1, composite2, comp0, comp1);
}

Id EmitCompositeShuffleU32x3(EmitContext& ctx, Id composite1, Id composite2, u32 comp0, u32 comp1,
                             u32 comp2) {
    return ctx.OpVectorShuffle(ctx.U32[3], composite1, composite2, comp0, comp1, comp2);
}

Id EmitCompositeShuffleU32x4(EmitContext& ctx, Id composite1, Id composite2, u32 comp0, u32 comp1,
                             u32 comp2, u32 comp3) {
    return ctx.OpVectorShuffle(ctx.U32[4], composite1, composite2, comp0, comp1, comp2, comp3);
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

} // namespace Shader::Backend::SPIRV
