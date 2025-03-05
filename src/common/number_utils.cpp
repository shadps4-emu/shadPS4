// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <half.hpp>

#include "common/number_utils.h"
#include "video_core/amdgpu/pixel_format.h"
#include "video_core/amdgpu/types.h"

#define UF11_EXPONENT_SHIFT 6
#define UF10_EXPONENT_SHIFT 5

#define RGB9E5_MANTISSA_BITS 9
#define RGB9E5_EXP_BIAS 1

#define F32_INFINITY 0x7f800000

namespace NumberUtils {

float Uf11ToF32(u16 val) {
    union {
        float f;
        u32 ui;
    } f32;

    int exponent = (val & 0x07c0) >> UF11_EXPONENT_SHIFT;
    int mantissa = (val & 0x003f);

    f32.f = 0.0;

    if (exponent == 0) {
        if (mantissa != 0) {
            const float scale = 1.0 / (1 << 20);
            f32.f = scale * mantissa;
        }
    } else if (exponent == 31) {
        f32.ui = F32_INFINITY | mantissa;
    } else {
        float scale, decimal;
        exponent -= 15;
        if (exponent < 0) {
            scale = 1.0f / (1 << -exponent);
        } else {
            scale = (float)(1 << exponent);
        }
        decimal = 1.0f + (float)mantissa / 64;
        f32.f = scale * decimal;
    }

    return f32.f;
}

float Uf10ToF32(u16 val) {
    union {
        float f;
        u32 ui;
    } f32;

    int exponent = (val & 0x03e0) >> UF10_EXPONENT_SHIFT;
    int mantissa = (val & 0x001f);

    f32.f = 0.0;

    if (exponent == 0) {
        if (mantissa != 0) {
            const float scale = 1.0 / (1 << 19);
            f32.f = scale * mantissa;
        }
    } else if (exponent == 31) {
        f32.ui = F32_INFINITY | mantissa;
    } else {
        float scale, decimal;
        exponent -= 15;
        if (exponent < 0) {
            scale = 1.0f / (1 << -exponent);
        } else {
            scale = (float)(1 << exponent);
        }
        decimal = 1.0f + (float)mantissa / 32;
        f32.f = scale * decimal;
    }

    return f32.f;
}

float Uf16ToF32(u16 val) {
    return half_float::half_cast<float>(reinterpret_cast<half_float::half&>(val));
}

float U2ToUnorm(u8 val) {
    static constexpr auto c = 1.0f / 3.0f;
    return float(val * c);
}

float S2ToSnorm(s8 val) {
    static constexpr auto c = 1.0f / 1.0f;
    return float(val * c);
}

float U4ToUnorm(u8 val) {
    static constexpr auto c = 1.0f / 15.0f;
    return float(val * c);
}

float S4ToSnorm(s8 val) {
    static constexpr auto c = 1.0f / 7.0f;
    return float(val * c);
}

float U5ToUnorm(u8 val) {
    static constexpr auto c = 1.0f / 31.0f;
    return float(val * c);
}

float S5ToSnorm(s8 val) {
    static constexpr auto c = 1.0f / 15.0f;
    return float(val * c);
}

float U6ToUnorm(u8 val) {
    static constexpr auto c = 1.0f / 63.0f;
    return float(val * c);
}

float S6ToSnorm(s8 val) {
    static constexpr auto c = 1.0f / 31.0f;
    return float(val * c);
}

float U8ToUnorm(u8 val) {
    static constexpr auto c = 1.0f / 255.0f;
    return float(val * c);
}

float S8ToSnorm(s8 val) {
    static constexpr auto c = 1.0f / 127.0f;
    return float(val * c);
}

float U10ToUnorm(u16 val) {
    static constexpr auto c = 1.0f / 1023.0f;
    return float(val * c);
}

float S10ToSnorm(s16 val) {
    static constexpr auto c = 1.0f / 511.0f;
    return float(val * c);
}

float U16ToUnorm(u16 val) {
    static constexpr auto c = 1.0f / 65535.0f;
    return float(val * c);
}

float S16ToSnorm(s16 val) {
    static constexpr auto c = 1.0f / 32767.0f;
    return float(val * c);
}

} // namespace NumberUtils
