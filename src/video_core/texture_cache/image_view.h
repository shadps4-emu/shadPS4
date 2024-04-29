// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "video_core/renderer_vulkan/vk_common.h"
#include "video_core/texture_cache/types.h"

namespace Vulkan {
class Instance;
class Scheduler;
} // namespace Vulkan

namespace VideoCore {

enum class ImageViewType : u32 {
    e1D,
    e2D,
    Cube,
    e3D,
    e1DArray,
    e2DArray,
    CubeArray,
    Buffer,
};

struct ImageViewInfo {
    vk::ImageViewType type{};
    vk::Format format{};
    SubresourceRange range;
    vk::ComponentMapping mapping{};

    auto operator<=>(const ImageViewInfo&) const = default;
};

struct ImageView {
    explicit ImageView(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
                       const ImageViewInfo& info, vk::Image image);
    ~ImageView();

    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;

    ImageView(ImageView&&) = default;
    ImageView& operator=(ImageView&&) = default;

    ImageId image_id{};
    Extent3D size{0, 0, 0};
    ImageViewInfo info{};
    vk::UniqueImageView image_view;
};

} // namespace VideoCore
