// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <functional>
#include <tuple>
#include <vector>
#include "shader_recompiler/ir/compute_value/compute.h"

namespace Shader::IR {

template <typename Func, typename OutputIt, size_t N, size_t Level, typename... ArgLists>
static void CartesianInvokeImpl(Func func, OutputIt out_it,
                                std::tuple<typename ArgLists::const_iterator...>& arglists_its,
                                const std::tuple<const ArgLists&...>& arglists_tuple) {
    if constexpr (Level == N) {
        auto get_tuple = [&]<size_t... I>(std::index_sequence<I...>) {
            return std::forward_as_tuple(*std::get<I>(arglists_its)...);
        };
        *out_it++ = std::move(std::apply(func, get_tuple(std::make_index_sequence<N>{})));
        return;
    } else {
        const auto& arglist = std::get<Level>(arglists_tuple);
        for (auto it = arglist.begin(); it != arglist.end(); ++it) {
            std::get<Level>(arglists_its) = it;
            CartesianInvokeImpl<Func, OutputIt, N, Level + 1, ArgLists...>(
                func, out_it, arglists_its, arglists_tuple);
        }
    }
}

template <typename Func, typename OutputIt, typename... ArgLists>
static void CartesianInvoke(Func func, OutputIt out_it, const ArgLists&... arg_lists) {
    constexpr size_t N = sizeof...(ArgLists);
    const std::tuple<const ArgLists&...> arglists_tuple = std::forward_as_tuple(arg_lists...);

    std::tuple<typename ArgLists::const_iterator...> arglists_it;
    CartesianInvokeImpl<Func, OutputIt, N, 0, ArgLists...>(func, out_it, arglists_it,
                                                           arglists_tuple);
}

static void SetSigned(ImmValueList& values, bool is_signed) {
    for (auto& value : values) {
        value.SetSigned(is_signed);
    }
}

static void OperationAbs(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) { return a.abs(); };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationAdd(Inst* inst, bool is_signed, ImmValueList& inst_values,
                         ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) { return a + b; };

    SetSigned(args0, is_signed);
    SetSigned(args1, is_signed);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationSub(Inst* inst, bool is_signed, ImmValueList& inst_values,
                         ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) { return a - b; };

    SetSigned(args0, is_signed);
    SetSigned(args1, is_signed);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationFma(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args0, args1, args2;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);
    ComputeImmValues(inst->Arg(2), args2, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b, const ImmValue& c) {
        return ImmValue::fma(ImmF32F64(a), ImmF32F64(b), ImmF32F64(c));
    };

    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1, args2);
}

static void OperationMin(Inst* inst, bool is_signed, ImmValueList& inst_values,
                         ComputeImmValuesCache& cache) {
    ImmValueList args0, args1, is_legacy_args;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);
    if (inst->NumArgs() > 2) {
        ComputeImmValues(inst->Arg(2), is_legacy_args, cache);
    } else {
        is_legacy_args.insert(ImmValue(false));
    }

    const auto op = [](const ImmValue& a, const ImmValue& b, const ImmValue& is_legacy) {
        if (is_legacy.U1()) {
            if (a.isnan())
                return b;
            if (b.isnan())
                return a;
        }
        return std::min(a, b);
    };

    SetSigned(args0, is_signed);
    SetSigned(args1, is_signed);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1,
                    is_legacy_args);
}

static void OperationMax(Inst* inst, bool is_signed, ImmValueList& inst_values,
                         ComputeImmValuesCache& cache) {
    ImmValueList args0, args1, is_legacy_args;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);
    if (inst->NumArgs() > 2) {
        ComputeImmValues(inst->Arg(2), is_legacy_args, cache);
    } else {
        is_legacy_args.insert(ImmValue(false));
    }

    const auto op = [](const ImmValue& a, const ImmValue& b, const ImmValue& is_legacy) {
        if (is_legacy.U1()) {
            if (a.isnan())
                return b;
            if (b.isnan())
                return a;
        }
        return std::max(a, b);
    };

    SetSigned(args0, is_signed);
    SetSigned(args1, is_signed);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1,
                    is_legacy_args);
}

static void OperationMul(Inst* inst, bool is_signed, ImmValueList& inst_values,
                         ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) { return a * b; };

    SetSigned(args0, is_signed);
    SetSigned(args1, is_signed);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationDiv(Inst* inst, bool is_signed, ImmValueList& inst_values,
                         ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) { return a / b; };

    SetSigned(args0, is_signed);
    SetSigned(args1, is_signed);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationMod(Inst* inst, bool is_signed, ImmValueList& inst_values,
                         ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) { return a % b; };

    SetSigned(args0, is_signed);
    SetSigned(args1, is_signed);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationNeg(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) { return -a; };

    SetSigned(args, true);
    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationRecip(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) { return a.recip(); };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationRecipSqrt(Inst* inst, ImmValueList& inst_values,
                               ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) { return a.rsqrt(); };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationSqrt(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) { return a.sqrt(); };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationSin(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) { return a.sin(); };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationExp2(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) { return a.exp2(); };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationLdexp(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) { return a.ldexp(ImmU32(b)); };

    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationCos(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) { return a.cos(); };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationLog2(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) { return a.log2(); };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationClamp(Inst* inst, bool is_signed, ImmValueList& inst_values,
                           ComputeImmValuesCache& cache) {
    ImmValueList args0, args1, args2;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);
    ComputeImmValues(inst->Arg(2), args2, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b, const ImmValue& c) {
        return a.clamp(b, c);
    };

    SetSigned(args0, is_signed);
    SetSigned(args1, is_signed);
    SetSigned(args2, is_signed);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1, args2);
}

static void OperationRound(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) { return a.round(); };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationFloor(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) { return a.floor(); };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationCeil(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) { return a.ceil(); };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationTrunc(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) { return a.trunc(); };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationFract(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) { return a.fract(); };
    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationShiftLeft(Inst* inst, ImmValueList& inst_values,
                               ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) { return a << ImmU32(b); };

    SetSigned(args1, false);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationShiftRight(Inst* inst, bool is_signed, ImmValueList& inst_values,
                                ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) { return a >> ImmU32(b); };

    SetSigned(args0, is_signed);
    SetSigned(args1, false);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationBitwiseNot(Inst* inst, ImmValueList& inst_values,
                                ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) { return ~a; };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationBitwiseAnd(Inst* inst, ImmValueList& inst_values,
                                ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) { return a & b; };

    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationBitwiseOr(Inst* inst, ImmValueList& inst_values,
                               ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) { return a | b; };

    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationBitwiseXor(Inst* inst, ImmValueList& inst_values,
                                ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) { return a ^ b; };

    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationConvert(Inst* inst, bool is_signed, Type new_type, bool new_signed,
                             ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [new_type, new_signed](const ImmValue& a) {
        return a.Convert(new_type, new_signed);
    };

    SetSigned(args, is_signed);
    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationBitCast(Inst* inst, Type new_type, bool new_signed, ImmValueList& inst_values,
                             ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [new_type, new_signed](const ImmValue& a) {
        return a.Bitcast(new_type, new_signed);
    };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

template <size_t N>
static void OperationCompositeConstruct(Inst* inst, ImmValueList& inst_values,
                                        ComputeImmValuesCache& cache) {
    std::array<ImmValueList, N> args;
    for (size_t i = 0; i < N; ++i) {
        ComputeImmValues(inst->Arg(i), args[i], cache);
    }

    const auto op = []<typename... Args>(const Args&... args) { return ImmValue(args...); };

    const auto call_cartesian = [&]<size_t... I>(std::index_sequence<I...>) {
        CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args[I]...);
    };
    call_cartesian(std::make_index_sequence<N>{});
}

static void OperationCompositeExtract(Inst* inst, ImmValueList& inst_values,
                                      ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) { return a.Extract(ImmU32(b)); };

    SetSigned(args1, false);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationInsert(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args0, args1, args2;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);
    ComputeImmValues(inst->Arg(2), args2, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b, const ImmValue& c) {
        return a.Insert(b, ImmU32(c));
    };

    SetSigned(args2, false);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1, args2);
}

static void DoInstructionOperation(Inst* inst, ImmValueList& inst_values,
                                   ComputeImmValuesCache& cache) {
    switch (inst->GetOpcode()) {
    case Opcode::CompositeConstructU32x2:
    case Opcode::CompositeConstructU32x2x2:
    case Opcode::CompositeConstructF16x2:
    case Opcode::CompositeConstructF32x2:
    case Opcode::CompositeConstructF32x2x2:
    case Opcode::CompositeConstructF64x2:
        OperationCompositeConstruct<2>(inst, inst_values, cache);
        break;
    case Opcode::CompositeConstructU32x3:
    case Opcode::CompositeConstructF16x3:
    case Opcode::CompositeConstructF32x3:
    case Opcode::CompositeConstructF64x3:
        OperationCompositeConstruct<3>(inst, inst_values, cache);
        break;
    case Opcode::CompositeConstructU32x4:
    case Opcode::CompositeConstructF16x4:
    case Opcode::CompositeConstructF32x4:
    case Opcode::CompositeConstructF64x4:
        OperationCompositeConstruct<4>(inst, inst_values, cache);
        break;
    case Opcode::CompositeExtractU32x2:
    case Opcode::CompositeExtractU32x3:
    case Opcode::CompositeExtractU32x4:
    case Opcode::CompositeExtractF16x2:
    case Opcode::CompositeExtractF16x3:
    case Opcode::CompositeExtractF16x4:
    case Opcode::CompositeExtractF32x2:
    case Opcode::CompositeExtractF32x3:
    case Opcode::CompositeExtractF32x4:
    case Opcode::CompositeExtractF64x2:
    case Opcode::CompositeExtractF64x3:
    case Opcode::CompositeExtractF64x4:
        OperationCompositeExtract(inst, inst_values, cache);
        break;
    case Opcode::CompositeInsertU32x2:
    case Opcode::CompositeInsertU32x3:
    case Opcode::CompositeInsertU32x4:
    case Opcode::CompositeInsertF16x2:
    case Opcode::CompositeInsertF16x3:
    case Opcode::CompositeInsertF16x4:
    case Opcode::CompositeInsertF32x2:
    case Opcode::CompositeInsertF32x3:
    case Opcode::CompositeInsertF32x4:
    case Opcode::CompositeInsertF64x2:
    case Opcode::CompositeInsertF64x3:
    case Opcode::CompositeInsertF64x4:
        OperationInsert(inst, inst_values, cache);
        break;
    case Opcode::BitCastU16F16:
        OperationBitCast(inst, IR::Type::U16, false, inst_values, cache);
        break;
    case Opcode::BitCastU32F32:
        OperationBitCast(inst, IR::Type::U32, false, inst_values, cache);
        break;
    case Opcode::BitCastU64F64:
        OperationBitCast(inst, IR::Type::U64, false, inst_values, cache);
        break;
    case Opcode::BitCastF16U16:
        OperationBitCast(inst, IR::Type::F16, true, inst_values, cache);
        break;
    case Opcode::BitCastF32U32:
        OperationBitCast(inst, IR::Type::F32, true, inst_values, cache);
        break;
    case Opcode::BitCastF64U64:
        OperationBitCast(inst, IR::Type::F64, true, inst_values, cache);
        break;
    case Opcode::FPAbs32:
    case Opcode::FPAbs64:
    case Opcode::IAbs32:
        OperationAbs(inst, inst_values, cache);
        break;
    case Opcode::FPAdd32:
    case Opcode::FPAdd64:
        OperationAdd(inst, true, inst_values, cache);
        break;
    case Opcode::IAdd32:
    case Opcode::IAdd64:
        OperationAdd(inst, false, inst_values, cache);
        break;
    case Opcode::FPSub32:
        OperationSub(inst, true, inst_values, cache);
        break;
    case Opcode::ISub32:
    case Opcode::ISub64:
        OperationSub(inst, false, inst_values, cache);
        break;
    case Opcode::FPMul32:
    case Opcode::FPMul64:
        OperationMul(inst, true, inst_values, cache);
        break;
    case Opcode::IMul32:
    case Opcode::IMul64:
        OperationMul(inst, false, inst_values, cache);
        break;
    case Opcode::FPDiv32:
    case Opcode::FPDiv64:
    case Opcode::SDiv32:
        OperationDiv(inst, true, inst_values, cache);
        break;
    case Opcode::UDiv32:
        OperationDiv(inst, false, inst_values, cache);
        break;
    case Opcode::SMod32:
        OperationMod(inst, true, inst_values, cache);
        break;
    case Opcode::UMod32:
        OperationMod(inst, false, inst_values, cache);
        break;
    case Opcode::INeg32:
    case Opcode::INeg64:
        OperationNeg(inst, inst_values, cache);
        break;
    case Opcode::FPFma32:
    case Opcode::FPFma64:
        OperationFma(inst, inst_values, cache);
        break;
    case Opcode::FPMin32:
    case Opcode::FPMin64:
    case Opcode::SMin32:
        OperationMin(inst, true, inst_values, cache);
        break;
    case Opcode::UMin32:
        OperationMin(inst, false, inst_values, cache);
        break;
    case Opcode::FPMax32:
    case Opcode::FPMax64:
    case Opcode::SMax32:
        OperationMax(inst, true, inst_values, cache);
        break;
    case Opcode::UMax32:
        OperationMax(inst, false, inst_values, cache);
        break;
    case Opcode::FPNeg32:
    case Opcode::FPNeg64:
        OperationNeg(inst, inst_values, cache);
        break;
    case Opcode::FPRecip32:
    case Opcode::FPRecip64:
        OperationRecip(inst, inst_values, cache);
        break;
    case Opcode::FPRecipSqrt32:
    case Opcode::FPRecipSqrt64:
        OperationRecipSqrt(inst, inst_values, cache);
        break;
    case Opcode::FPSqrt:
        OperationSqrt(inst, inst_values, cache);
        break;
    case Opcode::FPSin:
        OperationSin(inst, inst_values, cache);
        break;
    case Opcode::FPCos:
        OperationCos(inst, inst_values, cache);
        break;
    case Opcode::FPExp2:
        OperationExp2(inst, inst_values, cache);
        break;
    case Opcode::FPLdexp:
        OperationLdexp(inst, inst_values, cache);
        break;
    case Opcode::FPLog2:
        OperationLog2(inst, inst_values, cache);
        break;
    case Opcode::FPClamp32:
    case Opcode::FPClamp64:
    case Opcode::SClamp32:
        OperationClamp(inst, true, inst_values, cache);
        break;
    case Opcode::UClamp32:
        OperationClamp(inst, false, inst_values, cache);
        break;
    case Opcode::FPRoundEven32:
    case Opcode::FPRoundEven64:
        OperationRound(inst, inst_values, cache);
        break;
    case Opcode::FPFloor32:
    case Opcode::FPFloor64:
        OperationFloor(inst, inst_values, cache);
        break;
    case Opcode::FPCeil32:
    case Opcode::FPCeil64:
        OperationCeil(inst, inst_values, cache);
        break;
    case Opcode::FPTrunc32:
    case Opcode::FPTrunc64:
        OperationTrunc(inst, inst_values, cache);
        break;
    case Opcode::FPFract32:
    case Opcode::FPFract64:
        OperationFract(inst, inst_values, cache);
        break;
    case Opcode::ShiftLeftLogical32:
    case Opcode::ShiftLeftLogical64:
        OperationShiftLeft(inst, inst_values, cache);
        break;
    case Opcode::ShiftRightLogical32:
    case Opcode::ShiftRightLogical64:
        OperationShiftRight(inst, false, inst_values, cache);
        break;
    case Opcode::ShiftRightArithmetic32:
    case Opcode::ShiftRightArithmetic64:
        OperationShiftRight(inst, true, inst_values, cache);
        break;
    case Opcode::BitwiseAnd32:
    case Opcode::BitwiseAnd64:
    case Opcode::LogicalAnd:
        OperationBitwiseAnd(inst, inst_values, cache);
        break;
    case Opcode::BitwiseOr32:
    case Opcode::BitwiseOr64:
    case Opcode::LogicalOr:
        OperationBitwiseOr(inst, inst_values, cache);
        break;
    case Opcode::BitwiseXor32:
    case Opcode::LogicalXor:
        OperationBitwiseXor(inst, inst_values, cache);
        break;
    case Opcode::BitwiseNot32:
    case Opcode::LogicalNot:
        OperationBitwiseNot(inst, inst_values, cache);
        break;
    case Opcode::ConvertU16U32:
        OperationConvert(inst, false, Type::U16, false, inst_values, cache);
        break;
    case Opcode::ConvertS32F32:
    case Opcode::ConvertS32F64:
        OperationConvert(inst, true, Type::U32, true, inst_values, cache);
        break;
    case Opcode::ConvertU32F32:
        OperationConvert(inst, true, Type::U32, false, inst_values, cache);
        break;
    case Opcode::ConvertU32U16:
        OperationConvert(inst, false, Type::U32, false, inst_values, cache);
        break;
    case Opcode::ConvertF32F16:
    case Opcode::ConvertF32F64:
    case Opcode::ConvertF32S32:
        OperationConvert(inst, true, Type::F32, true, inst_values, cache);
        break;
    case Opcode::ConvertF32U32:
        OperationConvert(inst, false, Type::F32, true, inst_values, cache);
        break;
    case Opcode::ConvertF64F32:
    case Opcode::ConvertF64S32:
        OperationConvert(inst, true, Type::F64, true, inst_values, cache);
        break;
    case Opcode::ConvertF64U32:
        OperationConvert(inst, false, Type::F64, true, inst_values, cache);
        break;
    default:
        break;
    }
}

static bool IsSelectInst(Inst* inst) {
    switch (inst->GetOpcode()) {
    case Opcode::SelectU1:
    case Opcode::SelectU8:
    case Opcode::SelectU16:
    case Opcode::SelectU32:
    case Opcode::SelectU64:
    case Opcode::SelectF32:
    case Opcode::SelectF64:
        return true;
    default:
        return false;
    }
}

void ComputeImmValues(const Value& value, ImmValueList& values, ComputeImmValuesCache& cache) {
    Value resolved = value.Resolve();
    if (ImmValue::IsSupportedValue(resolved)) {
        values.insert(ImmValue(resolved));
        return;
    }
    if (resolved.Type() != Type::Opaque) {
        return;
    }
    Inst* inst = resolved.InstRecursive();
    auto it = cache.find(inst);
    if (it != cache.end()) {
        values.insert(it->second.begin(), it->second.end());
        return;
    }
    auto& inst_values = cache.emplace(inst, ImmValueList{}).first->second;
    if (inst->GetOpcode() == Opcode::Phi) {
        for (size_t i = 0; i < inst->NumArgs(); ++i) {
            ComputeImmValues(inst->Arg(i), inst_values, cache);
        }
    }
    if (IsSelectInst(inst)) {
        ComputeImmValues(inst->Arg(1), inst_values, cache);
        ComputeImmValues(inst->Arg(2), inst_values, cache);
    } else {
        DoInstructionOperation(inst, inst_values, cache);
    }
    values.insert(inst_values.begin(), inst_values.end());
}

} // namespace Shader::IR
