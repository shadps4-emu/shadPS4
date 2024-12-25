// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/ir/ir_emitter.h"
#include "video_core/amdgpu/resource.h"

namespace Shader::IR {

/// Applies a component swizzle to a vec4.
inline Value ApplySwizzle(IREmitter& ir, const Value& vector, const AmdGpu::CompMapping& swizzle) {
    // Constants are indexed as 0 and 1, and components are 4-7. Thus we can apply a swizzle
    // using two vectors and a shuffle, using one vector of constants and one of the components.
    const auto zero = ir.Imm32(0.f);
    const auto one = ir.Imm32(1.f);
    const auto constants_vec = ir.CompositeConstruct(zero, one, zero, zero);
    const auto swizzled =
        ir.CompositeShuffle(constants_vec, vector, size_t(swizzle.r), size_t(swizzle.g),
                            size_t(swizzle.b), size_t(swizzle.a));
    return swizzled;
}

} // namespace Shader::IR
