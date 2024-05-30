// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

void IdentityRemovalPass(IR::BlockList& program) {
    std::vector<IR::Inst*> to_invalidate;
    for (IR::Block* const block : program) {
        for (auto inst = block->begin(); inst != block->end();) {
            const size_t num_args{inst->NumArgs()};
            for (size_t i = 0; i < num_args; ++i) {
                IR::Value arg;
                while ((arg = inst->Arg(i)).IsIdentity()) {
                    inst->SetArg(i, arg.Inst()->Arg(0));
                }
            }
            if (inst->GetOpcode() == IR::Opcode::Identity ||
                inst->GetOpcode() == IR::Opcode::Void) {
                to_invalidate.push_back(&*inst);
                inst = block->Instructions().erase(inst);
            } else {
                ++inst;
            }
        }
    }
    for (IR::Inst* const inst : to_invalidate) {
        inst->Invalidate();
    }
}

} // namespace Shader::Optimization
