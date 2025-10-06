// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "video_core/amdgpu/pixel_format.h"
#include "video_core/amdgpu/tiling.h"

namespace AmdGpu {

static constexpr u32 NUM_COLOR_BUFFERS = 8;

using BlendConstants = std::array<float, 4>;

struct BlendControl {
    enum class BlendFactor : u32 {
        Zero = 0,
        One = 1,
        SrcColor = 2,
        OneMinusSrcColor = 3,
        SrcAlpha = 4,
        OneMinusSrcAlpha = 5,
        DstAlpha = 6,
        OneMinusDstAlpha = 7,
        DstColor = 8,
        OneMinusDstColor = 9,
        SrcAlphaSaturate = 10,
        ConstantColor = 13,
        OneMinusConstantColor = 14,
        Src1Color = 15,
        InvSrc1Color = 16,
        Src1Alpha = 17,
        InvSrc1Alpha = 18,
        ConstantAlpha = 19,
        OneMinusConstantAlpha = 20,
    };

    enum class BlendFunc : u32 {
        Add = 0,
        Subtract = 1,
        Min = 2,
        Max = 3,
        ReverseSubtract = 4,
    };

    BlendFactor color_src_factor : 5;
    BlendFunc color_func : 3;
    BlendFactor color_dst_factor : 5;
    u32 : 3;
    BlendFactor alpha_src_factor : 5;
    BlendFunc alpha_func : 3;
    BlendFactor alpha_dst_factor : 5;
    u32 separate_alpha_blend : 1;
    u32 enable : 1;
    u32 disable_rop3 : 1;

    bool operator==(const BlendControl& other) const = default;
};

struct ColorControl {
    enum class OperationMode : u32 {
        Disable = 0u,
        Normal = 1u,
        EliminateFastClear = 2u,
        Resolve = 3u,
        Err = 4u,
        FmaskDecompress = 5u,
    };
    enum class LogicOp : u32 {
        Clear = 0x00,
        Nor = 0x11,
        AndInverted = 0x22,
        CopyInverted = 0x33,
        AndReverse = 0x44,
        Invert = 0x55,
        Xor = 0x66,
        Nand = 0x77,
        And = 0x88,
        Equiv = 0x99,
        Noop = 0xAA,
        OrInverted = 0xBB,
        Copy = 0xCC,
        OrReverse = 0xDD,
        Or = 0xEE,
        Set = 0xFF,
    };

    u32 disable_dual_quad : 1;
    u32 : 2;
    u32 degamma_enable : 1;
    OperationMode mode : 3;
    u32 : 9;
    LogicOp rop3 : 8;
};

struct ColorBufferMask {
    enum ColorComponent : u32 {
        ComponentR = (1u << 0),
        ComponentG = (1u << 1),
        ComponentB = (1u << 2),
        ComponentA = (1u << 3),
    };

    u32 raw;

    u32 GetMask(u32 buf_id) const {
        return (raw >> (buf_id * 4)) & 0xfu;
    }

    void SetMask(u32 buf_id, u32 mask) {
        raw &= ~(0xf << (buf_id * 4));
        raw |= (mask << (buf_id * 4));
    }
};

struct ColorBuffer {
    enum class EndianSwap : u32 {
        None = 0,
        Swap8In16 = 1,
        Swap8In32 = 2,
        Swap8In64 = 3,
    };

    enum class SwapMode : u32 {
        Standard = 0,
        Alternate = 1,
        StandardReverse = 2,
        AlternateReverse = 3,
    };

    enum class RoundMode : u32 {
        ByHalf = 0,
        Truncate = 1,
    };

    u32 base_address;
    struct {
        u32 tile_max : 11;
        u32 : 9;
        u32 fmask_tile_max : 11;
    } pitch;
    struct {
        u32 tile_max : 22;
    } slice;
    struct {
        u32 slice_start : 11;
        u32 : 2;
        u32 slice_max : 11;
    } view;
    union Color0Info {
        u32 raw;
        struct {
            EndianSwap endian : 2;
            u32 format : 5;
            u32 linear_general : 1;
            u32 number_type : 3;
            SwapMode comp_swap : 2;
            u32 fast_clear : 1;
            u32 compression : 1;
            u32 blend_clamp : 1;
            u32 blend_bypass : 1;
            u32 simple_float : 1;
            RoundMode round_mode : 1;
            u32 cmask_is_linear : 1;
            u32 blend_opt_dont_rd_dst : 3;
            u32 blend_opt_discard_pixel : 3;
            u32 fmask_compression_disable_ci : 1;
            u32 fmask_compress_1frag_only : 1;
            u32 dcc_enable : 1;
            u32 cmask_addr_type : 2;
            u32 alt_tile_mode : 1;
        };
    } info;
    union Color0Attrib {
        u32 raw;
        struct {
            TileMode tile_mode_index : 5;
            u32 fmask_tile_mode_index : 5;
            u32 fmask_bank_height : 2;
            u32 num_samples_log2 : 3;
            u32 num_fragments_log2 : 2;
            u32 force_dst_alpha_1 : 1;
        };
    } attrib;
    u32 pad0;
    u32 cmask_base_address;
    struct {
        u32 tile_max : 14;
    } cmask_slice;
    u32 fmask_base_address;
    struct {
        u32 tile_max : 14;
    } fmask_slice;
    u32 clear_word0;
    u32 clear_word1;
    std::array<u32, 2> pad1;

    operator bool() const {
        return base_address && info.format;
    }

    u32 Pitch() const {
        return (pitch.tile_max + 1) << 3;
    }

    u32 Height() const {
        return (slice.tile_max + 1) * 64 / Pitch();
    }

    u64 Address() const {
        return u64(base_address) << 8 | (info.linear_general ? (view.slice_start & 0xff) : 0);
    }

    VAddr CmaskAddress() const {
        return VAddr(cmask_base_address) << 8;
    }

    VAddr FmaskAddress() const {
        return VAddr(fmask_base_address) << 8;
    }

    u32 NumSamples() const {
        return 1 << attrib.num_fragments_log2;
    }

    u32 BaseSlice() const {
        return info.linear_general ? 0 : view.slice_start;
    }

    u32 NumSlices() const {
        return view.slice_max + 1;
    }

    u32 GetColorSliceSize() const {
        const auto num_bytes_per_element = NumBitsPerBlock(DataFormat(info.format)) / 8u;
        const auto slice_size = num_bytes_per_element * (slice.tile_max + 1) * 64u * NumSamples();
        return slice_size;
    }

    TileMode GetTileMode() const {
        return info.linear_general ? TileMode::DisplayLinearGeneral : attrib.tile_mode_index;
    }

    bool IsTiled() const {
        return GetTileMode() != TileMode::DisplayLinearAligned &&
               GetTileMode() != TileMode::DisplayLinearGeneral;
    }

    DataFormat GetDataFmt() const {
        return RemapDataFormat(DataFormat(info.format));
    }

    NumberFormat GetNumberFmt() const {
        return RemapNumberFormat(GetFixedNumberFormat(), DataFormat(info.format));
    }

    NumberConversion GetNumberConversion() const {
        return MapNumberConversion(GetFixedNumberFormat(), DataFormat(info.format));
    }

    CompMapping Swizzle() const {
        // clang-format off
        static constexpr std::array<std::array<CompMapping, 4>, 4> mrt_swizzles{{
            // Standard
            std::array<CompMapping, 4>{{
                {.r = CompSwizzle::Red, .g = CompSwizzle::Zero, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                {.r = CompSwizzle::Red, .g = CompSwizzle::Green, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                {.r = CompSwizzle::Red, .g = CompSwizzle::Green, .b = CompSwizzle::Blue, .a = CompSwizzle::Zero},
                {.r = CompSwizzle::Red, .g = CompSwizzle::Green, .b = CompSwizzle::Blue, .a = CompSwizzle::Alpha},
            }},
            // Alternate
            std::array<CompMapping, 4>{{
                {.r = CompSwizzle::Green, .g = CompSwizzle::Zero, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                {.r = CompSwizzle::Red, .g = CompSwizzle::Alpha, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                {.r = CompSwizzle::Red, .g = CompSwizzle::Green, .b = CompSwizzle::Alpha, .a = CompSwizzle::Zero},
                {.r = CompSwizzle::Blue, .g = CompSwizzle::Green, .b = CompSwizzle::Red, .a = CompSwizzle::Alpha},
            }},
            // StandardReverse
            std::array<CompMapping, 4>{{
                {.r = CompSwizzle::Blue, .g = CompSwizzle::Zero, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                {.r = CompSwizzle::Green, .g = CompSwizzle::Red, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                {.r = CompSwizzle::Blue, .g = CompSwizzle::Green, .b = CompSwizzle::Red, .a = CompSwizzle::Zero},
                {.r = CompSwizzle::Alpha, .g = CompSwizzle::Blue, .b = CompSwizzle::Green, .a = CompSwizzle::Red},
            }},
            // AlternateReverse
            std::array<CompMapping, 4>{{
                {.r = CompSwizzle::Alpha, .g = CompSwizzle::Zero, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                {.r = CompSwizzle::Alpha, .g = CompSwizzle::Red, .b = CompSwizzle::Zero, .a = CompSwizzle::Zero},
                {.r = CompSwizzle::Alpha, .g = CompSwizzle::Green, .b = CompSwizzle::Red, .a = CompSwizzle::Zero},
                {.r = CompSwizzle::Alpha, .g = CompSwizzle::Red, .b = CompSwizzle::Green, .a = CompSwizzle::Blue},
            }},
        }};
        // clang-format on
        const auto swap_idx = static_cast<u32>(info.comp_swap);
        const auto components_idx = NumComponents(DataFormat(info.format)) - 1;
        const auto mrt_swizzle = mrt_swizzles[swap_idx][components_idx];
        return RemapSwizzle(DataFormat(info.format), mrt_swizzle);
    }

    NumberFormat GetFixedNumberFormat() const {
        // There is a small difference between T# and CB number types, account for it.
        const auto number_fmt = NumberFormat(info.number_type);
        return number_fmt == NumberFormat::SnormNz ? NumberFormat::Srgb : number_fmt;
    }
};

} // namespace AmdGpu
