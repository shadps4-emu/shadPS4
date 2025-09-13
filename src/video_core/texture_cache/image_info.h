// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/container/static_vector.hpp>

#include "common/types.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/renderer_vulkan/vk_common.h"
#include "video_core/texture_cache/types.h"

namespace AmdGpu {
enum class ImageType : u64;
}

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
    ImageInfo(const AmdGpu::Liverpool::ColorBuffer& buffer,
              const AmdGpu::Liverpool::CbDbExtent& hint = {}) noexcept;
    ImageInfo(const AmdGpu::Liverpool::DepthBuffer& buffer, u32 num_slices, VAddr htile_address,
              const AmdGpu::Liverpool::CbDbExtent& hint = {}, bool write_buffer = false) noexcept;
    ImageInfo(const AmdGpu::Image& image, const Shader::ImageResource& desc) noexcept;

    bool IsTiled() const {
        return tile_mode != AmdGpu::TileMode::DisplayLinearAligned;
    }
    Extent2D BlockDim() const {
        const auto dim = props.is_block ? 2 : 0;
        return Extent2D{size.width >> dim, size.height >> dim};
    }

    s32 MipOf(const ImageInfo& info) const;
    s32 SliceOf(const ImageInfo& info, s32 mip) const;

    bool IsCompatible(const ImageInfo& info) const;
    void UpdateSize();

    struct {
        VAddr cmask_addr;
        VAddr fmask_addr;
        VAddr htile_addr;
        u32 htile_clear_mask = u32(-1);
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
    boost::container::static_vector<MipInfo, 16> mips_layout;
    VAddr guest_address{};
    u32 guest_size{};
    u8 bank_swizzle{};
    bool alt_tile{};

    VAddr stencil_addr{};
    u32 stencil_size{};
};

} // namespace VideoCore
