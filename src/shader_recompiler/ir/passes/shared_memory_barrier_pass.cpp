// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

void SharedMemoryBarrierPass(IR::Program& program, const Profile& profile) {
    if (!program.info.uses_shared || !profile.needs_lds_barriers) {
        return;
    }
    using Type = IR::AbstractSyntaxNode::Type;
    u32 branch_depth{};
    for (const IR::AbstractSyntaxNode& node : program.syntax_list) {
        if (node.type == Type::EndIf) {
            --branch_depth;
            continue;
        }
        if (node.type != Type::If) {
            continue;
        }
        u32 curr_depth = branch_depth++;
        if (curr_depth != 0) {
            continue;
        }
        const IR::U1 cond = node.data.if_node.cond;
        const auto insert_barrier =
            IR::BreadthFirstSearch(cond, [](IR::Inst* inst) -> std::optional<bool> {
                if (inst->GetOpcode() == IR::Opcode::GetAttributeU32 &&
                    inst->Arg(0).Attribute() == IR::Attribute::LocalInvocationId) {
                    return true;
                }
                return std::nullopt;
            });
        if (insert_barrier) {
            IR::Block* const merge = node.data.if_node.merge;
            auto insert_point = std::ranges::find_if_not(merge->Instructions(), IR::IsPhi);
            IR::IREmitter ir{*merge, insert_point};
            ir.Barrier();
        }
    }
}

} // namespace Shader::Optimization
