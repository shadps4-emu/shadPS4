// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_map>

#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

static bool IsSharedMemoryInst(const IR::Inst& inst) {
    const auto opcode = inst.GetOpcode();
    return opcode == IR::Opcode::LoadSharedU32 || opcode == IR::Opcode::LoadSharedU64 ||
           opcode == IR::Opcode::WriteSharedU32 || opcode == IR::Opcode::WriteSharedU64;
}

static u32 GetSharedMemImmOffset(const IR::Inst& inst) {
    const auto* address = inst.Arg(0).InstRecursive();
    ASSERT(address->GetOpcode() == IR::Opcode::IAdd32);
    const auto ir_offset = address->Arg(1);
    ASSERT_MSG(ir_offset.IsImmediate());
    const auto offset = ir_offset.U32();
    // Typical usage is the compiler spilling registers into shared memory, with 256 bytes between
    // each register to account for 4 bytes per register times 64 threads per group. Ensure that
    // this assumption holds, as if it does not this approach may need to be revised.
    ASSERT_MSG(offset % 256 == 0, "Unexpected shared memory offset alignment: {}", offset);
    return offset;
}

static void ConvertSharedMemToVgpr(IR::IREmitter& ir, IR::Inst& inst, const IR::VectorReg vgpr) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::LoadSharedU32:
        inst.ReplaceUsesWithAndRemove(ir.GetVectorReg(vgpr));
        break;
    case IR::Opcode::LoadSharedU64:
        inst.ReplaceUsesWithAndRemove(
            ir.CompositeConstruct(ir.GetVectorReg(vgpr), ir.GetVectorReg(vgpr + 1)));
        break;
    case IR::Opcode::WriteSharedU32:
        ir.SetVectorReg(vgpr, IR::U32{inst.Arg(1)});
        inst.Invalidate();
        break;
    case IR::Opcode::WriteSharedU64: {
        const auto value = inst.Arg(1);
        ir.SetVectorReg(vgpr, IR::U32{ir.CompositeExtract(value, 0)});
        ir.SetVectorReg(vgpr, IR::U32{ir.CompositeExtract(value, 1)});
        inst.Invalidate();
        break;
    }
    default:
        UNREACHABLE_MSG("Unknown shared memory opcode: {}", inst.GetOpcode());
    }
}

void LowerSharedMemToRegisters(IR::Program& program, const RuntimeInfo& runtime_info) {
    u32 next_vgpr_num = runtime_info.num_allocated_vgprs;
    std::unordered_map<u32, IR::VectorReg> vgpr_map;
    const auto get_vgpr = [&next_vgpr_num, &vgpr_map](const u32 offset) {
        const auto [it, is_new] = vgpr_map.try_emplace(offset);
        if (is_new) {
            ASSERT_MSG(next_vgpr_num < 256, "Out of VGPRs");
            const auto new_vgpr = static_cast<IR::VectorReg>(next_vgpr_num++);
            it->second = new_vgpr;
        }
        return it->second;
    };

    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (!IsSharedMemoryInst(inst)) {
                continue;
            }
            const auto offset = GetSharedMemImmOffset(inst);
            const auto vgpr = get_vgpr(offset);
            IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
            ConvertSharedMemToVgpr(ir, inst, vgpr);
        }
    }
}

} // namespace Shader::Optimization
