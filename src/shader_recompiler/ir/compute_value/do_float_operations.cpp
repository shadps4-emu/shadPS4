// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/cartesian_invoke.h"
#include "shader_recompiler/ir/compute_value/do_float_operations.h"

namespace Shader::IR::ComputeValue {

void DoFPAbs32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Abs<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPAbs64(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Abs<Type::F64>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPAdd32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Add<Type::F32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoFPAdd64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Add<Type::F64, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoFPSub32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Sub<Type::F32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoFPFma32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1,
               const ImmValueList& args2) {
    Common::CartesianInvoke(ImmValue::Fma<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1,
                            args2);
}

void DoFPFma64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1,
               const ImmValueList& args2) {
    Common::CartesianInvoke(ImmValue::Fma<Type::F64>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1,
                            args2);
}

void DoFPMax32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1,
               const ImmValueList& args_legacy) {
    const auto& op = [](const ImmValue& a, const ImmValue& b, const ImmValue& legacy) {
        if (legacy.U1()) {
            if (ImmValue::IsNan<Type::F32>(a))
                return b;
            if (ImmValue::IsNan<Type::F32>(b))
                return a;
        }
        return ImmValue::Max<Type::F32, true>(a, b);
    };
    Common::CartesianInvoke(op, std::insert_iterator(inst_values, inst_values.begin()), args0,
                            args1, args_legacy);
}

void DoFPMax64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Max<Type::F64, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoFPMin32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1,
               const ImmValueList& args_legacy) {
    const auto& op = [](const ImmValue& a, const ImmValue& b, const ImmValue& legacy) {
        if (legacy.U1()) {
            if (ImmValue::IsNan<Type::F64>(a))
                return b;
            if (ImmValue::IsNan<Type::F64>(b))
                return a;
        }
        return ImmValue::Min<Type::F32, true>(a, b);
    };
    Common::CartesianInvoke(op, std::insert_iterator(inst_values, inst_values.begin()), args0,
                            args1, args_legacy);
}

void DoFPMin64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Min<Type::F64, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoFPMul32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Mul<Type::F32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoFPMul64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Mul<Type::F64, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoFPDiv32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Div<Type::F32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoFPDiv64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Div<Type::F64, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoFPNeg32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Neg<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPNeg64(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Neg<Type::F64>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPRecip32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Recip<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPRecip64(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Recip<Type::F64>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPRecipSqrt32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Rsqrt<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPRecipSqrt64(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Rsqrt<Type::F64>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPSqrt(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Sqrt<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPSin(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Sin<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPExp2(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Exp2<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPLdexp(ImmValueList& inst_values, const ImmValueList& args, const ImmValueList& exponents) {
    Common::CartesianInvoke(ImmValue::Ldexp<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args,
                            exponents);
}

void DoFPCos(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Cos<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPLog2(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Log2<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPSaturate32(ImmValueList& inst_values, const ImmValueList& args) {
    UNREACHABLE_MSG("FPSaturate32 not implemented");
}

void DoFPSaturate64(ImmValueList& inst_values, const ImmValueList& args) {
    UNREACHABLE_MSG("FPSaturate64 not implemented");
}

void DoFPClamp32(ImmValueList& inst_values, const ImmValueList& args, const ImmValueList& mins,
                 const ImmValueList& maxs) {
    Common::CartesianInvoke(ImmValue::Clamp<Type::F32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args, mins,
                            maxs);
}

void DoFPClamp64(ImmValueList& inst_values, const ImmValueList& args, const ImmValueList& mins,
                 const ImmValueList& maxs) {
    Common::CartesianInvoke(ImmValue::Clamp<Type::F64, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args, mins,
                            maxs);
}

void DoFPRoundEven32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Round<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPRoundEven64(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Round<Type::F64>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPFloor32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Floor<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPFloor64(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Floor<Type::F64>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPCeil32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Ceil<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPCeil64(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Ceil<Type::F64>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPTrunc32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Trunc<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPTrunc64(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Trunc<Type::F64>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPFract32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Fract<Type::F32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPFract64(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Fract<Type::F64>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoFPFrexpSig32(ImmValueList& inst_values, const ImmValueList& args) {
    UNREACHABLE_MSG("FPFrexpSig32 not implemented");
}

void DoFPFrexpSig64(ImmValueList& inst_values, const ImmValueList& args) {
    UNREACHABLE_MSG("FPFrexpSig64 not implemented");
}

void DoFPFrexpExp32(ImmValueList& inst_values, const ImmValueList& args) {
    UNREACHABLE_MSG("FPFrexpExp32 not implemented");
}

void DoFPFrexpExp64(ImmValueList& inst_values, const ImmValueList& args) {
    UNREACHABLE_MSG("FPFrexpExp64 not implemented");
}

} // namespace Shader::IR::ComputeValue