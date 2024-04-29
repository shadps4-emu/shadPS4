// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "video_core/pixel_format.h"
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

enum class SwizzleSource : u32 {
    Zero = 0,
    One = 1,
    R = 2,
    G = 3,
    B = 4,
    A = 5,
};

struct ImageViewInfo {
    ImageViewType type{};
    PixelFormat format{};
    SubresourceRange range;
    u8 x_source = static_cast<u8>(SwizzleSource::R);
    u8 y_source = static_cast<u8>(SwizzleSource::G);
    u8 z_source = static_cast<u8>(SwizzleSource::B);
    u8 w_source = static_cast<u8>(SwizzleSource::A);
};

class ImageView {
    explicit ImageView(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
                       const ImageViewInfo& info, vk::Image image);
    ~ImageView();

    ImageId image_id{};
    Extent3D size{0, 0, 0};
    ImageViewInfo info{};
    vk::UniqueImageView image_view;
};

} // namespace VideoCore
