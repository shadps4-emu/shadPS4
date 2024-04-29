// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/config.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/image.h"

#include <vk_mem_alloc.h>

namespace VideoCore {

using namespace Vulkan;
using VideoOutFormat = Libraries::VideoOut::PixelFormat;
using Libraries::VideoOut::TilingMode;

[[nodiscard]] vk::Format ConvertPixelFormat(const VideoOutFormat format) {
    switch (format) {
    case VideoOutFormat::A8R8G8B8Srgb:
        return vk::Format::eB8G8R8A8Srgb;
    case VideoOutFormat::A8B8G8R8Srgb:
        return vk::Format::eA8B8G8R8SrgbPack32;
    case VideoOutFormat::A2R10G10B10:
    case VideoOutFormat::A2R10G10B10Srgb:
        return vk::Format::eA2R10G10B10UnormPack32;
    default:
        break;
    }
    UNREACHABLE_MSG("Unknown format={}", static_cast<u32>(format));
    return {};
}

[[nodiscard]] vk::ImageUsageFlags ImageUsageFlags(const vk::Format format) {
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eTransferSrc |
                                vk::ImageUsageFlagBits::eTransferDst |
                                vk::ImageUsageFlagBits::eSampled;
    if (false /*&& IsDepthStencilFormat(format)*/) {
        usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
    } else {
        // usage |= vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage;
    }
    return usage;
}

ImageInfo::ImageInfo(const Libraries::VideoOut::BufferAttributeGroup& group) noexcept {
    const auto& attrib = group.attrib;
    is_tiled = attrib.tiling_mode == TilingMode::Tile;
    pixel_format = ConvertPixelFormat(attrib.pixel_format);
    type = vk::ImageType::e2D;
    size.width = attrib.width;
    size.height = attrib.height;
    pitch = attrib.tiling_mode == TilingMode::Linear ? size.width : (size.width + 127) >> 7;
    const bool is_32bpp = pixel_format == vk::Format::eB8G8R8A8Srgb ||
                          pixel_format == vk::Format::eA8B8G8R8SrgbPack32;
    ASSERT(is_32bpp);
    if (!is_tiled) {
        guest_size_bytes = pitch * size.height * 4;
        return;
    }
    if (Config::isNeoMode()) {
        guest_size_bytes = pitch * 128 * ((size.height + 127) & (~127)) * 4;
    } else {
        guest_size_bytes = pitch * 128 * ((size.height + 63) & (~63)) * 4;
    }
}

UniqueImage::UniqueImage(vk::Device device_, VmaAllocator allocator_)
    : device{device_}, allocator{allocator_} {}

UniqueImage::~UniqueImage() {
    if (image) {
        vmaDestroyImage(allocator, image, allocation);
    }
}

void UniqueImage::Create(const vk::ImageCreateInfo& image_ci) {
    const VmaAllocationCreateInfo alloc_info = {
        .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .requiredFlags = 0,
        .preferredFlags = 0,
        .pool = VK_NULL_HANDLE,
        .pUserData = nullptr,
    };

    const VkImageCreateInfo image_ci_unsafe = static_cast<VkImageCreateInfo>(image_ci);
    VkImage unsafe_image{};
    VkResult result = vmaCreateImage(allocator, &image_ci_unsafe, &alloc_info, &unsafe_image,
                                     &allocation, nullptr);
    ASSERT_MSG(result == VK_SUCCESS, "Failed allocating image with error {}",
               vk::to_string(vk::Result{result}));
    image = vk::Image{unsafe_image};
}

Image::Image(const Vulkan::Instance& instance_, Vulkan::Scheduler& scheduler_,
             const ImageInfo& info_, VAddr cpu_addr)
    : instance{&instance_}, scheduler{&scheduler_}, info{info_},
      image{instance->GetDevice(), instance->GetAllocator()}, cpu_addr{cpu_addr},
      cpu_addr_end{cpu_addr + info.guest_size_bytes} {
    vk::ImageCreateFlags flags{};
    if (info.type == vk::ImageType::e2D && info.resources.layers >= 6 &&
        info.size.width == info.size.height) {
        flags |= vk::ImageCreateFlagBits::eCubeCompatible;
    }
    if (info.type == vk::ImageType::e3D) {
        flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
    }
    const vk::ImageCreateInfo image_ci = {
        .flags = flags,
        .imageType = info.type,
        .format = info.pixel_format,
        .extent{
            .width = info.size.width,
            .height = info.size.height,
            .depth = info.size.depth,
        },
        .mipLevels = static_cast<u32>(info.resources.levels),
        .arrayLayers = static_cast<u32>(info.resources.layers),
        .tiling = vk::ImageTiling::eOptimal,
        .usage = ImageUsageFlags(info.pixel_format),
        .initialLayout = vk::ImageLayout::eUndefined,
    };

    image.Create(image_ci);

    const vk::ImageMemoryBarrier init_barrier = {
        .srcAccessMask = vk::AccessFlagBits::eNone,
        .dstAccessMask = vk::AccessFlagBits::eNone,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eGeneral,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
        },
    };

    const auto cmdbuf = scheduler->CommandBuffer();
    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                           vk::PipelineStageFlagBits::eTopOfPipe, vk::DependencyFlagBits::eByRegion,
                           {}, {}, init_barrier);
}

Image::~Image() = default;

} // namespace VideoCore
