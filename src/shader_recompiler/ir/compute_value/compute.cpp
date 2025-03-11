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
            CartesianInvokeImpl<Func, OutputIt, N, Level + 1, ArgLists...>(func, out_it, arglists_its, arglists_tuple);
        }
    }
}

template <typename Func, typename OutputIt, typename... ArgLists>
static void CartesianInvoke(Func func, OutputIt out_it, const ArgLists&... arg_lists) {
    constexpr size_t N = sizeof...(ArgLists);
    const std::tuple<const ArgLists&...> arglists_tuple = std::forward_as_tuple(arg_lists...);

    std::tuple<typename ArgLists::const_iterator...> arglists_it;
    CartesianInvokeImpl<Func, OutputIt, N, 0, ArgLists...>(func, out_it, arglists_it, arglists_tuple);
}

static void SetSigned(ImmValueList& values, bool is_signed) {
    for (auto& value : values) {
        value.SetSigned(is_signed);
    }
}

static void OperationAbs(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) {
        return a.abs();
    };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationAdd(Inst* inst, bool is_signed, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) {
        return a + b;
    };

    SetSigned(args0, is_signed);
    SetSigned(args1, is_signed);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationSub(Inst* inst, bool is_signed, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) {
        return a - b;
    };

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

static void OperationMin(Inst* inst, bool is_signed, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
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
            if (a.isnan()) return b;
            if (b.isnan()) return a;
        }
        return std::min(a, b);
    };

    SetSigned(args0, is_signed);
    SetSigned(args1, is_signed);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1, is_legacy_args);
}

static void OperationMax(Inst* inst, bool is_signed, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
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
            if (a.isnan()) return b;
            if (b.isnan()) return a;
        }
        return std::max(a, b);
    };

    SetSigned(args0, is_signed);
    SetSigned(args1, is_signed);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1, is_legacy_args);
}

static void OperationMul(Inst* inst, bool is_signed, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) {
        return a * b;
    };

    SetSigned(args0, is_signed);
    SetSigned(args1, is_signed);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationDiv(Inst* inst, bool is_signed, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) {
        return a / b;
    };

    SetSigned(args0, is_signed);
    SetSigned(args1, is_signed);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationNeg(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) {
        return -a;
    };

    SetSigned(args, true);
    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationRecip(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) {
        return a.recip();
    };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationRecipSqrt(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) {
        return a.rsqrt();
    };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationSqrt(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) {
        return a.sqrt();
    };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationSin(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) {
        return a.sin();
    };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationExp2(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) {
        return a.exp2();
    };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationLdexp(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) {
        return a.ldexp(ImmU32(b));
    };

    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationCos(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) {
        return a.cos();
    };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationLog2(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) {
        return a.log2();
    };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationClamp(Inst* inst, bool is_signed, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
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

    const auto op = [](const ImmValue& a) {
        return a.round();
    };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationFloor(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) {
        return a.floor();
    };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationCeil(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) {
        return a.ceil();
    };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationTrunc(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) {
        return a.trunc();
    };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationFract(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) {
        return a.fract();
    };
    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationShiftLeft(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) {
        return a << ImmU32(b);
    };

    SetSigned(args1, false);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationShiftRight(Inst* inst, bool is_signed, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) {
        return a >> ImmU32(b);
    };

    SetSigned(args0, is_signed);
    SetSigned(args1, false);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationBitwiseNot(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [](const ImmValue& a) {
        return ~a;
    };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationBitwiseAnd(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) {
        return a & b;
    };

    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationBitwiseOr(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) {
        return a | b;
    };

    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationBitwiseXor(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) {
        return a ^ b;
    };

    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void OperationConvert(Inst* inst, bool is_signed, Type new_type, bool new_signed, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [new_type, new_signed](const ImmValue& a) {
        return a.Convert(new_type, new_signed);
    };

    SetSigned(args, is_signed);
    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

static void OperationBitCast(Inst* inst, Type new_type, bool new_signed, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args;
    ComputeImmValues(inst->Arg(0), args, cache);

    const auto op = [new_type, new_signed](const ImmValue& a) {
        return a.Bitcast(new_type, new_signed);
    };

    std::transform(args.begin(), args.end(), std::inserter(inst_values, inst_values.begin()), op);
}

template<size_t N>
static void OperationCompositeConstruct(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    std::array<ImmValueList, N> args;
    for (size_t i = 0; i < N; ++i) {
        ComputeImmValues(inst->Arg(i), args[i], cache);
    }

    const auto op = []<typename... Args>(const Args&... args) {
        return ImmValue(args...);
    };

    const auto call_cartesian = [&]<size_t... I>(std::index_sequence<I...>) {
        CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args[I]...);
    };
    call_cartesian(std::make_index_sequence<N>{});
}

static void OperationCompositeExtract(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    ImmValueList args0, args1;
    ComputeImmValues(inst->Arg(0), args0, cache);
    ComputeImmValues(inst->Arg(1), args1, cache);

    const auto op = [](const ImmValue& a, const ImmValue& b) {
        return a.Extract(ImmU32(b));
    };

    SetSigned(args1, false);
    CartesianInvoke(op, std::inserter(inst_values, inst_values.begin()), args0, args1);
}

static void DoInstructionOperation(Inst* inst, ImmValueList& inst_values, ComputeImmValuesCache& cache) {
    switch (inst->GetOpcode()) {
        default:
            break;
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
    } else {

    }
    values.insert(inst_values.begin(), inst_values.end());
}

} // namespace Shader::IR
