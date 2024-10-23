// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "shader_recompiler/info.h"
#include "video_core/amdgpu/resource.h"
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
    case AmdGpu::ImageType::Color2DMsaa:
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

bool IsIdentityMapping(u32 dst_sel, u32 num_components) {
    return (num_components == 1 && dst_sel == 0b001'000'000'100) ||
           (num_components == 2 && dst_sel == 0b001'000'101'100) ||
           (num_components == 3 && dst_sel == 0b001'110'101'100) ||
           (num_components == 4 && dst_sel == 0b111'110'101'100);
}

vk::Format TrySwizzleFormat(vk::Format format, u32 dst_sel) {
    if (format == vk::Format::eR8G8B8A8Unorm && dst_sel == 0b111100101110) {
        return vk::Format::eB8G8R8A8Unorm;
    }
    if (format == vk::Format::eR8G8B8A8Srgb && dst_sel == 0b111100101110) {
        return vk::Format::eB8G8R8A8Srgb;
    }
    return format;
}

ImageViewInfo::ImageViewInfo(const AmdGpu::Image& image, const Shader::ImageResource& desc) noexcept
    : is_storage{desc.is_storage} {
    const auto dfmt = image.GetDataFmt();
    auto nfmt = image.GetNumberFmt();
    if (is_storage && nfmt == AmdGpu::NumberFormat::Srgb) {
        nfmt = AmdGpu::NumberFormat::Unorm;
    }
    format = Vulkan::LiverpoolToVK::SurfaceFormat(dfmt, nfmt);
    if (desc.is_depth) {
        format = Vulkan::LiverpoolToVK::PromoteFormatToDepth(format);
    }
    range.base.level = image.base_level;
    range.base.layer = image.base_array;
    if (image.GetType() == AmdGpu::ImageType::Color2DMsaa ||
        image.GetType() == AmdGpu::ImageType::Color2DMsaaArray) {
        range.extent.levels = 1;
    } else {
        range.extent.levels = image.last_level - image.base_level + 1;
    }
    range.extent.layers = image.last_array - image.base_array + 1;
    type = ConvertImageViewType(image.GetType());

    // Adjust view type for partial cubemaps and arrays
    if (image.IsPartialCubemap()) {
        type = vk::ImageViewType::e2DArray;
    }
    if (type == vk::ImageViewType::eCube) {
        if (desc.is_array) {
            type = vk::ImageViewType::eCubeArray;
        } else {
            // Some games try to bind an array of cubemaps while shader reads only single one.
            range.extent.layers = std::min(range.extent.layers, 6u);
        }
    }
    if (type == vk::ImageViewType::e3D && range.extent.layers > 1) {
        // Some games pass incorrect layer count for 3D textures so we need to fixup it.
        range.extent.layers = 1;
    }

    if (!is_storage) {
        mapping.r = ConvertComponentSwizzle(image.dst_sel_x);
        mapping.g = ConvertComponentSwizzle(image.dst_sel_y);
        mapping.b = ConvertComponentSwizzle(image.dst_sel_z);
        mapping.a = ConvertComponentSwizzle(image.dst_sel_w);
    }
    // Check for unfortunate case of storage images being swizzled
    const u32 num_comps = AmdGpu::NumComponents(image.GetDataFmt());
    const u32 dst_sel = image.DstSelect();
    if (is_storage && !IsIdentityMapping(dst_sel, num_comps)) {
        if (auto new_format = TrySwizzleFormat(format, dst_sel); new_format != format) {
            format = new_format;
            return;
        }
        LOG_ERROR(Render_Vulkan, "Storage image (num_comps = {}) requires swizzling {}", num_comps,
                  image.DstSelectName());
    }
}

ImageViewInfo::ImageViewInfo(const AmdGpu::Liverpool::ColorBuffer& col_buffer,
                             bool is_vo_surface) noexcept {
    const auto base_format =
        Vulkan::LiverpoolToVK::SurfaceFormat(col_buffer.info.format, col_buffer.NumFormat());
    range.base.layer = col_buffer.view.slice_start;
    range.extent.layers = col_buffer.NumSlices() - range.base.layer;
    format = Vulkan::LiverpoolToVK::AdjustColorBufferFormat(
        base_format, col_buffer.info.comp_swap.Value(), is_vo_surface);
}

ImageViewInfo::ImageViewInfo(const AmdGpu::Liverpool::DepthBuffer& depth_buffer,
                             AmdGpu::Liverpool::DepthView view,
                             AmdGpu::Liverpool::DepthControl ctl) {
    format = Vulkan::LiverpoolToVK::DepthFormat(depth_buffer.z_info.format,
                                                depth_buffer.stencil_info.format);
    is_storage = ctl.depth_write_enable;
    range.base.layer = view.slice_start;
    range.extent.layers = view.NumSlices() - range.base.layer;
}

ImageView::ImageView(const Vulkan::Instance& instance, const ImageViewInfo& info_, Image& image,
                     ImageId image_id_)
    : image_id{image_id_}, info{info_} {
    vk::ImageViewUsageCreateInfo usage_ci{.usage = image.usage};
    if (!info.is_storage) {
        usage_ci.usage &= ~vk::ImageUsageFlagBits::eStorage;
    }
    // When sampling D32 texture from shader, the T# specifies R32 Float format so adjust it.
    vk::Format format = info.format;
    vk::ImageAspectFlags aspect = image.aspect_mask;
    if (image.aspect_mask & vk::ImageAspectFlagBits::eDepth &&
        (format == vk::Format::eR32Sfloat || format == vk::Format::eD32Sfloat)) {
        format = image.info.pixel_format;
        aspect = vk::ImageAspectFlagBits::eDepth;
    }
    if (image.aspect_mask & vk::ImageAspectFlagBits::eStencil && format == vk::Format::eR8Unorm) {
        format = image.info.pixel_format;
        aspect = vk::ImageAspectFlagBits::eStencil;
    }

    const vk::ImageViewCreateInfo image_view_ci = {
        .pNext = &usage_ci,
        .image = image.image,
        .viewType = info.type,
        .format = instance.GetSupportedFormat(format, image.format_features),
        .components =
            instance.GetSupportedComponentSwizzle(format, info.mapping, image.format_features),
        .subresourceRange{
            .aspectMask = aspect,
            .baseMipLevel = info.range.base.level,
            .levelCount = info.range.extent.levels,
            .baseArrayLayer = info.range.base.layer,
            .layerCount = info.range.extent.layers,
        },
    };
    auto [view_result, view] = instance.GetDevice().createImageViewUnique(image_view_ci);
    ASSERT_MSG(view_result == vk::Result::eSuccess, "Failed to create image view: {}",
               vk::to_string(view_result));
    image_view = std::move(view);
}

ImageView::~ImageView() = default;

} // namespace VideoCore
