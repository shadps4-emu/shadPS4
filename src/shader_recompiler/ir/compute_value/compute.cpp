// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include "common/cartesian_invoke.h"
#include "common/func_traits.h"
#include "shader_recompiler/ir/compute_value/compute.h"
#include "shader_recompiler/ir/compute_value/do_bitcast.h"
#include "shader_recompiler/ir/compute_value/do_composite.h"
#include "shader_recompiler/ir/compute_value/do_convert.h"
#include "shader_recompiler/ir/compute_value/do_float_operations.h"
#include "shader_recompiler/ir/compute_value/do_integer_operations.h"
#include "shader_recompiler/ir/compute_value/do_logical_operations.h"
#include "shader_recompiler/ir/compute_value/do_nop_functions.h"
#include "shader_recompiler/ir/compute_value/do_packing.h"

namespace Shader::IR::ComputeValue {

template <auto func, size_t... I>
static void Invoke(ImmValueList& inst_values, const std::array<ImmValueList, sizeof...(I)>& args,
                   std::index_sequence<I...>) {
    func(inst_values, args[I]...);
}

template <auto func>
static void Invoke(Inst* inst, ImmValueList& inst_values, Cache& cache) {
    using Traits = Common::FuncTraits<decltype(func)>;
    constexpr size_t num_args = Traits::NUM_ARGS - 1;
    ASSERT(inst->NumArgs() >= num_args);
    std::array<ImmValueList, num_args> args{};
    for (size_t i = 0; i < num_args; ++i) {
        Compute(inst->Arg(i), args[i], cache);
    }
    Invoke<func>(inst_values, args, std::make_index_sequence<num_args>{});
}

static void DoInstructionOperation(Inst* inst, ImmValueList& inst_values, Cache& cache) {
    switch (inst->GetOpcode()) {
#define OPCODE(name, result_type, ...)                                                             \
    case Opcode::name:                                                                             \
        Invoke<&Do##name>(inst, inst_values, cache);                                               \
        break;
#include "shader_recompiler/ir/opcodes.inc"
#undef OPCODE
    default:
        UNREACHABLE_MSG("Invalid opcode: {}", inst->GetOpcode());
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

void Compute(const Value& value, ImmValueList& values, Cache& cache) {
    Value resolved = value.Resolve();
    if (ImmValue::IsSupportedValue(resolved)) {
        values.insert(ImmValue(resolved));
        return;
    }
    if (resolved.IsImmediate()) {
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
            Compute(inst->Arg(i), inst_values, cache);
        }
    } else if (IsSelectInst(inst)) {
        Compute(inst->Arg(1), inst_values, cache);
        Compute(inst->Arg(2), inst_values, cache);
    } else {
        DoInstructionOperation(inst, inst_values, cache);
    }
    values.insert(inst_values.begin(), inst_values.end());
}

} // namespace Shader::IR::ComputeValue