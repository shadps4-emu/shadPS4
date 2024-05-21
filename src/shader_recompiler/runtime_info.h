// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <boost/container/small_vector.hpp>
#include "shader_recompiler/ir/type.h"

namespace Shader {

enum class AttributeType : u8 {
    Float,
    SignedInt,
    UnsignedInt,
    SignedScaled,
    UnsignedScaled,
    Disabled,
};

enum class InputTopology {
    Points,
    Lines,
    LinesAdjacency,
    Triangles,
    TrianglesAdjacency,
};

enum class CompareFunction {
    Never,
    Less,
    Equal,
    LessThanEqual,
    Greater,
    NotEqual,
    GreaterThanEqual,
    Always,
};

enum class Stage : u32 {
    Vertex,
    TessellationControl,
    TessellationEval,
    Geometry,
    Fragment,
    Compute,
};
constexpr u32 MaxStageTypes = 6;

[[nodiscard]] constexpr Stage StageFromIndex(size_t index) noexcept {
    return static_cast<Stage>(static_cast<size_t>(Stage::Vertex) + index);
}

enum class TextureType : u32 {
    Color1D,
    ColorArray1D,
    Color2D,
    ColorArray2D,
    Color3D,
    ColorCube,
    Buffer,
};
constexpr u32 NUM_TEXTURE_TYPES = 7;

enum class Interpolation {
    Smooth,
    Flat,
    NoPerspective,
};

struct ConstantBufferDescriptor {
    u32 index;
    u32 count;

    auto operator<=>(const ConstantBufferDescriptor&) const = default;
};

struct TextureDescriptor {
    TextureType type;
    bool is_eud;
    bool is_depth;
    bool is_multisample;
    bool is_storage;
    u32 count;
    u32 eud_offset_dwords;
    u32 ud_index_dwords;

    auto operator<=>(const TextureDescriptor&) const = default;
};
using TextureDescriptors = boost::container::small_vector<TextureDescriptor, 12>;

struct Info {
    bool uses_workgroup_id{};
    bool uses_local_invocation_id{};
    bool uses_invocation_id{};
    bool uses_invocation_info{};
    bool uses_sample_id{};

    std::array<Interpolation, 32> interpolation{};
    // VaryingState loads;
    // VaryingState stores;
    // VaryingState passthrough;

    std::array<bool, 8> stores_frag_color{};
    bool stores_sample_mask{};
    bool stores_frag_depth{};

    bool uses_fp16{};
    bool uses_fp64{};
    bool uses_fp16_denorms_flush{};
    bool uses_fp16_denorms_preserve{};
    bool uses_fp32_denorms_flush{};
    bool uses_fp32_denorms_preserve{};
    bool uses_int8{};
    bool uses_int16{};
    bool uses_int64{};
    bool uses_image_1d{};
    bool uses_sampled_1d{};
    bool uses_subgroup_vote{};
    bool uses_subgroup_mask{};
    bool uses_derivatives{};

    IR::Type used_constant_buffer_types{};
    IR::Type used_storage_buffer_types{};
    IR::Type used_indirect_cbuf_types{};

    // std::array<u32, MAX_CBUFS> constant_buffer_used_sizes{};
    u32 used_clip_distances{};

    // boost::container::static_vector<ConstantBufferDescriptor, MAX_CBUFS>
    //     constant_buffer_descriptors;
    // boost::container::static_vector<StorageBufferDescriptor, MAX_SSBOS>
    // storage_buffers_descriptors; TextureBufferDescriptors texture_buffer_descriptors;
    // ImageBufferDescriptors image_buffer_descriptors;
    // TextureDescriptors texture_descriptors;
    // ImageDescriptors image_descriptors;
};

} // namespace Shader
