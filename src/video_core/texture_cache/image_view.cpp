// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/texture_cache/image_view.h"

namespace VideoCore {

[[nodiscard]] vk::ImageViewType ConvertImageViewType(const ImageViewType type) {
    switch (type) {
    case ImageViewType::e1D:
        return vk::ImageViewType::e1D;
    case ImageViewType::e2D:
        return vk::ImageViewType::e2D;
    case ImageViewType::e3D:
        return vk::ImageViewType::e3D;
    case ImageViewType::Buffer:
        break;
    default:
        break;
    }
    UNREACHABLE_MSG("Invalid image type={}", static_cast<u32>(type));
    return {};
}

[[nodiscard]] vk::Format ConvertPixelFormat(const PixelFormat format) {
    switch (format) {
    default:
        break;
    }
    UNREACHABLE_MSG("Unknown format={}", static_cast<u32>(format));
    return {};
}

ImageView::ImageView(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
                     const ImageViewInfo& info_, vk::Image image)
    : info{info_} {
    const vk::ImageViewCreateInfo image_view_ci = {
        .image = image,
        .viewType = ConvertImageViewType(info.type),
        .format = ConvertPixelFormat(info.format),
        .components{
            .r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity,
        },
        .subresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0U,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
        },
    };
    image_view = instance.GetDevice().createImageViewUnique(image_view_ci);
}

ImageView::~ImageView() = default;

} // namespace VideoCore
