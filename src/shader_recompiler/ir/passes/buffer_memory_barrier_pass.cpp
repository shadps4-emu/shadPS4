// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <vector>
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/passes/resource_pass.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

static bool IsLoadBuffer(const IR::Inst& inst) {
    const IR::Opcode opcode = inst.GetOpcode();
    return opcode >= IR::Opcode::LoadBufferU8 && opcode <= IR::Opcode::LoadBufferFormatF32;
}

static void EmitBufferExchangeBarriers(IR::Program& program) {
    using Type = IR::AbstractSyntaxNode::Type;
    u32 nesting_depth{};
    std::vector<bool> pending_writes(program.info.buffers.size());
    std::vector<bool> waited_writes(program.info.buffers.size());
    for (const IR::AbstractSyntaxNode& node : program.syntax_list) {
        switch (node.type) {
        case Type::If:
        case Type::Loop:
            ++nesting_depth;
            break;
        case Type::EndIf:
        case Type::Repeat:
            --nesting_depth;
            break;
        case Type::Block:
            for (IR::Inst& inst : node.data.block->Instructions()) {
                if (inst.GetOpcode() == IR::Opcode::WaitBufferMemory) {
                    if (nesting_depth == 0) {
                        for (size_t i = 0; i < pending_writes.size(); ++i) {
                            waited_writes[i] = waited_writes[i] || pending_writes[i];
                            pending_writes[i] = false;
                        }
                    }
                    continue;
                }
                if ((!IsBufferStore(inst) && !IsLoadBuffer(inst)) || !inst.Arg(0).IsImmediate()) {
                    continue;
                }
                const u32 handle = inst.Arg(0).U32();
                ASSERT(handle < pending_writes.size());
                if (IsBufferStore(inst)) {
                    pending_writes[handle] = true;
                    waited_writes[handle] = false;
                } else if (nesting_depth == 0 && waited_writes[handle]) {
                    IR::IREmitter ir{*node.data.block,
                                     IR::Block::InstructionList::s_iterator_to(inst)};
                    ir.WorkgroupMemoryBarrier();
                    ir.Barrier();
                    ir.WorkgroupMemoryBarrier();
                    std::ranges::fill(pending_writes, false);
                    std::ranges::fill(waited_writes, false);
                }
            }
            break;
        case Type::Return:
        case Type::Unreachable:
            return;
        default:
            break;
        }
    }
}

void BufferMemoryBarrierPass(IR::Program& program, const RuntimeInfo& runtime_info,
                             const Profile& profile) {
    if (program.info.hw_stage == HwStage::Compute && profile.needs_lds_barriers) {
        const auto& size = runtime_info.hw.cs.workgroup_size;
        if (u64(size[0]) * size[1] * size[2] == 64) {
            EmitBufferExchangeBarriers(program);
        }
    }
    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() == IR::Opcode::WaitBufferMemory) {
                inst.Invalidate();
            }
        }
    }
}

} // namespace Shader::Optimization
