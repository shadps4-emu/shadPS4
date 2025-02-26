// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <ranges>
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

bool ImageInfo::HasStencil() const {
    if (pixel_format == vk::Format::eD32SfloatS8Uint ||
        pixel_format == vk::Format::eD24UnormS8Uint ||
        pixel_format == vk::Format::eD16UnormS8Uint) {
        return true;
    }
    return false;
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

static vk::FormatFeatureFlags2 FormatFeatureFlags(const vk::ImageUsageFlags usage_flags) {
    vk::FormatFeatureFlags2 feature_flags{};
    if (usage_flags & vk::ImageUsageFlagBits::eTransferSrc) {
        feature_flags |= vk::FormatFeatureFlagBits2::eTransferSrc;
    }
    if (usage_flags & vk::ImageUsageFlagBits::eTransferDst) {
        feature_flags |= vk::FormatFeatureFlagBits2::eTransferDst;
    }
    if (usage_flags & vk::ImageUsageFlagBits::eSampled) {
        feature_flags |= vk::FormatFeatureFlagBits2::eSampledImage;
    }
    if (usage_flags & vk::ImageUsageFlagBits::eColorAttachment) {
        feature_flags |= vk::FormatFeatureFlagBits2::eColorAttachment;
    }
    if (usage_flags & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
        feature_flags |= vk::FormatFeatureFlagBits2::eDepthStencilAttachment;
    }
    // Note: StorageImage is intentionally ignored for now since it is always set, and can mess up
    // compatibility checks.
    return feature_flags;
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
      image{instance->GetDevice(), instance->GetAllocator()} {
    if (info.pixel_format == vk::Format::eUndefined) {
        return;
    }
    mip_hashes.resize(info.resources.levels);
    // Here we force `eExtendedUsage` as don't know all image usage cases beforehand. In normal case
    // the texture cache should re-create the resource with the usage requested
    vk::ImageCreateFlags flags{vk::ImageCreateFlagBits::eMutableFormat |
                               vk::ImageCreateFlagBits::eExtendedUsage};
    if (info.props.is_volume) {
        flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
    }

    usage_flags = ImageUsageFlags(info);
    format_features = FormatFeatureFlags(usage_flags);

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

    constexpr auto tiling = vk::ImageTiling::eOptimal;
    const auto supported_format = instance->GetSupportedFormat(info.pixel_format, format_features);
    const auto properties = instance->GetPhysicalDevice().getImageFormatProperties(
        supported_format, info.type, tiling, usage_flags, flags);
    const auto supported_samples = properties.result == vk::Result::eSuccess
                                       ? properties.value.sampleCounts
                                       : vk::SampleCountFlagBits::e1;

    const vk::ImageCreateInfo image_ci = {
        .flags = flags,
        .imageType = info.type,
        .format = supported_format,
        .extent{
            .width = info.size.width,
            .height = info.size.height,
            .depth = info.size.depth,
        },
        .mipLevels = static_cast<u32>(info.resources.levels),
        .arrayLayers = static_cast<u32>(info.resources.layers),
        .samples = LiverpoolToVK::NumSamples(info.num_samples, supported_samples),
        .tiling = tiling,
        .usage = usage_flags,
        .initialLayout = vk::ImageLayout::eUndefined,
    };

    image.Create(image_ci);

    Vulkan::SetObjectName(instance->GetDevice(), (vk::Image)image, "Image {}x{}x{} {:#x}:{:#x}",
                          info.size.width, info.size.height, info.size.depth, info.guest_address,
                          info.guest_size);
}

boost::container::small_vector<vk::ImageMemoryBarrier2, 32> Image::GetBarriers(
    vk::ImageLayout dst_layout, vk::Flags<vk::AccessFlagBits2> dst_mask,
    vk::PipelineStageFlags2 dst_stage, std::optional<SubresourceRange> subres_range) {
    const bool needs_partial_transition =
        subres_range &&
        (subres_range->base != SubresourceBase{} || subres_range->extent != info.resources);
    const bool partially_transited = !subresource_states.empty();

    boost::container::small_vector<vk::ImageMemoryBarrier2, 32> barriers{};
    if (needs_partial_transition || partially_transited) {
        if (!partially_transited) {
            subresource_states.resize(info.resources.levels * info.resources.layers);
            std::fill(subresource_states.begin(), subresource_states.end(), last_state);
        }

        // In case of partial transition, we need to change the specified subresources only.
        // Otherwise all subresources need to be set to the same state so we can use a full
        // resource transition for the next time.
        const auto mips =
            needs_partial_transition
                ? std::ranges::views::iota(subres_range->base.level,
                                           subres_range->base.level + subres_range->extent.levels)
                : std::views::iota(0u, info.resources.levels);
        const auto layers =
            needs_partial_transition
                ? std::ranges::views::iota(subres_range->base.layer,
                                           subres_range->base.layer + subres_range->extent.layers)
                : std::views::iota(0u, info.resources.layers);

        for (u32 mip : mips) {
            for (u32 layer : layers) {
                // NOTE: these loops may produce a lot of small barriers.
                // If this becomes a problem, we can optimize it by merging adjacent barriers.
                const auto subres_idx = mip * info.resources.layers + layer;
                ASSERT(subres_idx < subresource_states.size());
                auto& state = subresource_states[subres_idx];

                if (state.layout != dst_layout || state.access_mask != dst_mask) {
                    barriers.emplace_back(vk::ImageMemoryBarrier2{
                        .srcStageMask = state.pl_stage,
                        .srcAccessMask = state.access_mask,
                        .dstStageMask = dst_stage,
                        .dstAccessMask = dst_mask,
                        .oldLayout = state.layout,
                        .newLayout = dst_layout,
                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .image = image,
                        .subresourceRange{
                            .aspectMask = aspect_mask,
                            .baseMipLevel = mip,
                            .levelCount = 1,
                            .baseArrayLayer = layer,
                            .layerCount = 1,
                        },
                    });
                    state.layout = dst_layout;
                    state.access_mask = dst_mask;
                    state.pl_stage = dst_stage;
                }
            }
        }

        if (!needs_partial_transition) {
            subresource_states.clear();
        }
    } else { // Full resource transition
        if (last_state.layout == dst_layout && last_state.access_mask == dst_mask) {
            return {};
        }

        barriers.emplace_back(vk::ImageMemoryBarrier2{
            .srcStageMask = last_state.pl_stage,
            .srcAccessMask = last_state.access_mask,
            .dstStageMask = dst_stage,
            .dstAccessMask = dst_mask,
            .oldLayout = last_state.layout,
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
        });
    }

    last_state.layout = dst_layout;
    last_state.access_mask = dst_mask;
    last_state.pl_stage = dst_stage;

    return barriers;
}

void Image::Transit(vk::ImageLayout dst_layout, vk::Flags<vk::AccessFlagBits2> dst_mask,
                    std::optional<SubresourceRange> range, vk::CommandBuffer cmdbuf /*= {}*/) {
    // Adjust pipieline stage
    const vk::PipelineStageFlags2 dst_pl_stage =
        (dst_mask == vk::AccessFlagBits2::eTransferRead ||
         dst_mask == vk::AccessFlagBits2::eTransferWrite)
            ? vk::PipelineStageFlagBits2::eTransfer
            : vk::PipelineStageFlagBits2::eAllGraphics | vk::PipelineStageFlagBits2::eComputeShader;

    const auto barriers = GetBarriers(dst_layout, dst_mask, dst_pl_stage, range);
    if (barriers.empty()) {
        return;
    }

    if (!cmdbuf) {
        // When using external cmdbuf you are responsible for ending rp.
        scheduler->EndRendering();
        cmdbuf = scheduler->CommandBuffer();
    }
    cmdbuf.pipelineBarrier2(vk::DependencyInfo{
        .imageMemoryBarrierCount = static_cast<u32>(barriers.size()),
        .pImageMemoryBarriers = barriers.data(),
    });
}

void Image::Upload(vk::Buffer buffer, u64 offset) {
    scheduler->EndRendering();
    Transit(vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits2::eTransferWrite, {});

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
            vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eTransferRead, {});
}

void Image::CopyImage(const Image& image) {
    scheduler->EndRendering();
    Transit(vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits2::eTransferWrite, {});

    auto cmdbuf = scheduler->CommandBuffer();

    boost::container::small_vector<vk::ImageCopy, 14> image_copy{};
    for (u32 m = 0; m < image.info.resources.levels; ++m) {
        const auto mip_w = std::max(info.size.width >> m, 1u);
        const auto mip_h = std::max(info.size.height >> m, 1u);
        const auto mip_d = std::max(info.size.depth >> m, 1u);

        image_copy.emplace_back(vk::ImageCopy{
            .srcSubresource{
                .aspectMask = image.aspect_mask,
                .mipLevel = m,
                .baseArrayLayer = 0,
                .layerCount = image.info.resources.layers,
            },
            .dstSubresource{
                .aspectMask = image.aspect_mask,
                .mipLevel = m,
                .baseArrayLayer = 0,
                .layerCount = image.info.resources.layers,
            },
            .extent = {mip_w, mip_h, mip_d},
        });
    }
    cmdbuf.copyImage(image.image, image.last_state.layout, this->image, this->last_state.layout,
                     image_copy);

    Transit(vk::ImageLayout::eGeneral,
            vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eTransferRead, {});
}

void Image::CopyMip(const Image& image, u32 mip, u32 slice) {
    scheduler->EndRendering();
    Transit(vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits2::eTransferWrite, {});

    auto cmdbuf = scheduler->CommandBuffer();

    const auto mip_w = std::max(info.size.width >> mip, 1u);
    const auto mip_h = std::max(info.size.height >> mip, 1u);
    const auto mip_d = std::max(info.size.depth >> mip, 1u);

    ASSERT(mip_w == image.info.size.width);
    ASSERT(mip_h == image.info.size.height);

    const u32 num_layers = std::min(image.info.resources.layers, info.resources.layers);
    const vk::ImageCopy image_copy{
        .srcSubresource{
            .aspectMask = image.aspect_mask,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = num_layers,
        },
        .dstSubresource{
            .aspectMask = image.aspect_mask,
            .mipLevel = mip,
            .baseArrayLayer = slice,
            .layerCount = num_layers,
        },
        .extent = {mip_w, mip_h, mip_d},
    };
    cmdbuf.copyImage(image.image, image.last_state.layout, this->image, this->last_state.layout,
                     image_copy);

    Transit(vk::ImageLayout::eGeneral,
            vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eTransferRead, {});
}

Image::~Image() = default;

} // namespace VideoCore
