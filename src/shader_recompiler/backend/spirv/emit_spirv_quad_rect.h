// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include "common/types.h"

namespace Shader {
struct FragmentRuntimeInfo;
}

namespace Shader::Backend::SPIRV {

enum class AuxShaderType : u32 {
    RectListTCS,
    QuadListTCS,
    PassthroughTES,
};

[[nodiscard]] constexpr u32 AuxTessAttributeLocation(u32 param_index,
                                                     bool clip_distance_emulation) noexcept {
    return param_index + (clip_distance_emulation ? 1u : 0u);
}

[[nodiscard]] std::vector<u32> EmitAuxilaryTessShader(AuxShaderType type,
                                                      const FragmentRuntimeInfo& fs_info,
                                                      u64 previous_stage_output_mask = ~0ull);

} // namespace Shader::Backend::SPIRV
