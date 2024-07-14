// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/renderer_vulkan/liverpool_to_vk.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/texture_cache/image.h"
#include "video_core/texture_cache/image_view.h"

namespace VideoCore {

vk::ImageViewType ConvertImageViewType(AmdGpu::ImageType type) {
    switch (type) {
    case AmdGpu::ImageType::Color1D:
        return vk::ImageViewType::e1D;
    case AmdGpu::ImageType::Color1DArray:
        return vk::ImageViewType::e1DArray;
    case AmdGpu::ImageType::Color2D:
        return vk::ImageViewType::e2D;
    case AmdGpu::ImageType::Cube:
        return vk::ImageViewType::eCube;
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

ImageViewInfo::ImageViewInfo(const AmdGpu::Image& image, bool is_storage) noexcept
    : is_storage{is_storage} {
    type = ConvertImageViewType(image.GetType());
    format = Vulkan::LiverpoolToVK::SurfaceFormat(image.GetDataFmt(), image.GetNumberFmt());
    range.base.level = static_cast<u32>(image.base_level);
    range.base.layer = static_cast<u32>(image.base_array);
    range.extent.levels = image.NumLevels();
    range.extent.layers = image.NumLayers();
    if (!is_storage) {
        mapping.r = ConvertComponentSwizzle(image.dst_sel_x);
        mapping.g = ConvertComponentSwizzle(image.dst_sel_y);
        mapping.b = ConvertComponentSwizzle(image.dst_sel_z);
        mapping.a = ConvertComponentSwizzle(image.dst_sel_w);
    }
}

ImageViewInfo::ImageViewInfo(const AmdGpu::Liverpool::ColorBuffer& col_buffer,
                             bool is_vo_surface) noexcept {
    const auto base_format =
        Vulkan::LiverpoolToVK::SurfaceFormat(col_buffer.info.format, col_buffer.NumFormat());
    format = Vulkan::LiverpoolToVK::AdjustColorBufferFormat(
        base_format, col_buffer.info.comp_swap.Value(), is_vo_surface);
}

ImageView::ImageView(const Vulkan::Instance& instance, const ImageViewInfo& info_, Image& image,
                     ImageId image_id_, std::optional<vk::ImageUsageFlags> usage_override /*= {}*/)
    : info{info_}, image_id{image_id_} {
    vk::ImageViewUsageCreateInfo usage_ci{};
    if (usage_override) {
        usage_ci.usage = usage_override.value();
    }
    // When sampling D32 texture from shader, the T# specifies R32 Float format so adjust it.
    vk::Format format = info.format;
    vk::ImageAspectFlags aspect = image.aspect_mask;
    if (image.aspect_mask & vk::ImageAspectFlagBits::eDepth && format == vk::Format::eR32Sfloat) {
        format = image.info.pixel_format;
        aspect = vk::ImageAspectFlagBits::eDepth;
    }

    const vk::ImageViewCreateInfo image_view_ci = {
        .pNext = usage_override ? &usage_ci : nullptr,
        .image = image.image,
        .viewType = info.type,
        .format = instance.GetSupportedFormat(format),
        .components = instance.GetSupportedComponentSwizzle(format, info.mapping),
        .subresourceRange{
            .aspectMask = aspect,
            .baseMipLevel = 0U,
            .levelCount = 1,
            .baseArrayLayer = info_.range.base.layer,
            .layerCount = image.info.IsBlockCoded() ? 1 : VK_REMAINING_ARRAY_LAYERS,
        },
    };
    image_view = instance.GetDevice().createImageViewUnique(image_view_ci);
}

ImageView::~ImageView() = default;

} // namespace VideoCore
