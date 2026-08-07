// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include <gtest/gtest.h>

#include "common/object_pool.h"
#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/profile.h"

namespace Shader::Gcn {
namespace {

size_t CountOpcode(const IR::Block& block, IR::Opcode opcode) {
    return std::ranges::count_if(block.Instructions(), [opcode](const IR::Inst& inst) {
        return inst.GetOpcode() == opcode;
    });
}

void EmitMbcntLaneId(Translator& translator, bool is_low = true) {
    GcnInst inst{};
    inst.src[0].field = OperandField::SignedConstIntNeg;
    inst.src[0].code = 193;
    inst.src[1].field = OperandField::VectorGPR;
    inst.src[1].code = 0;
    inst.dst[0].field = OperandField::VectorGPR;
    inst.dst[0].code = 3;
    translator.V_MBCNT_U32_B32(is_low, inst);
}

TEST(GuestLaneIdTest, ComputeMbcntUsesGuestLaneId) {
    Info info{};
    info.stage = Stage::Compute;
    info.l_stage = LogicalStage::Compute;
    RuntimeInfo runtime_info{};
    runtime_info.Initialize(Stage::Compute);
    Profile profile{};
    Common::ObjectPool<IR::Inst> inst_pool{64};
    IR::Block block{inst_pool};
    Translator translator{info, runtime_info, profile};
    translator.EmitPrologue(&block);

    EmitMbcntLaneId(translator);

    EXPECT_EQ(CountOpcode(block, IR::Opcode::GuestLaneId), 1);
    EXPECT_EQ(CountOpcode(block, IR::Opcode::LaneId), 0);
    EXPECT_EQ(CountOpcode(block, IR::Opcode::BitFieldUExtract), 1);
    EXPECT_EQ(CountOpcode(block, IR::Opcode::BitwiseAnd32), 1);
    EXPECT_EQ(CountOpcode(block, IR::Opcode::BitCount32), 1);
    EXPECT_EQ(CountOpcode(block, IR::Opcode::GetVectorRegister), 1);
}

TEST(GuestLaneIdTest, ComputeMbcntHighUsesGuestLaneId) {
    Info info{};
    info.stage = Stage::Compute;
    info.l_stage = LogicalStage::Compute;
    RuntimeInfo runtime_info{};
    runtime_info.Initialize(Stage::Compute);
    Profile profile{};
    Common::ObjectPool<IR::Inst> inst_pool{64};
    IR::Block block{inst_pool};
    Translator translator{info, runtime_info, profile};
    translator.EmitPrologue(&block);

    EmitMbcntLaneId(translator, false);

    EXPECT_EQ(CountOpcode(block, IR::Opcode::GuestLaneId), 1);
    EXPECT_EQ(CountOpcode(block, IR::Opcode::BitFieldUExtract), 1);
    EXPECT_EQ(CountOpcode(block, IR::Opcode::BitwiseAnd32), 1);
    EXPECT_EQ(CountOpcode(block, IR::Opcode::BitCount32), 1);
    EXPECT_EQ(CountOpcode(block, IR::Opcode::GetVectorRegister), 1);
}

TEST(GuestLaneIdTest, NonComputeMbcntKeepsSubgroupLaneId) {
    Info info{};
    info.stage = Stage::Fragment;
    info.l_stage = LogicalStage::Fragment;
    RuntimeInfo runtime_info{};
    runtime_info.Initialize(Stage::Fragment);
    Profile profile{};
    Common::ObjectPool<IR::Inst> inst_pool{64};
    IR::Block block{inst_pool};
    Translator translator{info, runtime_info, profile};
    translator.EmitPrologue(&block);

    EmitMbcntLaneId(translator);

    EXPECT_EQ(CountOpcode(block, IR::Opcode::GuestLaneId), 0);
    EXPECT_EQ(CountOpcode(block, IR::Opcode::LaneId), 1);
}

TEST(GuestLaneIdTest, ShaderInfoTracksGuestAndSubgroupLaneIdsSeparately) {
    Info info{};
    info.stage = Stage::Compute;
    info.l_stage = LogicalStage::Compute;
    Profile profile{};
    Common::ObjectPool<IR::Inst> inst_pool{64};
    IR::Block block{inst_pool};
    IR::IREmitter ir{block};
    ir.Reference(ir.GuestLaneId());
    ir.Reference(ir.LaneId());
    IR::Program program{info};
    program.blocks = {&block};
    program.post_order_blocks = {&block};

    Optimization::CollectShaderInfoPass(program, profile);

    EXPECT_TRUE(info.uses_guest_lane_id);
    EXPECT_TRUE(info.uses_lane_id);
}

} // namespace
} // namespace Shader::Gcn
