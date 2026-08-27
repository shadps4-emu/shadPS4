// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include "common/object_pool.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/passes/control_flow/mask_to_predicate_pass.h"
#include "shader_recompiler/ir/passes/control_flow/predicate_interval_pass.h"

namespace {

struct IrFixture : testing::Test {
    Common::ObjectPool<Shader::IR::Inst> inst_pool{64};
    Shader::IR::Block block{inst_pool};
    Shader::IR::IREmitter ir{block};
};

using Shader::IR::Opcode;
using Shader::Optimization::ExtractMustFactors;

TEST_F(IrFixture, BallotProjectionReturnsOriginalPredicate) {
    const Shader::IR::U1 predicate = ir.GetScc();
    const Shader::IR::U1 projected = ir.ProjectLane(ir.Ballot64(predicate));

    Shader::Optimization::MaskToPredicatePass({&block});

    EXPECT_EQ(projected.Resolve(), predicate.Resolve());
}

TEST_F(IrFixture, NestedMaskAndBecomesBooleanFactors) {
    const Shader::IR::U1 outer = ir.GetScc();
    const Shader::IR::U1 inner = ir.GetVcc();
    const Shader::IR::U64 mask = ir.BitwiseAnd(ir.Ballot64(outer), ir.Ballot64(inner));
    const Shader::IR::U1 projected = ir.ProjectLane(mask);

    Shader::Optimization::MaskToPredicatePass({&block});

    Shader::IR::Inst* const result = projected.Resolve().TryInstRecursive();
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetOpcode(), Opcode::LogicalAnd);
    const auto factors = ExtractMustFactors(projected.Resolve());
    ASSERT_EQ(factors.size(), 2);
    EXPECT_NE(std::ranges::find(factors, outer.Resolve()), factors.end());
    EXPECT_NE(std::ranges::find(factors, inner.Resolve()), factors.end());
}

TEST_F(IrFixture, UnknownExactMaskIsPreservedAsProjection) {
    const Shader::IR::U64 exact_mask = ir.GetExecMask();
    const Shader::IR::U1 nested = ir.GetScc();
    const Shader::IR::U1 projected = ir.ProjectLane(ir.BitwiseAnd(exact_mask, ir.Ballot64(nested)));

    Shader::Optimization::MaskToPredicatePass({&block});

    Shader::IR::Inst* const result = projected.Resolve().TryInstRecursive();
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->GetOpcode(), Opcode::LogicalAnd);
    EXPECT_EQ(result->Arg(1).Resolve(), nested.Resolve());
    EXPECT_EQ(result->Arg(0).Resolve().TryInstRecursive()->GetOpcode(), Opcode::INotEqual64);
}

TEST_F(IrFixture, CommonOuterPredicateFormsOneInterval) {
    const Shader::IR::U1 outer = ir.GetScc();
    const Shader::IR::U1 inner = ir.GetVcc();
    const Shader::IR::U1 nested = ir.LogicalAnd(inner, outer);
    const std::array fragments{
        Shader::Optimization::PredicateFragment{.guard = outer, .instruction_count = 2},
        Shader::Optimization::PredicateFragment{.guard = nested, .instruction_count = 3},
        Shader::Optimization::PredicateFragment{.guard = outer, .instruction_count = 2},
    };

    const auto analysis = Shader::Optimization::AnalyzePredicateIntervals(fragments);
    const auto common = std::ranges::find_if(analysis.intervals, [](const auto& interval) {
        return interval.selected && interval.begin == 0 && interval.end == 2;
    });
    ASSERT_NE(common, analysis.intervals.end());
    ASSERT_EQ(common->factors.size(), 1);
    EXPECT_EQ(common->factors.front(), outer.Resolve());
    const auto nested_interval =
        std::ranges::find_if(analysis.intervals, [&](const auto& interval) {
            return interval.selected && interval.begin == 1 && interval.end == 1 &&
                   interval.factors.size() == 1 && interval.factors.front() == inner.Resolve();
        });
    EXPECT_NE(nested_interval, analysis.intervals.end());
    EXPECT_TRUE(analysis.residual_factors[1].empty());
}

TEST_F(IrFixture, ScalarBarrierPreventsPredicateIntervalSpanning) {
    const Shader::IR::U1 predicate = ir.GetScc();
    const std::array fragments{
        Shader::Optimization::PredicateFragment{.guard = predicate, .instruction_count = 2},
        Shader::Optimization::PredicateFragment{
            .guard = predicate, .instruction_count = 1, .barrier = true},
        Shader::Optimization::PredicateFragment{.guard = predicate, .instruction_count = 2},
    };

    const auto analysis = Shader::Optimization::AnalyzePredicateIntervals(fragments);
    EXPECT_TRUE(std::ranges::none_of(analysis.intervals, [](const auto& interval) {
        return interval.begin == 0 && interval.end == 2;
    }));
}

TEST_F(IrFixture, WaterfallExitPhiRetainsOnlyPredicateSharedByBothExits) {
    Shader::IR::Block continue_path{inst_pool};
    Shader::IR::Block early_exit_path{inst_pool};
    const Shader::IR::U1 outer = ir.GetScc();
    const Shader::IR::U1 continue_condition = ir.GetVcc();
    const Shader::IR::U1 early_exit_condition = ir.LogicalNot(ir.GetVcc());
    const Shader::IR::U1 continue_guard = ir.LogicalAnd(outer, continue_condition);
    const Shader::IR::U1 early_exit_guard = ir.LogicalAnd(early_exit_condition, outer);

    Shader::IR::Inst* const merge = &*block.PrependNewInst(block.begin(), Shader::IR::Opcode::Phi);
    merge->SetFlags(Shader::IR::Type::U1);
    merge->AddPhiOperand(&continue_path, continue_guard);
    merge->AddPhiOperand(&early_exit_path, early_exit_guard);

    const auto factors = ExtractMustFactors(Shader::IR::Value{merge});
    ASSERT_EQ(factors.size(), 1);
    EXPECT_EQ(factors.front(), outer.Resolve());
}

} // Anonymous namespace
