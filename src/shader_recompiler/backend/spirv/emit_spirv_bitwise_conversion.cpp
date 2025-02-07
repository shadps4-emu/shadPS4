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
    const auto unpacked{ctx.OpBitcast(ctx.U32[2], value)};
    const auto x{ctx.OpCompositeExtract(ctx.U32[1], unpacked, 0)};
    const auto y{ctx.OpCompositeExtract(ctx.U32[1], unpacked, 1)};

    const auto packed{ctx.OpBitFieldInsert(ctx.U32[1], x, y, ctx.ConstU32(16U), ctx.ConstU32(16U))};
    return packed;
}

Id EmitUnpackUint2x16(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto x{ctx.OpBitFieldUExtract(ctx.U32[1], value, ctx.ConstU32(0U), ctx.ConstU32(16U))};
    const auto y{ctx.OpBitFieldUExtract(ctx.U32[1], value, ctx.ConstU32(16U), ctx.ConstU32(16U))};

    const auto unpacked{ctx.OpCompositeConstruct(ctx.U32[2], x, y)};
    return ctx.OpBitcast(ctx.F32[2], unpacked);
}

Id EmitPackSint2x16(EmitContext& ctx, Id value) {
    return EmitPackUint2x16(ctx, value);
}

Id EmitUnpackSint2x16(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto x{ctx.OpBitFieldSExtract(ctx.U32[1], value, ctx.ConstU32(0U), ctx.ConstU32(16U))};
    const auto y{ctx.OpBitFieldSExtract(ctx.U32[1], value, ctx.ConstU32(16U), ctx.ConstU32(16U))};

    const auto unpacked{ctx.OpCompositeConstruct(ctx.U32[2], x, y)};
    return ctx.OpBitcast(ctx.F32[2], unpacked);
}

Id EmitPackHalf2x16(EmitContext& ctx, Id value) {
    return ctx.OpPackHalf2x16(ctx.U32[1], value);
}

Id EmitUnpackHalf2x16(EmitContext& ctx, Id value) {
    return ctx.OpUnpackHalf2x16(ctx.F32[2], value);
}

Id EmitPackUnorm4x8(EmitContext& ctx, Id value) {
    return ctx.OpPackUnorm4x8(ctx.U32[1], value);
}

Id EmitUnpackUnorm4x8(EmitContext& ctx, Id value) {
    return ctx.OpUnpackUnorm4x8(ctx.F32[4], value);
}

Id EmitPackSnorm4x8(EmitContext& ctx, Id value) {
    return ctx.OpPackSnorm4x8(ctx.U32[1], value);
}

Id EmitUnpackSnorm4x8(EmitContext& ctx, Id value) {
    return ctx.OpUnpackSnorm4x8(ctx.F32[4], value);
}

Id EmitPackUint4x8(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto unpacked{ctx.OpBitcast(ctx.U32[4], value)};
    const auto x{ctx.OpCompositeExtract(ctx.U32[1], unpacked, 0)};
    const auto y{ctx.OpCompositeExtract(ctx.U32[1], unpacked, 1)};
    const auto z{ctx.OpCompositeExtract(ctx.U32[1], unpacked, 2)};
    const auto w{ctx.OpCompositeExtract(ctx.U32[1], unpacked, 3)};

    auto packed = x;
    packed = ctx.OpBitFieldInsert(ctx.U32[1], packed, y, ctx.ConstU32(8U), ctx.ConstU32(8U));
    packed = ctx.OpBitFieldInsert(ctx.U32[1], packed, z, ctx.ConstU32(16U), ctx.ConstU32(8U));
    packed = ctx.OpBitFieldInsert(ctx.U32[1], packed, w, ctx.ConstU32(24U), ctx.ConstU32(8U));
    return packed;
}

Id EmitUnpackUint4x8(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto x{ctx.OpBitFieldUExtract(ctx.U32[1], value, ctx.ConstU32(0U), ctx.ConstU32(8U))};
    const auto y{ctx.OpBitFieldUExtract(ctx.U32[1], value, ctx.ConstU32(8U), ctx.ConstU32(8U))};
    const auto z{ctx.OpBitFieldUExtract(ctx.U32[1], value, ctx.ConstU32(16U), ctx.ConstU32(8U))};
    const auto w{ctx.OpBitFieldUExtract(ctx.U32[1], value, ctx.ConstU32(24U), ctx.ConstU32(8U))};

    const auto unpacked{ctx.OpCompositeConstruct(ctx.U32[4], x, y, z, w)};
    return ctx.OpBitcast(ctx.F32[4], unpacked);
}

Id EmitPackSint4x8(EmitContext& ctx, Id value) {
    return EmitPackUint4x8(ctx, value);
}

Id EmitUnpackSint4x8(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto x{ctx.OpBitFieldSExtract(ctx.U32[1], value, ctx.ConstU32(0U), ctx.ConstU32(8U))};
    const auto y{ctx.OpBitFieldSExtract(ctx.U32[1], value, ctx.ConstU32(8U), ctx.ConstU32(8U))};
    const auto z{ctx.OpBitFieldSExtract(ctx.U32[1], value, ctx.ConstU32(16U), ctx.ConstU32(8U))};
    const auto w{ctx.OpBitFieldSExtract(ctx.U32[1], value, ctx.ConstU32(24U), ctx.ConstU32(8U))};

    const auto unpacked{ctx.OpCompositeConstruct(ctx.U32[4], x, y, z, w)};
    return ctx.OpBitcast(ctx.F32[4], unpacked);
}

Id EmitPackUfloat10_11_11(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto x{ctx.OpCompositeExtract(ctx.F32[1], value, 0)};
    const auto y{ctx.OpCompositeExtract(ctx.F32[1], value, 1)};
    const auto z{ctx.OpCompositeExtract(ctx.F32[1], value, 2)};

    const auto cvt_x{ctx.OpFunctionCall(ctx.U32[1], ctx.f32_to_uf11, x)};
    const auto cvt_y{ctx.OpFunctionCall(ctx.U32[1], ctx.f32_to_uf11, y)};
    const auto cvt_z{ctx.OpFunctionCall(ctx.U32[1], ctx.f32_to_uf10, z)};

    auto result = cvt_x;
    result = ctx.OpBitFieldInsert(ctx.U32[1], result, cvt_y, ctx.ConstU32(11U), ctx.ConstU32(11U));
    result = ctx.OpBitFieldInsert(ctx.U32[1], result, cvt_z, ctx.ConstU32(22U), ctx.ConstU32(10U));
    return result;
}

Id EmitUnpackUfloat10_11_11(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto x{ctx.OpBitFieldUExtract(ctx.U32[1], value, ctx.ConstU32(0U), ctx.ConstU32(11U))};
    const auto y{ctx.OpBitFieldUExtract(ctx.U32[1], value, ctx.ConstU32(11U), ctx.ConstU32(11U))};
    const auto z{ctx.OpBitFieldUExtract(ctx.U32[1], value, ctx.ConstU32(22U), ctx.ConstU32(10U))};

    const auto cvt_x{ctx.OpFunctionCall(ctx.F32[1], ctx.uf11_to_f32, x)};
    const auto cvt_y{ctx.OpFunctionCall(ctx.F32[1], ctx.uf11_to_f32, y)};
    const auto cvt_z{ctx.OpFunctionCall(ctx.F32[1], ctx.uf10_to_f32, z)};

    return ctx.OpCompositeConstruct(ctx.F32[3], cvt_x, cvt_y, cvt_z);
}

Id EmitPackUnorm2_10_10_10(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto unorm_min{ctx.ConstantComposite(ctx.F32[4], ctx.ConstF32(0.f), ctx.ConstF32(0.f),
                                               ctx.ConstF32(0.f), ctx.ConstF32(0.f))};
    const auto unorm_max{ctx.ConstantComposite(ctx.F32[4], ctx.ConstF32(1.f), ctx.ConstF32(1.f),
                                               ctx.ConstF32(1.f), ctx.ConstF32(1.f))};
    const auto clamped{ctx.OpFClamp(ctx.F32[4], value, unorm_min, unorm_max)};
    const auto unorm_mul{ctx.ConstantComposite(ctx.F32[4], ctx.ConstF32(1023.f),
                                               ctx.ConstF32(1023.f), ctx.ConstF32(1023.f),
                                               ctx.ConstF32(3.f))};
    const auto as_float{ctx.OpFMul(ctx.F32[4], clamped, unorm_mul)};
    const auto as_uint{ctx.OpConvertFToU(ctx.U32[4], ctx.OpRoundEven(ctx.F32[4], as_float))};
    return EmitPackUint2_10_10_10(ctx, ctx.OpBitcast(ctx.F32[4], as_uint));
}

Id EmitUnpackUnorm2_10_10_10(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto unpacked{ctx.OpBitcast(ctx.U32[4], EmitUnpackUint2_10_10_10(ctx, value))};
    const auto as_float{ctx.OpConvertUToF(ctx.F32[4], unpacked)};
    const auto unorm_div{ctx.ConstantComposite(ctx.F32[4], ctx.ConstF32(1023.f),
                                               ctx.ConstF32(1023.f), ctx.ConstF32(1023.f),
                                               ctx.ConstF32(3.f))};
    return ctx.OpFDiv(ctx.F32[4], as_float, unorm_div);
}

Id EmitPackSnorm2_10_10_10(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto snorm_min{ctx.ConstantComposite(ctx.F32[4], ctx.ConstF32(-1.f), ctx.ConstF32(-1.f),
                                               ctx.ConstF32(-1.f), ctx.ConstF32(-1.f))};
    const auto snorm_max{ctx.ConstantComposite(ctx.F32[4], ctx.ConstF32(1.f), ctx.ConstF32(1.f),
                                               ctx.ConstF32(1.f), ctx.ConstF32(1.f))};
    const auto clamped{ctx.OpFClamp(ctx.F32[4], value, snorm_min, snorm_max)};
    const auto snorm_mul{ctx.ConstantComposite(ctx.F32[4], ctx.ConstF32(511.f), ctx.ConstF32(511.f),
                                               ctx.ConstF32(511.f), ctx.ConstF32(1.f))};
    const auto as_float{ctx.OpFMul(ctx.F32[4], clamped, snorm_mul)};
    const auto as_sint{ctx.OpConvertFToS(ctx.U32[4], ctx.OpRoundEven(ctx.F32[4], as_float))};
    return EmitPackSint2_10_10_10(ctx, ctx.OpBitcast(ctx.F32[4], as_sint));
}

Id EmitUnpackSnorm2_10_10_10(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto unpacked{ctx.OpBitcast(ctx.U32[4], EmitUnpackSint2_10_10_10(ctx, value))};
    const auto as_float{ctx.OpConvertSToF(ctx.F32[4], unpacked)};
    const auto snorm_div{ctx.ConstantComposite(ctx.F32[4], ctx.ConstF32(511.f), ctx.ConstF32(511.f),
                                               ctx.ConstF32(511.f), ctx.ConstF32(1.f))};
    return ctx.OpFDiv(ctx.F32[4], as_float, snorm_div);
}

Id EmitPackUint2_10_10_10(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto unpacked{ctx.OpBitcast(ctx.U32[4], value)};
    const auto x{ctx.OpCompositeExtract(ctx.U32[1], unpacked, 0)};
    const auto y{ctx.OpCompositeExtract(ctx.U32[1], unpacked, 1)};
    const auto z{ctx.OpCompositeExtract(ctx.U32[1], unpacked, 2)};
    const auto w{ctx.OpCompositeExtract(ctx.U32[1], unpacked, 3)};

    auto result = x;
    result = ctx.OpBitFieldInsert(ctx.U32[1], result, y, ctx.ConstU32(10U), ctx.ConstU32(10U));
    result = ctx.OpBitFieldInsert(ctx.U32[1], result, z, ctx.ConstU32(20U), ctx.ConstU32(10U));
    result = ctx.OpBitFieldInsert(ctx.U32[1], result, w, ctx.ConstU32(30U), ctx.ConstU32(2U));
    return result;
}

Id EmitUnpackUint2_10_10_10(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto x{ctx.OpBitFieldUExtract(ctx.U32[1], value, ctx.ConstU32(0U), ctx.ConstU32(10U))};
    const auto y{ctx.OpBitFieldUExtract(ctx.U32[1], value, ctx.ConstU32(10U), ctx.ConstU32(10U))};
    const auto z{ctx.OpBitFieldUExtract(ctx.U32[1], value, ctx.ConstU32(20U), ctx.ConstU32(10U))};
    const auto w{ctx.OpBitFieldUExtract(ctx.U32[1], value, ctx.ConstU32(30U), ctx.ConstU32(2U))};

    const auto unpacked{ctx.OpCompositeConstruct(ctx.U32[4], x, y, z, w)};
    return ctx.OpBitcast(ctx.F32[4], unpacked);
}

Id EmitPackSint2_10_10_10(EmitContext& ctx, Id value) {
    return EmitPackUint2_10_10_10(ctx, value);
}

Id EmitUnpackSint2_10_10_10(EmitContext& ctx, Id value) {
    // No SPIR-V instruction for this, do it manually.
    const auto x{ctx.OpBitFieldSExtract(ctx.U32[1], value, ctx.ConstU32(0U), ctx.ConstU32(10U))};
    const auto y{ctx.OpBitFieldSExtract(ctx.U32[1], value, ctx.ConstU32(10U), ctx.ConstU32(10U))};
    const auto z{ctx.OpBitFieldSExtract(ctx.U32[1], value, ctx.ConstU32(20U), ctx.ConstU32(10U))};
    const auto w{ctx.OpBitFieldSExtract(ctx.U32[1], value, ctx.ConstU32(30U), ctx.ConstU32(2U))};

    const auto unpacked{ctx.OpCompositeConstruct(ctx.U32[4], x, y, z, w)};
    return ctx.OpBitcast(ctx.F32[4], unpacked);
}

} // namespace Shader::Backend::SPIRV
