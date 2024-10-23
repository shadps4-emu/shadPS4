// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/alignment.h"
#include "common/assert.h"
#include "common/bit_field.h"
#include "common/types.h"
#include "video_core/amdgpu/pixel_format.h"

namespace AmdGpu {

enum class CompSwizzle : u32 {
    Zero = 0,
    One = 1,
    Red = 4,
    Green = 5,
    Blue = 6,
    Alpha = 7,
};

// Table 8.5 Buffer Resource Descriptor [Sea Islands Series Instruction Set Architecture]
struct Buffer {
    u64 base_address : 44;
    u64 _padding0 : 4;
    u64 stride : 14;
    u64 cache_swizzle : 1;
    u64 swizzle_enable : 1;
    u32 num_records;
    u32 dst_sel_x : 3;
    u32 dst_sel_y : 3;
    u32 dst_sel_z : 3;
    u32 dst_sel_w : 3;
    u32 num_format : 3;
    u32 data_format : 4;
    u32 element_size : 2;
    u32 index_stride : 2;
    u32 add_tid_enable : 1;
    u32 _padding1 : 6;
    u32 type : 2; // overlaps with T# type, so should be 0 for buffer

    bool Valid() const {
        return type == 0u;
    }

    operator bool() const noexcept {
        return base_address != 0;
    }

    bool operator==(const Buffer& other) const noexcept {
        return std::memcmp(this, &other, sizeof(Buffer)) == 0;
    }

    CompSwizzle GetSwizzle(u32 comp) const noexcept {
        const std::array select{dst_sel_x, dst_sel_y, dst_sel_z, dst_sel_w};
        return static_cast<CompSwizzle>(select[comp]);
    }

    NumberFormat GetNumberFmt() const noexcept {
        return static_cast<NumberFormat>(num_format);
    }

    DataFormat GetDataFmt() const noexcept {
        return static_cast<DataFormat>(data_format);
    }

    u32 GetStride() const noexcept {
        return stride;
    }

    u32 NumDwords() const noexcept {
        return Common::AlignUp(GetSize(), sizeof(u32)) >> 2;
    }

    u32 GetSize() const noexcept {
        return stride == 0 ? num_records : (stride * num_records);
    }
};
static_assert(sizeof(Buffer) == 16); // 128bits

enum class ImageType : u64 {
    Invalid = 0,
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
    case ImageType::Invalid:
        return "Invalid";
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
    Depth_MacroTiled = 0u,
    Display_Linear = 0x8u,
    Display_MacroTiled = 0xAu,
    Texture_MicroTiled = 0xDu,
    Texture_MacroTiled = 0xEu,
};

constexpr std::string_view NameOf(TilingMode type) {
    switch (type) {
    case TilingMode::Depth_MacroTiled:
        return "Depth_MacroTiled";
    case TilingMode::Display_Linear:
        return "Display_Linear";
    case TilingMode::Display_MacroTiled:
        return "Display_MacroTiled";
    case TilingMode::Texture_MicroTiled:
        return "Texture_MicroTiled";
    case TilingMode::Texture_MacroTiled:
        return "Texture_MacroTiled";
    default:
        return "Unknown";
    }
}

struct Image {
    u64 base_address : 38;
    u64 mtype_l2 : 2;
    u64 min_lod : 12;
    u64 data_format : 6;
    u64 num_format : 4;
    u64 mtype : 2;

    u64 width : 14;
    u64 height : 14;
    u64 perf_modulation : 3;
    u64 interlaced : 1;
    u64 dst_sel_x : 3;
    u64 dst_sel_y : 3;
    u64 dst_sel_z : 3;
    u64 dst_sel_w : 3;
    u64 base_level : 4;
    u64 last_level : 4;
    u64 tiling_index : 5;
    u64 pow2pad : 1;
    u64 mtype2 : 1;
    u64 atc : 1;
    u64 type : 4; // overlaps with V# type, so shouldn't be 0 for buffer

    u64 depth : 13;
    u64 pitch : 14;
    u64 : 5;
    u64 base_array : 13;
    u64 last_array : 13;
    u64 : 6;
    u64 min_lod_warn : 12;
    u64 counter_bank_id : 8;
    u64 lod_hw_cnt_en : 1;
    u64 : 43;

    static constexpr Image Null() {
        Image image{};
        image.data_format = u64(DataFormat::Format8_8_8_8);
        image.dst_sel_x = 4;
        image.dst_sel_y = 5;
        image.dst_sel_z = 6;
        image.dst_sel_w = 7;
        image.tiling_index = u64(TilingMode::Texture_MicroTiled);
        image.type = u64(ImageType::Color2D);
        return image;
    }

    bool Valid() const {
        return (type & 0x8u) != 0;
    }

    VAddr Address() const {
        return base_address << 8;
    }

    operator bool() const noexcept {
        return base_address != 0;
    }

    u32 DstSelect() const {
        return dst_sel_x | (dst_sel_y << 3) | (dst_sel_z << 6) | (dst_sel_w << 9);
    }

    static char SelectComp(u32 sel) {
        switch (sel) {
        case 0:
            return '0';
        case 1:
            return '1';
        case 4:
            return 'R';
        case 5:
            return 'G';
        case 6:
            return 'B';
        case 7:
            return 'A';
        default:
            UNREACHABLE();
        }
    }

    std::string DstSelectName() const {
        std::string result = "[";
        u32 dst_sel = DstSelect();
        for (u32 i = 0; i < 4; i++) {
            result += SelectComp(dst_sel & 7);
            dst_sel >>= 3;
        }
        result += ']';
        return result;
    }

    u32 Pitch() const {
        return pitch + 1;
    }

    u32 NumLayers(bool is_array) const {
        u32 slices = GetType() == ImageType::Color3D ? 1 : depth + 1;
        if (GetType() == ImageType::Cube) {
            if (is_array) {
                slices = last_array + 1;
                ASSERT(slices % 6 == 0);
            } else {
                slices = 6;
            }
        }
        if (pow2pad) {
            slices = std::bit_ceil(slices);
        }
        return slices;
    }

    u32 NumLevels() const {
        if (GetType() == ImageType::Color2DMsaa || GetType() == ImageType::Color2DMsaaArray) {
            return 1;
        }
        return last_level + 1;
    }

    u32 NumSamples() const {
        if (GetType() == ImageType::Color2DMsaa || GetType() == ImageType::Color2DMsaaArray) {
            return 1u << last_level;
        }
        return 1;
    }

    ImageType GetType() const noexcept {
        return static_cast<ImageType>(type);
    }

    DataFormat GetDataFmt() const noexcept {
        return static_cast<DataFormat>(data_format);
    }

    NumberFormat GetNumberFmt() const noexcept {
        return static_cast<NumberFormat>(num_format);
    }

    TilingMode GetTilingMode() const {
        if (tiling_index >= 0 && tiling_index <= 7) {
            return tiling_index == 5 ? TilingMode::Texture_MicroTiled
                                     : TilingMode::Depth_MacroTiled;
        }
        if (tiling_index == 0x13) {
            return TilingMode::Texture_MicroTiled;
        }
        return static_cast<TilingMode>(tiling_index);
    }

    bool IsTiled() const {
        return GetTilingMode() != TilingMode::Display_Linear;
    }

    bool IsFmask() const noexcept {
        return GetDataFmt() >= DataFormat::FormatFmask8_1 &&
               GetDataFmt() <= DataFormat::FormatFmask64_8;
    }

    bool IsPartialCubemap() const {
        const auto viewed_slice = last_array - base_array + 1;
        return GetType() == ImageType::Cube && viewed_slice < 6;
    }
};
static_assert(sizeof(Image) == 32); // 256bits

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
        u64 raw0;
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
        u64 raw1;
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

    operator bool() const noexcept {
        return raw0 != 0 || raw1 != 0;
    }

    bool operator==(const Sampler& other) const noexcept {
        return std::memcmp(this, &other, sizeof(Sampler)) == 0;
    }

    float LodBias() const noexcept {
        return static_cast<float>(static_cast<int16_t>((lod_bias.Value() ^ 0x2000u) - 0x2000u)) /
               256.0f;
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
