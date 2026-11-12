// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <span>
#include <vector>
#include <gtest/gtest.h>

#include "common/object_pool.h"
#include "shader_recompiler/frontend/control_flow_graph.h"
#include "shader_recompiler/frontend/decode.h"

using namespace Shader::Gcn;

namespace {

std::vector<GcnInst> Decode(std::span<const u32> code) {
    GcnCodeSlice slice(code.data(), code.data() + code.size());
    GcnDecodeContext decoder;
    std::vector<GcnInst> insts;
    while (!slice.atEnd()) {
        insts.push_back(decoder.decodeInstruction(slice));
    }
    return insts;
}

// Returns the divergence blocks, the targets of the blocks that branch on EXEC.
std::vector<const Block*> DivergenceBlocks(CFG& cfg) {
    std::vector<const Block*> result;
    for (const Block& block : cfg) {
        if (block.cond == Shader::IR::Condition::Execnz && block.branch_true) {
            result.push_back(block.branch_true);
        }
    }
    return result;
}

bool InScope(const std::vector<const Block*>& scopes, u32 index) {
    return std::ranges::any_of(scopes, [index](const Block* block) {
        return block->begin_index <= index && index <= block->end_index;
    });
}

} // namespace

// Waterfall loop where the mask of remaining threads is updated between two vector instructions
// of the EXEC scope. The scalar instruction runs for the whole wave, so it must stay out of it.
TEST(ControlFlowGraphTest, ScalarInstructionInsideExecScopeRunsForWholeWave) {
    const std::vector<u32> code = {
        0x7e26051b,             // 0: v_readfirstlane_b32 s19, v27
        0xbefc0313,             // 1: s_mov_b32 m0, s19
        0xd1a4002a, 0x00023613, // 2: v_cmpx_eq_u32 s[42:43], s19, v27
        0x7e00851c,             // 3: v_movreld_b32 v0, v28
        0x7e02851d,             // 4: v_movreld_b32 v1, v29
        0x7e048521,             // 5: v_movreld_b32 v2, v33
        0x8aa82a28,             // 6: s_andn2_b64 s[40:41], s[40:41], s[42:43]
        0x7e068523,             // 7: v_movreld_b32 v3, v35
        0xbefe0428,             // 8: s_mov_b64 exec, s[40:41]
        0xbf85fff5,             // 9: s_cbranch_scc1 0
        0xbf810000,             // 10: s_endpgm
    };
    const auto insts = Decode(code);
    ASSERT_EQ(insts.size(), 11u);

    Common::ObjectPool<Block> block_pool{64};
    CFG cfg{block_pool, insts};
    const auto scopes = DivergenceBlocks(cfg);

    EXPECT_FALSE(InScope(scopes, 6)) << cfg.Dot();
    for (const u32 index : {3u, 4u, 5u, 7u}) {
        EXPECT_TRUE(InScope(scopes, index)) << "instruction " << index << '\n' << cfg.Dot();
    }
    EXPECT_FALSE(InScope(scopes, 8)) << cfg.Dot();
}

// A scalar instruction that reads a scalar register written inside the scope only has a value in
// the active threads, so the scope is left as it is.
TEST(ControlFlowGraphTest, ScalarInstructionReadingScopeResultStaysInScope) {
    const std::vector<u32> code = {
        0x7da40080,             // 0: v_cmpx_eq_u32 vcc, 0, v0
        0xd184002c, 0x00020280, // 1: v_cmp_eq_u32 s[44:45], 0, v1
        0x87aa2c28,             // 2: s_and_b64 s[42:43], s[40:41], s[44:45]
        0x7e020300,             // 3: v_mov_b32 v1, v0
        0xbefe0428,             // 4: s_mov_b64 exec, s[40:41]
        0xbf810000,             // 5: s_endpgm
    };
    const auto insts = Decode(code);
    ASSERT_EQ(insts.size(), 6u);

    Common::ObjectPool<Block> block_pool{64};
    CFG cfg{block_pool, insts};
    const auto scopes = DivergenceBlocks(cfg);

    ASSERT_EQ(scopes.size(), 1u) << cfg.Dot();
    EXPECT_EQ(scopes[0]->begin_index, 1u);
    EXPECT_EQ(scopes[0]->end_index, 3u);
}
