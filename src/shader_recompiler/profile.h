// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Shader {

struct Profile {
    u32 supported_spirv{0x00010000};
    u32 subgroup_size{};
    bool unified_descriptor_binding{};
    bool support_descriptor_aliasing{};
    bool support_int8{};
    bool support_int16{};
    bool support_int64{};
    bool support_vertex_instance_id{};
    bool support_float_controls{};
    bool support_separate_denorm_behavior{};
    bool support_separate_rounding_mode{};
    bool support_fp32_denorm_preserve{};
    bool support_fp32_denorm_flush{};
    bool support_explicit_workgroup_layout{};
    bool has_broken_spirv_clamp{};
    bool lower_left_origin_mode{};
    u64 min_ssbo_alignment{};
};

} // namespace Shader
