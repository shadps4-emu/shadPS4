// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Shader {

struct TessellationDataConstantBuffer {
    u32 m_lsStride;
    u32 m_hsCpStride;      // HullStateConstants::m_cpStride != 0 ? HullStateConstants::m_cpStride :
                           // ls_stride
    u32 m_hsNumPatch;      // num patches submitted in threadgroup
    u32 m_hsOutputBase;    // HullStateConstants::m_numInputCP::m_cpStride != 0 ?
                           // HullStateConstants::m_numInputCP * ls_stride * num_patches : 0
    u32 m_patchConstSize;  // 16 * num_patch_attrs
    u32 m_patchConstBase;  // hs_output_base + patch_output_size
    u32 m_patchOutputSize; // output_cp_stride * num_output_cp
    f32 m_offChipTessellationFactorThreshold;
    u32 m_firstEdgeTessFactorIndex;
};

} // namespace Shader