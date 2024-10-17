// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/io_file.h"
#include "common/path_util.h"
#include "shader_recompiler/frontend/control_flow_graph.h"
#include "shader_recompiler/frontend/decode.h"
#include "shader_recompiler/frontend/structured_control_flow.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/post_order.h"
#include "shader_recompiler/recompiler.h"

namespace Shader {

IR::BlockList GenerateBlocks(const IR::AbstractSyntaxList& syntax_list) {
    size_t num_syntax_blocks{};
    for (const auto& node : syntax_list) {
        if (node.type == IR::AbstractSyntaxNode::Type::Block) {
            ++num_syntax_blocks;
        }
    }
    IR::BlockList blocks;
    blocks.reserve(num_syntax_blocks);
    u32 order_index{};
    for (const auto& node : syntax_list) {
        if (node.type == IR::AbstractSyntaxNode::Type::Block) {
            blocks.push_back(node.data.block);
        }
    }
    return blocks;
}

IR::Program TranslateProgram(std::span<const u32> code, Pools& pools, Info& info,
                             const RuntimeInfo& runtime_info, const Profile& profile) {
    // Ensure first instruction is expected.
    constexpr u32 token_mov_vcchi = 0xBEEB03FF;
    ASSERT_MSG(code[0] == token_mov_vcchi, "First instruction is not s_mov_b32 vcc_hi, #imm");

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

    bool dump_ir = false;
    bool extra_id_removal = true; // TODO remove all this stuff
    if (true /*info.pgm_hash == 0x6fd3463f*/) {
        dump_ir = true;
    }
    auto dumpMatchingIR = [&](std::string phase) {
        if (dump_ir) {
            if (Config::dumpShaders()) {
                std::string s = IR::DumpProgram(program);
                using namespace Common::FS;
                const auto dump_dir = GetUserPath(PathType::ShaderDir) / "dumps";
                if (!std::filesystem::exists(dump_dir)) {
                    std::filesystem::create_directories(dump_dir);
                }
                const auto filename = fmt::format("{}_{:#018x}_{}.{}.ir.txt", info.stage,
                                                  info.pgm_hash, info.perm_idx, phase);
                const auto file = IOFile{dump_dir / filename, FileAccessMode::Write};
                file.WriteString(s);
            }
        }
    };

    // Structurize control flow graph and create program.
    program.syntax_list = Shader::Gcn::BuildASL(pools.inst_pool, pools.block_pool, cfg,
                                                program.info, runtime_info, profile);
    program.blocks = GenerateBlocks(program.syntax_list);
    program.post_order_blocks = Shader::IR::PostOrder(program.syntax_list.front());

    // Run optimization passes
    dumpMatchingIR("pre_ssa");
    Shader::Optimization::SsaRewritePass(program.post_order_blocks);
    dumpMatchingIR("pre_const_prop");
    Shader::Optimization::ConstantPropagationPass(program.post_order_blocks);
    if (program.info.stage != Stage::Compute) {
        Shader::Optimization::LowerSharedMemToRegisters(program);
    }
    Shader::Optimization::RingAccessElimination(program, runtime_info, program.info.stage);
    dumpMatchingIR("pre_hoist_pre_id");
    // Shader::Optimization::IdentityRemovalPass(program.blocks); // temp
    if (extra_id_removal) {
        Shader::Optimization::IdentityRemovalPass(program.blocks); // temp
    }
    dumpMatchingIR("pre_flatten");
    Shader::Optimization::FlattenExtendedUserdataPass(program);
    dumpMatchingIR("pre_resource_tracking");
    Shader::Optimization::ResourceTrackingPass(program);
    Shader::Optimization::IdentityRemovalPass(program.blocks);
    dumpMatchingIR("pre_dce");
    Shader::Optimization::DeadCodeEliminationPass(program);
    Shader::Optimization::CollectShaderInfoPass(program);
    dumpMatchingIR("final");

    return program;
}

} // namespace Shader
