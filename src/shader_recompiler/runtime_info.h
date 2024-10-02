// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <boost/container/static_vector.hpp>

#include "common/assert.h"
#include "common/types.h"

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
constexpr u32 MaxStageTypes = 6;

[[nodiscard]] constexpr Stage StageFromIndex(size_t index) noexcept {
    return static_cast<Stage>(index);
}

enum class MrtSwizzle : u8 {
    Identity = 0,
    Alt = 1,
    Reverse = 2,
    ReverseAlt = 3,
};
static constexpr u32 MaxColorBuffers = 8;

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
    boost::container::static_vector<VsOutputMap, 3> outputs;
    bool emulate_depth_negative_one_to_one{};

    bool operator==(const VertexRuntimeInfo& other) const noexcept {
        return emulate_depth_negative_one_to_one == other.emulate_depth_negative_one_to_one;
    }
};

struct FragmentRuntimeInfo {
    struct PsInput {
        u8 param_index;
        bool is_default;
        bool is_flat;
        u8 default_value;

        auto operator<=>(const PsInput&) const noexcept = default;
    };
    boost::container::static_vector<PsInput, 32> inputs;
    struct PsColorBuffer {
        AmdGpu::NumberFormat num_format;
        MrtSwizzle mrt_swizzle;

        auto operator<=>(const PsColorBuffer&) const noexcept = default;
    };
    std::array<PsColorBuffer, MaxColorBuffers> color_buffers;

    bool operator==(const FragmentRuntimeInfo& other) const noexcept {
        return std::ranges::equal(color_buffers, other.color_buffers) &&
               std::ranges::equal(inputs, other.inputs);
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
    VertexRuntimeInfo vs_info;
    FragmentRuntimeInfo fs_info;
    ComputeRuntimeInfo cs_info;

    RuntimeInfo(Stage stage_) : stage{stage_} {}

    bool operator==(const RuntimeInfo& other) const noexcept {
        switch (stage) {
        case Stage::Fragment:
            return fs_info == other.fs_info;
        case Stage::Vertex:
            return vs_info == other.vs_info;
        case Stage::Compute:
            return cs_info == other.cs_info;
        default:
            return true;
        }
    }
};

} // namespace Shader
