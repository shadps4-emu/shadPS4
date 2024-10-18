// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/videoout/buffer.h"
#include "shader_recompiler/info.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/texture_cache/types.h"

#include <boost/container/small_vector.hpp>

namespace VideoCore {

struct ImageInfo {
    ImageInfo() = default;
    ImageInfo(const Libraries::VideoOut::BufferAttributeGroup& group, VAddr cpu_address) noexcept;
    ImageInfo(const AmdGpu::Liverpool::ColorBuffer& buffer,
              const AmdGpu::Liverpool::CbDbExtent& hint = {}) noexcept;
    ImageInfo(const AmdGpu::Liverpool::DepthBuffer& buffer, u32 num_slices, VAddr htile_address,
              const AmdGpu::Liverpool::CbDbExtent& hint = {}) noexcept;
    ImageInfo(const AmdGpu::Image& image, const Shader::ImageResource& desc) noexcept;

    bool IsTiled() const {
        return tiling_mode != AmdGpu::TilingMode::Display_Linear;
    }
    bool IsBlockCoded() const;
    bool IsPacked() const;
    bool IsDepthStencil() const;

    bool IsMipOf(const ImageInfo& info) const;
    bool IsSliceOf(const ImageInfo& info) const;

    /// Verifies if images are compatible for subresource merging.
    bool IsCompatible(const ImageInfo& info) const {
        return (pixel_format == info.pixel_format && tiling_idx == info.tiling_idx &&
                num_samples == info.num_samples && num_bits == info.num_bits);
    }

    void UpdateSize();

    struct {
        VAddr cmask_addr;
        VAddr fmask_addr;
        VAddr htile_addr;
    } meta_info{};

    struct {
        u32 texture : 1;
        u32 storage : 1;
        u32 render_target : 1;
        u32 depth_target : 1;
        u32 stencil : 1;
        u32 vo_buffer : 1;
    } usage{}; // Usage data tracked during image lifetime

    struct {
        u32 is_cube : 1;
        u32 is_volume : 1;
        u32 is_tiled : 1;
        u32 is_pow2 : 1;
        u32 is_block : 1;
    } props{}; // Surface properties with impact on various calculation factors

    vk::Format pixel_format = vk::Format::eUndefined;
    vk::ImageType type = vk::ImageType::e1D;
    SubresourceExtent resources;
    Extent3D size{1, 1, 1};
    u32 num_bits{};
    u32 num_samples = 1;
    u32 pitch = 0;
    AmdGpu::TilingMode tiling_mode{AmdGpu::TilingMode::Display_Linear};
    struct MipInfo {
        u32 size;
        u32 pitch;
        u32 height;
        u32 offset;
    };
    boost::container::small_vector<MipInfo, 14> mips_layout;
    VAddr guest_address{0};
    u32 guest_size_bytes{0};
    u32 tiling_idx{0}; // TODO: merge with existing!
};

} // namespace VideoCore
