// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/classes.h"
#include "shader_recompiler/frontend/control_flow_graph.h"
#include "shader_recompiler/frontend/decode.h"
#include "shader_recompiler/frontend/structured_control_flow.h"
#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/ir/attribute.h"
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/post_order.h"
#include "shader_recompiler/profile.h"
#include "shader_recompiler/recompiler.h"

namespace Shader {

IR::BlockList GenerateBlocks(const IR::AbstractSyntaxList& syntax_list) {
    size_t num_syntax_blocks{};
    for (const auto& [_, type] : syntax_list) {
        if (type == IR::AbstractSyntaxNode::Type::Block) {
            ++num_syntax_blocks;
        }
    }
    IR::BlockList blocks{};
    blocks.reserve(num_syntax_blocks);
    for (const auto& [data, type] : syntax_list) {
        if (type == IR::AbstractSyntaxNode::Type::Block) {
            blocks.push_back(data.block);
        }
    }
    return blocks;
}

void EmitControlFlowGraph(IR::Program& program, Pools& pools, Gcn::CFG& cfg,
                          RuntimeInfo& runtime_info, const Profile& profile) {
    Gcn::Translator translator{program.info, runtime_info, profile};
    bool emit_prologue = true;
    for (auto& block : cfg) {
        const u32 start = block.begin_index;
        const u32 size = block.end_index - start + 1;
        auto* ir_block = pools.block_pool.Create(pools.inst_pool);
        ir_block->cfg_block = &block;
        block.ir_block = ir_block;
        translator.Translate(ir_block, block.begin, block.cond,
                             std::span{program.ins_list}.subspan(start, size));
        if (emit_prologue) {
            translator.EmitPrologue(ir_block);
            emit_prologue = false;
        }
        program.blocks.push_back(ir_block);
    }
    ASSERT_MSG(!program.info.translation_failed, "Shader translation has failed");
    for (auto& block : cfg) {
        auto* ir_block = block.ir_block;
        if (block.branch_true) {
            auto* true_block = block.branch_true->ir_block;
            ir_block->AddBranch(true_block);
        }
        if (block.branch_false) {
            auto* false_block = block.branch_false->ir_block;
            ir_block->AddBranch(false_block);
        }
    }
    program.post_order_blocks = Shader::IR::PostOrder(program.blocks.front());
}

void LowerLdsSpillsToRegistersPass(IR::Program& program, const RuntimeInfo& runtime_info) {
    if (program.info.stage != Stage::Compute) {
        return;
    }

    struct DsInst {
        IR::Inst* inst;
        u32 offset;
    };
    boost::container::small_vector<DsInst, 16> ds_insts;
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() != IR::Opcode::WriteSharedU32 &&
                inst.GetOpcode() != IR::Opcode::LoadSharedU32) {
                    continue;
                }

            u32 offset{};
            IR::Value addr = inst.Arg(0);
            if (auto* prod = addr.TryInst(); prod && prod->GetOpcode() == IR::Opcode::IAdd32) {
                if (!prod->Arg(1).IsImmediate()) {
                    continue;
                }
                offset = prod->Arg(1).U32();
                addr = prod->Arg(0);
            }
            if (addr.IsImmediate()) {
                continue;
            }
            IR::Inst* prod = addr.Inst();
            if (prod->GetOpcode() != IR::Opcode::ShiftLeftLogical32 || prod->Arg(0).IsImmediate() ||
                !prod->Arg(1).IsImmediate() ||prod->Arg(1).U32() != 2) {
                    continue;
                }
            IR::Inst* lane_id = prod->Arg(0).Inst();
            if (lane_id->GetOpcode() != IR::Opcode::LaneId) {
                continue;
            }

            ds_insts.emplace_back(&inst, offset);
        }
    }

    if (ds_insts.empty()) {
        return;
    }

    LOG_CRITICAL(Render_Recompiler, "SPILL PATCH TRIGGER FOR {:#x}", program.info.pgm_hash);
    std::unordered_map<u32, IR::VirtualReg> reg_map;
    for (auto [inst, offset] : ds_insts) {
        auto [it, new_reg] = reg_map.try_emplace(offset);
        if (new_reg) {
            it->second = IR::VirtualReg{program.next_reg_index++, IR::Type::U32};
        }
        IR::IREmitter ir{*inst->GetParent(), IR::Block::InstructionList::s_iterator_to(*inst)};
        if (inst->GetOpcode() == IR::Opcode::LoadSharedU32) {
            inst->ReplaceUsesWithAndRemove(ir.GetVirtualReg(it->second));
        } else {
            ir.SetVirtualReg(it->second, IR::U32{inst->Arg(1)});
            inst->Invalidate();
        }
    }

    for (auto* ir_block : program.blocks) {
        ir_block->ssa_state.Reset();
    }
    Shader::Optimization::SsaRewritePass(program);
}

IR::Program TranslateProgram(const std::span<const u32>& code, Pools& pools, Info& info,
                             RuntimeInfo& runtime_info, const Profile& profile) {
    // Ensure first instruction is expected.
    constexpr u32 token_mov_vcchi = 0xBEEB03FF;
    if (code[0] != token_mov_vcchi) {
        LOG_WARNING(Render_Recompiler, "First instruction is not s_mov_b32 vcc_hi, #imm");
    }

    Gcn::GcnCodeSlice slice(code.data(), code.data() + code.size());
    Gcn::GcnDecodeContext decoder;

    // Decode and save instructions
    IR::Program program{info};
    program.ins_list.reserve(code.size());
    while (!slice.atEnd()) {
        program.ins_list.emplace_back(decoder.decodeInstruction(slice));
    }

    // Clear any previous pooled data.
    pools.ReleaseContents();

    // Create control flow graph
    Common::ObjectPool<Gcn::Block> gcn_block_pool{64};
    Gcn::CFG cfg{gcn_block_pool, program.ins_list};
    EmitControlFlowGraph(program, pools, cfg, runtime_info, profile);

    // On NVIDIA GPUs HW interpolation of clip distance values seems broken, and we need to emulate
    // it with expensive discard in PS.
    Shader::InjectClipDistanceAttributes(program, runtime_info);

    // Run optimization passes on unstructured graph
    if (!profile.support_float64) {
        Shader::Optimization::LowerFp64ToFp32(program);
    }
    Shader::Optimization::SsaRewritePass(program);
    Shader::IR::DumpProgram(program, info, "post-ssa1.");
    Shader::Optimization::ConstantPropagationPass(program.post_order_blocks);
    LowerLdsSpillsToRegistersPass(program, runtime_info);
    if (info.l_stage == LogicalStage::TessellationControl) {
        Shader::Optimization::TessellationPreprocess(program, runtime_info);
        Shader::Optimization::HullShaderTransform(program, runtime_info);
    } else if (info.l_stage == LogicalStage::TessellationEval) {
        Shader::Optimization::TessellationPreprocess(program, runtime_info);
        Shader::Optimization::DomainShaderTransform(program, runtime_info);
    }
    Shader::Optimization::RingAccessElimination(program, runtime_info);
    Shader::Optimization::ReadLaneEliminationPass(program);
    auto resources = Shader::Optimization::ResourceDiscoverPass(program, profile);
    Shader::Optimization::FlattenExtendedUserdataPass(program);
    Shader::Optimization::ResourcePatchingPass(program.info, resources, profile);
    Shader::Optimization::LowerBufferFormatToRaw(program);
    Shader::Optimization::SharedMemorySimplifyPass(program, profile);
    Shader::Optimization::SharedMemoryToStoragePass(program, runtime_info, profile);
    Shader::Optimization::LowerUserClipPlanes(program, runtime_info);
    Shader::Optimization::PhiSimplificationPass(program);
    Shader::IR::DumpProgram(program, info, "pre-ballot-elim.");
    Shader::Optimization::InverseBallotEliminationPass(program);
    Shader::IR::DumpProgram(program, info, "pre-lower-phi.");

    // Prepare for structurization by clearing flow graph and lowering phis
    for (auto* ir_block : program.blocks) {
        ir_block->imm_predecessors.clear();
        ir_block->imm_successors.clear();
        ir_block->ssa_state.Reset();
    }
    Shader::Optimization::LowerPhisToRegsPass(program);

    // Structurize control flow graph and create program.
    program.syntax_list = Shader::Gcn::BuildASL(pools, cfg, info);
    program.blocks = GenerateBlocks(program.syntax_list);
    program.post_order_blocks = Shader::IR::PostOrder(program.syntax_list.front().data.block);

    // Run optimization passes on structured graph
    Shader::Optimization::SsaRepairPass(program);
    Shader::Optimization::SsaRewritePass(program);
    Shader::Optimization::SharedMemoryBarrierPass(program, runtime_info, profile);
    Shader::Optimization::DeadCodeEliminationPass(program);
    Shader::Optimization::LowerWave64BallotPass(program, runtime_info, profile);
    Shader::Optimization::ConstantPropagationPass(program.post_order_blocks);
    Shader::Optimization::CollectShaderInfoPass(program, profile);
    Shader::IR::DumpProgram(program, info);

    return program;
}

} // namespace Shader
