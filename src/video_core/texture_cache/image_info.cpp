// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/config.h"
#include "video_core/renderer_vulkan/liverpool_to_vk.h"
#include "video_core/texture_cache/image_info.h"

namespace VideoCore {

using namespace Vulkan;
using Libraries::VideoOut::TilingMode;
using VideoOutFormat = Libraries::VideoOut::PixelFormat;

static vk::Format ConvertPixelFormat(const VideoOutFormat format) {
    switch (format) {
    case VideoOutFormat::A8R8G8B8Srgb:
        return vk::Format::eB8G8R8A8Srgb;
    case VideoOutFormat::A8B8G8R8Srgb:
        return vk::Format::eR8G8B8A8Srgb;
    case VideoOutFormat::A2R10G10B10:
    case VideoOutFormat::A2R10G10B10Srgb:
        return vk::Format::eA2R10G10B10UnormPack32;
    default:
        break;
    }
    UNREACHABLE_MSG("Unknown format={}", static_cast<u32>(format));
    return {};
}

static vk::ImageType ConvertImageType(AmdGpu::ImageType type) noexcept {
    switch (type) {
    case AmdGpu::ImageType::Color1D:
    case AmdGpu::ImageType::Color1DArray:
        return vk::ImageType::e1D;
    case AmdGpu::ImageType::Color2D:
    case AmdGpu::ImageType::Color2DMsaa:
    case AmdGpu::ImageType::Cube:
    case AmdGpu::ImageType::Color2DArray:
        return vk::ImageType::e2D;
    case AmdGpu::ImageType::Color3D:
        return vk::ImageType::e3D;
    default:
        UNREACHABLE();
    }
}

// clang-format off
// The table of macro tiles parameters for given tiling index (row) and bpp (column)
static constexpr std::array macro_tile_extents{
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, // 00
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 01
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 02
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 03
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   // 04
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 05
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 06
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   // 07
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 08
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 09
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   // 0A
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   // 0B
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  // 0C
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 0D
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   // 0E
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   // 0F
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   // 10
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   // 11
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  // 12
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 13
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 14
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 15
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 16
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 17
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{128u, 64u},  // 18
    std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 19
    std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 1A
};
// clang-format on

static constexpr std::pair micro_tile_extent{8u, 8u};
static constexpr auto hw_pipe_interleave = 256u;

static constexpr std::pair<u32, u32> GetMacroTileExtents(u32 tiling_idx, u32 bpp, u32 num_samples) {
    ASSERT(num_samples == 1);
    const auto row = tiling_idx * 5;
    const auto column = std::bit_width(bpp) - 4; // bpps are 8, 16, 32, 64, 128
    return macro_tile_extents[row + column];
}

static constexpr std::pair<u32, size_t> ImageSizeLinearAligned(u32 pitch, u32 height, u32 bpp,
                                                               u32 num_samples) {
    const auto pitch_align = std::max(8u, 64u / ((bpp + 7) / 8));
    auto pitch_aligned = (pitch + pitch_align - 1) & ~(pitch_align - 1);
    const auto height_aligned = height;
    size_t log_sz = pitch_aligned * height_aligned * num_samples;
    const auto slice_align = std::max(64u, 256u / ((bpp + 7) / 8));
    while (log_sz % slice_align) {
        pitch_aligned += pitch_align;
        log_sz = pitch_aligned * height_aligned * num_samples;
    }
    return {pitch_aligned, (log_sz * bpp + 7) / 8};
}

static constexpr std::pair<u32, size_t> ImageSizeMicroTiled(u32 pitch, u32 height, u32 bpp,
                                                            u32 num_samples) {
    const auto& [pitch_align, height_align] = micro_tile_extent;
    auto pitch_aligned = (pitch + pitch_align - 1) & ~(pitch_align - 1);
    const auto height_aligned = (height + height_align - 1) & ~(height_align - 1);
    size_t log_sz = (pitch_aligned * height_aligned * bpp * num_samples + 7) / 8;
    while (log_sz % 256) {
        pitch_aligned += 8;
        log_sz = (pitch_aligned * height_aligned * bpp * num_samples + 7) / 8;
    }
    return {pitch_aligned, log_sz};
}

static constexpr std::pair<u32, size_t> ImageSizeMacroTiled(u32 pitch, u32 height, u32 bpp,
                                                            u32 num_samples, u32 tiling_idx) {
    const auto& [pitch_align, height_align] = GetMacroTileExtents(tiling_idx, bpp, num_samples);
    ASSERT(pitch_align != 0 && height_align != 0);
    const auto pitch_aligned = (pitch + pitch_align - 1) & ~(pitch_align - 1);
    const auto height_aligned = (height + height_align - 1) & ~(height_align - 1);
    const auto log_sz = pitch_aligned * height_aligned * num_samples;
    return {pitch_aligned, (log_sz * bpp + 7) / 8};
}

ImageInfo::ImageInfo(const Libraries::VideoOut::BufferAttributeGroup& group,
                     VAddr cpu_address) noexcept {
    const auto& attrib = group.attrib;
    props.is_tiled = attrib.tiling_mode == TilingMode::Tile;
    tiling_mode = props.is_tiled ? AmdGpu::TilingMode::Display_MacroTiled
                                 : AmdGpu::TilingMode::Display_Linear;
    pixel_format = ConvertPixelFormat(attrib.pixel_format);
    type = vk::ImageType::e2D;
    size.width = attrib.width;
    size.height = attrib.height;
    pitch = attrib.tiling_mode == TilingMode::Linear ? size.width : (size.width + 127) & (~127);
    usage.vo_buffer = true;
    num_bits = attrib.pixel_format != VideoOutFormat::A16R16G16B16Float ? 32 : 64;
    ASSERT(num_bits == 32);

    guest_address = cpu_address;
    if (!props.is_tiled) {
        guest_size_bytes = pitch * size.height * 4;
    } else {
        if (Config::isNeoMode()) {
            guest_size_bytes = pitch * ((size.height + 127) & (~127)) * 4;
        } else {
            guest_size_bytes = pitch * ((size.height + 63) & (~63)) * 4;
        }
    }
    mips_layout.emplace_back(guest_size_bytes, pitch, 0);
}

ImageInfo::ImageInfo(const AmdGpu::Liverpool::ColorBuffer& buffer,
                     const AmdGpu::Liverpool::CbDbExtent& hint /*= {}*/) noexcept {
    props.is_tiled = buffer.IsTiled();
    tiling_mode = buffer.GetTilingMode();
    pixel_format = LiverpoolToVK::SurfaceFormat(buffer.info.format, buffer.NumFormat());
    num_samples = 1 << buffer.attrib.num_fragments_log2;
    num_bits = NumBits(buffer.info.format);
    type = vk::ImageType::e2D;
    size.width = hint.Valid() ? hint.width : buffer.Pitch();
    size.height = hint.Valid() ? hint.height : buffer.Height();
    size.depth = 1;
    pitch = buffer.Pitch();
    resources.layers = buffer.NumSlices();
    meta_info.cmask_addr = buffer.info.fast_clear ? buffer.CmaskAddress() : 0;
    meta_info.fmask_addr = buffer.info.compression ? buffer.FmaskAddress() : 0;
    usage.render_target = true;

    guest_address = buffer.Address();
    const auto color_slice_sz = buffer.GetColorSliceSize();
    guest_size_bytes = color_slice_sz * buffer.NumSlices();
    mips_layout.emplace_back(color_slice_sz, pitch, 0);
    tiling_idx = static_cast<u32>(buffer.attrib.tile_mode_index.Value());
}

ImageInfo::ImageInfo(const AmdGpu::Liverpool::DepthBuffer& buffer, u32 num_slices,
                     VAddr htile_address, const AmdGpu::Liverpool::CbDbExtent& hint) noexcept {
    props.is_tiled = false;
    pixel_format = LiverpoolToVK::DepthFormat(buffer.z_info.format, buffer.stencil_info.format);
    type = vk::ImageType::e2D;
    num_samples = 1 << buffer.z_info.num_samples; // spec doesn't say it is a log2
    num_bits = buffer.NumBits();
    size.width = hint.Valid() ? hint.width : buffer.Pitch();
    size.height = hint.Valid() ? hint.height : buffer.Height();
    size.depth = 1;
    pitch = buffer.Pitch();
    resources.layers = num_slices;
    meta_info.htile_addr = buffer.z_info.tile_surface_en ? htile_address : 0;
    usage.depth_target = true;
    usage.stencil =
        buffer.stencil_info.format != AmdGpu::Liverpool::DepthBuffer::StencilFormat::Invalid;

    guest_address = buffer.Address();
    const auto depth_slice_sz = buffer.GetDepthSliceSize();
    guest_size_bytes = depth_slice_sz * num_slices;
    mips_layout.emplace_back(depth_slice_sz, pitch, 0);
}

ImageInfo::ImageInfo(const AmdGpu::Image& image, bool force_depth /*= false*/) noexcept {
    tiling_mode = image.GetTilingMode();
    pixel_format = LiverpoolToVK::SurfaceFormat(image.GetDataFmt(), image.GetNumberFmt());
    // Override format if image is forced to be a depth target
    if (force_depth) {
        if (pixel_format == vk::Format::eR32Sfloat || pixel_format == vk::Format::eR8Unorm) {
            pixel_format = vk::Format::eD32SfloatS8Uint;
        } else if (pixel_format == vk::Format::eR16Unorm) {
            pixel_format = vk::Format::eD16UnormS8Uint;
        } else {
            UNREACHABLE();
        }
    }
    type = ConvertImageType(image.GetType());
    props.is_tiled = image.IsTiled();
    props.is_cube = image.GetType() == AmdGpu::ImageType::Cube;
    props.is_volume = image.GetType() == AmdGpu::ImageType::Color3D;
    props.is_pow2 = image.pow2pad;
    props.is_block = IsBlockCoded();
    size.width = image.width + 1;
    size.height = image.height + 1;
    size.depth = props.is_volume ? image.depth + 1 : 1;
    pitch = image.Pitch();
    resources.levels = image.NumLevels();
    resources.layers = image.NumLayers();
    num_bits = NumBits(image.GetDataFmt());
    usage.texture = true;

    guest_address = image.Address();

    mips_layout.reserve(resources.levels);
    tiling_idx = image.tiling_index;
    UpdateSize();
}

void ImageInfo::UpdateSize() {
    mips_layout.clear();
    MipInfo mip_info{};
    guest_size_bytes = 0;
    for (auto mip = 0u; mip < resources.levels; ++mip) {
        auto bpp = num_bits;
        auto mip_w = pitch >> mip;
        auto mip_h = size.height >> mip;
        if (props.is_block) {
            mip_w = (mip_w + 3) / 4;
            mip_h = (mip_h + 3) / 4;
            bpp *= 16;
        }
        mip_w = std::max(mip_w, 1u);
        mip_h = std::max(mip_h, 1u);
        auto mip_d = std::max(size.depth >> mip, 1u);

        if (props.is_pow2) {
            mip_w = std::bit_ceil(mip_w);
            mip_h = std::bit_ceil(mip_h);
            mip_d = std::bit_ceil(mip_d);
        }

        switch (tiling_mode) {
        case AmdGpu::TilingMode::Display_Linear: {
            std::tie(mip_info.pitch, mip_info.size) =
                ImageSizeLinearAligned(mip_w, mip_h, bpp, num_samples);
            mip_info.height = mip_h;
            break;
        }
        case AmdGpu::TilingMode::Texture_MicroTiled: {
            std::tie(mip_info.pitch, mip_info.size) =
                ImageSizeMicroTiled(mip_w, mip_h, bpp, num_samples);
            mip_info.height = std::max(mip_h, 8u);
            if (props.is_block) {
                mip_info.pitch = std::max(mip_info.pitch * 4, 32u);
                mip_info.height = std::max(mip_info.height * 4, 32u);
            }
            break;
        }
        case AmdGpu::TilingMode::Display_MacroTiled:
        case AmdGpu::TilingMode::Texture_MacroTiled:
        case AmdGpu::TilingMode::Depth_MacroTiled: {
            ASSERT(!props.is_block);
            ASSERT(num_samples == 1);
            std::tie(mip_info.pitch, mip_info.size) =
                ImageSizeMacroTiled(mip_w, mip_h, bpp, num_samples, tiling_idx);
            break;
        }
        default: {
            UNREACHABLE();
        }
        }
        mip_info.size *= mip_d;

        mip_info.offset = guest_size_bytes;
        mips_layout.emplace_back(mip_info);
        guest_size_bytes += mip_info.size;
    }
    guest_size_bytes *= resources.layers;
}

bool ImageInfo::IsMipOf(const ImageInfo& info) const {
    if (!IsCompatible(info)) {
        return false;
    }

    // Currently we expect only on level to be copied.
    if (resources.levels != 1) {
        return false;
    }

    const int mip = info.resources.levels - resources.levels;
    if (mip < 1) {
        return false;
    }

    const auto mip_w = std::max(info.size.width >> mip, 1u);
    const auto mip_h = std::max(info.size.height >> mip, 1u);
    if ((size.width != mip_w) || (size.height != mip_h)) {
        return false;
    }

    const auto mip_d = std::max(info.size.depth >> mip, 1u);
    if (info.type == vk::ImageType::e3D && type == vk::ImageType::e2D) {
        // In case of 2D array to 3D copy, make sure we have proper number of layers.
        if (resources.layers != mip_d) {
            return false;
        }
    } else {
        if (type != info.type) {
            return false;
        }
    }

    // Check if the mip has correct size.
    if (info.mips_layout.size() <= mip || info.mips_layout[mip].size != guest_size_bytes) {
        return false;
    }

    return true;
}

bool ImageInfo::IsSliceOf(const ImageInfo& info) const {
    if (!IsCompatible(info)) {
        return false;
    }

    // Array slices should be of the same type.
    if (type != info.type) {
        return false;
    }

    // 2D dimensions of both images should be the same.
    if ((size.width != info.size.width) || (size.height != info.size.height)) {
        return false;
    }

    // Check for size alignment.
    const bool slice_size = info.guest_size_bytes / info.resources.layers;
    if (guest_size_bytes % slice_size != 0) {
        return false;
    }

    // Ensure that address is aligned too.
    if (((info.guest_address - guest_address) % guest_size_bytes) != 0) {
        return false;
    }

    return true;
}

} // namespace VideoCore
