// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <span>
#include <boost/container/static_vector.hpp>
#include "common/types.h"
#include "shader_recompiler/frontend/tessellation.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/amdgpu/types.h"

namespace Shader {

enum class Stage : u32 {
    Fragment,
    Vertex,
    Geometry,
    Export,
    Hull,
    Local,
    Compute,
};
constexpr u32 MaxStageTypes = 7;

// Vertex intentionally comes after TCS/TES because in a tess pipeline,
// we need to find the ls_stride from tess constants V# (in order to define an output attribute
// array with correct length), and finding the tess constant V# requires analysis while compiling
// TCS/TES (for now)
enum class LogicalStage : u32 {
    Fragment,
    TessellationControl,
    TessellationEval,
    Vertex,
    Geometry,
    GsCopy,
    Compute,
};

[[nodiscard]] constexpr Stage StageFromIndex(size_t index) noexcept {
    return static_cast<Stage>(index);
}

struct LocalRuntimeInfo {
    u32 ls_stride;
    bool links_with_tcs;

    auto operator<=>(const LocalRuntimeInfo&) const noexcept = default;
};

struct ExportRuntimeInfo {
    u32 vertex_data_size;

    auto operator<=>(const ExportRuntimeInfo&) const noexcept = default;
};

enum class VsOutput : u8 {
    None,
    PointSprite,
    EdgeFlag,
    KillFlag,
    GsCutFlag,
    GsMrtIndex,
    GsVpIndex,
    CullDist0,
    CullDist1,
    CullDist2,
    CullDist3,
    CullDist4,
    CullDist5,
    CullDist6,
    CullDist7,
    ClipDist0,
    ClipDist1,
    ClipDist2,
    ClipDist3,
    ClipDist4,
    ClipDist5,
    ClipDist6,
    ClipDist7,
};
using VsOutputMap = std::array<VsOutput, 4>;

struct VertexRuntimeInfo {
    u32 num_outputs;
    std::array<VsOutputMap, 3> outputs;
    bool emulate_depth_negative_one_to_one{};
    // Domain
    AmdGpu::TessellationType tess_type;
    AmdGpu::TessellationTopology tess_topology;
    AmdGpu::TessellationPartitioning tess_partitioning;
    u32 hs_output_cp_stride{};

    bool operator==(const VertexRuntimeInfo& other) const noexcept {
        return emulate_depth_negative_one_to_one == other.emulate_depth_negative_one_to_one &&
               tess_type == other.tess_type && tess_topology == other.tess_topology &&
               tess_partitioning == other.tess_partitioning &&
               hs_output_cp_stride == other.hs_output_cp_stride;
    }

    void InitFromTessConstants(Shader::TessellationDataConstantBuffer& tess_constants) {
        hs_output_cp_stride = tess_constants.m_hsCpStride;
    }
};

struct HullRuntimeInfo {
    // from registers
    u32 output_control_points;

    // from HullStateConstants in HsProgram (TODO dont rely on this)
    u32 tess_factor_stride;

    // from tess constants buffer
    u32 ls_stride;
    u32 hs_output_cp_stride;
    u32 hs_num_patch;
    u32 hs_output_base;
    u32 patch_const_size;
    u32 patch_const_base;
    u32 patch_output_size;
    u32 first_edge_tess_factor_index;

    auto operator<=>(const HullRuntimeInfo&) const noexcept = default;

    bool IsPassthrough() const {
        return hs_output_base == 0;
    };

    void InitFromTessConstants(Shader::TessellationDataConstantBuffer& tess_constants) {
        ls_stride = tess_constants.m_lsStride;
        hs_output_cp_stride = tess_constants.m_hsCpStride;
        hs_num_patch = tess_constants.m_hsNumPatch;
        hs_output_base = tess_constants.m_hsOutputBase;
        patch_const_size = tess_constants.m_patchConstSize;
        patch_const_base = tess_constants.m_patchConstBase;
        patch_output_size = tess_constants.m_patchOutputSize;
        first_edge_tess_factor_index = tess_constants.m_firstEdgeTessFactorIndex;
    }
};

static constexpr auto GsMaxOutputStreams = 4u;
using GsOutputPrimTypes = std::array<AmdGpu::GsOutputPrimitiveType, GsMaxOutputStreams>;
struct GeometryRuntimeInfo {
    u32 num_invocations{};
    u32 output_vertices{};
    u32 in_vertex_data_size{};
    u32 out_vertex_data_size{};
    AmdGpu::PrimitiveType in_primitive;
    GsOutputPrimTypes out_primitive;
    std::span<const u32> vs_copy;
    u64 vs_copy_hash;

    bool operator==(const GeometryRuntimeInfo& other) const noexcept {
        return num_invocations && other.num_invocations &&
               output_vertices == other.output_vertices && in_primitive == other.in_primitive &&
               std::ranges::equal(out_primitive, other.out_primitive);
    }
};

enum class MrtSwizzle : u8 {
    Identity = 0,
    Alt = 1,
    Reverse = 2,
    ReverseAlt = 3,
};
static constexpr u32 MaxColorBuffers = 8;

struct FragmentRuntimeInfo {
    struct PsInput {
        u8 param_index;
        bool is_default;
        bool is_flat;
        u8 default_value;

        auto operator<=>(const PsInput&) const noexcept = default;
    };
    AmdGpu::Liverpool::PsInput en_flags;
    AmdGpu::Liverpool::PsInput addr_flags;
    u32 num_inputs;
    std::array<PsInput, 32> inputs;
    struct PsColorBuffer {
        AmdGpu::NumberFormat num_format;
        MrtSwizzle mrt_swizzle;

        auto operator<=>(const PsColorBuffer&) const noexcept = default;
    };
    std::array<PsColorBuffer, MaxColorBuffers> color_buffers;

    bool operator==(const FragmentRuntimeInfo& other) const noexcept {
        return std::ranges::equal(color_buffers, other.color_buffers) &&
               en_flags.raw == other.en_flags.raw && addr_flags.raw == other.addr_flags.raw &&
               num_inputs == other.num_inputs &&
               std::ranges::equal(inputs.begin(), inputs.begin() + num_inputs, other.inputs.begin(),
                                  other.inputs.begin() + num_inputs);
    }
};

struct ComputeRuntimeInfo {
    u32 shared_memory_size;
    std::array<u32, 3> workgroup_size;
    std::array<bool, 3> tgid_enable;

    bool operator==(const ComputeRuntimeInfo& other) const noexcept {
        return workgroup_size == other.workgroup_size && tgid_enable == other.tgid_enable;
    }
};

/**
 * Stores information relevant to shader compilation sourced from liverpool registers.
 * It may potentially differ with the same shader module so must be checked.
 * It's also possible to store any other custom information that needs to be part of shader key.
 */
struct RuntimeInfo {
    Stage stage;
    u32 num_user_data;
    u32 num_input_vgprs;
    u32 num_allocated_vgprs;
    AmdGpu::FpDenormMode fp_denorm_mode32;
    AmdGpu::FpRoundMode fp_round_mode32;
    union {
        LocalRuntimeInfo ls_info;
        ExportRuntimeInfo es_info;
        VertexRuntimeInfo vs_info;
        HullRuntimeInfo hs_info;
        GeometryRuntimeInfo gs_info;
        FragmentRuntimeInfo fs_info;
        ComputeRuntimeInfo cs_info;
    };

    RuntimeInfo(Stage stage_) {
        memset(this, 0, sizeof(*this));
        stage = stage_;
    }

    bool operator==(const RuntimeInfo& other) const noexcept {
        switch (stage) {
        case Stage::Fragment:
            return fs_info == other.fs_info;
        case Stage::Vertex:
            return vs_info == other.vs_info;
        case Stage::Compute:
            return cs_info == other.cs_info;
        case Stage::Export:
            return es_info == other.es_info;
        case Stage::Geometry:
            return gs_info == other.gs_info;
        case Stage::Hull:
            return hs_info == other.hs_info;
        case Stage::Local:
            return ls_info == other.ls_info;
        default:
            return true;
        }
    }
};

} // namespace Shader
