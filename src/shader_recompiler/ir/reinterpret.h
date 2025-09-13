// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/ir/ir_emitter.h"
#include "video_core/amdgpu/pixel_format.h"

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

/// Converts gamma corrected value to linear space
inline F32 ApplyGammaToLinear(IREmitter& ir, F32& c) {
    const F32 a =
        ir.FPPow(ir.FPMul(ir.FPAdd(c, ir.Imm32(0.055f)), ir.Imm32(1.0f / 1.055f)), ir.Imm32(2.4f));
    const F32 b = ir.FPMul(c, ir.Imm32(1.0f / 12.92f));
    return IR::F32{ir.Select(ir.FPGreaterThan(c, ir.Imm32(0.04045f)), a, b)};
}

inline Value ApplyForceDegamma(IREmitter& ir, const Value& value,
                               const AmdGpu::CompMapping& mapping) {
    auto x = F32{ir.CompositeExtract(value, 0)};
    auto y = F32{ir.CompositeExtract(value, 1)};
    auto z = F32{ir.CompositeExtract(value, 2)};
    auto w = F32{ir.CompositeExtract(value, 3)};
    // Gamma correction is only applied to RGB components
    if (AmdGpu::IsRgb(mapping.r)) {
        x = ApplyGammaToLinear(ir, x);
    }
    if (AmdGpu::IsRgb(mapping.g)) {
        y = ApplyGammaToLinear(ir, y);
    }
    if (AmdGpu::IsRgb(mapping.b)) {
        z = ApplyGammaToLinear(ir, z);
    }
    if (AmdGpu::IsRgb(mapping.a)) {
        w = ApplyGammaToLinear(ir, w);
    }
    return ir.CompositeConstruct(x, y, z, w);
}

/// Applies a number conversion in the read direction.
inline F32 ApplyReadNumberConversion(IREmitter& ir, const F32& value,
                                     const AmdGpu::NumberConversion& conversion) {
    switch (conversion) {
    case AmdGpu::NumberConversion::None:
        return value;
    case AmdGpu::NumberConversion::UintToUscaled:
        return ir.ConvertUToF(32, 32, ir.BitCast<U32>(value));
    case AmdGpu::NumberConversion::SintToSscaled:
        return ir.ConvertSToF(32, 32, ir.BitCast<U32>(value));
    case AmdGpu::NumberConversion::UnormToUbnorm:
        // Convert 0...1 to -1...1
        return ir.FPSub(ir.FPMul(value, ir.Imm32(2.f)), ir.Imm32(1.f));
    case AmdGpu::NumberConversion::Sint8ToSnormNz: {
        const IR::U32 additon = ir.IAdd(ir.IMul(ir.BitCast<U32>(value), ir.Imm32(2)), ir.Imm32(1));
        const IR::F32 left = ir.ConvertSToF(32, 32, additon);
        const IR::F32 max = ir.Imm32(float(std::numeric_limits<u8>::max()));
        return ir.FPDiv(left, max);
    }
    case AmdGpu::NumberConversion::Sint16ToSnormNz: {
        const IR::U32 additon = ir.IAdd(ir.IMul(ir.BitCast<U32>(value), ir.Imm32(2)), ir.Imm32(1));
        const IR::F32 left = ir.ConvertSToF(32, 32, additon);
        const IR::F32 max = ir.Imm32(float(std::numeric_limits<u16>::max()));
        return ir.FPDiv(left, max);
    }
    case AmdGpu::NumberConversion::Uint32ToUnorm: {
        const auto float_val = ir.ConvertUToF(32, 32, ir.BitCast<U32>(value));
        return ir.FPDiv(float_val, ir.Imm32(static_cast<float>(std::numeric_limits<u32>::max())));
    }
    default:
        UNREACHABLE();
    }
}

inline Value ApplyReadNumberConversionVec4(IREmitter& ir, const Value& value,
                                           const AmdGpu::NumberConversion& conversion) {
    if (conversion == AmdGpu::NumberConversion::None) {
        return value;
    }
    const auto x = ApplyReadNumberConversion(ir, F32{ir.CompositeExtract(value, 0)}, conversion);
    const auto y = ApplyReadNumberConversion(ir, F32{ir.CompositeExtract(value, 1)}, conversion);
    const auto z = ApplyReadNumberConversion(ir, F32{ir.CompositeExtract(value, 2)}, conversion);
    const auto w = ApplyReadNumberConversion(ir, F32{ir.CompositeExtract(value, 3)}, conversion);
    return ir.CompositeConstruct(x, y, z, w);
}

/// Applies a number conversion in the write direction.
inline F32 ApplyWriteNumberConversion(IREmitter& ir, const F32& value,
                                      const AmdGpu::NumberConversion& conversion) {
    switch (conversion) {
    case AmdGpu::NumberConversion::None:
        return value;
    case AmdGpu::NumberConversion::UintToUscaled:
        // Need to return float type to maintain IR semantics.
        return ir.BitCast<F32>(U32{ir.ConvertFToU(32, value)});
    case AmdGpu::NumberConversion::SintToSscaled:
        // Need to return float type to maintain IR semantics.
        return ir.BitCast<F32>(U32{ir.ConvertFToS(32, value)});
    case AmdGpu::NumberConversion::UnormToUbnorm:
        // Convert -1...1 to 0...1
        return ir.FPDiv(ir.FPAdd(value, ir.Imm32(1.f)), ir.Imm32(2.f));
    case AmdGpu::NumberConversion::Sint8ToSnormNz: {
        const IR::F32 max = ir.Imm32(float(std::numeric_limits<u8>::max()));
        const IR::F32 mul = ir.FPMul(ir.FPClamp(value, ir.Imm32(-1.f), ir.Imm32(1.f)), max);
        const IR::F32 left = ir.FPSub(mul, ir.Imm32(1.f));
        const IR::U32 raw = ir.ConvertFToS(32, ir.FPDiv(left, ir.Imm32(2.f)));
        return ir.BitCast<F32>(raw);
    }
    case AmdGpu::NumberConversion::Sint16ToSnormNz: {
        const IR::F32 max = ir.Imm32(float(std::numeric_limits<u16>::max()));
        const IR::F32 mul = ir.FPMul(ir.FPClamp(value, ir.Imm32(-1.f), ir.Imm32(1.f)), max);
        const IR::F32 left = ir.FPSub(mul, ir.Imm32(1.f));
        const IR::U32 raw = ir.ConvertFToS(32, ir.FPDiv(left, ir.Imm32(2.f)));
        return ir.BitCast<F32>(raw);
    }
    case AmdGpu::NumberConversion::Uint32ToUnorm: {
        const auto clamped = ir.FPClamp(value, ir.Imm32(0.f), ir.Imm32(1.f));
        const auto unnormalized =
            ir.FPMul(clamped, ir.Imm32(static_cast<float>(std::numeric_limits<u32>::max())));
        return ir.BitCast<F32>(U32{ir.ConvertFToU(32, unnormalized)});
    }
    default:
        UNREACHABLE();
    }
}

inline Value ApplyWriteNumberConversionVec4(IREmitter& ir, const Value& value,
                                            const AmdGpu::NumberConversion& conversion) {
    if (conversion == AmdGpu::NumberConversion::None) {
        return value;
    }
    const auto x = ApplyWriteNumberConversion(ir, F32{ir.CompositeExtract(value, 0)}, conversion);
    const auto y = ApplyWriteNumberConversion(ir, F32{ir.CompositeExtract(value, 1)}, conversion);
    const auto z = ApplyWriteNumberConversion(ir, F32{ir.CompositeExtract(value, 2)}, conversion);
    const auto w = ApplyWriteNumberConversion(ir, F32{ir.CompositeExtract(value, 3)}, conversion);
    return ir.CompositeConstruct(x, y, z, w);
}

} // namespace Shader::IR
