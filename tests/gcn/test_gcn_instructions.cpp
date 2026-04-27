// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>

#include <gtest/gtest.h>
#include <half.hpp>

#include "gcn_test_runner.hpp"
#include "instructions.hpp"
#include "translator.hpp"

class GcnTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

struct F32x2 {
    float a;
    float b;
};

// Example
// TEST_F(GcnTest, test_name) {
//     // Runner sets the vulkan context
//     auto runner = gcn_test::Runner::instance().value();
//
//     // v_add_f32 v0, v0, v1
//     auto spirv = TranslateToSpirv(VOP2(OpcodeVOP2::V_ADD_F32, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
//
//     // run<T> tells how to interpret the result (only 32bit as of now)
//     // the second argument is templated, it can be at most 4 u32s
//     // the data is accessible by the instruction in v0-4 and s0-4 (mirrored)
//     // the result has to be placed in v0
//     auto result = runner->run<float>(spirv, F32x2{1.5f, 6.0f});
//
//     EXPECT_TRUE(result.has_value());
//     EXPECT_EQ(*result, 7.5f);
// }

TEST_F(GcnTest, add_f32) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP2(OpcodeVOP2::V_ADD_F32, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
    auto result = runner->run<float>(spirv, F32x2{1.5f, 6.0f});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 7.5f);
}

TEST_F(GcnTest, add_nan) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP2(OpcodeVOP2::V_ADD_F32, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
    auto result = runner->run<float>(spirv, F32x2{1.0f, std::numeric_limits<float>::quiet_NaN()});

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(*result));
}

using half = half_float::half;

struct F16x2 {
    half a;
    half b = half(0.0f);

    bool operator==(const F16x2& rhs) const = default;
};

static_assert(sizeof(F16x2) == sizeof(float));

TEST_F(GcnTest, add_f16) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP2(OpcodeVOP2::V_ADD_F16, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
    auto result = runner->run<F16x2>(spirv, std::array{F16x2{half(1.0f)}, F16x2{half(1.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, F16x2{half(2.0f)});
}

TEST_F(GcnTest, add_f16_clamp) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_ADD_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1).SetClamp(true).Get());
    auto result = runner->run<F16x2>(spirv, std::array{F16x2{half(1.0f)}, F16x2{half(1.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, F16x2{half(1.0f)}); //confirmed with neo
}

TEST_F(GcnTest, add_f16_neg) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_ADD_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1).SetNeg({true, true, false}).Get());
    auto result = runner->run<F16x2>(spirv, std::array{F16x2{half(1.0f)}, F16x2{half(1.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ((*result).a, half(-2.0f)); //confirmed with neo
}

TEST_F(GcnTest, add_f16_opsel_hi) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_ADD_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1).SetOpSel({true, true, false, true}).Get());
    auto result = runner->run<F16x2>(spirv, std::array{F16x2{half(1.0f), half(2.0f)}, F16x2{half(1.0f), half(2.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ((*result).a, half(1.0f));
    EXPECT_EQ((*result).b, half(4.0f));
}

TEST_F(GcnTest, sub_f16) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP2(OpcodeVOP2::V_SUB_F16, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
    auto result = runner->run<F16x2>(spirv, std::array{F16x2{half(0.0f)}, F16x2{half(1.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, F16x2{half(-1.0f)}); //confirmed with neo
}

TEST_F(GcnTest, div_scale_f32_2_2_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_DIV_SCALE_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<float>(spirv, std::array{2.0f, 2.0f, 1.0f});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 2.0f);
}

TEST_F(GcnTest, div_scale_f32_1_2_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_DIV_SCALE_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<float>(spirv, std::array{1.0f, 2.0f, 1.0f});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 1.0f);
}

TEST_F(GcnTest, div_scale_f32_denorm_denorm_1) {
    auto runner = gcn_test::Runner::instance().value();
    auto denorm = std::bit_cast<float>(0x00000001);

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_DIV_SCALE_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<u32>(spirv, std::array{denorm, denorm, 1.0f});

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(std::bit_cast<float>(*result)));
    EXPECT_EQ(*result, 0xffc00000);
}

TEST_F(GcnTest, div_scale_f32_denorm_1_denorm) {
    auto runner = gcn_test::Runner::instance().value();
    auto denorm = std::bit_cast<float>(0x00000001);

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_DIV_SCALE_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<u32>(spirv, std::array{denorm, 1.0f, denorm});

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(std::bit_cast<float>(*result)));
    EXPECT_EQ(*result, 0xffc00000);
}

TEST_F(GcnTest, div_scale_f32_MINFLT_MINFLT_MAXFLT) {
    auto runner = gcn_test::Runner::instance().value();
    auto min_flt = std::bit_cast<float>(0x00800000);
    auto max_flt = std::bit_cast<float>(0x7f7fffff);

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_DIV_SCALE_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<u32>(spirv, std::array{min_flt, min_flt, max_flt});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x20800000); // 2.1684043e-19
}

TEST_F(GcnTest, div_scale_f32_0_0_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_DIV_SCALE_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<u32>(spirv, std::array{0.0f, 0.0f, 1.0f});

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(std::bit_cast<float>(*result)));
    EXPECT_EQ(*result, 0xffc00000);
}

TEST_F(GcnTest, div_scale_f32_0_1_0) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_DIV_SCALE_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<u32>(spirv, std::array{0.0f, 1.0f, 0.0f});

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(std::bit_cast<float>(*result)));
    EXPECT_EQ(*result, 0xffc00000);
}

TEST_F(GcnTest, div_scale_f32_MAXFLT_MAXFLT_1) {
    auto runner = gcn_test::Runner::instance().value();
    auto max_flt = std::bit_cast<float>(0x7f7fffff);

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_DIV_SCALE_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<u32>(spirv, std::array{max_flt, max_flt, 1.0f});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x5F7FFFFF); // 18446726481523507000 = 2^63 * 1.9999981
}

TEST_F(GcnTest, div_scale_f32_1_1_Inf) {
    auto runner = gcn_test::Runner::instance().value();
    auto inf = std::bit_cast<float>(0x7f800000);

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_DIV_SCALE_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<u32>(spirv, std::array{1.0f, 1.0f, inf});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x5f800000); // 18446744073709552000
}

TEST_F(GcnTest, div_scale_f32_1_Inf_1) {
    auto runner = gcn_test::Runner::instance().value();
    auto inf = std::bit_cast<float>(0x7f800000);

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_DIV_SCALE_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<float>(spirv, std::array{1.0f, inf, 1.0f});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 1.0f);
}

TEST_F(GcnTest, div_scale_f32_1_1_0) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_DIV_SCALE_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<u32>(spirv, std::array{1.0f, 1.0f, 0.0f});

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(std::bit_cast<float>(*result)));
    EXPECT_EQ(*result, 0xffc00000);
}

TEST_F(GcnTest, div_scale_f32_1_1_negInf) {
    auto runner = gcn_test::Runner::instance().value();
    auto neg_inf = std::bit_cast<float>(0xff800000);

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_DIV_SCALE_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<u32>(spirv, std::array{1.0f, 1.0f, neg_inf});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x5f800000); // 18446744073709552000
}

TEST_F(GcnTest, div_scale_f32_small_1_small) {
    auto runner = gcn_test::Runner::instance().value();
    auto small = std::bit_cast<float>(0x07800000); // 1.92593e-34

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_DIV_SCALE_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<u32>(spirv, std::array{small, 1.0f, small});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x27800000); // 3.5527136e-15
}

TEST_F(GcnTest, div_scale_f32_1_1_small) {
    auto runner = gcn_test::Runner::instance().value();
    auto small = std::bit_cast<float>(0x07800000); // 1.92593e-34

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_DIV_SCALE_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<float>(spirv, std::array{1.0f, 1.0f, small});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 1.0f);
}
