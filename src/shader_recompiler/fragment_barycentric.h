// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "common/assert.h"
#include "shader_recompiler/profile.h"
#include "shader_recompiler/runtime_info.h"

namespace Shader {

struct FragmentBarycentricMapping {
    std::array<u32, 2> even{1, 2};
    std::array<u32, 2> odd{1, 2};
    bool uses_primitive_parity{};

    [[nodiscard]] u32 Component(u32 guest_component, bool odd_primitive = false) const {
        ASSERT_MSG(guest_component < 2, "Invalid guest barycentric component {}", guest_component);
        return (odd_primitive ? odd : even)[guest_component];
    }
};

constexpr std::array<u32, 2> BarycentricsForProvokingVertex(u32 provoking_vertex) {
    // This is the inverse of RADV's lower_triangle():
    //   v0 -> (1-I-J, I, J), v1 -> (I, J, 1-I-J), v2 -> (J, 1-I-J, I).
    constexpr std::array mappings = {
        std::array<u32, 2>{1, 2},
        std::array<u32, 2>{0, 1},
        std::array<u32, 2>{2, 0},
    };
    ASSERT_MSG(provoking_vertex < mappings.size(), "Invalid provoking vertex {}", provoking_vertex);
    return mappings[provoking_vertex];
}

inline FragmentBarycentricMapping GetFragmentBarycentricMapping(const RuntimeInfo& runtime_info,
                                                                const Profile& profile) {
    FragmentBarycentricMapping mapping{};
    const auto& fs_info = runtime_info.fs_info;
    if (!fs_info.provoking_vtx_last) {
        return mapping;
    }

    const auto set_provoking_vertices = [&](u32 even, u32 odd) {
        mapping.even = BarycentricsForProvokingVertex(even);
        mapping.odd = BarycentricsForProvokingVertex(odd);
        mapping.uses_primitive_parity = even != odd;
    };

    switch (fs_info.primitive_type) {
    case AmdGpu::PrimitiveType::TriangleList:
    case AmdGpu::PrimitiveType::AdjTriangleList:
        set_provoking_vertices(2, 2);
        break;
    case AmdGpu::PrimitiveType::TriangleFan:
        // Without VK_EXT_provoking_vertex the host remains in first-vertex mode. In the
        // first-vertex fan ordering, the guest's last vertex occupies KHR component 1.
        set_provoking_vertices(profile.supports_provoking_vertex ? 2 : 1,
                               profile.supports_provoking_vertex ? 2 : 1);
        break;
    case AmdGpu::PrimitiveType::TriangleStrip:
        // With independent strip ordering, an odd primitive's last vertex is KHR component 1.
        // Implementations without last-provoking support also retain the first-vertex ordering.
        set_provoking_vertices(
            2, profile.supports_provoking_vertex &&
                       !profile.tri_strip_vertex_order_independent_of_provoking_vertex
                   ? 2
                   : 1);
        break;
    case AmdGpu::PrimitiveType::AdjTriangleStrip:
        // Vulkan defines a last-provoking ordering for odd adjacency strips independently of
        // triStripVertexOrderIndependentOfProvokingVertex.
        set_provoking_vertices(2, profile.supports_provoking_vertex ? 2 : 1);
        break;
    default:
        // Point coordinates are (1,0,0). Line lowering preserves only I+J, so there is no
        // general inverse; retain the existing canonical (component 1, component 2) choice.
        break;
    }
    return mapping;
}

} // namespace Shader
