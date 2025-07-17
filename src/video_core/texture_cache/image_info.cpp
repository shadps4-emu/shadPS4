// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/config.h"
#include "core/libraries/kernel/process.h"
#include "video_core/renderer_vulkan/liverpool_to_vk.h"
#include "video_core/texture_cache/image_info.h"
#include "video_core/texture_cache/tile.h"

namespace VideoCore {

using namespace Vulkan;
using Libraries::VideoOut::TilingMode;
using VideoOutFormat = Libraries::VideoOut::PixelFormat;

static vk::Format ConvertPixelFormat(const VideoOutFormat format) {
    switch (format) {
    case VideoOutFormat::A8B8G8R8Srgb:
    // Remaining formats are mapped to RGBA for internal consistency and changed to BGRA in the
    // frame image view.
    case VideoOutFormat::A8R8G8B8Srgb:
        return vk::Format::eR8G8B8A8Srgb;
    case VideoOutFormat::A2R10G10B10:
    case VideoOutFormat::A2R10G10B10Srgb:
    case VideoOutFormat::A2R10G10B10Bt2020Pq:
        return vk::Format::eA2B10G10R10UnormPack32;
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
    case AmdGpu::ImageType::Color2DArray:
        return vk::ImageType::e2D;
    case AmdGpu::ImageType::Color3D:
        return vk::ImageType::e3D;
    default:
        UNREACHABLE();
    }
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
    num_bits = attrib.pixel_format != VideoOutFormat::A16R16G16B16Float ? 32 : 64;
    ASSERT(num_bits == 32);

    guest_address = cpu_address;
    if (!props.is_tiled) {
        guest_size = pitch * size.height * 4;
    } else {
        if (Libraries::Kernel::sceKernelIsNeoMode()) {
            guest_size = pitch * ((size.height + 127) & (~127)) * 4;
        } else {
            guest_size = pitch * ((size.height + 63) & (~63)) * 4;
        }
    }
    mips_layout.emplace_back(guest_size, pitch, 0);
}

ImageInfo::ImageInfo(const AmdGpu::Liverpool::ColorBuffer& buffer,
                     const AmdGpu::Liverpool::CbDbExtent& hint /*= {}*/) noexcept {
    props.is_tiled = buffer.IsTiled();
    tiling_mode = buffer.GetTilingMode();
    pixel_format = LiverpoolToVK::SurfaceFormat(buffer.GetDataFmt(), buffer.GetNumberFmt());
    num_samples = buffer.NumSamples();
    num_bits = NumBitsPerBlock(buffer.GetDataFmt());
    type = vk::ImageType::e2D;
    size.width = hint.Valid() ? hint.width : buffer.Pitch();
    size.height = hint.Valid() ? hint.height : buffer.Height();
    size.depth = 1;
    pitch = buffer.Pitch();
    resources.layers = buffer.NumSlices();
    meta_info.cmask_addr = buffer.info.fast_clear ? buffer.CmaskAddress() : 0;
    meta_info.fmask_addr = buffer.info.compression ? buffer.FmaskAddress() : 0;

    guest_address = buffer.Address();
    const auto color_slice_sz = buffer.GetColorSliceSize();
    guest_size = color_slice_sz * buffer.NumSlices();
    mips_layout.emplace_back(color_slice_sz, pitch, 0);
    tiling_idx = static_cast<u32>(buffer.attrib.tile_mode_index.Value());
    alt_tile = Libraries::Kernel::sceKernelIsNeoMode() && buffer.info.alt_tile_mode;
}

ImageInfo::ImageInfo(const AmdGpu::Liverpool::DepthBuffer& buffer, u32 num_slices,
                     VAddr htile_address, const AmdGpu::Liverpool::CbDbExtent& hint,
                     bool write_buffer) noexcept {
    props.is_tiled = false;
    pixel_format = LiverpoolToVK::DepthFormat(buffer.z_info.format, buffer.stencil_info.format);
    type = vk::ImageType::e2D;
    num_samples = buffer.NumSamples();
    num_bits = buffer.NumBits();
    size.width = hint.Valid() ? hint.width : buffer.Pitch();
    size.height = hint.Valid() ? hint.height : buffer.Height();
    size.depth = 1;
    pitch = buffer.Pitch();
    resources.layers = num_slices;
    meta_info.htile_addr = buffer.z_info.tile_surface_en ? htile_address : 0;

    stencil_addr = write_buffer ? buffer.StencilWriteAddress() : buffer.StencilAddress();
    stencil_size = pitch * size.height * sizeof(u8);

    guest_address = write_buffer ? buffer.DepthWriteAddress() : buffer.DepthAddress();
    const auto depth_slice_sz = buffer.GetDepthSliceSize();
    guest_size = depth_slice_sz * num_slices;
    mips_layout.emplace_back(depth_slice_sz, pitch, 0);
}

ImageInfo::ImageInfo(const AmdGpu::Image& image, const Shader::ImageResource& desc) noexcept {
    tiling_mode = image.GetTilingMode();
    pixel_format = LiverpoolToVK::SurfaceFormat(image.GetDataFmt(), image.GetNumberFmt());
    // Override format if image is forced to be a depth target
    if (desc.is_depth) {
        pixel_format = LiverpoolToVK::PromoteFormatToDepth(pixel_format);
    }
    type = ConvertImageType(image.GetType());
    props.is_tiled = image.IsTiled();
    props.is_volume = image.GetType() == AmdGpu::ImageType::Color3D;
    props.is_pow2 = image.pow2pad;
    props.is_block = IsBlockCoded();
    size.width = image.width + 1;
    size.height = image.height + 1;
    size.depth = props.is_volume ? image.depth + 1 : 1;
    pitch = image.Pitch();
    resources.levels = image.NumLevels();
    resources.layers = image.NumLayers();
    num_samples = image.NumSamples();
    num_bits = NumBitsPerBlock(image.GetDataFmt());

    guest_address = image.Address();

    mips_layout.reserve(resources.levels);
    tiling_idx = image.tiling_index;
    alt_tile = Libraries::Kernel::sceKernelIsNeoMode() && image.alt_tile_mode;
    UpdateSize();
}

bool ImageInfo::IsBlockCoded() const {
    switch (pixel_format) {
    case vk::Format::eBc1RgbaSrgbBlock:
    case vk::Format::eBc1RgbaUnormBlock:
    case vk::Format::eBc1RgbSrgbBlock:
    case vk::Format::eBc1RgbUnormBlock:
    case vk::Format::eBc2SrgbBlock:
    case vk::Format::eBc2UnormBlock:
    case vk::Format::eBc3SrgbBlock:
    case vk::Format::eBc3UnormBlock:
    case vk::Format::eBc4SnormBlock:
    case vk::Format::eBc4UnormBlock:
    case vk::Format::eBc5SnormBlock:
    case vk::Format::eBc5UnormBlock:
    case vk::Format::eBc6HSfloatBlock:
    case vk::Format::eBc6HUfloatBlock:
    case vk::Format::eBc7SrgbBlock:
    case vk::Format::eBc7UnormBlock:
        return true;
    default:
        return false;
    }
}

bool ImageInfo::IsDepthStencil() const {
    switch (pixel_format) {
    case vk::Format::eD16Unorm:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD32Sfloat:
    case vk::Format::eD32SfloatS8Uint:
        return true;
    default:
        return false;
    }
}

bool ImageInfo::HasStencil() const {
    if (pixel_format == vk::Format::eD32SfloatS8Uint ||
        pixel_format == vk::Format::eD24UnormS8Uint ||
        pixel_format == vk::Format::eD16UnormS8Uint) {
        return true;
    }
    return false;
}

bool ImageInfo::IsCompatible(const ImageInfo& info) const {
    return (pixel_format == info.pixel_format && num_samples == info.num_samples &&
            num_bits == info.num_bits);
}

bool ImageInfo::IsTilingCompatible(u32 lhs, u32 rhs) const {
    if (lhs == rhs) {
        return true;
    }
    if (lhs == 0x0e && rhs == 0x0d) {
        return true;
    }
    if (lhs == 0x0d && rhs == 0x0e) {
        return true;
    }
    return false;
}

void ImageInfo::UpdateSize() {
    mips_layout.clear();
    MipInfo mip_info{};
    guest_size = 0;
    for (auto mip = 0u; mip < resources.levels; ++mip) {
        auto bpp = num_bits;
        auto mip_w = pitch >> mip;
        auto mip_h = size.height >> mip;
        if (props.is_block) {
            mip_w = (mip_w + 3) / 4;
            mip_h = (mip_h + 3) / 4;
        }
        mip_w = std::max(mip_w, 1u);
        mip_h = std::max(mip_h, 1u);
        auto mip_d = std::max(size.depth >> mip, 1u);
        auto thickness = 1;

        if (props.is_pow2) {
            mip_w = std::bit_ceil(mip_w);
            mip_h = std::bit_ceil(mip_h);
            mip_d = std::bit_ceil(mip_d);
        }

        switch (tiling_mode) {
        case AmdGpu::TilingMode::Display_Linear: {
            std::tie(mip_info.pitch, mip_info.size) =
                ImageSizeLinearAligned(mip_w, mip_h, bpp, num_samples);
            break;
        }
        case AmdGpu::TilingMode::Texture_Volume:
            thickness = 4;
            mip_d += (-mip_d) & (thickness - 1);
            [[fallthrough]];
        case AmdGpu::TilingMode::Display_MicroTiled:
        case AmdGpu::TilingMode::Texture_MicroTiled: {
            std::tie(mip_info.pitch, mip_info.size) =
                ImageSizeMicroTiled(mip_w, mip_h, thickness, bpp, num_samples);
            break;
        }
        case AmdGpu::TilingMode::Display_MacroTiled:
        case AmdGpu::TilingMode::Texture_MacroTiled:
        case AmdGpu::TilingMode::Depth_MacroTiled: {
            ASSERT(!props.is_block);
            std::tie(mip_info.pitch, mip_info.size) = ImageSizeMacroTiled(
                mip_w, mip_h, thickness, bpp, num_samples, tiling_idx, mip, alt_tile);
            break;
        }
        default: {
            UNREACHABLE();
        }
        }
        mip_info.height = mip_h;
        if (props.is_block) {
            mip_info.pitch = std::max(mip_info.pitch * 4, 32u);
            mip_info.height = std::max(mip_info.height * 4, 32u);
        }
        mip_info.size *= mip_d * resources.layers;
        mip_info.offset = guest_size;
        mips_layout.emplace_back(mip_info);
        guest_size += mip_info.size;
    }
}

s32 ImageInfo::MipOf(const ImageInfo& info) const {
    if (!IsCompatible(info)) {
        return -1;
    }

    if (!IsTilingCompatible(info.tiling_idx, tiling_idx)) {
        return -1;
    }

    // Currently we expect only on level to be copied.
    if (resources.levels != 1) {
        return -1;
    }

    if (info.mips_layout.empty()) {
        UNREACHABLE();
    }

    // Find mip
    auto mip = -1;
    for (auto m = 0; m < info.mips_layout.size(); ++m) {
        const auto& [mip_size, mip_pitch, mip_height, mip_ofs] = info.mips_layout[m];
        const VAddr mip_base = info.guest_address + mip_ofs;
        const VAddr mip_end = mip_base + mip_size;
        const u32 slice_size = mip_size / info.resources.layers;
        if (guest_address >= mip_base && guest_address < mip_end &&
            (guest_address - mip_base) % slice_size == 0) {
            mip = m;
            break;
        }
    }

    if (mip < 0) {
        return -1;
    }

    const auto mip_w = std::max(info.size.width >> mip, 1u);
    const auto mip_h = std::max(info.size.height >> mip, 1u);
    if ((size.width != mip_w) || (size.height != mip_h)) {
        return -1;
    }

    const auto mip_d = std::max(info.size.depth >> mip, 1u);
    if (info.type == vk::ImageType::e3D && type == vk::ImageType::e2D) {
        // In case of 2D array to 3D copy, make sure we have proper number of layers.
        if (resources.layers != mip_d) {
            return -1;
        }
    } else {
        if (type != info.type) {
            return -1;
        }
    }

    return mip;
}

s32 ImageInfo::SliceOf(const ImageInfo& info, s32 mip) const {
    if (!IsCompatible(info)) {
        return -1;
    }

    // Array slices should be of the same type.
    if (type != info.type) {
        return -1;
    }

    // 2D dimensions of both images should be the same.
    const auto mip_w = std::max(info.size.width >> mip, 1u);
    const auto mip_h = std::max(info.size.height >> mip, 1u);
    if ((size.width != mip_w) || (size.height != mip_h)) {
        return -1;
    }

    // Check for size alignment.
    const u32 slice_size = info.mips_layout[mip].size / info.resources.layers;
    if (guest_size % slice_size != 0) {
        return -1;
    }

    // Ensure that address is aligned too.
    const auto addr_diff = guest_address - (info.guest_address + info.mips_layout[mip].offset);
    if ((addr_diff % guest_size) != 0) {
        return -1;
    }

    return addr_diff / guest_size;
}

} // namespace VideoCore
