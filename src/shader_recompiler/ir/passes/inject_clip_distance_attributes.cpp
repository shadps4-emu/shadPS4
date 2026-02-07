// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"

namespace Shader {

void InjectClipDistanceAttributes(IR::Program& program, RuntimeInfo& runtime_info) {
    auto& info = runtime_info.fs_info;

    if (!info.clip_distance_emulation || program.info.l_stage != LogicalStage::Fragment) {
        return;
    }

    auto* first_block = *program.blocks.begin();
    auto it = std::ranges::find_if(first_block->Instructions(), [](const IR::Inst& inst) {
        return inst.GetOpcode() == IR::Opcode::Prologue;
    });
    ASSERT(it != first_block->end());
    ++it;
    ASSERT(it != first_block->end());
    ++it;

    IR::IREmitter ir{*first_block, it};

    // We don't know how many clip distances are exported by VS as it is not processed at this point
    // yet. Here is an assumption that we will have not more than 4 of them (while max is 8) to save
    // one attributes export slot.
    const auto attrib = IR::Attribute::Param0 + info.num_inputs;
    for (u32 comp = 0; comp < MaxEmulatedClipDistances; ++comp) {
        const auto attr_read = ir.GetAttribute(attrib, comp);
        const auto cond_id = ir.FPLessThan(attr_read, ir.Imm32(0.0f));
        ir.Discard(cond_id);
    }
    ++info.num_inputs;
}

} // namespace Shader
