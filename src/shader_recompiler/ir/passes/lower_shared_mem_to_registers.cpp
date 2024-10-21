// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/container/small_vector.hpp>
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

void LowerSharedMemToRegisters(IR::Program& program) {
    boost::container::small_vector<IR::Inst*, 8> ds_writes;
    Info& info{program.info};
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            const auto opcode = inst.GetOpcode();
            if (opcode == IR::Opcode::WriteSharedU32 || opcode == IR::Opcode::WriteSharedU64) {
                ds_writes.emplace_back(&inst);
                continue;
            }
            if (opcode == IR::Opcode::LoadSharedU32 || opcode == IR::Opcode::LoadSharedU64) {
                // Search for write instruction with same offset
                const IR::Inst* prod = inst.Arg(0).InstRecursive();
                const auto it = std::ranges::find_if(ds_writes, [&](const IR::Inst* write) {
                    const IR::Inst* write_prod = write->Arg(0).InstRecursive();
                    return write_prod->Arg(1).U32() == prod->Arg(1).U32();
                });
                ASSERT(it != ds_writes.end());
                // Replace data read with value written.
                inst.ReplaceUsesWithAndRemove((*it)->Arg(1));
            }
        }
    }
    // We should have eliminated everything. Invalidate data write instructions.
    for (const auto inst : ds_writes) {
        inst->Invalidate();
    }
}

} // namespace Shader::Optimization
