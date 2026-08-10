// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>
#include <optional>

#include <gtest/gtest.h>
#include <half.hpp>

#include "gcn_test_runner.hpp"
#include "instructions.hpp"
#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/profile.h"
#include "shader_recompiler/recompiler.h"
#include "shader_recompiler/runtime_info.h"
#include "translator.hpp"
#include "video_core/amdgpu/resource.h"

class GcnTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}

    static void TearDownTestSuite() {
        gcn_test::Runner::DestroyInstance();
    }
};

struct F32x2 {
    float a;
    float b;
};

namespace {

struct InterpMovIr {
    std::vector<u32> attribute_vertex_indices;
    std::optional<std::pair<u32, u32>> subtraction_vertex_indices;
    Shader::Qualifier qualifier;
};

struct Bcnt64Ir {
    bool reads_thread_mask{};
    bool builds_ballot{};
    u32 bit_count_count{};
};

Bcnt64Ir TranslateBcnt64ToIr() {
    Shader::Pools pools;
    Shader::Info info{};
    info.stage = Shader::Stage::Compute;
    info.l_stage = Shader::LogicalStage::Compute;

    Shader::RuntimeInfo runtime_info{};
    runtime_info.Initialize(Shader::Stage::Compute);

    Shader::Profile profile{};
    Shader::IR::Block block{pools.inst_pool};
    Shader::Gcn::Translator translator{info, runtime_info, profile};
    translator.EmitPrologue(&block);
    const auto prologue_size = block.size();

    Shader::Gcn::GcnInst inst{};
    inst.src[0].field = Shader::Gcn::OperandField::ScalarGPR;
    inst.src[0].code = 22;
    inst.dst[0].field = Shader::Gcn::OperandField::ScalarGPR;
    inst.dst[0].code = 24;
    translator.S_BCNT1_I32_B64(inst);

    Bcnt64Ir result{};
    auto it = block.begin();
    std::advance(it, prologue_size);
    for (; it != block.end(); ++it) {
        result.reads_thread_mask |=
            it->GetOpcode() == Shader::IR::Opcode::GetThreadBitScalarReg;
        result.builds_ballot |= it->GetOpcode() == Shader::IR::Opcode::Ballot;
        result.bit_count_count += it->GetOpcode() == Shader::IR::Opcode::BitCount32;
    }
    return result;
}

InterpMovIr TranslateInterpMovToIr(u32 selector) {
    Shader::Pools pools;
    Shader::Info info{};
    info.stage = Shader::Stage::Fragment;
    info.l_stage = Shader::LogicalStage::Fragment;

    Shader::RuntimeInfo runtime_info{};
    runtime_info.Initialize(Shader::Stage::Fragment);
    runtime_info.fs_info.num_inputs = 1;
    runtime_info.fs_info.inputs[0].param_index = 0;

    Shader::Profile profile{};
    profile.supports_fragment_shader_barycentric = true;

    Shader::IR::Block block{pools.inst_pool};
    Shader::Gcn::Translator translator{info, runtime_info, profile};
    translator.EmitPrologue(&block);
    const auto prologue_size = block.size();

    Shader::Gcn::GcnInst inst{};
    inst.src[0].code = selector;
    inst.dst[0].field = Shader::Gcn::OperandField::VectorGPR;
    inst.dst[0].code = 0;
    inst.control.vintrp.attr = 0;
    inst.control.vintrp.chan = 2;
    translator.V_INTERP_MOV_F32(inst);

    InterpMovIr result{.qualifier = info.fs_interpolation[0].primary};
    auto it = block.begin();
    std::advance(it, prologue_size);
    for (; it != block.end(); ++it) {
        if (it->GetOpcode() == Shader::IR::Opcode::GetAttribute) {
            result.attribute_vertex_indices.push_back(it->Arg(2).U32());
        } else if (it->GetOpcode() == Shader::IR::Opcode::FPSub32) {
            result.subtraction_vertex_indices = {
                it->Arg(0).Inst()->Arg(2).U32(),
                it->Arg(1).Inst()->Arg(2).U32(),
            };
        }
    }
    return result;
}

} // namespace

TEST_F(GcnTest, bcnt1_i32_b64_counts_saved_thread_mask) {
    const auto result = TranslateBcnt64ToIr();

    EXPECT_TRUE(result.reads_thread_mask);
    EXPECT_TRUE(result.builds_ballot);
    EXPECT_EQ(result.bit_count_count, 2U);
}

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

TEST_F(GcnTest, interp_mov_builds_p10_from_vertex_difference) {
    const auto result = TranslateInterpMovToIr(0);

    EXPECT_EQ(result.qualifier, Shader::Qualifier::PerVertex);
    EXPECT_EQ(result.attribute_vertex_indices, (std::vector<u32>{0, 1}));
    ASSERT_TRUE(result.subtraction_vertex_indices.has_value());
    EXPECT_EQ(*result.subtraction_vertex_indices, std::pair(1U, 0U));
}

TEST_F(GcnTest, interp_mov_builds_p20_from_vertex_difference) {
    const auto result = TranslateInterpMovToIr(1);

    EXPECT_EQ(result.qualifier, Shader::Qualifier::PerVertex);
    EXPECT_EQ(result.attribute_vertex_indices, (std::vector<u32>{0, 2}));
    ASSERT_TRUE(result.subtraction_vertex_indices.has_value());
    EXPECT_EQ(*result.subtraction_vertex_indices, std::pair(2U, 0U));
}

TEST_F(GcnTest, interp_mov_loads_p0_from_vertex_zero) {
    const auto result = TranslateInterpMovToIr(2);

    EXPECT_EQ(result.qualifier, Shader::Qualifier::PerVertex);
    EXPECT_EQ(result.attribute_vertex_indices, (std::vector<u32>{0}));
    EXPECT_FALSE(result.subtraction_vertex_indices.has_value());
}

TEST_F(GcnTest, sampler_descriptor_rejects_reserved_encodings) {
    AmdGpu::Sampler sampler{};
    EXPECT_TRUE(sampler.Valid());

    sampler.mip_filter.Assign(static_cast<AmdGpu::MipFilter>(3));
    EXPECT_FALSE(sampler.Valid());
    sampler.mip_filter.Assign(AmdGpu::MipFilter::None);

    sampler.z_filter.Assign(3);
    EXPECT_FALSE(sampler.Valid());
    sampler.z_filter.Assign(0);

    sampler.filter_mode.Assign(static_cast<AmdGpu::FilterMode>(3));
    EXPECT_FALSE(sampler.Valid());
    sampler.filter_mode.Assign(AmdGpu::FilterMode::Blend);

    sampler.max_aniso.Assign(static_cast<AmdGpu::AnisoRatio>(5));
    EXPECT_FALSE(sampler.Valid());
}

TEST_F(GcnTest, add_f32) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP2(OpcodeVOP2::V_ADD_F32, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
    auto result = runner->run<float>(spirv, F32x2{1.5f, 6.0f});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 7.5f);
}

TEST_F(GcnTest, add_i32_carry_feeds_addc_u32) {
    auto runner = gcn_test::Runner::instance().value();
    const std::array<u64, 2> instructions{
        VOP2(OpcodeVOP2::V_ADD_I32, VOperand8::V1, SOperand9::S0, VOperand8::V1).Get(),
        VOP2(OpcodeVOP2::V_ADDC_U32, VOperand8::V0, SOperand9::S2, VOperand8::V3).Get(),
    };
    const auto spirv = TranslateToSpirv(instructions);

    const auto overflow = runner->run<u32>(spirv, std::array{0xffffffffU, 1U, 7U, 0U});
    ASSERT_TRUE(overflow.has_value());
    EXPECT_EQ(*overflow, 8U);

    const auto no_overflow = runner->run<u32>(spirv, std::array{2U, 1U, 7U, 0U});
    ASSERT_TRUE(no_overflow.has_value());
    EXPECT_EQ(*no_overflow, 7U);
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

TEST_F(GcnTest, mul_legacy_nan) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP2(OpcodeVOP2::V_MUL_LEGACY_F32, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
    auto result = runner->run<u32>(spirv, std::array{u32(0), u32(0x7fc00000)});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0);
}

TEST_F(GcnTest, mul_nan) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP2(OpcodeVOP2::V_MUL_F32, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
    auto result = runner->run<float>(spirv, std::array{u32(0), u32(0x7fc00000)});

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(*result));
}

TEST_F(GcnTest, min_legacy_nan) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP2(OpcodeVOP2::V_MIN_LEGACY_F32, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
    auto result = runner->run<u32>(spirv, std::array{u32(0), u32(0x7fc00000)});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x7fc00000);
}

TEST_F(GcnTest, min_nan) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP2(OpcodeVOP2::V_MIN_F32, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
    auto result = runner->run<float>(spirv, std::array{u32(0), u32(0x7fc00000)});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0);
}

TEST_F(GcnTest, add3_u32_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_ADD3_U32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<u32>(spirv, std::array{0, 1, 2});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 3);
}

TEST_F(GcnTest, add3_u32_2) {
    auto runner = gcn_test::Runner::instance().value();
    auto big = 2000000000;

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_ADD3_U32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<u32>(spirv, std::array{big, big, big});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x65A0BC00);
}

TEST_F(GcnTest, add3_u32_3) {
    auto runner = gcn_test::Runner::instance().value();
    auto big = 2000000000;

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_ADD3_U32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).SetClamp(true).Get());
    auto result = runner->run<u32>(spirv, std::array{big, big, big});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x65A0BC00);
}

TEST_F(GcnTest, add3_u32_4) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_ADD3_U32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).SetNeg({1,0,0}).Get());
    auto result = runner->run<u32>(spirv, std::array{0, 1, 2});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x80000003);
}

TEST_F(GcnTest, or3_u32_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_OR3_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<u32>(spirv, std::array<u32,3>{0xF0F0F0F0, 0x07070707, 0x11111111});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0xF7F7F7F7);
}

TEST_F(GcnTest, or3_u32_2) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_OR3_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<u32>(spirv, std::array{0x07070707, 0x11111111, 0x40404040});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x57575757);
}

TEST_F(GcnTest, or3_u32_3) {
    auto runner = gcn_test::Runner::instance().value();
    auto big = 2000000000;

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_OR3_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).SetClamp(true).Get());
    auto result = runner->run<u32>(spirv, std::array{0x07070707, 0x11111111, 0x40404040});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x57575757);
}

TEST_F(GcnTest, or3_u32_4) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_OR3_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).SetNeg({0,0,1}).Get());
    auto result = runner->run<u32>(spirv, std::array{0x07070707, 0x11111111, 0x40404040});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0xD7575757);
}

TEST_F(GcnTest, and_or_b32_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).Get());
    auto result = runner->run<u32>(spirv, std::array<u32,3>{0xF0F0F0F0, 0x07070707, 0x11111111});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x11111111);
}

TEST_F(GcnTest, and_or_b32_2) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).SetOmod(Omod::Mul2).Get());
    auto result = runner->run<u32>(spirv, std::array{0x40404040, 0x40404040, 0x40404040});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x40404040);
}

TEST_F(GcnTest, and_or_b32_3) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).SetClamp(true).Get());
    auto result = runner->run<u32>(spirv, std::array{0x40404040, 0x40404040, 0x40404040});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x40404040);
}

TEST_F(GcnTest, and_or_b32_4) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).SetNeg({1,0,0}).Get());
    auto result = runner->run<u32>(spirv, std::array<u32,3>{0x07070707, 0x11111111, 0xF0F0F0F0});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0xF1F1F1F1);
}

TEST_F(GcnTest, and_or_b32_5) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).SetNeg({1,0,0}).SetAbs({1,0,0}).Get());
    auto result = runner->run<u32>(spirv, std::array<u32,3>{0x77777777, 0xB0B0B0B0, 0x11111111});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0xB1313131);
}

TEST_F(GcnTest, and_or_b32_6) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).SetOmod(Omod::Mul2).Get());
    auto result = runner->run<u32>(spirv, std::array<u32,3>{0x40404040, 0xB0B0B0B0, 0x11111111});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x11111111);
}

TEST_F(GcnTest, and_or_b32_7) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).SetOmod(Omod::Div2).Get());
    auto result = runner->run<u32>(spirv, std::array<u32,3>{0xB0B0B0B0, 0x77777777, 0x40404040});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x70707070);
}

TEST_F(GcnTest, and_or_b32_8) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).SetAbs({1,1,0}).Get());
    auto result = runner->run<u32>(spirv, std::array<u32,3>{0xB0B0B0B0, 0x11111111, 0x11111111});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x11111111);
}

TEST_F(GcnTest, mad_mix_f32_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst = VOP3P(OpcodeVOP3P::V_MAD_MIX_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).SetOpSelHi({0}).Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<float>(spirv, std::array{2.0f, 3.0f, 4.0f});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 10.0f);
}

TEST_F(GcnTest, mad_mix_f32_2) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst = VOP3P(OpcodeVOP3P::V_MAD_MIX_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).SetOpSelHi({1,1,0}).SetOpSel({1,0,0}).Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<float>(spirv, std::array<u32,3>{
        std::bit_cast<u32>(F16x2{half(44.0f), half(0.5f)}), std::bit_cast<u32>(F16x2{half(44.0f), half(0.5f)}), std::bit_cast<u32>(4.0f)}
    );

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 26.0f);
}

TEST_F(GcnTest, mad_mixlo_f16_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst = VOP3P(OpcodeVOP3P::V_MAD_MIXLO_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).SetOpSelHi({1,1,0}).SetOpSel({1,0,0}).Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<F16x2>(spirv, std::array<u32,3>{
        std::bit_cast<u32>(F16x2{half(44.0f), half(0.5f)}), std::bit_cast<u32>(F16x2{half(44.0f), half(0.5f)}), std::bit_cast<u32>(4.0f)}
    );

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, (F16x2{half(26.0f), half(0.5f)}));
}

TEST_F(GcnTest, mad_mixhi_f16_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst = VOP3P(OpcodeVOP3P::V_MAD_MIXHI_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2).SetOpSelHi({1,1,0}).SetOpSel({1,0,0}).Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<F16x2>(spirv, std::array<u32,3>{
        std::bit_cast<u32>(F16x2{half(44.0f), half(0.5f)}), std::bit_cast<u32>(F16x2{half(44.0f), half(0.5f)}), std::bit_cast<u32>(4.0f)}
    );

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, (F16x2{half(44.0f), half(26.0f)}));
}

TEST_F(GcnTest, lshrrev_b16_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_LSHRREV_B16, VOperand8::V0, SOperand9::V0, SOperand9::V1).Get());
    auto result = runner->run<u32>(spirv, std::array<u32,2>{0xFFFFFFF2, 0x88881000});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0xFFFF0400);
}

TEST_F(GcnTest, lshrrev_b16_2) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_LSHRREV_B16, VOperand8::V0, SOperand9::V0, SOperand9::V1).SetOpSel({0,0,0,1}).Get());
    auto result = runner->run<u32>(spirv, std::array<u32,2>{0xFFFFFFF2, 0x88881000});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x0400FFF2);
}

TEST_F(GcnTest, lshrrev_b16_3) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_LSHRREV_B16, VOperand8::V0, SOperand9::V0, SOperand9::V1).SetOpSel({0,1,0,0}).Get());
    auto result = runner->run<u32>(spirv, std::array<u32,2>{0xFFFFFFF2, 0x88881000});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0xFFFF2222);
}

TEST_F(GcnTest, lshlrev_b16_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_LSHLREV_B16, VOperand8::V0, SOperand9::V0, SOperand9::V1).Get());
    auto result = runner->run<u32>(spirv, std::array<u32,2>{0xFFFFFFF3, 0x88888888});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0xFFFF4440);
}

TEST_F(GcnTest, ashrrev_i16_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3A(OpcodeVOP3::V_ASHRREV_I16, VOperand8::V0, SOperand9::V0, SOperand9::V1).Get());
    auto result = runner->run<u32>(spirv, std::array<u32,2>{0x1234FFF3, 0x88888888});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x1234F111);
}

TEST_F(GcnTest, pk_add_f16_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1).Get());
    auto result = runner->run<F16x2>(spirv, std::array{F16x2{half(1.0f), half(2.0f)}, F16x2{half(3.0f), half(4.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, (F16x2{half(4.0f), half(6.0f)}));
}

TEST_F(GcnTest, pk_add_f16_2) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst = VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::Const0, SOperand9::ConstInv2Pi).Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<u32>(spirv, 0U);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x00003118);
}

TEST_F(GcnTest, pk_add_f16_3) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst = VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::Const0, SOperand9::ConstInv2Pi).SetOpSel({0,1,1}).Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<u32>(spirv, 0U);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0);
}

TEST_F(GcnTest, pk_add_f16_4) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst = VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::Const0p5, SOperand9::Const0p5).Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<u32>(spirv, 0U);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x3C00);
}

TEST_F(GcnTest, pk_add_f16_5) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst = VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::Const0, SOperand9::ConstInv2Pi).SetOpSelHi({0,0,0}).Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<u32>(spirv, 0U);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x31183118);
}

TEST_F(GcnTest, pk_add_f16_neg_lo) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1).SetNeg({1,1,0}).Get());
    auto result = runner->run<F16x2>(spirv, std::array{F16x2{half(1.0f), half(2.0f)}, F16x2{half(3.0f), half(4.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, (F16x2{half(-4.0f), half(6.0f)}));
}

TEST_F(GcnTest, pk_add_f16_neg_hi) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1).SetNegHi({1,1,0}).Get());
    auto result = runner->run<F16x2>(spirv, std::array{F16x2{half(1.0f), half(2.0f)}, F16x2{half(3.0f), half(4.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, (F16x2{half(4.0f), half(-6.0f)}));
}

TEST_F(GcnTest, pk_add_f16_op_sel_reversed) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1).SetOpSel({1,1,1}).SetOpSelHi({0,0,0}).Get());
    auto result = runner->run<F16x2>(spirv, std::array{F16x2{half(1.0f), half(2.0f)}, F16x2{half(3.0f), half(4.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, (F16x2{half(6.0f), half(4.0f)}));
}
