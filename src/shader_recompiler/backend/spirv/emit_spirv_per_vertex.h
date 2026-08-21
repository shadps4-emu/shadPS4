// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <vector>
#include "common/types.h"

namespace Shader {
struct Info;
struct Profile;
struct RuntimeInfo;
} // namespace Shader

namespace Shader::Backend::SPIRV {

// Output location layout shared by the auxiliary geometry shader (output side) and the
// fragment shader input declaration (input side). Locations are assigned by a single
// running counter rather than anchoring on param_index: per-vertex attributes have
// contiguous param_index (spacing 1), so anchoring each expanded attribute onto
// [param_index, param_index+2] would overlap.
struct PerVertexLocations {
    // Output location for each non-default input slot. For per-vertex attributes this is
    // the start of 3 consecutive locations; for others it is a single location.
    std::array<u32, 32> slot_loc{};
    // Location of the injected smooth barycentric-coordinate attribute (vec2, I/J).
    u32 bary_coord_loc = 0;
};

// Whether the per-vertex interpolation fallback is eligible for this fragment shader:
// direct VS->FS triangle pipeline, no AMD explicit vertex parameter nor KHR barycentric, and no
// unsupported barycentric variant (only persp_center is covered). Shared by the translation side
// (restoring per-vertex instead of degrading to flat) and the FS compile side (aux GS generation).
[[nodiscard]] bool CanPerVertexInterp(const Shader::Profile& profile,
                                      const Shader::RuntimeInfo& runtime_info);

// Whether the fragment shader requests a barycentric variant the fallback cannot reproduce
// (anything other than persp_center: sample/centroid/pull-model/linear). Shared by the translation
// side (boundary warning) and the FS compile side (CanPerVertexInterp gate).
[[nodiscard]] bool HasUnsupportedBarycentricVariant(const Shader::RuntimeInfo& runtime_info);

// Bitmask of the unsupported barycentric variants the shader requests, so the fallback warning can
// report which variant (rather than a single bool): bit0=persp_sample, bit1=persp_centroid,
// bit2=persp_pull_model, bit3=linear_sample, bit4=linear_center, bit5=linear_centroid.
[[nodiscard]] u32 UnsupportedBarycentricMask(const Shader::RuntimeInfo& runtime_info);

// Whether an auxiliary geometry shader must be generated for this fragment shader: the fallback
// is eligible and the shader either reads a per-vertex attribute or the persp-center barycentric.
[[nodiscard]] bool NeedsPerVertexAuxGS(const Shader::Profile& profile, const Shader::Info& info,
                                       const Shader::RuntimeInfo& runtime_info);

// Computes the output location layout for the fragment shader's non-default inputs.
// Per-vertex attributes occupy 3 consecutive locations only when the fallback applies
// (CanPerVertexInterp); otherwise they occupy 1, matching the AMD/barycentric single-location paths.
// The optional clip-distance emulation input occupies location 0 and is skipped by the counter.
[[nodiscard]] PerVertexLocations ComputePerVertexLocations(const Shader::Profile& profile,
                                                           const Shader::Info& info,
                                                           const Shader::RuntimeInfo& runtime_info);

// Emits an auxiliary geometry shader that expands per-vertex attributes into three flat
// attributes (one per vertex, P0/P10/P20) and injects a smooth barycentric-coordinate
// attribute assigned (0,0)/(1,0)/(0,1) across the triangle, for GPUs that
// lack both AMD explicit vertex parameter and KHR fragment shader barycentric.
[[nodiscard]] std::vector<u32> EmitPerVertexAuxGS(const Shader::Profile& profile,
                                                  const Shader::Info& info,
                                                  const Shader::RuntimeInfo& runtime_info);

} // namespace Shader::Backend::SPIRV
