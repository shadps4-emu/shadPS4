// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

void DeadCodeEliminationPass(IR::Program& program) {
    // We iterate over the instructions in reverse order.
    // This is because removing an instruction reduces the number of uses for earlier instructions.
    for (IR::Block* const block : program.post_order_blocks) {
        auto it{block->end()};
        while (it != block->begin()) {
            --it;
            if (!it->HasUses() && !it->MayHaveSideEffects()) {
                it->Invalidate();
                it = block->Instructions().erase(it);
            }
        }
    }
}

} // namespace Shader::Optimization
