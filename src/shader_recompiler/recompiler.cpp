// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_map>

#include "common/logging/classes.h"
#include "shader_recompiler/frontend/control_flow_graph.h"
#include "shader_recompiler/frontend/decode.h"
#include "shader_recompiler/frontend/structured_control_flow.h"
#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/post_order.h"
#include "shader_recompiler/ir/program.h"
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
    for (auto& block : cfg) {
        const u32 start = block.begin_index;
        const u32 size = block.end_index - start + 1;
        auto* ir_block = pools.block_pool.Create(pools.inst_pool);
        ir_block->cfg_block = &block;
        block.ir_block = ir_block;
        translator.Translate(ir_block, block.begin, block.cond,
                             std::span{program.ins_list}.subspan(start, size));
        program.blocks.push_back(ir_block);
    }
    translator.EmitPrologue(program.blocks.front());
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

IR::Program CloneProgram(const IR::Program& program, Pools& pools) {
    IR::Program new_program{program.info};
    new_program.ins_list = program.ins_list;
    new_program.next_reg_index = program.next_reg_index;

    std::unordered_map<const IR::Block*, IR::Block*> block_map;
    new_program.blocks.reserve(program.blocks.size());
    for (const auto* block : program.blocks) {
        auto* new_block = pools.block_pool.Create(pools.inst_pool);
        new_block->cfg_block = block->cfg_block;
        new_block->SetDefinition(block->Definition<u32>());
        new_block->po_index = block->po_index;
        block_map[block] = new_block;
        new_program.blocks.push_back(new_block);
    }

    for (const auto* block : program.blocks) {
        auto* new_block = block_map[block];
        new_block->imm_predecessors.reserve(block->imm_predecessors.size());
        for (const auto* pred : block->imm_predecessors) {
            new_block->imm_predecessors.push_back(block_map[pred]);
        }
        new_block->imm_successors.reserve(block->imm_successors.size());
        for (const auto* succ : block->imm_successors) {
            new_block->imm_successors.push_back(block_map[succ]);
        }
    }

    new_program.post_order_blocks.reserve(program.post_order_blocks.size());
    for (const auto* block : program.post_order_blocks) {
        new_program.post_order_blocks.push_back(block_map[block]);
    }

    std::unordered_map<const IR::Inst*, IR::Inst*> inst_map;
    for (const auto* block : program.blocks) {
        auto* new_block = block_map[block];
        for (const auto& inst : block->Instructions()) {
            ASSERT_MSG(inst.GetOpcode() != IR::Opcode::Phi, "Cloning phi node");
            auto* new_inst = pools.inst_pool.Create(inst.GetOpcode(), inst.Flags<u32>());
            new_inst->SetFlags(inst.Flags<u64>());
            new_inst->SetParent(new_block);
            new_inst->SetDefinition(inst.Definition<u32>());
            new_inst->SetRegTag(inst.GetRegTag());
            inst_map[&inst] = new_inst;
            new_block->Instructions().push_back(*new_inst);
        }
    }

    for (const auto* block : program.blocks) {
        auto* new_block = block_map[block];
        for (const auto& inst : block->Instructions()) {
            auto* new_inst = inst_map[&inst];
            const size_t num_args = inst.NumArgs();
            for (size_t i = 0; i < num_args; ++i) {
                const auto& arg = inst.Arg(i);
                if (const auto* arg_inst = arg.TryInst()) {
                    auto it = inst_map.find(arg_inst);
                    ASSERT_MSG(it != inst_map.end(), "Argument instruction not found in clone map");
                    new_inst->SetArg(i, IR::Value{it->second});
                } else {
                    new_inst->SetArg(i, arg);
                }
            }
        }
        if (const auto* cond_inst = block->branch_cond.TryInst()) {
            auto it = inst_map.find(cond_inst);
            ASSERT_MSG(it != inst_map.end(), "Branch condition instruction not found in clone map");
            new_block->branch_cond = IR::U1{IR::Value{it->second}};
        } else {
            new_block->branch_cond = block->branch_cond;
        }
    }

    return new_program;
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
    if (info.has_movrel) {
        Pools temp_pools;
        IR::Program temp_program = CloneProgram(program, temp_pools);
        Shader::Optimization::SsaRewritePass(temp_program);
        Shader::Optimization::ConstantPropagationPass(temp_program.post_order_blocks);

        const auto movrel_bounds =
            Shader::Optimization::AnalyzeMovRelBounds(temp_program, runtime_info);

        Shader::Optimization::LowerMovRelPass(program, runtime_info, movrel_bounds);
    }
    Shader::Optimization::SsaRewritePass(program);
    Shader::Optimization::ConstantPropagationPass(program.post_order_blocks);
    Shader::Optimization::ReadLaneEliminationPass(program);
    if (info.sw_stage == SwStage::TessellationControl) {
        Shader::Optimization::TessellationPreprocess(program, runtime_info);
        Shader::Optimization::HullShaderTransform(program, runtime_info);
    } else if (info.sw_stage == SwStage::TessellationEval) {
        Shader::Optimization::TessellationPreprocess(program, runtime_info);
        Shader::Optimization::DomainShaderTransform(program, runtime_info);
    }
    Shader::Optimization::RingAccessElimination(program, runtime_info);
    Shader::IR::DumpProgram(program, info, "pre-res-discover.");
    auto resources = Shader::Optimization::ResourceDiscoverPass(program, profile);
    Shader::Optimization::FlattenExtendedUserdataPass(program, profile);
    Shader::IR::DumpProgram(program, info, "pre-res-patch.");
    Shader::Optimization::ResourcePatchingPass(program.info, resources, profile);
    Shader::Optimization::LowerBufferFormatToRaw(program);
    Shader::Optimization::SharedMemorySimplifyPass(program, profile);
    Shader::Optimization::SharedMemoryToStoragePass(program, runtime_info, profile);
    Shader::Optimization::LowerUserClipPlanes(program, runtime_info);
    Shader::Optimization::PhiSimplificationPass(program);
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
    Shader::Optimization::LowerHardwareIntrinsics(program);
    Shader::Optimization::ConstantPropagationPass(program.post_order_blocks);
    Shader::Optimization::DeadCodeEliminationPass(program);
    Shader::Optimization::CollectShaderInfoPass(program, profile);
    Shader::IR::DumpProgram(program, info);

    return program;
}

} // namespace Shader
