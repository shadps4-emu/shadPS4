// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/bit_field.h"
#include "common/types.h"
#include "video_core/amdgpu/pixel_format.h"

namespace AmdGpu {

// Table 8.5 Buffer Resource Descriptor [Sea Islands Series Instruction Set Architecture]
struct Buffer {
    union {
        BitField<0, 44, u64> base_address;
        BitField<48, 14, u64> stride;
        BitField<62, 1, u64> cache_swizzle;
        BitField<63, 1, u64> swizzle_enable;
    };
    u32 num_records;
    union {
        BitField<0, 3, u32> dst_sel_x;
        BitField<3, 3, u32> dst_sel_y;
        BitField<6, 3, u32> dst_sel_z;
        BitField<9, 3, u32> dst_sel_w;
        BitField<12, 3, NumberFormat> num_format;
        BitField<15, 4, DataFormat> data_format;
        BitField<19, 2, u32> element_size;
        BitField<21, 2, u32> index_stride;
        BitField<23, 1, u32> add_tid_enable;
    };

    u32 GetStride() const noexcept {
        return stride == 0 ? 1U : stride.Value();
    }

    u32 GetStrideElements(u32 element_size) const noexcept {
        if (stride == 0) {
            return 1U;
        }
        ASSERT(stride % element_size == 0);
        return stride / element_size;
    }

    u32 GetSize() const noexcept {
        return GetStride() * num_records;
    }
};

enum class ImageType : u64 {
    Buffer = 0,
    Color1D = 8,
    Color2D = 9,
    Color3D = 10,
    Cube = 11,
    Color1DArray = 12,
    Color2DArray = 13,
    Color2DMsaa = 14,
    Color2DMsaaArray = 15,
};

constexpr std::string_view NameOf(ImageType type) {
    switch (type) {
    case ImageType::Buffer:
        return "Buffer";
    case ImageType::Color1D:
        return "Color1D";
    case ImageType::Color2D:
        return "Color2D";
    case ImageType::Color3D:
        return "Color3D";
    case ImageType::Cube:
        return "Cube";
    case ImageType::Color1DArray:
        return "Color1DArray";
    case ImageType::Color2DArray:
        return "Color2DArray";
    case ImageType::Color2DMsaa:
        return "Color2DMsaa";
    case ImageType::Color2DMsaaArray:
        return "Color2DMsaaArray";
    default:
        return "Unknown";
    }
}

enum class TilingMode : u32 {
    Depth_MicroTiled = 0x5u,
    Display_Linear = 0x8u,
    Display_MacroTiled = 0xAu,
    Texture_MicroTiled = 0xDu,
};

constexpr std::string_view NameOf(TilingMode type) {
    switch (type) {
    case TilingMode::Depth_MicroTiled:
        return "Depth_MicroTiled";
    case TilingMode::Display_Linear:
        return "Display_Linear";
    case TilingMode::Display_MacroTiled:
        return "Display_MacroTiled";
    case TilingMode::Texture_MicroTiled:
        return "Texture_MicroTiled";
    default:
        return "Unknown";
    }
}

struct Image {
    union {
        BitField<0, 38, u64> base_address;
        BitField<40, 12, u64> min_lod;
        BitField<52, 6, u64> data_format;
        BitField<58, 4, u64> num_format;
        BitField<62, 2, u64> mtype;
    };
    union {
        BitField<0, 14, u64> width;
        BitField<14, 14, u64> height;
        BitField<28, 3, u64> perf_modulation;
        BitField<31, 1, u64> interlaced;
        BitField<32, 3, u64> dst_sel_x;
        BitField<35, 3, u64> dst_sel_y;
        BitField<38, 3, u64> dst_sel_z;
        BitField<41, 3, u64> dst_sel_w;
        BitField<44, 4, u64> base_level;
        BitField<48, 4, u64> last_level;
        BitField<52, 5, u64> tiling_index;
        BitField<57, 1, u64> pow2pad;
        BitField<58, 1, u64> mtype2;
        BitField<59, 1, u64> atc;
        BitField<60, 4, ImageType> type;
    };
    union {
        BitField<0, 13, u64> depth;
        BitField<13, 14, u64> pitch;
        BitField<32, 13, u64> base_array;
        BitField<45, 13, u64> last_array;
    };

    VAddr Address() const {
        return base_address << 8;
    }

    u32 Pitch() const {
        return pitch + 1;
    }

    u32 NumLayers() const {
        u32 slices = type == ImageType::Color3D ? 1 : depth.Value() + 1;
        if (type == ImageType::Cube) {
            slices *= 6;
        }
        if (pow2pad) {
            slices = std::bit_ceil(slices);
        }
        return slices;
    }

    u32 NumLevels() const {
        if (type == ImageType::Color2DMsaa || type == ImageType::Color2DMsaaArray) {
            return 1;
        }
        return last_level + 1;
    }

    DataFormat GetDataFmt() const noexcept {
        return static_cast<DataFormat>(data_format.Value());
    }

    NumberFormat GetNumberFmt() const noexcept {
        return static_cast<NumberFormat>(num_format.Value());
    }

    [[nodiscard]] TilingMode GetTilingMode() const {
        return static_cast<TilingMode>(tiling_index.Value());
    }

    [[nodiscard]] bool IsTiled() const {
        return GetTilingMode() != TilingMode::Display_Linear;
    }

    [[nodiscard]] size_t GetSizeAligned() const {
        // TODO: Derive this properly from tiling params
        return (width + 1) * (height + 1) * NumComponents(GetDataFmt());
    }
};

// 8.2.7. Image Sampler [RDNA 2 Instruction Set Architecture]
enum class ClampMode : u64 {
    Wrap = 0,
    Mirror = 1,
    ClampLastTexel = 2,
    MirrorOnceLastTexel = 3,
    ClampHalfBorder = 4,
    MirrorOnceHalfBorder = 5,
    ClampBorder = 6,
    MirrorOnceBorder = 7,
};

enum class AnisoRatio : u64 {
    One,
    Two,
    Four,
    Eight,
    Sixteen,
};

enum class DepthCompare : u64 {
    Never = 0,
    Less = 1,
    Equal = 2,
    LessEqual = 3,
    Greater = 4,
    NotEqual = 5,
    GreaterEqual = 6,
    Always = 7,
};

enum class FilterMode : u64 {
    Blend = 0,
    Min = 1,
    Max = 2,
};

enum class Filter : u64 {
    Point = 0,
    Bilinear = 1,
    AnisoPoint = 2,
    AnisoLinear = 3,
};

enum class MipFilter : u64 {
    None = 0,
    Point = 1,
    Linear = 2,
};

enum class BorderColor : u64 {
    OpaqueBlack = 0,
    TransparentBlack = 1,
    White = 2,
    Custom = 3,
};

// Table 8.12 Sampler Resource Definition
struct Sampler {
    union {
        BitField<0, 3, ClampMode> clamp_x;
        BitField<3, 3, ClampMode> clamp_y;
        BitField<6, 3, ClampMode> clamp_z;
        BitField<9, 3, AnisoRatio> max_aniso;
        BitField<12, 3, DepthCompare> depth_compare_func;
        BitField<15, 1, u64> force_unnormalized;
        BitField<16, 3, u64> aniso_threshold;
        BitField<19, 1, u64> mc_coord_trunc;
        BitField<20, 1, u64> force_degamma;
        BitField<21, 6, u64> aniso_bias;
        BitField<27, 1, u64> trunc_coord;
        BitField<28, 1, u64> disable_cube_wrap;
        BitField<29, 2, FilterMode> filter_mode;
        BitField<32, 12, u64> min_lod;
        BitField<44, 12, u64> max_lod;
        BitField<56, 4, u64> perf_mip;
        BitField<60, 4, u64> perf_z;
    };
    union {
        BitField<0, 14, u64> lod_bias;
        BitField<14, 6, u64> lod_bias_sec;
        BitField<20, 2, Filter> xy_mag_filter;
        BitField<22, 2, Filter> xy_min_filter;
        BitField<24, 2, u64> z_filter;
        BitField<26, 2, MipFilter> mip_filter;
        BitField<28, 1, u64> mip_point_preclamp;
        BitField<29, 1, u64> disable_lsb_ceil;
        BitField<30, 2, u64> unused0;
        BitField<32, 12, u64> border_color_ptr;
        BitField<42, 18, u64> unused1;
        BitField<62, 2, BorderColor> border_color_type;
    };

    float LodBias() const noexcept {
        return static_cast<float>(lod_bias);
    }

    float MinLod() const noexcept {
        return static_cast<float>(min_lod);
    }

    float MaxLod() const noexcept {
        return static_cast<float>(max_lod);
    }
};

} // namespace AmdGpu

template <>
struct fmt::formatter<AmdGpu::ImageType> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(AmdGpu::ImageType type, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", AmdGpu::NameOf(type));
    }
};
