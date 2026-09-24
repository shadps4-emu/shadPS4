// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <span>
#include "common/types.h"
#include "shader_recompiler/frontend/tessellation.h"
#include "video_core/amdgpu/pixel_format.h"
#include "video_core/amdgpu/regs_shader.h"
#include "video_core/amdgpu/regs_vertex.h"

namespace Shader {

enum class HwStage : u32 {
    Fragment,
    Vertex,
    Geometry,
    Export,
    Hull,
    Local,
    Compute,
};

// Vertex intentionally comes after TCS/TES due to order of compilation
enum class SwStage : u32 {
    Fragment,
    TessellationControl,
    TessellationEval,
    Vertex,
    Geometry,
    Compute,
    NumLogicalStages
};

constexpr u32 MaxStageTypes = static_cast<u32>(SwStage::NumLogicalStages);
constexpr auto MaxEmulatedClipDistances = 4u;

constexpr HwStage StageFromIndex(size_t index) noexcept {
    return static_cast<HwStage>(index);
}

enum class Output : u8 {
    None,
    PointSize,
    EdgeFlag,
    KillFlag,
    GsCutFlag,
    RenderTargetIndex,
    ViewportIndex,
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

struct SwVertexRuntimeInfo {
    u32 step_rate_0;
    u32 step_rate_1;
    u16 vertex_sgpr_offset{};
    u16 instance_sgpr_offset{};
    bool tess_emulated_primitive{};

    bool operator==(const SwVertexRuntimeInfo& other) const noexcept = default;
};

struct HwLocalRuntimeInfo {
    u32 ls_stride;

    bool operator==(const HwLocalRuntimeInfo& other) const noexcept = default;
};

struct HwExportRuntimeInfo {
    u32 vertex_data_size;

    bool operator==(const HwExportRuntimeInfo& other) const noexcept = default;
};

struct HwVertexRuntimeInfo {
    u32 num_outputs;
    std::array<OutputMap, 3> outputs;
    bool emulate_depth_negative_one_to_one{};
    bool clip_disable{};
    u32 user_clip_plane_mask{};

    bool operator==(const HwVertexRuntimeInfo& other) const noexcept = default;
};

struct SwTessControlRuntimeInfo {
    u32 hs_output_cp_stride;
    u32 num_input_control_points;
    u32 num_threads;
    AmdGpu::TessellationType tess_type;
    bool offchip_lds_enable;
    u32 ls_stride;
    u32 hs_output_base;

    bool operator==(const SwTessControlRuntimeInfo&) const = default;

    // It might be possible for a non-passthrough TCS to have these conditions, in some dumb
    // situation. In that case, it should be fine to assume passthrough and declare some extra
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
};

struct SwTessEvalRuntimeInfo {
    u32 hs_output_cp_stride;
    AmdGpu::TessellationType tess_type;
    AmdGpu::TessellationTopology tess_topology;
    AmdGpu::TessellationPartitioning tess_partitioning;

    bool operator==(const SwTessEvalRuntimeInfo&) const noexcept = default;
};

static constexpr auto GsMaxOutputStreams = 4u;
using GsOutputPrimTypes = std::array<AmdGpu::GsOutputPrimitiveType, GsMaxOutputStreams>;
struct HwGeometryRuntimeInfo {
    u32 num_outputs;
    std::array<OutputMap, 3> outputs;
    u32 num_invocations{};
    u32 output_vertices{};
    u32 in_vertex_data_size{};
    u32 out_vertex_data_size{};
    AmdGpu::PrimitiveType in_primitive;
    GsOutputPrimTypes out_primitive;
    AmdGpu::GsScenario mode;
    std::span<const u32> vs_copy;
    u64 vs_copy_hash;

    bool operator==(const HwGeometryRuntimeInfo& other) const {
        return num_outputs == other.num_outputs && outputs == other.outputs && num_invocations &&
               other.num_invocations && output_vertices == other.output_vertices &&
               in_primitive == other.in_primitive &&
               std::ranges::equal(out_primitive, other.out_primitive) &&
               vs_copy_hash == other.vs_copy_hash;
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
    AmdGpu::ShaderExportFormat export_format : 4;
    // GCN applies blend factors to min/max ops while Vulkan ignores them. For the self-scaled
    // pattern min/max(src*src, dst*dst) the shader squares its color output instead, keeping
    // the attachment in the squared domain end to end.
    u32 blend_self_scale : 1;
    AmdGpu::CompMapping swizzle;

    bool operator==(const PsColorBuffer& other) const = default;
};

struct HwFragmentRuntimeInfo {
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
    AmdGpu::PsInput en_flags;
    AmdGpu::PsInput addr_flags;
    u32 num_inputs;
    std::array<PsInput, 32> inputs;
    std::array<PsColorBuffer, MaxColorBuffers> color_buffers;
    AmdGpu::ShaderExportFormat z_export_format;
    u8 num_samples{1};
    u8 mrtz_mask{};
    bool front_face_all_bits{false};
    bool dual_source_blending{false};
    bool clip_distance_emulation{false};

    bool operator==(const HwFragmentRuntimeInfo& other) const noexcept {
        return std::ranges::equal(color_buffers, other.color_buffers) &&
               en_flags == other.en_flags && addr_flags == other.addr_flags &&
               num_inputs == other.num_inputs && z_export_format == other.z_export_format &&
               num_samples == other.num_samples && mrtz_mask == other.mrtz_mask &&
               front_face_all_bits == other.front_face_all_bits &&
               dual_source_blending == other.dual_source_blending &&
               clip_distance_emulation == other.clip_distance_emulation &&
               std::ranges::equal(inputs.begin(), inputs.begin() + num_inputs, other.inputs.begin(),
                                  other.inputs.begin() + num_inputs);
    }
};

struct HwComputeRuntimeInfo {
    u32 shared_memory_size;
    std::array<u32, 3> workgroup_size;
    std::array<bool, 3> tgid_enable;

    bool operator==(const HwComputeRuntimeInfo& other) const noexcept {
        return workgroup_size == other.workgroup_size && tgid_enable == other.tgid_enable;
    }
};

/**
 * Stores information relevant to shader compilation sourced from liverpool registers.
 * It may potentially differ with the same shader module so must be checked.
 * It's also possible to store any other custom information that needs to be part of shader key.
 */
struct RuntimeInfo {
    SwStage sw_stage;
    HwStage hw_stage;
    struct Properties {
        AmdGpu::FpDenormMode fp_denorm_mode32;
        AmdGpu::FpDenormMode fp_denorm_mode16_64;
        AmdGpu::FpRoundMode fp_round_mode32;
        AmdGpu::FpRoundMode fp_round_mode16_64;
        u32 num_user_data;
        u32 num_input_vgprs;
        u32 num_allocated_vgprs;
        bool operator==(const Properties&) const noexcept = default;
    } props;
    union SwInfo {
        SwVertexRuntimeInfo vs;
        SwTessControlRuntimeInfo tcs;
        SwTessEvalRuntimeInfo tes;
    } sw;
    union HwInfo {
        HwLocalRuntimeInfo ls;
        HwExportRuntimeInfo es;
        HwGeometryRuntimeInfo gs;
        HwVertexRuntimeInfo vs;
        HwFragmentRuntimeInfo fs;
        HwComputeRuntimeInfo cs;
    } hw;

    void Initialize(HwStage stage, SwStage l_stage) {
        memset(this, 0, sizeof(*this));
        this->hw_stage = stage;
        this->sw_stage = l_stage;
        if (stage == HwStage::Fragment) {
            hw.fs.num_samples = 1;
        }
    }

    bool operator==(const RuntimeInfo& other) const noexcept {
        return hw_stage == other.hw_stage && sw_stage == other.sw_stage && props == other.props &&
               HasSameSwInfo(other.sw) && HasSameHwInfo(other.hw);
    }

    bool HasSameSwInfo(const SwInfo& other) const noexcept {
        switch (sw_stage) {
        case SwStage::Vertex:
            return sw.vs == other.vs;
        case SwStage::TessellationControl:
            return sw.tcs == other.tcs;
        case SwStage::TessellationEval:
            return sw.tes == other.tes;
        default:
            return true;
        }
    }

    bool HasSameHwInfo(const HwInfo& other) const noexcept {
        switch (hw_stage) {
        case HwStage::Local:
            return hw.ls == other.ls;
        case HwStage::Export:
            return hw.es == other.es;
        case HwStage::Geometry:
            return hw.gs == other.gs;
        case HwStage::Vertex:
            return hw.vs == other.vs;
        case HwStage::Fragment:
            return hw.fs == other.fs;
        case HwStage::Compute:
            return hw.cs == other.cs;
        default:
            return true;
        }
    }

    void InitFromTessConstants(Shader::TessellationDataConstantBuffer& tess_constants) {
        if (sw_stage == SwStage::TessellationControl) {
            sw.tcs.hs_output_cp_stride = tess_constants.hs_cp_stride;
            sw.tcs.ls_stride = tess_constants.ls_stride;
            sw.tcs.hs_output_base = tess_constants.hs_output_base;
        } else if (sw_stage == SwStage::TessellationEval) {
            sw.tes.hs_output_cp_stride = tess_constants.hs_cp_stride;
        }
    }
};

} // namespace Shader

template <>
struct fmt::formatter<Shader::HwStage> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(const Shader::HwStage stage, format_context& ctx) const {
        constexpr static std::array names = {"fs", "vs", "gs", "es", "hs", "ls", "cs"};
        return fmt::format_to(ctx.out(), "{}", names[static_cast<size_t>(stage)]);
    }
};
