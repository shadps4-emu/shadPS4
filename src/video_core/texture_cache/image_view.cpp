// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/texture_cache/image_view.h"

namespace VideoCore {

ImageView::ImageView(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
                     const ImageViewInfo& info_, vk::Image image)
    : info{info_} {
    const vk::ImageViewCreateInfo image_view_ci = {
        .image = image,
        .viewType = info.type,
        .format = info.format,
        .components = info.mapping,
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
