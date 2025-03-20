// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/cartesian_invoke.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/compute_value/compute.h"
#include "shader_recompiler/ir/num_executions.h"

namespace Shader::IR {

static bool Is64BitCondition(const Inst* inst) {
    switch (inst->GetOpcode()) {
    case Opcode::SLessThan64:
    case Opcode::ULessThan64:
    case Opcode::IEqual64:
    case Opcode::INotEqual64:
        return true;
    default:
        return false;
    }
}

static u64 GetDistance32(const ComputeValue::ImmValue& a, const ComputeValue::ImmValue& b) {
    return a.U32() < b.U32() ? b.U32() - a.U32() : a.U32() - b.U32();
}

static u64 GetDistance64(const ComputeValue::ImmValue& a, const ComputeValue::ImmValue& b) {
    return a.U64() < b.U64() ? b.U64() - a.U64() : a.U64() - b.U64();
}

u64 GetNumExecutions(const Inst* inst) {
    u64 num_executions = 1;
    const auto* cond_data = &inst->GetParent()->CondData();
    while (cond_data->asl_node) {
        if (cond_data->asl_node->type == AbstractSyntaxNode::Type::Loop) {
            ComputeValue::ImmValueList cond_arg0, cond_arg1;
            ComputeValue::Cache cache;
            Block* cont_block = cond_data->asl_node->data.loop.continue_block;
            Inst* cond_inst = cont_block->back().Arg(0).InstRecursive();
            ASSERT(cond_inst);
            ComputeValue::Compute(cond_inst->Arg(0), cond_arg0, cache);
            ComputeValue::Compute(cond_inst->Arg(1), cond_arg1, cache);
            std::unordered_set<u64> distances;
            if (Is64BitCondition(cond_inst)) {
                Common::CartesianInvoke(GetDistance64,
                                        std::insert_iterator(distances, distances.end()), cond_arg0,
                                        cond_arg1);
            } else {
                Common::CartesianInvoke(GetDistance32,
                                        std::insert_iterator(distances, distances.end()), cond_arg0,
                                        cond_arg1);
            }
            num_executions *=
                std::max<u64>(1, *std::max_element(distances.begin(), distances.end()));
        }
        cond_data = cond_data->parent;
    }
    return num_executions;
}

} // namespace Shader::IR