// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "common/assert.h"
#include "shader_recompiler/profile.h"
#include "shader_recompiler/runtime_info.h"

namespace Shader {

struct FragmentBarycentricMapping {
    std::array<u32, 3> even{0, 1, 2};
    std::array<u32, 3> odd{0, 1, 2};
    bool uses_primitive_parity{};

    [[nodiscard]] u32 Vertex(u32 relative_index, bool odd_primitive = false) const {
        ASSERT_MSG(relative_index < 3, "Invalid interpolation vertex {}", relative_index);
        return (odd_primitive ? odd : even)[relative_index];
    }
};

inline FragmentBarycentricMapping GetFragmentBarycentricMapping(const RuntimeInfo& runtime_info,
                                                                const Profile& profile) {
    FragmentBarycentricMapping mapping{};
    if (profile.supports_amd_shader_explicit_vertex_parameter) {
        return mapping;
    }
    ASSERT(profile.supports_fragment_shader_barycentric);

    if (!runtime_info.fs_info.provoking_vtx_last) {
        return mapping;
    }

    const auto primitive = runtime_info.fs_info.primitive_type;
    const std::array<u32, 3> last_triangle_basis{2, 0, 1};
    const std::array<u32, 3> last_line_basis{1, 0, 2};
    switch (primitive) {
    case AmdGpu::PrimitiveType::LineList:
    case AmdGpu::PrimitiveType::LineStrip:
    case AmdGpu::PrimitiveType::AdjLineList:
    case AmdGpu::PrimitiveType::AdjLineStrip:
        mapping.even = last_line_basis;
        mapping.odd = mapping.even;
        return mapping;
    case AmdGpu::PrimitiveType::TriangleList:
    case AmdGpu::PrimitiveType::AdjTriangleList:
        mapping.even = last_triangle_basis;
        mapping.odd = mapping.even;
        return mapping;
    case AmdGpu::PrimitiveType::TriangleFan:
        mapping.even =
            profile.supports_provoking_vertex ? last_triangle_basis : std::array<u32, 3>{1, 2, 0};
        mapping.odd = mapping.even;
        return mapping;
    case AmdGpu::PrimitiveType::TriangleStrip:
        mapping.even = last_triangle_basis;
        mapping.odd = mapping.even;
        if (!profile.supports_provoking_vertex ||
            profile.tri_strip_vertex_order_independent_of_provoking_vertex) {
            mapping.odd = {1, 2, 0};
            mapping.uses_primitive_parity = true;
        }
        return mapping;
    case AmdGpu::PrimitiveType::AdjTriangleStrip:
        mapping.even = last_triangle_basis;
        mapping.odd = mapping.even;
        if (!profile.supports_provoking_vertex) {
            mapping.odd = {1, 2, 0};
            mapping.uses_primitive_parity = true;
        }
        return mapping;
    default:
        return mapping;
    }
}

} // namespace Shader
