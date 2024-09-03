// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "video_core/renderer_vulkan/liverpool_to_vk.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/image.h"

#include <vk_mem_alloc.h>

namespace VideoCore {

using namespace Vulkan;

bool ImageInfo::IsBlockCoded() const {
    switch (pixel_format) {
    case vk::Format::eBc1RgbaSrgbBlock:
    case vk::Format::eBc1RgbaUnormBlock:
    case vk::Format::eBc1RgbSrgbBlock:
    case vk::Format::eBc1RgbUnormBlock:
    case vk::Format::eBc2SrgbBlock:
    case vk::Format::eBc2UnormBlock:
    case vk::Format::eBc3SrgbBlock:
    case vk::Format::eBc3UnormBlock:
    case vk::Format::eBc4SnormBlock:
    case vk::Format::eBc4UnormBlock:
    case vk::Format::eBc5SnormBlock:
    case vk::Format::eBc5UnormBlock:
    case vk::Format::eBc6HSfloatBlock:
    case vk::Format::eBc6HUfloatBlock:
    case vk::Format::eBc7SrgbBlock:
    case vk::Format::eBc7UnormBlock:
        return true;
    default:
        return false;
    }
}

bool ImageInfo::IsPacked() const {
    switch (pixel_format) {
    case vk::Format::eB5G5R5A1UnormPack16:
        [[fallthrough]];
    case vk::Format::eB5G6R5UnormPack16:
        return true;
    default:
        return false;
    }
}

bool ImageInfo::IsDepthStencil() const {
    switch (pixel_format) {
    case vk::Format::eD16Unorm:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD32Sfloat:
    case vk::Format::eD32SfloatS8Uint:
        return true;
    default:
        return false;
    }
}

static vk::ImageUsageFlags ImageUsageFlags(const ImageInfo& info) {
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eTransferSrc |
                                vk::ImageUsageFlagBits::eTransferDst |
                                vk::ImageUsageFlagBits::eSampled;
    if (info.IsDepthStencil()) {
        usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
    } else {
        if (!info.IsBlockCoded() && !info.IsPacked()) {
            usage |= vk::ImageUsageFlagBits::eColorAttachment;
        }

        // In cases where an image is created as a render/depth target and cleared with compute,
        // we cannot predict whether it will be used as a storage image. A proper solution would
        // involve re-creating the resource with a new configuration and copying previous content
        // into it. However, for now, we will set storage usage for all images (if the format
        // allows), sacrificing a bit of performance. Note use of ExtendedUsage flag set by default.
        usage |= vk::ImageUsageFlagBits::eStorage;
    }

    return usage;
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
             const ImageInfo& info_)
    : instance{&instance_}, scheduler{&scheduler_}, info{info_},
      image{instance->GetDevice(), instance->GetAllocator()}, cpu_addr{info.guest_address},
      cpu_addr_end{cpu_addr + info.guest_size_bytes} {
    mip_hashes.resize(info.resources.levels);
    ASSERT(info.pixel_format != vk::Format::eUndefined);
    // Here we force `eExtendedUsage` as don't know all image usage cases beforehand. In normal case
    // the texture cache should re-create the resource with the usage requested
    vk::ImageCreateFlags flags{vk::ImageCreateFlagBits::eMutableFormat |
                               vk::ImageCreateFlagBits::eExtendedUsage};
    if (info.props.is_cube) {
        flags |= vk::ImageCreateFlagBits::eCubeCompatible;
    } else if (info.props.is_volume) {
        flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
    }

    usage = ImageUsageFlags(info);

    switch (info.pixel_format) {
    case vk::Format::eD16Unorm:
    case vk::Format::eD32Sfloat:
    case vk::Format::eX8D24UnormPack32:
        aspect_mask = vk::ImageAspectFlagBits::eDepth;
        break;
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
        aspect_mask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        break;
    default:
        break;
    }

    const vk::ImageCreateInfo image_ci = {
        .flags = flags,
        .imageType = info.type,
        .format = instance->GetSupportedFormat(info.pixel_format),
        .extent{
            .width = info.size.width,
            .height = info.size.height,
            .depth = info.size.depth,
        },
        .mipLevels = static_cast<u32>(info.resources.levels),
        .arrayLayers = static_cast<u32>(info.resources.layers),
        .samples = LiverpoolToVK::NumSamples(info.num_samples),
        .tiling = vk::ImageTiling::eOptimal,
        .usage = usage,
        .initialLayout = vk::ImageLayout::eUndefined,
    };

    image.Create(image_ci);

    Vulkan::SetObjectName(instance->GetDevice(), (vk::Image)image, "Image {:#x}:{:#x}",
                          info.guest_address, info.guest_size_bytes);
}

void Image::Transit(vk::ImageLayout dst_layout, vk::Flags<vk::AccessFlagBits> dst_mask,
                    vk::CommandBuffer cmdbuf) {
    if (dst_layout == layout && dst_mask == access_mask) {
        return;
    }

    const vk::ImageMemoryBarrier barrier = {
        .srcAccessMask = access_mask,
        .dstAccessMask = dst_mask,
        .oldLayout = layout,
        .newLayout = dst_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange{
            .aspectMask = aspect_mask,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
        },
    };

    // Adjust pipieline stage
    const vk::PipelineStageFlags dst_pl_stage =
        (dst_mask == vk::AccessFlagBits::eTransferRead ||
         dst_mask == vk::AccessFlagBits::eTransferWrite)
            ? vk::PipelineStageFlagBits::eTransfer
            : vk::PipelineStageFlagBits::eAllGraphics | vk::PipelineStageFlagBits::eComputeShader;

    if (!cmdbuf) {
        // When using external cmdbuf you are responsible for ending rp.
        scheduler->EndRendering();
        cmdbuf = scheduler->CommandBuffer();
    }
    cmdbuf.pipelineBarrier(pl_stage, dst_pl_stage, vk::DependencyFlagBits::eByRegion, {}, {},
                           barrier);

    layout = dst_layout;
    access_mask = dst_mask;
    pl_stage = dst_pl_stage;
}

void Image::Upload(vk::Buffer buffer, u64 offset) {
    scheduler->EndRendering();
    Transit(vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eTransferWrite);

    // Copy to the image.
    const auto aspect = aspect_mask & vk::ImageAspectFlagBits::eStencil
                            ? vk::ImageAspectFlagBits::eDepth
                            : aspect_mask;
    const vk::BufferImageCopy image_copy = {
        .bufferOffset = offset,
        .bufferRowLength = info.pitch,
        .bufferImageHeight = info.size.height,
        .imageSubresource{
            .aspectMask = aspect,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {info.size.width, info.size.height, 1},
    };

    const auto cmdbuf = scheduler->CommandBuffer();
    cmdbuf.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, image_copy);

    Transit(vk::ImageLayout::eGeneral,
            vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eTransferRead);
}

Image::~Image() = default;

} // namespace VideoCore
