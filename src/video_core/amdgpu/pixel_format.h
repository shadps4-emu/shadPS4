// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>
#include <fmt/format.h>
#include "common/assert.h"
#include "common/types.h"

namespace AmdGpu {

// Table 8.13 Data and Image Formats [Sea Islands Series Instruction Set Architecture]
enum class DataFormat : u32 {
    FormatInvalid = 0,
    Format8 = 1,
    Format16 = 2,
    Format8_8 = 3,
    Format32 = 4,
    Format16_16 = 5,
    Format10_11_11 = 6,
    Format11_11_10 = 7,
    Format10_10_10_2 = 8,
    Format2_10_10_10 = 9,
    Format8_8_8_8 = 10,
    Format32_32 = 11,
    Format16_16_16_16 = 12,
    Format32_32_32 = 13,
    Format32_32_32_32 = 14,
    Format5_6_5 = 16,
    Format1_5_5_5 = 17,
    Format5_5_5_1 = 18,
    Format4_4_4_4 = 19,
    Format8_24 = 20,
    Format24_8 = 21,
    FormatX24_8_32 = 22,
    FormatGB_GR = 32,
    FormatBG_RG = 33,
    Format5_9_9_9 = 34,
    FormatBc1 = 35,
    FormatBc2 = 36,
    FormatBc3 = 37,
    FormatBc4 = 38,
    FormatBc5 = 39,
    FormatBc6 = 40,
    FormatBc7 = 41,
    FormatFmask8_1 = 47,
    FormatFmask8_2 = 48,
    FormatFmask8_4 = 49,
    FormatFmask16_1 = 50,
    FormatFmask16_2 = 51,
    FormatFmask32_2 = 52,
    FormatFmask32_4 = 53,
    FormatFmask32_8 = 54,
    FormatFmask64_4 = 55,
    FormatFmask64_8 = 56,
    Format4_4 = 57,
    Format6_5_5 = 58,
    Format1 = 59,
    Format1_Reversed = 60,
    Format32_As_8 = 61,
    Format32_As_8_8 = 62,
    Format32_As_32_32_32_32 = 63,
};

enum class NumberFormat : u32 {
    Unorm = 0,
    Snorm = 1,
    Uscaled = 2,
    Sscaled = 3,
    Uint = 4,
    Sint = 5,
    SnormNz = 6,
    Float = 7,
    Srgb = 9,
    Ubnorm = 10,
    UbnormNz = 11,
    Ubint = 12,
    Ubscaled = 13,
};

enum class NumberClass {
    Float,
    Sint,
    Uint,
};

enum class CompSwizzle : u8 {
    Zero = 0,
    One = 1,
    Red = 4,
    Green = 5,
    Blue = 6,
    Alpha = 7,
};

enum class NumberConversion : u32 {
    None = 0,
    UintToUscaled = 1,
    SintToSscaled = 2,
    UnormToUbnorm = 3,
    Sint8ToSnormNz = 4,
    Sint16ToSnormNz = 5,
    Uint32ToUnorm = 6,
};

struct CompMapping {
    CompSwizzle r;
    CompSwizzle g;
    CompSwizzle b;
    CompSwizzle a;

    auto operator<=>(const CompMapping& other) const = default;

    template <typename T>
    [[nodiscard]] std::array<T, 4> Apply(const std::array<T, 4>& data) const {
        return {
            ApplySingle(data, r),
            ApplySingle(data, g),
            ApplySingle(data, b),
            ApplySingle(data, a),
        };
    }

    [[nodiscard]] CompMapping Inverse() const {
        CompMapping result{};
        InverseSingle(result.r, CompSwizzle::Red);
        InverseSingle(result.g, CompSwizzle::Green);
        InverseSingle(result.b, CompSwizzle::Blue);
        InverseSingle(result.a, CompSwizzle::Alpha);
        return result;
    }

private:
    template <typename T>
    T ApplySingle(const std::array<T, 4>& data, const CompSwizzle swizzle) const {
        switch (swizzle) {
        case CompSwizzle::Zero:
            return T(0);
        case CompSwizzle::One:
            return T(1);
        case CompSwizzle::Red:
            return data[0];
        case CompSwizzle::Green:
            return data[1];
        case CompSwizzle::Blue:
            return data[2];
        case CompSwizzle::Alpha:
            return data[3];
        default:
            UNREACHABLE();
        }
    }

    void InverseSingle(CompSwizzle& dst, const CompSwizzle target) const {
        if (r == target) {
            dst = CompSwizzle::Red;
        } else if (g == target) {
            dst = CompSwizzle::Green;
        } else if (b == target) {
            dst = CompSwizzle::Blue;
        } else if (a == target) {
            dst = CompSwizzle::Alpha;
        } else {
            dst = CompSwizzle::Zero;
        }
    }
};

static constexpr CompMapping IdentityMapping = {
    .r = CompSwizzle::Red,
    .g = CompSwizzle::Green,
    .b = CompSwizzle::Blue,
    .a = CompSwizzle::Alpha,
};

constexpr DataFormat RemapDataFormat(const DataFormat format) {
    switch (format) {
    case DataFormat::Format11_11_10:
        return DataFormat::Format10_11_11;
    case DataFormat::Format10_10_10_2:
        return DataFormat::Format2_10_10_10;
    case DataFormat::Format5_5_5_1:
        return DataFormat::Format1_5_5_5;
    default:
        return format;
    }
}

constexpr NumberFormat RemapNumberFormat(const NumberFormat format, const DataFormat data_format) {
    switch (format) {
    case NumberFormat::Unorm: {
        switch (data_format) {
        case DataFormat::Format32:
        case DataFormat::Format32_32:
        case DataFormat::Format32_32_32:
        case DataFormat::Format32_32_32_32:
            return NumberFormat::Uint;
        default:
            return format;
        }
    }
    case NumberFormat::Uscaled:
        return NumberFormat::Uint;
    case NumberFormat::Sscaled:
    case NumberFormat::SnormNz:
        return NumberFormat::Sint;
    case NumberFormat::Ubnorm:
        return NumberFormat::Unorm;
    case NumberFormat::Float:
        if (data_format == DataFormat::Format8) {
            // Games may ask for 8-bit float when they want to access the stencil component
            // of a depth-stencil image. Change to unsigned int to match the stencil format.
            // This is also the closest approximation to pass the bits through unconverted.
            return NumberFormat::Uint;
        }
        [[fallthrough]];
    default:
        return format;
    }
}

constexpr CompMapping RemapSwizzle(const DataFormat format, const CompMapping swizzle) {
    switch (format) {
    case DataFormat::Format1_5_5_5:
    case DataFormat::Format11_11_10: {
        CompMapping result;
        result.r = swizzle.b;
        result.g = swizzle.g;
        result.b = swizzle.r;
        result.a = swizzle.a;
        return result;
    }
    case DataFormat::Format10_10_10_2: {
        CompMapping result;
        result.r = swizzle.a;
        result.g = swizzle.b;
        result.b = swizzle.g;
        result.a = swizzle.r;
        return result;
    }
    case DataFormat::Format4_4_4_4: {
        // Remap to a more supported component order.
        CompMapping result;
        result.r = swizzle.g;
        result.g = swizzle.b;
        result.b = swizzle.a;
        result.a = swizzle.r;
        return result;
    }
    case DataFormat::Format5_6_5: {
        // Remap to a more supported component order.
        CompMapping result;
        result.r = swizzle.b;
        result.g = swizzle.g;
        result.b = swizzle.r;
        result.a = swizzle.a;
        return result;
    }
    default:
        return swizzle;
    }
}

constexpr NumberConversion MapNumberConversion(const NumberFormat num_fmt,
                                               const DataFormat data_fmt) {
    switch (num_fmt) {
    case NumberFormat::Unorm: {
        switch (data_fmt) {
        case DataFormat::Format32:
        case DataFormat::Format32_32:
        case DataFormat::Format32_32_32:
        case DataFormat::Format32_32_32_32:
            return NumberConversion::Uint32ToUnorm;
        default:
            return NumberConversion::None;
        }
    }
    case NumberFormat::Uscaled:
        return NumberConversion::UintToUscaled;
    case NumberFormat::Sscaled:
        return NumberConversion::SintToSscaled;
    case NumberFormat::Ubnorm:
        return NumberConversion::UnormToUbnorm;
    case NumberFormat::SnormNz: {
        switch (data_fmt) {
        case DataFormat::Format8:
        case DataFormat::Format8_8:
        case DataFormat::Format8_8_8_8:
            return NumberConversion::Sint8ToSnormNz;
        case DataFormat::Format16:
        case DataFormat::Format16_16:
        case DataFormat::Format16_16_16_16:
            return NumberConversion::Sint16ToSnormNz;
        default:
            UNREACHABLE_MSG("data_fmt = {}", u32(data_fmt));
        }
    }
    default:
        return NumberConversion::None;
    }
}

constexpr NumberClass GetNumberClass(const NumberFormat nfmt) {
    switch (nfmt) {
    case NumberFormat::Sint:
        return NumberClass::Sint;
    case NumberFormat::Uint:
        return NumberClass::Uint;
    default:
        return NumberClass::Float;
    }
}

constexpr bool IsInteger(const NumberFormat nfmt) {
    return nfmt == AmdGpu::NumberFormat::Sint || nfmt == AmdGpu::NumberFormat::Uint;
}

std::string_view NameOf(DataFormat fmt);
std::string_view NameOf(NumberFormat fmt);

u32 NumComponents(DataFormat format);
u32 NumBitsPerBlock(DataFormat format);

} // namespace AmdGpu

template <>
struct fmt::formatter<AmdGpu::DataFormat> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(AmdGpu::DataFormat fmt, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", AmdGpu::NameOf(fmt));
    }
};

template <>
struct fmt::formatter<AmdGpu::NumberFormat> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(AmdGpu::NumberFormat fmt, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", AmdGpu::NameOf(fmt));
    }
};
