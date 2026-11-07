// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <vector>
#include <gtest/gtest.h>

#include "common/object_pool.h"
#include "shader_recompiler/frontend/control_flow_graph.h"
#include "shader_recompiler/frontend/decode.h"

namespace {

using namespace Shader::Gcn;

std::vector<GcnInst> Decode(std::span<const u32> code) {
    GcnCodeSlice slice(code.data(), code.data() + code.size());
    GcnDecodeContext decoder;
    std::vector<GcnInst> insts;
    while (!slice.atEnd()) {
        insts.emplace_back(decoder.decodeInstruction(slice));
    }
    return insts;
}

// Switch statement lowered to a jump table, as emitted by the PS4 shader compiler.
TEST(ControlFlowGraph, SetPcThroughJumpTable) {
    constexpr size_t NumCodeWords = 15;
    constexpr std::array<u32, 26> code = {
        0x83948200, // 0x00 s_min_u32      s20, s0, 2
        0x8F148314, // 0x04 s_lshl_b32     s20, s20, 3
        0x80149014, // 0x08 s_add_u32      s20, s20, 16
        0xBE8C1F00, // 0x0c s_getpc_b64    s[12:13]
        0x800CFF0C, // 0x10 s_add_u32      s12, s12, 0x30   ; table at 0x40
        0x00000030,
        0x820D800D, // 0x18 s_addc_u32     s13, s13, 0
        0xC0460C14, // 0x1c s_load_dwordx2 s[12:13], s[12:13], s20
        0xBE901F00, // 0x20 s_getpc_b64    s[16:17]
        0x80100C10, // 0x24 s_add_u32      s16, s16, s12
        0x82110D11, // 0x28 s_addc_u32     s17, s17, s13
        0xBE802010, // 0x2c s_setpc_b64    s[16:17]
        0xBF810000, // 0x30 s_endpgm
        0xBF810000, // 0x34 s_endpgm                         ; not referenced by the table
        0xBF810000, // 0x38 s_endpgm
        0,          0, 0, 0, 0, // 0x3c padding, table at 0x40, entries start at 0x40 + 16
        0x0C,       0,          // case 0 -> 0x24 + 0x0c = 0x30
        0x14,       0,          // case 1 -> 0x38
        0x0C,       0,          // case 2 -> 0x30
    };
    const auto insts = Decode(std::span{code}.first(NumCodeWords));
    ASSERT_EQ(insts.size(), 14u);

    Common::ObjectPool<Block> pool;
    CFG cfg{pool, insts, code.data()};

    std::vector<u32> block_begins;
    const Block* switch_block = nullptr;
    for (const Block& block : cfg) {
        block_begins.push_back(block.begin);
        if (block.end_class == EndClass::IndirectBranch) {
            switch_block = &block;
        }
    }
    EXPECT_EQ(block_begins, (std::vector<u32>{0x00, 0x30, 0x38}));

    ASSERT_NE(switch_block, nullptr);
    EXPECT_EQ(switch_block->end, 0x30);
    EXPECT_EQ(switch_block->jump_table.index_reg, 20);
    EXPECT_EQ(switch_block->jump_table.load_index, 6);
    std::vector<std::pair<u32, u32>> targets;
    for (const auto& [value, target] : switch_block->jump_table.targets) {
        targets.emplace_back(value, target->begin);
    }
    EXPECT_EQ(targets, (std::vector<std::pair<u32, u32>>{{16, 0x30}, {24, 0x38}, {32, 0x30}}));
}

// Same as above, but the table index is used without a byte offset added to it.
TEST(ControlFlowGraph, SetPcThroughJumpTableWithoutBias) {
    constexpr size_t NumCodeWords = 13;
    constexpr std::array<u32, 18> code = {
        0x83948100, // 0x00 s_min_u32      s20, s0, 1
        0x8F148314, // 0x04 s_lshl_b32     s20, s20, 3
        0xBE8C1F00, // 0x08 s_getpc_b64    s[12:13]
        0x800CFF0C, // 0x0c s_add_u32      s12, s12, 0x2c   ; table at 0x38
        0x0000002C,
        0x820D800D,    // 0x14 s_addc_u32     s13, s13, 0
        0xC0460C14,    // 0x18 s_load_dwordx2 s[12:13], s[12:13], s20
        0xBE901F00,    // 0x1c s_getpc_b64    s[16:17]
        0x80100C10,    // 0x20 s_add_u32      s16, s16, s12
        0x82110D11,    // 0x24 s_addc_u32     s17, s17, s13
        0xBE802010,    // 0x28 s_setpc_b64    s[16:17]
        0xBF810000,    // 0x2c s_endpgm
        0xBF810000,    // 0x30 s_endpgm
        0,             // 0x34 padding
        0x10,       0, // case 0 -> 0x20 + 0x10 = 0x30
        0x0C,       0, // case 1 -> 0x2c
    };
    const auto insts = Decode(std::span{code}.first(NumCodeWords));
    ASSERT_EQ(insts.size(), 12u);

    Common::ObjectPool<Block> pool;
    CFG cfg{pool, insts, code.data()};

    const auto switch_block = std::find_if(cfg.begin(), cfg.end(), [](const Block& block) {
        return block.end_class == EndClass::IndirectBranch;
    });
    ASSERT_NE(switch_block, cfg.end());
    EXPECT_EQ(switch_block->jump_table.load_index, 5);
    std::vector<std::pair<u32, u32>> targets;
    for (const auto& [value, target] : switch_block->jump_table.targets) {
        targets.emplace_back(value, target->begin);
    }
    EXPECT_EQ(targets, (std::vector<std::pair<u32, u32>>{{0, 0x30}, {8, 0x2c}}));
}

} // Anonymous namespace
