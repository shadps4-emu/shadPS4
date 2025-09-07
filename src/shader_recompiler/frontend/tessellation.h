// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Shader {

struct TessellationDataConstantBuffer {
    u32 ls_stride;
    u32 hs_cp_stride;      // HullStateConstants::m_cpStride != 0 ? HullStateConstants::m_cpStride :
                           // ls_stride
    u32 num_patches;       // num patches submitted in threadgroup
    u32 hs_output_base;    // HullStateConstants::m_numInputCP::m_cpStride != 0 ?
                           // HullStateConstants::m_numInputCP * ls_stride * num_patches : 0
                           // basically 0 when passthrough
    u32 patch_const_size;  // 16 * num_patch_attrs
    u32 patch_const_base;  // hs_output_base + patch_output_size
    u32 patch_output_size; // output_cp_stride * num_output_cp_per_patch
    f32 off_chip_tessellation_factor_threshold;
    u32 first_edge_tess_factor_index;
};

// Assign names to dword fields of TessellationDataConstantBuffer
enum class TessConstantAttribute : u32 {
    LsStride,
    HsCpStride,
    HsNumPatch,
    HsOutputBase,
    PatchConstSize,
    PatchConstBase,
    PatchOutputSize,
    OffChipTessellationFactorThreshold,
    FirstEdgeTessFactorIndex,
};

} // namespace Shader