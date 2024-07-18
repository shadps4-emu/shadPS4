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
    case AmdGpu::ImageType::Cube:
    case AmdGpu::ImageType::Color2DArray:
        return vk::ImageType::e2D;
    case AmdGpu::ImageType::Color3D:
        return vk::ImageType::e3D;
    default:
        UNREACHABLE();
    }
}

ImageInfo::ImageInfo(const Libraries::VideoOut::BufferAttributeGroup& group) noexcept {
    const auto& attrib = group.attrib;
    is_tiled = attrib.tiling_mode == TilingMode::Tile;
    tiling_mode =
        is_tiled ? AmdGpu::TilingMode::Display_MacroTiled : AmdGpu::TilingMode::Display_Linear;
    pixel_format = ConvertPixelFormat(attrib.pixel_format);
    type = vk::ImageType::e2D;
    size.width = attrib.width;
    size.height = attrib.height;
    pitch = attrib.tiling_mode == TilingMode::Linear ? size.width : (size.width + 127) & (~127);
    const bool is_32bpp = attrib.pixel_format != VideoOutFormat::A16R16G16B16Float;
    ASSERT(is_32bpp);
    if (!is_tiled) {
        guest_size_bytes = pitch * size.height * 4;
        return;
    }
    if (Config::isNeoMode()) {
        guest_size_bytes = pitch * ((size.height + 127) & (~127)) * 4;
    } else {
        guest_size_bytes = pitch * ((size.height + 63) & (~63)) * 4;
    }
    usage.vo_buffer = true;
}

ImageInfo::ImageInfo(const AmdGpu::Liverpool::ColorBuffer& buffer,
                     const AmdGpu::Liverpool::CbDbExtent& hint /*= {}*/) noexcept {
    is_tiled = buffer.IsTiled();
    tiling_mode = buffer.GetTilingMode();
    pixel_format = LiverpoolToVK::SurfaceFormat(buffer.info.format, buffer.NumFormat());
    num_samples = 1 << buffer.attrib.num_fragments_log2;
    type = vk::ImageType::e2D;
    size.width = hint.Valid() ? hint.width : buffer.Pitch();
    size.height = hint.Valid() ? hint.height : buffer.Height();
    size.depth = 1;
    pitch = size.width;
    guest_size_bytes = buffer.GetColorSliceSize();
    meta_info.cmask_addr = buffer.info.fast_clear ? buffer.CmaskAddress() : 0;
    meta_info.fmask_addr = buffer.info.compression ? buffer.FmaskAddress() : 0;
    usage.render_target = true;
}

ImageInfo::ImageInfo(const AmdGpu::Liverpool::DepthBuffer& buffer, VAddr htile_address,
                     const AmdGpu::Liverpool::CbDbExtent& hint) noexcept {
    is_tiled = false;
    pixel_format = LiverpoolToVK::DepthFormat(buffer.z_info.format, buffer.stencil_info.format);
    type = vk::ImageType::e2D;
    num_samples = 1 << buffer.z_info.num_samples; // spec doesn't say it is a log2
    size.width = hint.Valid() ? hint.width : buffer.Pitch();
    size.height = hint.Valid() ? hint.height : buffer.Height();
    size.depth = 1;
    pitch = size.width;
    guest_size_bytes = buffer.GetDepthSliceSize();
    meta_info.htile_addr = buffer.z_info.tile_surface_en ? htile_address : 0;
    usage.depth_target = true;
}

ImageInfo::ImageInfo(const AmdGpu::Image& image) noexcept {
    is_tiled = image.IsTiled();
    tiling_mode = image.GetTilingMode();
    pixel_format = LiverpoolToVK::SurfaceFormat(image.GetDataFmt(), image.GetNumberFmt());
    type = ConvertImageType(image.GetType());
    size.width = image.width + 1;
    size.height = image.height + 1;
    size.depth = 1;
    pitch = image.Pitch();
    resources.levels = image.NumLevels();
    resources.layers = image.NumLayers();
    guest_size_bytes = image.GetSize();
    usage.texture = true;
}

} // namespace VideoCore
