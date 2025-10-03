// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/info.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/amdgpu/resource.h"
#include "video_core/renderer_vulkan/vk_common.h"
#include "video_core/texture_cache/types.h"

namespace Vulkan {
class Instance;
class Scheduler;
} // namespace Vulkan

namespace VideoCore {

struct ImageViewInfo {
    ImageViewInfo() = default;
    ImageViewInfo(const AmdGpu::Image& image, const Shader::ImageResource& desc) noexcept;
    ImageViewInfo(const AmdGpu::Liverpool::ColorBuffer& col_buffer) noexcept;
    ImageViewInfo(const AmdGpu::Liverpool::DepthBuffer& depth_buffer,
                  AmdGpu::Liverpool::DepthView view, AmdGpu::Liverpool::DepthControl ctl);

    AmdGpu::ImageType type = AmdGpu::ImageType::Color2D;
    vk::Format format = vk::Format::eR8G8B8A8Unorm;
    SubresourceRange range;
    vk::ComponentMapping mapping{};
    bool is_storage = false;

    auto operator<=>(const ImageViewInfo&) const = default;
};

struct Image;

struct ImageView {
    ImageView(const Vulkan::Instance& instance, const ImageViewInfo& info, const Image& image);
    ~ImageView();

    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;

    ImageView(ImageView&&) = default;
    ImageView& operator=(ImageView&&) = default;

    ImageViewInfo info;
    vk::UniqueImageView image_view;
};

} // namespace VideoCore
