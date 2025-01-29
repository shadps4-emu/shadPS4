// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {

Id EmitBitCastU16F16(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.U16, value);
}

Id EmitBitCastU32F32(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.U32[1], value);
}

Id EmitBitCastU64F64(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.U64, value);
}

Id EmitBitCastF16U16(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.F16[1], value);
}

Id EmitBitCastF32U32(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.F32[1], value);
}

void EmitBitCastF64U64(EmitContext&) {
    UNREACHABLE_MSG("SPIR-V Instruction");
}

Id EmitPackUint2x32(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.U64, value);
}

Id EmitUnpackUint2x32(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.U32[2], value);
}

Id EmitPackFloat2x32(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.F64[1], value);
}

Id EmitPackFloat2x16(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.U32[1], value);
}

Id EmitUnpackFloat2x16(EmitContext& ctx, Id value) {
    return ctx.OpBitcast(ctx.F16[2], value);
}

Id EmitPackHalf2x16(EmitContext& ctx, Id value) {
    return ctx.OpPackHalf2x16(ctx.U32[1], value);
}

Id EmitUnpackHalf2x16(EmitContext& ctx, Id value) {
    return ctx.OpUnpackHalf2x16(ctx.F32[2], value);
}

Id EmitPackUnorm2x16(EmitContext& ctx, Id value) {
    return ctx.OpPackUnorm2x16(ctx.U32[1], value);
}

Id EmitUnpackUnorm2x16(EmitContext& ctx, Id value) {
    return ctx.OpUnpackUnorm2x16(ctx.F32[2], value);
}

Id EmitPackSnorm2x16(EmitContext& ctx, Id value) {
    return ctx.OpPackSnorm2x16(ctx.U32[1], value);
}

Id EmitUnpackSnorm2x16(EmitContext& ctx, Id value) {
    return ctx.OpUnpackSnorm2x16(ctx.F32[2], value);
}

Id EmitPackUint2x16(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto x{ctx.OpCompositeExtract(ctx.U32[1], value, 0)};
    const auto y{ctx.OpCompositeExtract(ctx.U32[1], value, 1)};
    return ctx.OpBitFieldInsert(ctx.U32[1], x, y, ctx.ConstU32(16U), ctx.ConstU32(16U));
}

Id EmitUnpackUint2x16(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto x{ctx.OpBitFieldUExtract(ctx.U32[1], value, ctx.ConstU32(0U), ctx.ConstU32(16U))};
    const auto y{ctx.OpBitFieldUExtract(ctx.U32[1], value, ctx.ConstU32(16U), ctx.ConstU32(16U))};
    return ctx.OpCompositeConstruct(ctx.U32[2], x, y);
}

Id EmitPackSint2x16(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto x{ctx.OpCompositeExtract(ctx.U32[1], value, 0)};
    const auto y{ctx.OpCompositeExtract(ctx.U32[1], value, 1)};
    return ctx.OpBitFieldInsert(ctx.U32[1], x, y, ctx.ConstU32(16U), ctx.ConstU32(16U));
}

Id EmitUnpackSint2x16(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto x{ctx.OpBitFieldSExtract(ctx.U32[1], value, ctx.ConstU32(0U), ctx.ConstU32(16U))};
    const auto y{ctx.OpBitFieldSExtract(ctx.U32[1], value, ctx.ConstU32(16U), ctx.ConstU32(16U))};
    return ctx.OpCompositeConstruct(ctx.U32[2], x, y);
}

} // namespace Shader::Backend::SPIRV
