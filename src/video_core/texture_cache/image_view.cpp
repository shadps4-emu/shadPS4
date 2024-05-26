// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/renderer_vulkan/liverpool_to_vk.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/texture_cache/image_view.h"

namespace VideoCore {

vk::ImageViewType ConvertImageViewType(AmdGpu::ImageType type) {
    switch (type) {
    case AmdGpu::ImageType::Color1D:
        return vk::ImageViewType::e1D;
    case AmdGpu::ImageType::Color1DArray:
        return vk::ImageViewType::e1DArray;
    case AmdGpu::ImageType::Color2D:
    case AmdGpu::ImageType::Cube:
        return vk::ImageViewType::e2D;
    case AmdGpu::ImageType::Color2DArray:
        return vk::ImageViewType::e2DArray;
    case AmdGpu::ImageType::Color3D:
        return vk::ImageViewType::e3D;
    default:
        UNREACHABLE();
    }
}

vk::ComponentSwizzle ConvertComponentSwizzle(u32 dst_sel) {
    switch (dst_sel) {
    case 0:
        return vk::ComponentSwizzle::eZero;
    case 1:
        return vk::ComponentSwizzle::eOne;
    case 4:
        return vk::ComponentSwizzle::eR;
    case 5:
        return vk::ComponentSwizzle::eG;
    case 6:
        return vk::ComponentSwizzle::eB;
    case 7:
        return vk::ComponentSwizzle::eA;
    default:
        UNREACHABLE();
    }
}

ImageViewInfo::ImageViewInfo(const AmdGpu::Image& image) noexcept {
    type = ConvertImageViewType(image.type);
    format = Vulkan::LiverpoolToVK::SurfaceFormat(image.GetDataFmt(), image.GetNumberFmt());
    range.base.level = image.base_level;
    range.base.layer = 0;
    range.extent.levels = 1;
    range.extent.layers = 1;
    mapping.r = ConvertComponentSwizzle(image.dst_sel_x);
    mapping.g = ConvertComponentSwizzle(image.dst_sel_y);
    mapping.b = ConvertComponentSwizzle(image.dst_sel_z);
    mapping.a = ConvertComponentSwizzle(image.dst_sel_w);
}

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
