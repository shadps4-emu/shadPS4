// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>
#include <optional>
#include <unordered_map>

#include <gtest/gtest.h>
#include <half.hpp>
#include <spirv/unified1/GLSL.std.450.h>
#include <spirv/unified1/spirv.hpp11>

#include "gcn_test_runner.hpp"
#include "instructions.hpp"
#include "shader_recompiler/fragment_barycentric.h"
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
    std::vector<std::pair<u32, u32>> selected_odd_even_vertex_indices;
    bool reads_primitive_id{};
    Shader::Qualifier qualifier;
};

struct InterpStageIr {
    std::vector<u32> attribute_vertex_indices;
    u32 fma_count{};
    u32 destination_write_count{};
    Shader::Qualifier qualifier;
};

struct SpirvFacts {
    std::unordered_map<u32, u32> capabilities;
    std::unordered_map<u32, u32> builtins;
    std::unordered_map<u32, u32> glsl_ext_instructions;
    std::unordered_map<u32, u32> opcodes;
    std::unordered_map<u32, u32> composite_extract_indices;

    u32 CountCapability(spv::Capability capability) const {
        const auto it = capabilities.find(static_cast<u32>(capability));
        return it == capabilities.end() ? 0U : it->second;
    }

    u32 CountBuiltin(spv::BuiltIn builtin) const {
        const auto it = builtins.find(static_cast<u32>(builtin));
        return it == builtins.end() ? 0U : it->second;
    }

    u32 CountGlslInstruction(u32 instruction) const {
        const auto it = glsl_ext_instructions.find(instruction);
        return it == glsl_ext_instructions.end() ? 0U : it->second;
    }

    u32 CountOpcode(spv::Op opcode) const {
        const auto it = opcodes.find(static_cast<u32>(opcode));
        return it == opcodes.end() ? 0U : it->second;
    }

    u32 CountCompositeExtractIndex(u32 index) const {
        const auto it = composite_extract_indices.find(index);
        return it == composite_extract_indices.end() ? 0U : it->second;
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
        case spv::Op::OpExtInst:
            EXPECT_GE(word_count, 5U);
            ++facts.glsl_ext_instructions[spirv[offset + 4]];
            break;
        case spv::Op::OpCompositeExtract:
            EXPECT_GE(word_count, 5U);
            ++facts.composite_extract_indices[spirv[offset + 4]];
            break;
        default:
            break;
        }
        offset += word_count;
    }
    return facts;
}

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
        result.reads_thread_mask |= it->GetOpcode() == Shader::IR::Opcode::GetThreadBitScalarReg;
        result.builds_ballot |= it->GetOpcode() == Shader::IR::Opcode::Ballot;
        result.bit_count_count += it->GetOpcode() == Shader::IR::Opcode::BitCount32;
    }
    return result;
}

InterpMovIr TranslateInterpMovToIr(
    u32 selector, bool provoking_last = false,
    AmdGpu::PrimitiveType primitive_type = AmdGpu::PrimitiveType::TriangleList,
    bool supports_provoking_vertex = true, bool independent_triangle_strip_order = false,
    bool supports_fragment_shader_barycentric = true) {
    Shader::Pools pools;
    Shader::Info info{};
    info.stage = Shader::Stage::Fragment;
    info.l_stage = Shader::LogicalStage::Fragment;

    Shader::RuntimeInfo runtime_info{};
    runtime_info.Initialize(Shader::Stage::Fragment);
    runtime_info.fs_info.num_inputs = 1;
    runtime_info.fs_info.inputs[0].param_index = 0;
    runtime_info.fs_info.provoking_vtx_last = provoking_last;
    runtime_info.fs_info.primitive_type = primitive_type;

    Shader::Profile profile{};
    profile.supports_fragment_shader_barycentric = supports_fragment_shader_barycentric;
    profile.supports_provoking_vertex = supports_provoking_vertex;
    profile.tri_strip_vertex_order_independent_of_provoking_vertex =
        independent_triangle_strip_order;

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
            const auto* lhs = it->Arg(0).Inst();
            const auto* rhs = it->Arg(1).Inst();
            if (lhs->GetOpcode() == Shader::IR::Opcode::GetAttribute &&
                rhs->GetOpcode() == Shader::IR::Opcode::GetAttribute) {
                result.subtraction_vertex_indices = {lhs->Arg(2).U32(), rhs->Arg(2).U32()};
            }
        } else if (it->GetOpcode() == Shader::IR::Opcode::SelectF32) {
            const auto* odd_value = it->Arg(1).Inst();
            const auto* even_value = it->Arg(2).Inst();
            if (odd_value->GetOpcode() == Shader::IR::Opcode::GetAttribute &&
                even_value->GetOpcode() == Shader::IR::Opcode::GetAttribute) {
                result.selected_odd_even_vertex_indices.emplace_back(odd_value->Arg(2).U32(),
                                                                     even_value->Arg(2).U32());
            }
        } else if (it->GetOpcode() == Shader::IR::Opcode::GetAttributeU32 &&
                   it->Arg(0).Attribute() == Shader::IR::Attribute::PrimitiveId) {
            result.reads_primitive_id = true;
        }
    }
    return result;
}

InterpStageIr TranslateInterpStageToIr(bool second_phase, bool is_default = false,
                                       bool is_flat = false) {
    Shader::Pools pools;
    Shader::Info info{};
    info.stage = Shader::Stage::Fragment;
    info.l_stage = Shader::LogicalStage::Fragment;

    Shader::RuntimeInfo runtime_info{};
    runtime_info.Initialize(Shader::Stage::Fragment);
    runtime_info.fs_info.num_inputs = 1;
    runtime_info.fs_info.inputs[0].param_index = 0;
    runtime_info.fs_info.inputs[0].is_default = is_default;
    runtime_info.fs_info.inputs[0].is_flat = is_flat;
    runtime_info.fs_info.inputs[0].default_value = 3;
    runtime_info.fs_info.primitive_type = AmdGpu::PrimitiveType::TriangleList;

    Shader::Profile profile{};
    profile.supports_fragment_shader_barycentric = true;

    Shader::IR::Block block{pools.inst_pool};
    Shader::Gcn::Translator translator{info, runtime_info, profile};
    translator.EmitPrologue(&block);
    const auto prologue_size = block.size();

    Shader::Gcn::GcnInst inst{};
    inst.src[0].field = Shader::Gcn::OperandField::VectorGPR;
    inst.src[0].code = 5;
    inst.dst[0].field = Shader::Gcn::OperandField::VectorGPR;
    inst.dst[0].code = 7;
    inst.control.vintrp.attr = 0;
    inst.control.vintrp.chan = 0;
    if (second_phase) {
        translator.V_INTERP_P2_F32(inst);
    } else {
        translator.V_INTERP_P1_F32(inst);
    }

    InterpStageIr result{.qualifier = info.fs_interpolation[0].primary};
    auto it = block.begin();
    std::advance(it, prologue_size);
    for (; it != block.end(); ++it) {
        switch (it->GetOpcode()) {
        case Shader::IR::Opcode::GetAttribute:
            result.attribute_vertex_indices.push_back(it->Arg(2).U32());
            break;
        case Shader::IR::Opcode::FPFma32:
            ++result.fma_count;
            break;
        case Shader::IR::Opcode::SetVectorRegister:
            ++result.destination_write_count;
            break;
        default:
            break;
        }
    }
    return result;
}

Shader::RuntimeInfo BarycentricRuntimeInfo() {
    Shader::RuntimeInfo runtime_info{};
    runtime_info.Initialize(Shader::Stage::Fragment);
    runtime_info.fs_info.addr_flags.persp_sample_ena = 1;
    runtime_info.fs_info.addr_flags.persp_center_ena = 1;
    runtime_info.fs_info.addr_flags.persp_centroid_ena = 1;
    runtime_info.fs_info.addr_flags.persp_pull_model_ena = 1;
    runtime_info.fs_info.addr_flags.linear_sample_ena = 1;
    runtime_info.fs_info.addr_flags.linear_center_ena = 1;
    runtime_info.fs_info.addr_flags.linear_centroid_ena = 1;
    runtime_info.fs_info.primitive_type = AmdGpu::PrimitiveType::TriangleList;
    runtime_info.fs_info.color_buffers[0].num_format = AmdGpu::NumberFormat::Float;
    return runtime_info;
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
//     auto spirv = TranslateToSpirv(VOP2(OpcodeVOP2::V_ADD_F32, VOperand8::V0, SOperand9::V0,
//     VOperand8::V1).Get());
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

TEST_F(GcnTest, interp_mov_loads_p0_flat_without_explicit_pervertex_support) {
    const auto result =
        TranslateInterpMovToIr(2, false, AmdGpu::PrimitiveType::TriangleList, true, false, false);

    EXPECT_EQ(result.qualifier, Shader::Qualifier::Flat);
    EXPECT_EQ(result.attribute_vertex_indices, (std::vector<u32>{0}));
    EXPECT_FALSE(result.subtraction_vertex_indices.has_value());
}

TEST_F(GcnTest, interp_mov_maps_last_provoking_triangle_to_native_parameter_basis) {
    const auto p10 = TranslateInterpMovToIr(0, true);
    EXPECT_EQ(p10.attribute_vertex_indices, (std::vector<u32>{2, 0}));
    ASSERT_TRUE(p10.subtraction_vertex_indices.has_value());
    EXPECT_EQ(*p10.subtraction_vertex_indices, std::pair(0U, 2U));

    const auto p20 = TranslateInterpMovToIr(1, true);
    EXPECT_EQ(p20.attribute_vertex_indices, (std::vector<u32>{2, 1}));
    ASSERT_TRUE(p20.subtraction_vertex_indices.has_value());
    EXPECT_EQ(*p20.subtraction_vertex_indices, std::pair(1U, 2U));

    const auto p0 = TranslateInterpMovToIr(2, true);
    EXPECT_EQ(p0.attribute_vertex_indices, (std::vector<u32>{2}));
}

TEST_F(GcnTest, interp_mov_maps_last_provoking_point_and_line_basis) {
    const auto point = TranslateInterpMovToIr(0, true, AmdGpu::PrimitiveType::PointList);
    EXPECT_EQ(point.attribute_vertex_indices, (std::vector<u32>{0, 1}));
    ASSERT_TRUE(point.subtraction_vertex_indices.has_value());
    EXPECT_EQ(*point.subtraction_vertex_indices, std::pair(1U, 0U));

    const auto line_p10 = TranslateInterpMovToIr(0, true, AmdGpu::PrimitiveType::LineList);
    EXPECT_EQ(line_p10.attribute_vertex_indices, (std::vector<u32>{1, 0}));
    ASSERT_TRUE(line_p10.subtraction_vertex_indices.has_value());
    EXPECT_EQ(*line_p10.subtraction_vertex_indices, std::pair(0U, 1U));

    const auto line_p20 = TranslateInterpMovToIr(1, true, AmdGpu::PrimitiveType::LineStrip);
    EXPECT_EQ(line_p20.attribute_vertex_indices, (std::vector<u32>{1, 2}));
    ASSERT_TRUE(line_p20.subtraction_vertex_indices.has_value());
    EXPECT_EQ(*line_p20.subtraction_vertex_indices, std::pair(2U, 1U));
}

TEST_F(GcnTest, interp_p1_and_p2_evaluate_native_coefficients) {
    const auto p1 = TranslateInterpStageToIr(false);
    EXPECT_EQ(p1.qualifier, Shader::Qualifier::PerVertex);
    EXPECT_EQ(p1.attribute_vertex_indices, (std::vector<u32>{0, 1}));
    EXPECT_EQ(p1.fma_count, 1U);
    EXPECT_EQ(p1.destination_write_count, 1U);

    const auto p2 = TranslateInterpStageToIr(true);
    EXPECT_EQ(p2.qualifier, Shader::Qualifier::PerVertex);
    EXPECT_EQ(p2.attribute_vertex_indices, (std::vector<u32>{0, 2}));
    EXPECT_EQ(p2.fma_count, 1U);
    EXPECT_EQ(p2.destination_write_count, 1U);
}

TEST_F(GcnTest, interp_constant_and_flat_coefficients_follow_p1_p2_isa_semantics) {
    const auto default_p1 = TranslateInterpStageToIr(false, true);
    EXPECT_EQ(default_p1.destination_write_count, 1U);
    EXPECT_TRUE(default_p1.attribute_vertex_indices.empty());
    const auto default_p2 = TranslateInterpStageToIr(true, true);
    EXPECT_EQ(default_p2.destination_write_count, 1U);
    EXPECT_TRUE(default_p2.attribute_vertex_indices.empty());

    const auto flat_p1 = TranslateInterpStageToIr(false, false, true);
    EXPECT_EQ(flat_p1.qualifier, Shader::Qualifier::Flat);
    EXPECT_EQ(flat_p1.destination_write_count, 1U);
    EXPECT_EQ(flat_p1.attribute_vertex_indices, (std::vector<u32>{0}));
    const auto flat_p2 = TranslateInterpStageToIr(true, false, true);
    EXPECT_EQ(flat_p2.qualifier, Shader::Qualifier::Flat);
    EXPECT_EQ(flat_p2.destination_write_count, 0U);
    EXPECT_TRUE(flat_p2.attribute_vertex_indices.empty());
}

TEST_F(GcnTest, interpolation_mapping_covers_khr_primitive_numbering_tables) {
    Shader::Profile profile{};
    profile.supports_fragment_shader_barycentric = true;
    profile.supports_provoking_vertex = true;
    Shader::RuntimeInfo runtime_info{};
    runtime_info.Initialize(Shader::Stage::Fragment);

    static constexpr std::array all_topologies = {
        AmdGpu::PrimitiveType::PointList,       AmdGpu::PrimitiveType::LineList,
        AmdGpu::PrimitiveType::LineStrip,       AmdGpu::PrimitiveType::TriangleList,
        AmdGpu::PrimitiveType::TriangleFan,     AmdGpu::PrimitiveType::TriangleStrip,
        AmdGpu::PrimitiveType::AdjLineList,     AmdGpu::PrimitiveType::AdjLineStrip,
        AmdGpu::PrimitiveType::AdjTriangleList, AmdGpu::PrimitiveType::AdjTriangleStrip,
    };
    for (const auto topology : all_topologies) {
        runtime_info.fs_info.primitive_type = topology;
        const auto mapping = Shader::GetFragmentBarycentricMapping(runtime_info, profile);
        EXPECT_EQ(mapping.even, (std::array<u32, 3>{0, 1, 2}));
        EXPECT_EQ(mapping.odd, mapping.even);
        EXPECT_FALSE(mapping.uses_primitive_parity);
    }

    runtime_info.fs_info.provoking_vtx_last = true;
    runtime_info.fs_info.primitive_type = AmdGpu::PrimitiveType::PointList;
    const auto point = Shader::GetFragmentBarycentricMapping(runtime_info, profile);
    EXPECT_EQ(point.even, (std::array<u32, 3>{0, 1, 2}));
    EXPECT_EQ(point.odd, point.even);
    EXPECT_FALSE(point.uses_primitive_parity);

    for (const auto topology :
         {AmdGpu::PrimitiveType::LineList, AmdGpu::PrimitiveType::LineStrip,
          AmdGpu::PrimitiveType::AdjLineList, AmdGpu::PrimitiveType::AdjLineStrip}) {
        runtime_info.fs_info.primitive_type = topology;
        const auto mapping = Shader::GetFragmentBarycentricMapping(runtime_info, profile);
        EXPECT_EQ(mapping.even, (std::array<u32, 3>{1, 0, 2}));
        EXPECT_EQ(mapping.odd, mapping.even);
        EXPECT_FALSE(mapping.uses_primitive_parity);
    }

    for (const auto topology :
         {AmdGpu::PrimitiveType::TriangleList, AmdGpu::PrimitiveType::TriangleFan,
          AmdGpu::PrimitiveType::TriangleStrip, AmdGpu::PrimitiveType::AdjTriangleList,
          AmdGpu::PrimitiveType::AdjTriangleStrip}) {
        runtime_info.fs_info.primitive_type = topology;
        const auto mapping = Shader::GetFragmentBarycentricMapping(runtime_info, profile);
        EXPECT_EQ(mapping.even, (std::array<u32, 3>{2, 0, 1}));
        EXPECT_EQ(mapping.odd, mapping.even);
        EXPECT_FALSE(mapping.uses_primitive_parity);
    }

    profile.tri_strip_vertex_order_independent_of_provoking_vertex = true;
    runtime_info.fs_info.primitive_type = AmdGpu::PrimitiveType::TriangleStrip;
    const auto independent_strip = Shader::GetFragmentBarycentricMapping(runtime_info, profile);
    EXPECT_EQ(independent_strip.even, (std::array<u32, 3>{2, 0, 1}));
    EXPECT_EQ(independent_strip.odd, (std::array<u32, 3>{1, 2, 0}));
    EXPECT_TRUE(independent_strip.uses_primitive_parity);

    profile.supports_provoking_vertex = false;
    profile.tri_strip_vertex_order_independent_of_provoking_vertex = false;
    runtime_info.fs_info.primitive_type = AmdGpu::PrimitiveType::TriangleStrip;
    const auto unsupported_strip = Shader::GetFragmentBarycentricMapping(runtime_info, profile);
    EXPECT_EQ(unsupported_strip.even, (std::array<u32, 3>{2, 0, 1}));
    EXPECT_EQ(unsupported_strip.odd, (std::array<u32, 3>{1, 2, 0}));
    EXPECT_TRUE(unsupported_strip.uses_primitive_parity);
}

TEST_F(GcnTest, interp_mov_maps_host_last_triangle_strip_without_parity) {
    const auto result =
        TranslateInterpMovToIr(0, true, AmdGpu::PrimitiveType::TriangleStrip, true, false);

    EXPECT_EQ(result.attribute_vertex_indices, (std::vector<u32>{2, 0}));
    EXPECT_TRUE(result.selected_odd_even_vertex_indices.empty());
    EXPECT_FALSE(result.reads_primitive_id);
    ASSERT_TRUE(result.subtraction_vertex_indices.has_value());
    EXPECT_EQ(*result.subtraction_vertex_indices, std::pair(0U, 2U));
}

TEST_F(GcnTest, interp_mov_maps_khr_fan_for_host_last_provoking_order) {
    const auto result = TranslateInterpMovToIr(0, true, AmdGpu::PrimitiveType::TriangleFan, true);

    EXPECT_EQ(result.attribute_vertex_indices, (std::vector<u32>{2, 0}));
    ASSERT_TRUE(result.subtraction_vertex_indices.has_value());
    EXPECT_EQ(*result.subtraction_vertex_indices, std::pair(0U, 2U));
    EXPECT_FALSE(result.reads_primitive_id);
}

TEST_F(GcnTest, interp_mov_selects_khr_odd_adjacency_strip_host_last_order) {
    const auto result =
        TranslateInterpMovToIr(0, true, AmdGpu::PrimitiveType::AdjTriangleStrip, true);

    EXPECT_EQ(result.attribute_vertex_indices, (std::vector<u32>{2, 0}));
    EXPECT_TRUE(result.selected_odd_even_vertex_indices.empty());
    EXPECT_FALSE(result.reads_primitive_id);
    ASSERT_TRUE(result.subtraction_vertex_indices.has_value());
    EXPECT_EQ(*result.subtraction_vertex_indices, std::pair(0U, 2U));
}

TEST_F(GcnTest, khr_barycentrics_emit_one_builtin_per_mode_family) {
    Shader::Profile profile{};
    profile.supported_spirv = 0x00010600;
    profile.supports_fragment_shader_barycentric = true;
    profile.supports_provoking_vertex = true;
    const auto spirv = TranslateFragmentBarycentricsToSpirv(profile, BarycentricRuntimeInfo());
    const auto facts = InspectSpirv(spirv);

    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::BaryCoordKHR), 1U);
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::BaryCoordNoPerspKHR), 1U);
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::SampleId), 1U);
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::FragCoord), 1U);
    EXPECT_EQ(facts.CountGlslInstruction(GLSLstd450InterpolateAtCentroid), 4U);
    EXPECT_EQ(facts.CountGlslInstruction(GLSLstd450InterpolateAtSample), 4U);
    EXPECT_EQ(facts.CountCapability(spv::Capability::FragmentBarycentricKHR), 1U);
    EXPECT_EQ(facts.CountCapability(spv::Capability::InterpolationFunction), 1U);
    EXPECT_EQ(facts.CountCapability(spv::Capability::SampleRateShading), 1U);
    EXPECT_EQ(facts.CountOpcode(spv::Op::OpFMul), 2U);
}

TEST_F(GcnTest, amd_barycentrics_use_all_native_builtins) {
    Shader::Profile profile{};
    profile.supported_spirv = 0x00010600;
    profile.supports_amd_shader_explicit_vertex_parameter = true;
    const auto spirv = TranslateFragmentBarycentricsToSpirv(profile, BarycentricRuntimeInfo());
    const auto facts = InspectSpirv(spirv);

    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::BaryCoordSmoothAMD), 1U);
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::BaryCoordSmoothCentroidAMD), 1U);
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::BaryCoordSmoothSampleAMD), 1U);
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::BaryCoordNoPerspAMD), 1U);
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::BaryCoordNoPerspCentroidAMD), 1U);
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::BaryCoordNoPerspSampleAMD), 1U);
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::BaryCoordPullModelAMD), 1U);
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::FragCoord), 0U);
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::SampleId), 0U);
    EXPECT_EQ(facts.CountGlslInstruction(GLSLstd450InterpolateAtCentroid), 0U);
    EXPECT_EQ(facts.CountGlslInstruction(GLSLstd450InterpolateAtSample), 0U);
    EXPECT_EQ(facts.CountCapability(spv::Capability::SampleRateShading), 1U);
    EXPECT_EQ(facts.CountOpcode(spv::Op::OpFMul), 0U);
}

TEST_F(GcnTest, khr_and_amd_support_scalar_pervertex_parameters) {
    auto runtime_info = BarycentricRuntimeInfo();
    runtime_info.fs_info.num_inputs = 1;
    runtime_info.fs_info.inputs[0].param_index = 0;

    Shader::Profile khr_profile{};
    khr_profile.supported_spirv = 0x00010600;
    khr_profile.supports_fragment_shader_barycentric = true;
    const auto khr_facts =
        InspectSpirv(TranslateFragmentScalarPerVertexToSpirv(khr_profile, runtime_info));
    EXPECT_EQ(khr_facts.CountCapability(spv::Capability::FragmentBarycentricKHR), 1U);

    Shader::Profile amd_profile{};
    amd_profile.supported_spirv = 0x00010600;
    amd_profile.supports_amd_shader_explicit_vertex_parameter = true;
    const auto amd_facts =
        InspectSpirv(TranslateFragmentScalarPerVertexToSpirv(amd_profile, runtime_info));
    EXPECT_EQ(amd_facts.CountCapability(spv::Capability::InterpolationFunction), 1U);
}

TEST_F(GcnTest, khr_point_and_line_barycentrics_follow_spec_vertex_slots) {
    Shader::Profile profile{};
    profile.supported_spirv = 0x00010600;
    profile.supports_fragment_shader_barycentric = true;
    profile.supports_provoking_vertex = true;
    auto runtime_info = BarycentricRuntimeInfo();
    runtime_info.fs_info.provoking_vtx_last = true;

    runtime_info.fs_info.primitive_type = AmdGpu::PrimitiveType::PointList;
    const auto point = InspectSpirv(TranslateFragmentBarycentricsToSpirv(profile, runtime_info));
    EXPECT_EQ(point.CountCompositeExtractIndex(0), 0U);
    EXPECT_EQ(point.CountCompositeExtractIndex(1), 7U);
    EXPECT_EQ(point.CountCompositeExtractIndex(2), 7U);

    runtime_info.fs_info.primitive_type = AmdGpu::PrimitiveType::LineList;
    const auto line = InspectSpirv(TranslateFragmentBarycentricsToSpirv(profile, runtime_info));
    EXPECT_EQ(line.CountCompositeExtractIndex(0), 7U);
    EXPECT_EQ(line.CountCompositeExtractIndex(1), 0U);
    EXPECT_EQ(line.CountCompositeExtractIndex(2), 7U);
}

TEST_F(GcnTest, khr_host_last_odd_strip_uses_spec_primitive_order) {
    Shader::Profile profile{};
    profile.supported_spirv = 0x00010600;
    profile.supports_fragment_shader_barycentric = true;
    profile.supports_provoking_vertex = true;
    profile.tri_strip_vertex_order_independent_of_provoking_vertex = false;
    auto runtime_info = BarycentricRuntimeInfo();
    runtime_info.fs_info.provoking_vtx_last = true;
    runtime_info.fs_info.primitive_type = AmdGpu::PrimitiveType::TriangleStrip;

    const auto facts = InspectSpirv(TranslateFragmentBarycentricsToSpirv(profile, runtime_info));
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::PrimitiveId), 0U);
    EXPECT_EQ(facts.CountOpcode(spv::Op::OpSelect), 0U);

    profile.tri_strip_vertex_order_independent_of_provoking_vertex = true;
    const auto independent_facts =
        InspectSpirv(TranslateFragmentBarycentricsToSpirv(profile, runtime_info));
    EXPECT_EQ(independent_facts.CountBuiltin(spv::BuiltIn::PrimitiveId), 1U);
    EXPECT_EQ(independent_facts.CountCapability(spv::Capability::Geometry), 1U);
    EXPECT_GT(independent_facts.CountOpcode(spv::Op::OpSelect), 0U);
}

TEST_F(GcnTest, khr_fan_without_host_last_provoking_keeps_first_vertex_order) {
    Shader::Profile profile{};
    profile.supported_spirv = 0x00010600;
    profile.supports_fragment_shader_barycentric = true;
    auto runtime_info = BarycentricRuntimeInfo();
    runtime_info.fs_info.provoking_vtx_last = true;
    runtime_info.fs_info.primitive_type = AmdGpu::PrimitiveType::TriangleFan;

    const auto facts = InspectSpirv(TranslateFragmentBarycentricsToSpirv(profile, runtime_info));
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::PrimitiveId), 0U);
    EXPECT_EQ(facts.CountOpcode(spv::Op::OpSelect), 0U);
    EXPECT_EQ(facts.CountCompositeExtractIndex(0), 7U);
    EXPECT_EQ(facts.CountCompositeExtractIndex(1), 0U);
    EXPECT_EQ(facts.CountCompositeExtractIndex(2), 7U);
}

TEST_F(GcnTest, khr_adjacency_strip_host_last_provoking_has_stable_native_basis) {
    Shader::Profile profile{};
    profile.supported_spirv = 0x00010600;
    profile.supports_fragment_shader_barycentric = true;
    profile.supports_provoking_vertex = true;
    auto runtime_info = BarycentricRuntimeInfo();
    runtime_info.fs_info.provoking_vtx_last = true;
    runtime_info.fs_info.primitive_type = AmdGpu::PrimitiveType::AdjTriangleStrip;

    const auto facts = InspectSpirv(TranslateFragmentBarycentricsToSpirv(profile, runtime_info));
    EXPECT_EQ(facts.CountBuiltin(spv::BuiltIn::PrimitiveId), 0U);
    EXPECT_EQ(facts.CountOpcode(spv::Op::OpSelect), 0U);
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

    auto spirv = TranslateToSpirv(
        VOP2(OpcodeVOP2::V_ADD_F32, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
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

    auto spirv = TranslateToSpirv(
        VOP2(OpcodeVOP2::V_ADD_F32, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
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

    auto spirv = TranslateToSpirv(
        VOP2(OpcodeVOP2::V_ADD_F16, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
    auto result = runner->run<F16x2>(spirv, std::array{F16x2{half(1.0f)}, F16x2{half(1.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, F16x2{half(2.0f)});
}

TEST_F(GcnTest, add_f16_clamp) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv =
        TranslateToSpirv(VOP3A(OpcodeVOP3::V_ADD_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1)
                             .SetClamp(true)
                             .Get());
    auto result = runner->run<F16x2>(spirv, std::array{F16x2{half(1.0f)}, F16x2{half(1.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, F16x2{half(1.0f)});
}

TEST_F(GcnTest, add_f16_neg) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv =
        TranslateToSpirv(VOP3A(OpcodeVOP3::V_ADD_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1)
                             .SetNeg({true, true, false})
                             .Get());
    auto result = runner->run<F16x2>(spirv, std::array{F16x2{half(1.0f)}, F16x2{half(1.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ((*result).a, half(-2.0f));
}

TEST_F(GcnTest, add_f16_opsel_hi) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv =
        TranslateToSpirv(VOP3A(OpcodeVOP3::V_ADD_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1)
                             .SetOpSel({true, true, false, true})
                             .Get());
    auto result = runner->run<F16x2>(
        spirv, std::array{F16x2{half(1.0f), half(2.0f)}, F16x2{half(1.0f), half(2.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ((*result).a, half(1.0f));
    EXPECT_EQ((*result).b, half(4.0f));
}

TEST_F(GcnTest, sub_f16) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP2(OpcodeVOP2::V_SUB_F16, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
    auto result = runner->run<F16x2>(spirv, std::array{F16x2{half(0.0f)}, F16x2{half(1.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, F16x2{half(-1.0f)});
}

TEST_F(GcnTest, mul_legacy_nan) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP2(OpcodeVOP2::V_MUL_LEGACY_F32, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
    auto result = runner->run<u32>(spirv, std::array{u32(0), u32(0x7fc00000)});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0);
}

TEST_F(GcnTest, mul_nan) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP2(OpcodeVOP2::V_MUL_F32, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
    auto result = runner->run<float>(spirv, std::array{u32(0), u32(0x7fc00000)});

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(*result));
}

TEST_F(GcnTest, min_legacy_nan) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP2(OpcodeVOP2::V_MIN_LEGACY_F32, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
    auto result = runner->run<u32>(spirv, std::array{u32(0), u32(0x7fc00000)});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x7fc00000);
}

TEST_F(GcnTest, min_nan) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP2(OpcodeVOP2::V_MIN_F32, VOperand8::V0, SOperand9::V0, VOperand8::V1).Get());
    auto result = runner->run<float>(spirv, std::array{u32(0), u32(0x7fc00000)});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0);
}

TEST_F(GcnTest, add3_u32_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_ADD3_U32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .Get());
    auto result = runner->run<u32>(spirv, std::array{0, 1, 2});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 3);
}

TEST_F(GcnTest, add3_u32_2) {
    auto runner = gcn_test::Runner::instance().value();
    auto big = 2000000000;

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_ADD3_U32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .Get());
    auto result = runner->run<u32>(spirv, std::array{big, big, big});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x65A0BC00);
}

TEST_F(GcnTest, add3_u32_3) {
    auto runner = gcn_test::Runner::instance().value();
    auto big = 2000000000;

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_ADD3_U32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .SetClamp(true)
            .Get());
    auto result = runner->run<u32>(spirv, std::array{big, big, big});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x65A0BC00);
}

TEST_F(GcnTest, add3_u32_4) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_ADD3_U32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .SetNeg({1, 0, 0})
            .Get());
    auto result = runner->run<u32>(spirv, std::array{0, 1, 2});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x80000003);
}

TEST_F(GcnTest, or3_u32_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_OR3_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .Get());
    auto result = runner->run<u32>(spirv, std::array<u32, 3>{0xF0F0F0F0, 0x07070707, 0x11111111});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0xF7F7F7F7);
}

TEST_F(GcnTest, or3_u32_2) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_OR3_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .Get());
    auto result = runner->run<u32>(spirv, std::array{0x07070707, 0x11111111, 0x40404040});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x57575757);
}

TEST_F(GcnTest, or3_u32_3) {
    auto runner = gcn_test::Runner::instance().value();
    auto big = 2000000000;

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_OR3_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .SetClamp(true)
            .Get());
    auto result = runner->run<u32>(spirv, std::array{0x07070707, 0x11111111, 0x40404040});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x57575757);
}

TEST_F(GcnTest, or3_u32_4) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_OR3_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .SetNeg({0, 0, 1})
            .Get());
    auto result = runner->run<u32>(spirv, std::array{0x07070707, 0x11111111, 0x40404040});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0xD7575757);
}

TEST_F(GcnTest, and_or_b32_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .Get());
    auto result = runner->run<u32>(spirv, std::array<u32, 3>{0xF0F0F0F0, 0x07070707, 0x11111111});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x11111111);
}

TEST_F(GcnTest, and_or_b32_2) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .SetOmod(Omod::Mul2)
            .Get());
    auto result = runner->run<u32>(spirv, std::array{0x40404040, 0x40404040, 0x40404040});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x40404040);
}

TEST_F(GcnTest, and_or_b32_3) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .SetClamp(true)
            .Get());
    auto result = runner->run<u32>(spirv, std::array{0x40404040, 0x40404040, 0x40404040});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x40404040);
}

TEST_F(GcnTest, and_or_b32_4) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .SetNeg({1, 0, 0})
            .Get());
    auto result = runner->run<u32>(spirv, std::array<u32, 3>{0x07070707, 0x11111111, 0xF0F0F0F0});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0xF1F1F1F1);
}

TEST_F(GcnTest, and_or_b32_5) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .SetNeg({1, 0, 0})
            .SetAbs({1, 0, 0})
            .Get());
    auto result = runner->run<u32>(spirv, std::array<u32, 3>{0x77777777, 0xB0B0B0B0, 0x11111111});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0xB1313131);
}

TEST_F(GcnTest, and_or_b32_6) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .SetOmod(Omod::Mul2)
            .Get());
    auto result = runner->run<u32>(spirv, std::array<u32, 3>{0x40404040, 0xB0B0B0B0, 0x11111111});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x11111111);
}

TEST_F(GcnTest, and_or_b32_7) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .SetOmod(Omod::Div2)
            .Get());
    auto result = runner->run<u32>(spirv, std::array<u32, 3>{0xB0B0B0B0, 0x77777777, 0x40404040});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x70707070);
}

TEST_F(GcnTest, and_or_b32_8) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_AND_OR_B32, VOperand8::V0, SOperand9::V0, SOperand9::V1, SOperand9::V2)
            .SetAbs({1, 1, 0})
            .Get());
    auto result = runner->run<u32>(spirv, std::array<u32, 3>{0xB0B0B0B0, 0x11111111, 0x11111111});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x11111111);
}

TEST_F(GcnTest, mad_mix_f32_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst = VOP3P(OpcodeVOP3P::V_MAD_MIX_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1,
                      SOperand9::V2)
                    .SetOpSelHi({0})
                    .Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<float>(spirv, std::array{2.0f, 3.0f, 4.0f});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 10.0f);
}

TEST_F(GcnTest, mad_mix_f32_2) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst = VOP3P(OpcodeVOP3P::V_MAD_MIX_F32, VOperand8::V0, SOperand9::V0, SOperand9::V1,
                      SOperand9::V2)
                    .SetOpSelHi({1, 1, 0})
                    .SetOpSel({1, 0, 0})
                    .Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<float>(
        spirv, std::array<u32, 3>{std::bit_cast<u32>(F16x2{half(44.0f), half(0.5f)}),
                                  std::bit_cast<u32>(F16x2{half(44.0f), half(0.5f)}),
                                  std::bit_cast<u32>(4.0f)});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 26.0f);
}

TEST_F(GcnTest, mad_mixlo_f16_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst = VOP3P(OpcodeVOP3P::V_MAD_MIXLO_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1,
                      SOperand9::V2)
                    .SetOpSelHi({1, 1, 0})
                    .SetOpSel({1, 0, 0})
                    .Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<F16x2>(
        spirv, std::array<u32, 3>{std::bit_cast<u32>(F16x2{half(44.0f), half(0.5f)}),
                                  std::bit_cast<u32>(F16x2{half(44.0f), half(0.5f)}),
                                  std::bit_cast<u32>(4.0f)});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, (F16x2{half(26.0f), half(0.5f)}));
}

TEST_F(GcnTest, mad_mixhi_f16_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst = VOP3P(OpcodeVOP3P::V_MAD_MIXHI_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1,
                      SOperand9::V2)
                    .SetOpSelHi({1, 1, 0})
                    .SetOpSel({1, 0, 0})
                    .Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<F16x2>(
        spirv, std::array<u32, 3>{std::bit_cast<u32>(F16x2{half(44.0f), half(0.5f)}),
                                  std::bit_cast<u32>(F16x2{half(44.0f), half(0.5f)}),
                                  std::bit_cast<u32>(4.0f)});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, (F16x2{half(44.0f), half(26.0f)}));
}

TEST_F(GcnTest, lshrrev_b16_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_LSHRREV_B16, VOperand8::V0, SOperand9::V0, SOperand9::V1).Get());
    auto result = runner->run<u32>(spirv, std::array<u32, 2>{0xFFFFFFF2, 0x88881000});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0xFFFF0400);
}

TEST_F(GcnTest, lshrrev_b16_2) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_LSHRREV_B16, VOperand8::V0, SOperand9::V0, SOperand9::V1)
            .SetOpSel({0, 0, 0, 1})
            .Get());
    auto result = runner->run<u32>(spirv, std::array<u32, 2>{0xFFFFFFF2, 0x88881000});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x0400FFF2);
}

TEST_F(GcnTest, lshrrev_b16_3) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_LSHRREV_B16, VOperand8::V0, SOperand9::V0, SOperand9::V1)
            .SetOpSel({0, 1, 0, 0})
            .Get());
    auto result = runner->run<u32>(spirv, std::array<u32, 2>{0xFFFFFFF2, 0x88881000});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0xFFFF2222);
}

TEST_F(GcnTest, lshlrev_b16_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_LSHLREV_B16, VOperand8::V0, SOperand9::V0, SOperand9::V1).Get());
    auto result = runner->run<u32>(spirv, std::array<u32, 2>{0xFFFFFFF3, 0x88888888});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0xFFFF4440);
}

TEST_F(GcnTest, ashrrev_i16_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3A(OpcodeVOP3::V_ASHRREV_I16, VOperand8::V0, SOperand9::V0, SOperand9::V1).Get());
    auto result = runner->run<u32>(spirv, std::array<u32, 2>{0x1234FFF3, 0x88888888});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x1234F111);
}

TEST_F(GcnTest, pk_add_f16_1) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1).Get());
    auto result = runner->run<F16x2>(
        spirv, std::array{F16x2{half(1.0f), half(2.0f)}, F16x2{half(3.0f), half(4.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, (F16x2{half(4.0f), half(6.0f)}));
}

TEST_F(GcnTest, pk_add_f16_2) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst =
        VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::Const0, SOperand9::ConstInv2Pi)
            .Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<u32>(spirv, 0U);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x00003118);
}

TEST_F(GcnTest, pk_add_f16_3) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst =
        VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::Const0, SOperand9::ConstInv2Pi)
            .SetOpSel({0, 1, 1})
            .Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<u32>(spirv, 0U);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0);
}

TEST_F(GcnTest, pk_add_f16_4) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst =
        VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::Const0p5, SOperand9::Const0p5)
            .Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<u32>(spirv, 0U);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x3C00);
}

TEST_F(GcnTest, pk_add_f16_5) {
    auto runner = gcn_test::Runner::instance().value();

    auto inst =
        VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::Const0, SOperand9::ConstInv2Pi)
            .SetOpSelHi({0, 0, 0})
            .Get();
    auto spirv = TranslateToSpirv(inst);
    auto result = runner->run<u32>(spirv, 0U);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0x31183118);
}

TEST_F(GcnTest, pk_add_f16_neg_lo) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1)
            .SetNeg({1, 1, 0})
            .Get());
    auto result = runner->run<F16x2>(
        spirv, std::array{F16x2{half(1.0f), half(2.0f)}, F16x2{half(3.0f), half(4.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, (F16x2{half(-4.0f), half(6.0f)}));
}

TEST_F(GcnTest, pk_add_f16_neg_hi) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1)
            .SetNegHi({1, 1, 0})
            .Get());
    auto result = runner->run<F16x2>(
        spirv, std::array{F16x2{half(1.0f), half(2.0f)}, F16x2{half(3.0f), half(4.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, (F16x2{half(4.0f), half(-6.0f)}));
}

TEST_F(GcnTest, pk_add_f16_op_sel_reversed) {
    auto runner = gcn_test::Runner::instance().value();

    auto spirv = TranslateToSpirv(
        VOP3P(OpcodeVOP3P::V_PK_ADD_F16, VOperand8::V0, SOperand9::V0, SOperand9::V1)
            .SetOpSel({1, 1, 1})
            .SetOpSelHi({0, 0, 0})
            .Get());
    auto result = runner->run<F16x2>(
        spirv, std::array{F16x2{half(1.0f), half(2.0f)}, F16x2{half(3.0f), half(4.0f)}});

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, (F16x2{half(6.0f), half(4.0f)}));
}
