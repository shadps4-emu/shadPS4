// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"
#include "shader_recompiler/recompiler.h"
#include "shader_recompiler/runtime_info.h"

namespace {

using namespace Shader;

u32 CountBarriers(const IR::Block& block) {
    return static_cast<u32>(std::ranges::count_if(block.Instructions(), [](const IR::Inst& inst) {
        return inst.GetOpcode() == IR::Opcode::Barrier;
    }));
}

struct DivergentLoopProgram {
    Info info{};
    IR::Program program{info};
    Pools pools{};
    IR::Block* body = pools.block_pool.Create(pools.inst_pool);
    IR::Block* continue_block = pools.block_pool.Create(pools.inst_pool);
    IR::Block* merge = pools.block_pool.Create(pools.inst_pool);

    DivergentLoopProgram(bool divergent_break, bool divergent_repeat) {
        info.stage = Stage::Compute;
        program.info.stage = Stage::Compute;
        program.blocks = {body, continue_block, merge};

        IR::IREmitter ir{*body};
        const IR::U32 local_id = ir.GetAttributeU32(IR::Attribute::LocalInvocationId);
        const IR::U1 break_cond =
            divergent_break ? ir.INotEqual(local_id, ir.Imm32(0U)) : ir.Imm1(false);
        ir.WriteShared(32, ir.Imm32(1U), ir.Imm32(0U));
        ir.Reference(ir.LoadShared(32, false, ir.Imm32(0U)));

        IR::IREmitter continue_ir{*continue_block};
        const IR::U32 continue_local_id =
            continue_ir.GetAttributeU32(IR::Attribute::LocalInvocationId);
        const IR::U1 repeat_cond =
            divergent_repeat ? continue_ir.INotEqual(continue_local_id, continue_ir.Imm32(0U))
                             : continue_ir.Imm1(true);

        IR::AbstractSyntaxNode node{};
        node.type = IR::AbstractSyntaxNode::Type::Loop;
        node.data.loop = {body, continue_block, merge};
        program.syntax_list.push_back(node);

        node = {};
        node.type = IR::AbstractSyntaxNode::Type::Block;
        node.data.block = body;
        program.syntax_list.push_back(node);

        node = {};
        node.type = IR::AbstractSyntaxNode::Type::Break;
        node.data.break_node = {break_cond, merge, continue_block};
        program.syntax_list.push_back(node);

        node = {};
        node.type = IR::AbstractSyntaxNode::Type::Block;
        node.data.block = continue_block;
        program.syntax_list.push_back(node);

        node = {};
        node.type = IR::AbstractSyntaxNode::Type::Repeat;
        node.data.repeat = {repeat_cond, body, merge};
        program.syntax_list.push_back(node);

        node = {};
        node.type = IR::AbstractSyntaxNode::Type::Block;
        node.data.block = merge;
        program.syntax_list.push_back(node);
    }
};

void RunBarrierPass(IR::Program& program) {
    RuntimeInfo runtime_info{};
    runtime_info.Initialize(Stage::Compute);
    runtime_info.cs_info.shared_memory_size = 4;
    runtime_info.cs_info.workgroup_size = {64, 1, 1};

    Profile profile{};
    profile.needs_lds_barriers = true;
    Optimization::SharedMemoryBarrierPass(program, runtime_info, profile);
}

TEST(SharedMemoryBarrierPass, MovesBarrierOutOfDivergentLoop) {
    DivergentLoopProgram test{true, false};
    RunBarrierPass(test.program);

    EXPECT_EQ(CountBarriers(*test.body), 0U);
    EXPECT_EQ(CountBarriers(*test.continue_block), 0U);
    EXPECT_EQ(CountBarriers(*test.merge), 1U);
}

TEST(SharedMemoryBarrierPass, MovesBarrierOutOfLoopWithDivergentRepeat) {
    DivergentLoopProgram test{false, true};
    RunBarrierPass(test.program);

    EXPECT_EQ(CountBarriers(*test.body), 0U);
    EXPECT_EQ(CountBarriers(*test.continue_block), 0U);
    EXPECT_EQ(CountBarriers(*test.merge), 1U);
}

TEST(SharedMemoryBarrierPass, KeepsBarriersInUniformLoop) {
    DivergentLoopProgram test{false, false};
    RunBarrierPass(test.program);

    EXPECT_GT(CountBarriers(*test.body), 0U);
    EXPECT_EQ(CountBarriers(*test.merge), 0U);
}

} // namespace
