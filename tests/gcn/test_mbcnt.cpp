// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>

#include <gtest/gtest.h>

#include "gcn_test_runner.hpp"
#include "instructions.hpp"
#include "translator.hpp"

class MbcntGcnTest : public ::testing::Test {
protected:
    static void TearDownTestSuite() {
        gcn_test::Runner::DestroyInstance();
    }
};

TEST_F(MbcntGcnTest, PreservesAccumulator) {
    auto runner = gcn_test::Runner::instance().value();
    const auto instruction =
        VOP2(OpcodeVOP2::V_MBCNT_LO_U32_B32, VOperand8::V0, SOperand9::ConstNeg1, VOperand8::V0)
            .Get();
    EXPECT_EQ(instruction, 0x460000c1u);

    auto spirv = TranslateToSpirv(instruction);
    auto result = runner->run<u32>(spirv, std::array{7u});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 7u);
}
