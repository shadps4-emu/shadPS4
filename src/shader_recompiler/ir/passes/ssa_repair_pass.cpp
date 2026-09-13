// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/container/small_vector.hpp>

#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/dominance.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

void SsaRepairPass(IR::Program& program) {
    // Compute dominance on the current CFG
    auto& post_order = program.post_order_blocks;
    IR::ComputeDominators(post_order);
    IR::ComputeDominanceFrontiers(post_order);

    boost::container::small_vector<IR::Use, 2> invalid_uses;
    for (IR::Block* block : post_order) {
        for (IR::Inst& inst : block->Instructions()) {
            if (!inst.HasUses()) {
                continue;
            }

            for (const auto& use : inst.Uses()) {
                IR::Block* use_block = use.user->GetParent();
                if (!Dominates(block, use_block)) {
                    invalid_uses.push_back(use);
                }
            }
            if (invalid_uses.empty()) {
                continue;
            }

            // Add a store to the virtual register
            const IR::VirtualReg reg{program.next_reg_index++, inst.Type()};
            IR::IREmitter ir{*block, ++IR::Block::InstructionList::s_iterator_to(inst)};
            ir.SetVirtualReg(reg, IR::Value{&inst});

            // Replace all invalid uses with load to virtual register
            for (auto& [user, operand] : invalid_uses) {
                IR::IREmitter ir{*user->GetParent(),
                                 IR::Block::InstructionList::s_iterator_to(*user)};
                user->SetArg(operand, ir.GetVirtualReg(reg));
            }

            invalid_uses.clear();
        }
    }
}

} // namespace Shader::Optimization