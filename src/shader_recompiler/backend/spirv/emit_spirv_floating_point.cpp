// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {

Id Decorate(EmitContext& ctx, IR::Inst* inst, Id op) {
    ctx.Decorate(op, spv::Decoration::NoContraction);
    return op;
}

Id EmitFPAbs16(EmitContext& ctx, Id value) {
    return ctx.OpFAbs(ctx.F16[1], value);
}

Id EmitFPAbs32(EmitContext& ctx, Id value) {
    return ctx.OpFAbs(ctx.F32[1], value);
}

Id EmitFPAbs64(EmitContext& ctx, Id value) {
    return ctx.OpFAbs(ctx.F64[1], value);
}

Id EmitFPAdd16(EmitContext& ctx, IR::Inst* inst, Id a, Id b) {
    return Decorate(ctx, inst, ctx.OpFAdd(ctx.F16[1], a, b));
}

Id EmitFPAdd32(EmitContext& ctx, IR::Inst* inst, Id a, Id b) {
    return Decorate(ctx, inst, ctx.OpFAdd(ctx.F32[1], a, b));
}

Id EmitFPAdd64(EmitContext& ctx, IR::Inst* inst, Id a, Id b) {
    return Decorate(ctx, inst, ctx.OpFAdd(ctx.F64[1], a, b));
}

Id EmitFPSub32(EmitContext& ctx, IR::Inst* inst, Id a, Id b) {
    return Decorate(ctx, inst, ctx.OpFSub(ctx.F32[1], a, b));
}

Id EmitFPFma16(EmitContext& ctx, IR::Inst* inst, Id a, Id b, Id c) {
    return Decorate(ctx, inst, ctx.OpFma(ctx.F16[1], a, b, c));
}

Id EmitFPFma32(EmitContext& ctx, IR::Inst* inst, Id a, Id b, Id c) {
    return Decorate(ctx, inst, ctx.OpFma(ctx.F32[1], a, b, c));
}

Id EmitFPFma64(EmitContext& ctx, IR::Inst* inst, Id a, Id b, Id c) {
    return Decorate(ctx, inst, ctx.OpFma(ctx.F64[1], a, b, c));
}

Id EmitFPMax32(EmitContext& ctx, Id a, Id b, bool is_legacy) {
    if (is_legacy) {
        return ctx.OpNMax(ctx.F32[1], a, b);
    }

    return ctx.OpFMax(ctx.F32[1], a, b);
}

Id EmitFPMax64(EmitContext& ctx, Id a, Id b) {
    return ctx.OpFMax(ctx.F64[1], a, b);
}

Id EmitFPMin32(EmitContext& ctx, Id a, Id b, bool is_legacy) {
    if (is_legacy) {
        return ctx.OpNMin(ctx.F32[1], a, b);
    }

    return ctx.OpFMin(ctx.F32[1], a, b);
}

Id EmitFPMin64(EmitContext& ctx, Id a, Id b) {
    return ctx.OpFMin(ctx.F64[1], a, b);
}

Id EmitFPMinTri32(EmitContext& ctx, Id a, Id b, Id c) {
    if (ctx.profile.supports_trinary_minmax) {
        return ctx.OpFMin3AMD(ctx.F32[1], a, b, c);
    }
    return ctx.OpFMin(ctx.F32[1], a, ctx.OpFMin(ctx.F32[1], b, c));
}

Id EmitFPMaxTri32(EmitContext& ctx, Id a, Id b, Id c) {
    if (ctx.profile.supports_trinary_minmax) {
        return ctx.OpFMax3AMD(ctx.F32[1], a, b, c);
    }
    return ctx.OpFMax(ctx.F32[1], a, ctx.OpFMax(ctx.F32[1], b, c));
}

Id EmitFPMedTri32(EmitContext& ctx, Id a, Id b, Id c) {
    if (ctx.profile.supports_trinary_minmax) {
        return ctx.OpFMid3AMD(ctx.F32[1], a, b, c);
    }
    const Id mmx{ctx.OpFMin(ctx.F32[1], ctx.OpFMax(ctx.F32[1], a, b), c)};
    return ctx.OpFMax(ctx.F32[1], ctx.OpFMin(ctx.F32[1], a, b), mmx);
}

Id EmitFPMul16(EmitContext& ctx, IR::Inst* inst, Id a, Id b) {
    return Decorate(ctx, inst, ctx.OpFMul(ctx.F16[1], a, b));
}

Id EmitFPMul32(EmitContext& ctx, IR::Inst* inst, Id a, Id b) {
    return Decorate(ctx, inst, ctx.OpFMul(ctx.F32[1], a, b));
}

Id EmitFPMul64(EmitContext& ctx, IR::Inst* inst, Id a, Id b) {
    return Decorate(ctx, inst, ctx.OpFMul(ctx.F64[1], a, b));
}

Id EmitFPDiv32(EmitContext& ctx, IR::Inst* inst, Id a, Id b) {
    return Decorate(ctx, inst, ctx.OpFDiv(ctx.F32[1], a, b));
}

Id EmitFPDiv64(EmitContext& ctx, IR::Inst* inst, Id a, Id b) {
    return Decorate(ctx, inst, ctx.OpFDiv(ctx.F64[1], a, b));
}

Id EmitFPNeg16(EmitContext& ctx, Id value) {
    return ctx.OpFNegate(ctx.F16[1], value);
}

Id EmitFPNeg32(EmitContext& ctx, Id value) {
    return ctx.OpFNegate(ctx.F32[1], value);
}

Id EmitFPNeg64(EmitContext& ctx, Id value) {
    return ctx.OpFNegate(ctx.F64[1], value);
}

Id EmitFPSin(EmitContext& ctx, Id value) {
    return ctx.OpSin(ctx.F32[1], ctx.OpFMul(ctx.F32[1], ctx.pi_x2, value));
}

Id EmitFPCos(EmitContext& ctx, Id value) {
    return ctx.OpCos(ctx.F32[1], ctx.OpFMul(ctx.F32[1], ctx.pi_x2, value));
}

Id EmitFPExp2(EmitContext& ctx, Id value) {
    return ctx.OpExp2(ctx.F32[1], value);
}

Id EmitFPPow(EmitContext& ctx, Id x, Id y) {
    return ctx.OpPow(ctx.F32[1], x, y);
}

Id EmitFPLdexp(EmitContext& ctx, Id value, Id exp) {
    return ctx.OpLdexp(ctx.F32[1], value, exp);
}

Id EmitFPLog2(EmitContext& ctx, Id value) {
    return ctx.OpLog2(ctx.F32[1], value);
}

Id EmitFPRecip32(EmitContext& ctx, Id value) {
    return ctx.OpFDiv(ctx.F32[1], ctx.ConstF32(1.0f), value);
}

Id EmitFPRecip64(EmitContext& ctx, Id value) {
    return ctx.OpFDiv(ctx.F64[1], ctx.Constant(ctx.F64[1], f64{1.0}), value);
}

Id EmitFPRecipSqrt32(EmitContext& ctx, Id value) {
    return ctx.OpInverseSqrt(ctx.F32[1], value);
}

Id EmitFPRecipSqrt64(EmitContext& ctx, Id value) {
    return ctx.OpInverseSqrt(ctx.F64[1], value);
}

Id EmitFPSqrt(EmitContext& ctx, Id value) {
    return ctx.OpSqrt(ctx.F32[1], value);
}

Id EmitFPSaturate16(EmitContext& ctx, Id value) {
    const Id zero{ctx.Constant(ctx.F16[1], u16{0})};
    const Id one{ctx.Constant(ctx.F16[1], u16{0x3c00})};
    return ctx.OpFClamp(ctx.F16[1], value, zero, one);
}

Id EmitFPSaturate32(EmitContext& ctx, Id value) {
    const Id zero{ctx.ConstF32(f32{0.0})};
    const Id one{ctx.ConstF32(f32{1.0})};
    return ctx.OpFClamp(ctx.F32[1], value, zero, one);
}

Id EmitFPSaturate64(EmitContext& ctx, Id value) {
    const Id zero{ctx.Constant(ctx.F64[1], f64{0.0})};
    const Id one{ctx.Constant(ctx.F64[1], f64{1.0})};
    return ctx.OpFClamp(ctx.F64[1], value, zero, one);
}

Id EmitFPClamp16(EmitContext& ctx, Id value, Id min_value, Id max_value) {
    return ctx.OpFClamp(ctx.F16[1], value, min_value, max_value);
}

Id EmitFPClamp32(EmitContext& ctx, Id value, Id min_value, Id max_value) {
    return ctx.OpFClamp(ctx.F32[1], value, min_value, max_value);
}

Id EmitFPClamp64(EmitContext& ctx, Id value, Id min_value, Id max_value) {
    return ctx.OpFClamp(ctx.F64[1], value, min_value, max_value);
}

Id EmitFPRoundEven16(EmitContext& ctx, Id value) {
    return ctx.OpRoundEven(ctx.F16[1], value);
}

Id EmitFPRoundEven32(EmitContext& ctx, Id value) {
    return ctx.OpRoundEven(ctx.F32[1], value);
}

Id EmitFPRoundEven64(EmitContext& ctx, Id value) {
    return ctx.OpRoundEven(ctx.F64[1], value);
}

Id EmitFPFloor16(EmitContext& ctx, Id value) {
    return ctx.OpFloor(ctx.F16[1], value);
}

Id EmitFPFloor32(EmitContext& ctx, Id value) {
    return ctx.OpFloor(ctx.F32[1], value);
}

Id EmitFPFloor64(EmitContext& ctx, Id value) {
    return ctx.OpFloor(ctx.F64[1], value);
}

Id EmitFPCeil16(EmitContext& ctx, Id value) {
    return ctx.OpCeil(ctx.F16[1], value);
}

Id EmitFPCeil32(EmitContext& ctx, Id value) {
    return ctx.OpCeil(ctx.F32[1], value);
}

Id EmitFPCeil64(EmitContext& ctx, Id value) {
    return ctx.OpCeil(ctx.F64[1], value);
}

Id EmitFPTrunc16(EmitContext& ctx, Id value) {
    return ctx.OpTrunc(ctx.F16[1], value);
}

Id EmitFPTrunc32(EmitContext& ctx, Id value) {
    return ctx.OpTrunc(ctx.F32[1], value);
}

Id EmitFPTrunc64(EmitContext& ctx, Id value) {
    return ctx.OpTrunc(ctx.F64[1], value);
}

Id EmitFPFract32(EmitContext& ctx, Id value) {
    return ctx.OpFract(ctx.F32[1], value);
}

Id EmitFPFract64(EmitContext& ctx, Id value) {
    return ctx.OpFract(ctx.F64[1], value);
}

Id EmitFPFrexpSig32(EmitContext& ctx, Id value) {
    const auto frexp = ctx.OpFrexpStruct(ctx.frexp_result_f32, value);
    return ctx.OpCompositeExtract(ctx.F32[1], frexp, 0);
}

Id EmitFPFrexpSig64(EmitContext& ctx, Id value) {
    const auto frexp = ctx.OpFrexpStruct(ctx.frexp_result_f64, value);
    return ctx.OpCompositeExtract(ctx.F64[1], frexp, 0);
}

Id EmitFPFrexpExp32(EmitContext& ctx, Id value) {
    const auto frexp = ctx.OpFrexpStruct(ctx.frexp_result_f32, value);
    return ctx.OpBitcast(ctx.U32[1], ctx.OpCompositeExtract(ctx.S32[1], frexp, 1));
}

Id EmitFPFrexpExp64(EmitContext& ctx, Id value) {
    const auto frexp = ctx.OpFrexpStruct(ctx.frexp_result_f64, value);
    return ctx.OpBitcast(ctx.U32[1], ctx.OpCompositeExtract(ctx.S32[1], frexp, 1));
}

Id EmitFPOrdEqual16(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdEqual32(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdEqual64(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordEqual16(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordEqual32(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordEqual64(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdNotEqual16(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdNotEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdNotEqual32(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdNotEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdNotEqual64(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdNotEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordNotEqual16(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordNotEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordNotEqual32(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordNotEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordNotEqual64(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordNotEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdLessThan16(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdLessThan(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdLessThan32(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdLessThan(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdLessThan64(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdLessThan(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordLessThan16(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordLessThan(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordLessThan32(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordLessThan(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordLessThan64(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordLessThan(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdGreaterThan16(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdGreaterThan(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdGreaterThan32(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdGreaterThan(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdGreaterThan64(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdGreaterThan(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordGreaterThan16(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordGreaterThan(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordGreaterThan32(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordGreaterThan(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordGreaterThan64(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordGreaterThan(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdLessThanEqual16(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdLessThanEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdLessThanEqual32(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdLessThanEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdLessThanEqual64(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdLessThanEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordLessThanEqual16(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordLessThanEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordLessThanEqual32(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordLessThanEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordLessThanEqual64(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordLessThanEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdGreaterThanEqual16(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdGreaterThanEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdGreaterThanEqual32(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdGreaterThanEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPOrdGreaterThanEqual64(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFOrdGreaterThanEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordGreaterThanEqual16(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordGreaterThanEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordGreaterThanEqual32(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordGreaterThanEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPUnordGreaterThanEqual64(EmitContext& ctx, Id lhs, Id rhs) {
    return ctx.OpFUnordGreaterThanEqual(ctx.U1[1], lhs, rhs);
}

Id EmitFPIsNan16(EmitContext& ctx, Id value) {
    return ctx.OpIsNan(ctx.U1[1], value);
}

Id EmitFPIsNan32(EmitContext& ctx, Id value) {
    return ctx.OpIsNan(ctx.U1[1], value);
}

Id EmitFPIsNan64(EmitContext& ctx, Id value) {
    return ctx.OpIsNan(ctx.U1[1], value);
}

Id EmitFPIsInf32(EmitContext& ctx, Id value) {
    return ctx.OpIsInf(ctx.U1[1], value);
}

Id EmitFPIsInf64(EmitContext& ctx, Id value) {
    return ctx.OpIsInf(ctx.U1[1], value);
}

void EmitFPCmpClass32(EmitContext&) {
    UNREACHABLE();
}

} // namespace Shader::Backend::SPIRV
