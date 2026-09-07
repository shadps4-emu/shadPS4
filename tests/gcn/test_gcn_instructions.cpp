// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include <gtest/gtest.h>
#include <half.hpp>
#include <spirv/unified1/spirv.hpp11>

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

struct SpirvFacts {
    std::unordered_map<u32, u32> capabilities;
    std::unordered_map<u32, u32> builtins;
    std::unordered_map<u32, u32> opcodes;
    std::unordered_map<u32, u32> constant_words;

    u32 CountCapability(spv::Capability capability) const {
        const auto it = capabilities.find(static_cast<u32>(capability));
        return it == capabilities.end() ? 0U : it->second;
    }

    u32 CountBuiltin(spv::BuiltIn builtin) const {
        const auto it = builtins.find(static_cast<u32>(builtin));
        return it == builtins.end() ? 0U : it->second;
    }

    u32 CountOpcode(spv::Op opcode) const {
        const auto it = opcodes.find(static_cast<u32>(opcode));
        return it == opcodes.end() ? 0U : it->second;
    }

    u32 CountConstantWord(u32 value) const {
        const auto it = constant_words.find(value);
        return it == constant_words.end() ? 0U : it->second;
    }
};

SpirvFacts InspectSpirv(std::span<const u32> spirv) {
    EXPECT_GE(spirv.size(), 5U);
    SpirvFacts facts{};
    for (size_t offset = 5; offset < spirv.size();) {
        const u32 instruction = spirv[offset];
        const u32 word_count = instruction >> 16;
        const u32 opcode = instruction & 0xffffU;
        EXPECT_GT(word_count, 0U);
        EXPECT_LE(offset + word_count, spirv.size());
        if (word_count == 0 || offset + word_count > spirv.size()) {
            break;
        }
        ++facts.opcodes[opcode];
        switch (static_cast<spv::Op>(opcode)) {
        case spv::Op::OpCapability:
            EXPECT_GE(word_count, 2U);
            ++facts.capabilities[spirv[offset + 1]];
            break;
        case spv::Op::OpDecorate:
            EXPECT_GE(word_count, 3U);
            if (spirv[offset + 2] == static_cast<u32>(spv::Decoration::BuiltIn)) {
                EXPECT_GE(word_count, 4U);
                ++facts.builtins[spirv[offset + 3]];
            }
            break;
        case spv::Op::OpConstant:
            EXPECT_GE(word_count, 4U);
            if (word_count == 4U) {
                ++facts.constant_words[spirv[offset + 3]];
            }
            break;
        default:
            break;
        }
        offset += word_count;
    }
    return facts;
}

bool FragmentPrologueLoadsSampleCoverage(bool enabled) {
    Shader::Pools pools;
    Shader::Info info{};
    info.stage = Shader::Stage::Fragment;
    info.l_stage = Shader::LogicalStage::Fragment;

    Shader::RuntimeInfo runtime_info{};
    runtime_info.Initialize(Shader::Stage::Fragment);
    runtime_info.fs_info.addr_flags.sample_coverage_ena = 1;
    runtime_info.fs_info.en_flags.sample_coverage_ena = enabled;

    Shader::Profile profile{};
    Shader::IR::Block block{pools.inst_pool};
    Shader::Gcn::Translator translator{info, runtime_info, profile};
    translator.EmitPrologue(&block);

    return std::ranges::any_of(block, [](const Shader::IR::Inst& inst) {
        return inst.GetOpcode() == Shader::IR::Opcode::GetAttributeU32 &&
               inst.Arg(0).Attribute() == Shader::IR::Attribute::SampleCoverage;
    });
}

struct InterpMovResult {
    Shader::Qualifier qualifier{};
    u32 vertex_index{};
};

InterpMovResult TranslateInterpMovToIr(u32 selector) {
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
    Shader::Gcn::GcnInst inst{};
    inst.src[0].code = selector;
    inst.dst[0].field = Shader::Gcn::OperandField::VectorGPR;
    inst.dst[0].code = 0;
    inst.control.vintrp.attr = 0;
    translator.V_INTERP_MOV_F32(inst);

    const auto attr = std::ranges::find_if(block, [](const Shader::IR::Inst& inst) {
        return inst.GetOpcode() == Shader::IR::Opcode::GetAttribute;
    });
    EXPECT_NE(attr, block.end());
    return {
        .qualifier = info.fs_interpolation[0].primary,
        .vertex_index = attr == block.end() ? 0U : attr->Arg(2).U32(),
    };
}

Shader::RuntimeInfo PullModelRuntimeInfo() {
    Shader::RuntimeInfo runtime_info{};
    runtime_info.Initialize(Shader::Stage::Fragment);
    runtime_info.fs_info.addr_flags.persp_pull_model_ena = 1;
    runtime_info.fs_info.color_buffers[0].num_format = AmdGpu::NumberFormat::Float;
    return runtime_info;
}

} // namespace

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

TEST_F(GcnTest, interp_mov_uses_explicit_p10_and_p20_coefficients) {
    const auto p10 = TranslateInterpMovToIr(0);
    EXPECT_EQ(p10.qualifier, Shader::Qualifier::PerVertex);
    EXPECT_EQ(p10.vertex_index, 1U);

    const auto p20 = TranslateInterpMovToIr(1);
    EXPECT_EQ(p20.qualifier, Shader::Qualifier::PerVertex);
    EXPECT_EQ(p20.vertex_index, 2U);
}

TEST_F(GcnTest, khr_barycentrics_reconstruct_pull_model) {
    Shader::Profile profile{};
    profile.supported_spirv = 0x00010600;
    profile.supports_fragment_shader_barycentric = true;
    const auto spirv = TranslateFragmentPullModelToSpirv(profile, PullModelRuntimeInfo());
    const auto facts = InspectSpirv(spirv);

    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::BaryCoordKHR), 1U);
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::FragCoord), 1U);
    EXPECT_EQ(facts.CountCapability(spv::Capability::FragmentBarycentricKHR), 1U);
    EXPECT_EQ(facts.CountOpcode(spv::Op::OpFMul), 2U);
}

TEST_F(GcnTest, amd_barycentrics_use_native_pull_model) {
    Shader::Profile profile{};
    profile.supported_spirv = 0x00010600;
    profile.supports_amd_shader_explicit_vertex_parameter = true;
    const auto spirv = TranslateFragmentPullModelToSpirv(profile, PullModelRuntimeInfo());
    const auto facts = InspectSpirv(spirv);

    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::BaryCoordPullModelAMD), 1U);
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::FragCoord), 0U);
    EXPECT_EQ(facts.CountOpcode(spv::Op::OpFMul), 0U);
}

TEST_F(GcnTest, fragment_front_face_uses_float_sign_bits) {
    Shader::Profile profile{};
    profile.supported_spirv = 0x00010600;
    Shader::RuntimeInfo runtime_info{};
    runtime_info.Initialize(Shader::Stage::Fragment);
    runtime_info.fs_info.addr_flags.front_face_ena = 1;
    runtime_info.fs_info.en_flags.front_face_ena = 1;
    runtime_info.fs_info.color_buffers[0].num_format = AmdGpu::NumberFormat::Float;

    const auto facts = InspectSpirv(TranslateFragmentFrontFaceToSpirv(profile, runtime_info));

    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::FrontFacing), 1U);
    EXPECT_EQ(facts.CountOpcode(spv::Op::OpSelect), 1U);
    EXPECT_GE(facts.CountConstantWord(0x3f800000U), 1U);
    EXPECT_GE(facts.CountConstantWord(0xbf800000U), 1U);
}

TEST_F(GcnTest, fragment_front_face_uses_all_bits) {
    Shader::Profile profile{};
    profile.supported_spirv = 0x00010600;
    Shader::RuntimeInfo runtime_info{};
    runtime_info.Initialize(Shader::Stage::Fragment);
    runtime_info.fs_info.addr_flags.front_face_ena = 1;
    runtime_info.fs_info.en_flags.front_face_ena = 1;
    runtime_info.fs_info.front_face_all_bits = true;
    runtime_info.fs_info.color_buffers[0].num_format = AmdGpu::NumberFormat::Float;

    const auto facts = InspectSpirv(TranslateFragmentFrontFaceToSpirv(profile, runtime_info));

    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::FrontFacing), 1U);
    EXPECT_EQ(facts.CountOpcode(spv::Op::OpSelect), 1U);
    EXPECT_GE(facts.CountConstantWord(1U), 1U);
    EXPECT_EQ(facts.CountConstantWord(0x3f800000U), 0U);
    EXPECT_EQ(facts.CountConstantWord(0xbf800000U), 0U);
}

TEST_F(GcnTest, fragment_sample_coverage_uses_fast_single_sample_path) {
    Shader::Profile profile{};
    profile.supported_spirv = 0x00010600;
    Shader::RuntimeInfo runtime_info{};
    runtime_info.Initialize(Shader::Stage::Fragment);
    runtime_info.fs_info.color_buffers[0].num_format = AmdGpu::NumberFormat::Uint;

    const auto facts = InspectSpirv(TranslateFragmentSampleCoverageToSpirv(profile, runtime_info));

    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::SampleMask), 0U);
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::HelperInvocation), 1U);
    EXPECT_EQ(facts.CountOpcode(spv::Op::OpBitwiseAnd), 0U);
    EXPECT_EQ(facts.CountOpcode(spv::Op::OpSelect), 1U);
    EXPECT_GE(facts.CountConstantWord(1U), 1U);
}

TEST_F(GcnTest, fragment_prologue_loads_enabled_sample_coverage) {
    EXPECT_TRUE(FragmentPrologueLoadsSampleCoverage(true));
    EXPECT_FALSE(FragmentPrologueLoadsSampleCoverage(false));
}

TEST_F(GcnTest, fragment_sample_coverage_masks_multisample_input) {
    Shader::Profile profile{};
    profile.supported_spirv = 0x00010600;
    Shader::RuntimeInfo runtime_info{};
    runtime_info.Initialize(Shader::Stage::Fragment);
    runtime_info.fs_info.num_samples = 4;
    runtime_info.fs_info.color_buffers[0].num_format = AmdGpu::NumberFormat::Uint;

    const auto facts = InspectSpirv(TranslateFragmentSampleCoverageToSpirv(profile, runtime_info));

    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::SampleMask), 1U);
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::HelperInvocation), 1U);
    EXPECT_EQ(facts.CountOpcode(spv::Op::OpBitwiseAnd), 1U);
    EXPECT_EQ(facts.CountOpcode(spv::Op::OpSelect), 1U);
    EXPECT_GE(facts.CountConstantWord(0xfU), 1U);
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
