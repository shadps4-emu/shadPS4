// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <limits>
#include <unordered_map>
#include <unordered_set>

#include <gtest/gtest.h>
#include <spirv/unified1/spirv.hpp>

#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/ir/post_order.h"
#include "shader_recompiler/recompiler.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/buffer_cache/buffer_cache.h"

namespace {

std::vector<u32> EmitDmaShader() {
    Shader::Info info{};
    info.stage = Shader::Stage::Compute;
    info.l_stage = Shader::LogicalStage::Compute;
    info.uses_dma = true;
    info.buffers.push_back({
        .used_types = Shader::IR::Type::U64,
        .inline_cbuf = AmdGpu::Buffer::Placeholder(VideoCore::BufferCache::BDA_PAGETABLE_SIZE),
        .buffer_type = Shader::BufferType::BdaPagetable,
        .is_written = true,
    });
    info.buffers.push_back({
        .used_types = Shader::IR::Type::U32,
        .inline_cbuf = AmdGpu::Buffer::Placeholder(std::numeric_limits<u32>::max()),
        .buffer_type = Shader::BufferType::FaultBuffer,
        .is_written = true,
    });

    Shader::IR::Program program{info};
    Shader::Pools pools{};
    auto* const block = pools.block_pool.Create(pools.inst_pool);
    program.blocks.push_back(block);
    program.syntax_list.emplace_back();
    program.syntax_list.back().type = Shader::IR::AbstractSyntaxNode::Type::Block;
    program.syntax_list.back().data.block = block;
    program.syntax_list.emplace_back();
    program.syntax_list.back().type = Shader::IR::AbstractSyntaxNode::Type::Return;
    program.post_order_blocks = Shader::IR::PostOrder(program.syntax_list.front());

    Shader::Profile profile{};
    profile.supported_spirv = 0x00010600;
    profile.subgroup_size = 32;
    Shader::RuntimeInfo runtime_info{};
    runtime_info.Initialize(Shader::Stage::Compute);
    runtime_info.cs_info.workgroup_size = {1, 1, 1};
    Shader::Backend::Bindings bindings{};
    return Shader::Backend::SPIRV::EmitSPIRV(profile, runtime_info, program, bindings);
}

TEST(DmaFaultBitmap, AtomicallyPreservesConcurrentPageFaults) {
    const std::vector<u32> spirv = EmitDmaShader();
    std::unordered_map<u32, u32> constants;
    std::unordered_set<u32> access_chains;
    std::unordered_set<u32> shift_results;
    std::vector<u32> stored_pointers;
    bool has_page_bounds_check = false;
    u32 conditional_branch_count = 0;

    struct AtomicOr {
        u32 pointer;
        u32 scope;
        u32 semantics;
        u32 value;
    };
    std::vector<AtomicOr> atomic_ors;

    for (size_t offset = 5; offset < spirv.size();) {
        const u32 instruction = spirv[offset];
        const u16 word_count = instruction >> 16;
        ASSERT_NE(word_count, 0);
        ASSERT_LE(offset + word_count, spirv.size());
        const auto opcode = static_cast<spv::Op>(instruction & 0xffff);
        switch (opcode) {
        case spv::Op::OpConstant:
            if (word_count == 4) {
                constants.emplace(spirv[offset + 2], spirv[offset + 3]);
            }
            break;
        case spv::Op::OpAccessChain:
            ASSERT_GE(word_count, 4);
            access_chains.emplace(spirv[offset + 2]);
            break;
        case spv::Op::OpShiftLeftLogical:
            ASSERT_EQ(word_count, 5);
            shift_results.emplace(spirv[offset + 2]);
            break;
        case spv::Op::OpAtomicOr:
            ASSERT_EQ(word_count, 7);
            atomic_ors.push_back({
                .pointer = spirv[offset + 3],
                .scope = spirv[offset + 4],
                .semantics = spirv[offset + 5],
                .value = spirv[offset + 6],
            });
            break;
        case spv::Op::OpULessThan:
            has_page_bounds_check = true;
            break;
        case spv::Op::OpBranchConditional:
            ++conditional_branch_count;
            break;
        case spv::Op::OpStore:
            ASSERT_GE(word_count, 3);
            stored_pointers.push_back(spirv[offset + 1]);
            break;
        default:
            break;
        }
        offset += word_count;
    }

    ASSERT_EQ(atomic_ors.size(), 1);
    const AtomicOr& atomic = atomic_ors.front();
    EXPECT_TRUE(access_chains.contains(atomic.pointer));
    EXPECT_TRUE(shift_results.contains(atomic.value));
    ASSERT_TRUE(constants.contains(atomic.scope));
    EXPECT_EQ(constants.at(atomic.scope), static_cast<u32>(spv::ScopeDevice));
    ASSERT_TRUE(constants.contains(atomic.semantics));
    EXPECT_EQ(constants.at(atomic.semantics), 0U);
    EXPECT_FALSE(std::ranges::contains(stored_pointers, atomic.pointer));
    EXPECT_TRUE(has_page_bounds_check);
    EXPECT_GE(conditional_branch_count, 2U);
}

} // namespace
