// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"
#include "core/libraries/videoout/buffer.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/texture_cache/types.h"

namespace VideoCore {

struct ImageInfo {
    ImageInfo() = default;
    ImageInfo(const Libraries::VideoOut::BufferAttributeGroup& group, VAddr cpu_address) noexcept;
    ImageInfo(const AmdGpu::Liverpool::ColorBuffer& buffer,
              const AmdGpu::Liverpool::CbDbExtent& hint = {}) noexcept;
    ImageInfo(const AmdGpu::Liverpool::DepthBuffer& buffer, u32 num_slices, VAddr htile_address,
              const AmdGpu::Liverpool::CbDbExtent& hint = {}) noexcept;
    ImageInfo(const AmdGpu::Image& image) noexcept;

    bool IsTiled() const {
        return tiling_mode != AmdGpu::TilingMode::Display_Linear;
    }
    bool IsBlockCoded() const;
    bool IsPacked() const;
    bool IsDepthStencil() const;

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

    bool is_cube = false;
    bool is_volume = false;
    bool is_tiled = false;
    bool is_read_only = false;
    vk::Format pixel_format = vk::Format::eUndefined;
    vk::ImageType type = vk::ImageType::e1D;
    SubresourceExtent resources;
    Extent3D size{1, 1, 1};
    u32 num_samples = 1;
    u32 pitch = 0;
    AmdGpu::TilingMode tiling_mode{AmdGpu::TilingMode::Display_Linear};
    std::vector<std::pair<u32, u32>> mips_layout;
    VAddr guest_address{0};
    u32 guest_size_bytes{0};
};

} // namespace VideoCore
