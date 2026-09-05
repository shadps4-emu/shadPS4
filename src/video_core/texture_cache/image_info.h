// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "video_core/amdgpu/cb_db_extent.h"
#include "video_core/amdgpu/resource.h"
#include "video_core/amdgpu/tiling.h"
#include "video_core/renderer_vulkan/vk_common.h"
#include "video_core/texture_cache/types.h"

namespace AmdGpu {
struct ColorBuffer;
struct DepthBuffer;
} // namespace AmdGpu

namespace Libraries::VideoOut {
struct BufferAttributeGroup;
}

namespace Shader {
struct ImageResource;
}

namespace VideoCore {

struct ImageProperties {
    u32 is_volume : 1;
    u32 is_tiled : 1;
    u32 is_pow2 : 1;
    u32 is_block : 1;
    u32 is_depth : 1;
    u32 has_stencil : 1;
};

struct ImageInfo {
    ImageInfo() = default;
    ImageInfo(const Libraries::VideoOut::BufferAttributeGroup& group, VAddr cpu_address) noexcept;
    ImageInfo(const AmdGpu::ColorBuffer& buffer, AmdGpu::CbDbExtent hint) noexcept;
    ImageInfo(const AmdGpu::DepthBuffer& buffer, u32 num_slices, VAddr htile_address,
              AmdGpu::CbDbExtent hint, bool write_buffer = false) noexcept;
    ImageInfo(const AmdGpu::Image& image, const Shader::ImageResource& desc) noexcept;

    bool IsTiled() const {
        return tile_mode != AmdGpu::TileMode::DisplayLinearAligned;
    }

    Extent2D BlockDim() const {
        const auto dim = props.is_block ? 2 : 0;
        return Extent2D{pitch >> dim, size.height >> dim};
    }

    s32 MipOf(const ImageInfo& info) const;
    s32 SliceOf(const ImageInfo& info, s32 mip) const;

    bool IsCompatible(const ImageInfo& info) const;

    /// Whether this single-level color image is an origin-aligned crop of the same layout.
    bool IsSubrectOf(const ImageInfo& info) const {
        // A Vulkan image view cannot restrict the dimensions of a mip. Keep a separate
        // image for a cropped descriptor, but only copy texels when the guest layouts
        // really agree. Array slices and mip chains need separate offset calculations.
        return guest_address == info.guest_address && type == AmdGpu::ImageType::Color2D &&
               type == info.type && resources.levels == 1 && resources.layers == 1 &&
               info.resources.levels == 1 && info.resources.layers == 1 && !props.is_depth &&
               !info.props.is_depth && !props.is_block && !info.props.is_block &&
               pixel_format == info.pixel_format && num_bits == info.num_bits &&
               num_samples == info.num_samples && pitch == info.pitch &&
               tile_mode == info.tile_mode && bank_swizzle == info.bank_swizzle &&
               alt_tile == info.alt_tile && mips_layout[0].pitch == info.mips_layout[0].pitch &&
               (size.width < info.size.width || size.height < info.size.height) &&
               size.width <= info.size.width && size.height <= info.size.height &&
               size.depth == 1 && info.size.depth == 1;
    }
    void UpdateSize();

    struct {
        VAddr cmask_addr;
        VAddr fmask_addr;
        VAddr htile_addr;
        s32 htile_clear_mask = -1;
    } meta_info{};

    ImageProperties props{};
    vk::Format pixel_format = vk::Format::eUndefined;
    AmdGpu::ImageType type;
    SubresourceExtent resources;
    Extent3D size{1, 1, 1};
    u32 num_bits{};
    u32 num_samples = 1;
    u32 pitch{};
    AmdGpu::TileMode tile_mode = AmdGpu::TileMode::DisplayLinearAligned;
    AmdGpu::ArrayMode array_mode = AmdGpu::ArrayMode::ArrayLinearAligned;
    struct MipInfo {
        u32 size;
        u32 pitch;
        u32 height;
        u32 offset;
    };
    std::array<MipInfo, 16> mips_layout;
    VAddr guest_address{};
    u32 guest_size{};
    u8 bank_swizzle{};
    bool alt_tile{};

    VAddr stencil_addr{};
    u32 stencil_size{};
};

} // namespace VideoCore
