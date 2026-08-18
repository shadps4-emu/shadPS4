// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/frontend/control_flow_graph.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/value.h"

namespace Shader::Gcn {

bool IsCfgBlockDominatedBy(const Shader::Gcn::Block* maybe_dominator,
                           const Shader::Gcn::Block* block, const Shader::Gcn::Block* dest_block);

template <typename Container>
    requires std::same_as<typename Container::value_type, IR::Inst*>
void FindDominantInstruction(Container& insts, const IR::Block& current_parent) {
    size_t num_insts = insts.size();
    for (s32 i = 0; i < num_insts;) {
        const IR::Block* block = insts[i]->GetParent();
        ASSERT(block->cfg_block);
        bool was_removed = false;
        for (s32 j = 0; j < num_insts;) {
            const IR::Block* dominator = insts[j]->GetParent();
            ASSERT(dominator->cfg_block);
            if (i != j && Gcn::IsCfgBlockDominatedBy(dominator->cfg_block, block->cfg_block,
                                                     current_parent.cfg_block)) {
                std::swap(insts[i], insts[num_insts - 1]);
                --num_insts;
                insts.pop_back();
                was_removed = true;
                break;
            } else {
                ++j;
            }
        }
        if (!was_removed) {
            ++i;
        }
    }
}

} // namespace Shader::Gcn