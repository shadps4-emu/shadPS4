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
IR::Inst* FindDominantInstruction(const Container& insts, const IR::Block& current_parent) {
    IR::Inst* result = nullptr;

    for (IR::Inst* inst : insts) {
        const IR::Block* block = inst->GetParent();
        ASSERT(block->cfg_block);

        bool dominated = false;

        for (IR::Inst* other : insts) {
            const IR::Block* dominator = other->GetParent();
            ASSERT(dominator->cfg_block);

            if (inst != other && Gcn::IsCfgBlockDominatedBy(dominator->cfg_block, block->cfg_block,
                                                            current_parent.cfg_block)) {
                dominated = true;
                break;
            }
        }

        if (!dominated) {
            if (result != nullptr) {
                return nullptr;
            }
            result = inst;
        }
    }

    return result;
}

} // namespace Shader::Gcn