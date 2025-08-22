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

// Vertex intentionally comes after TCS/TES due to order of compilation
enum class LogicalStage : u32 {
    Fragment,
    TessellationControl,
    TessellationEval,
    Vertex,
    Geometry,
    Compute,
    NumLogicalStages
};

constexpr u32 MaxStageTypes = static_cast<u32>(LogicalStage::NumLogicalStages);

[[nodiscard]] constexpr Stage StageFromIndex(size_t index) noexcept {
    return static_cast<Stage>(index);
}

struct LocalRuntimeInfo {
    u32 ls_stride;

    auto operator<=>(const LocalRuntimeInfo&) const noexcept = default;
};

struct ExportRuntimeInfo {
    u32 vertex_data_size;

    auto operator<=>(const ExportRuntimeInfo&) const noexcept = default;
};

enum class Output : u8 {
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
using OutputMap = std::array<Output, 4>;

struct VertexRuntimeInfo {
    u32 num_outputs;
    std::array<OutputMap, 3> outputs;
    bool emulate_depth_negative_one_to_one{};
    bool clip_disable{};
    u32 step_rate_0;
    u32 step_rate_1;
    // Domain
    AmdGpu::TessellationType tess_type;
    AmdGpu::TessellationTopology tess_topology;
    AmdGpu::TessellationPartitioning tess_partitioning;
    u32 hs_output_cp_stride{};

    bool operator==(const VertexRuntimeInfo& other) const noexcept {
        return emulate_depth_negative_one_to_one == other.emulate_depth_negative_one_to_one &&
               clip_disable == other.clip_disable && tess_type == other.tess_type &&
               tess_topology == other.tess_topology &&
               tess_partitioning == other.tess_partitioning &&
               hs_output_cp_stride == other.hs_output_cp_stride &&
               step_rate_0 == other.step_rate_0 && step_rate_1 == other.step_rate_1;
    }

    void InitFromTessConstants(Shader::TessellationDataConstantBuffer& tess_constants) {
        hs_output_cp_stride = tess_constants.hs_cp_stride;
    }
};

struct HullRuntimeInfo {
    // from registers
    u32 num_input_control_points;
    u32 num_threads;
    AmdGpu::TessellationType tess_type;

    // from tess constants buffer
    u32 ls_stride;
    u32 hs_output_cp_stride;
    u32 hs_output_base;

    auto operator<=>(const HullRuntimeInfo&) const noexcept = default;

    // It might be possible for a non-passthrough TCS to have these conditions, in some
    // dumb situation.
    // In that case, it should be fine to assume passthrough and declare some extra
    // output control points and attributes that shouldnt be read by the TES anyways
    bool IsPassthrough() const {
        return hs_output_base == 0 && ls_stride == hs_output_cp_stride && num_threads == 1;
    };

    // regs.ls_hs_config.hs_output_control_points contains the number of threads, which
    // isn't exactly the number of output control points.
    // For passthrough shaders, the register field is set to 1, so use the number of
    // input control points
    u32 NumOutputControlPoints() const {
        return IsPassthrough() ? num_input_control_points : num_threads;
    }

    void InitFromTessConstants(Shader::TessellationDataConstantBuffer& tess_constants) {
        ls_stride = tess_constants.ls_stride;
        hs_output_cp_stride = tess_constants.hs_cp_stride;
        hs_output_base = tess_constants.hs_output_base;
    }
};

static constexpr auto GsMaxOutputStreams = 4u;
using GsOutputPrimTypes = std::array<AmdGpu::GsOutputPrimitiveType, GsMaxOutputStreams>;
struct GeometryRuntimeInfo {
    u32 num_outputs;
    std::array<OutputMap, 3> outputs;
    u32 num_invocations{};
    u32 output_vertices{};
    u32 in_vertex_data_size{};
    u32 out_vertex_data_size{};
    AmdGpu::PrimitiveType in_primitive;
    GsOutputPrimTypes out_primitive;
    AmdGpu::Liverpool::GsMode::Mode mode;
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

struct PsColorBuffer {
    AmdGpu::DataFormat data_format : 6;
    AmdGpu::NumberFormat num_format : 4;
    AmdGpu::NumberConversion num_conversion : 3;
    AmdGpu::Liverpool::ShaderExportFormat export_format : 4;
    u32 needs_unorm_fixup : 1;
    u32 pad : 20;
    AmdGpu::CompMapping swizzle;

    bool operator==(const PsColorBuffer& other) const noexcept = default;
};

struct FragmentRuntimeInfo {
    struct PsInput {
        u8 param_index;
        bool is_default;
        bool is_flat;
        u8 default_value;

        bool IsDefault() const {
            return is_default && !is_flat;
        }

        bool operator==(const PsInput&) const noexcept = default;
    };
    AmdGpu::Liverpool::PsInput en_flags;
    AmdGpu::Liverpool::PsInput addr_flags;
    u32 num_inputs;
    std::array<PsInput, 32> inputs;
    std::array<PsColorBuffer, MaxColorBuffers> color_buffers;
    bool dual_source_blending;

    bool operator==(const FragmentRuntimeInfo& other) const noexcept {
        return std::ranges::equal(color_buffers, other.color_buffers) &&
               en_flags.raw == other.en_flags.raw && addr_flags.raw == other.addr_flags.raw &&
               num_inputs == other.num_inputs &&
               dual_source_blending == other.dual_source_blending &&
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

    void Initialize(Stage stage_) {
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

template <>
struct fmt::formatter<Shader::Stage> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(const Shader::Stage stage, format_context& ctx) const {
        constexpr static std::array names = {"fs", "vs", "gs", "es", "hs", "ls", "cs"};
        return fmt::format_to(ctx.out(), "{}", names[static_cast<size_t>(stage)]);
    }
};
