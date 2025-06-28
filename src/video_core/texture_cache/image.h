// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"
#include "video_core/renderer_vulkan/vk_common.h"
#include "video_core/texture_cache/image_info.h"
#include "video_core/texture_cache/image_view.h"

#include <optional>

namespace Vulkan {
class Instance;
class Scheduler;
} // namespace Vulkan

VK_DEFINE_HANDLE(VmaAllocation)
VK_DEFINE_HANDLE(VmaAllocator)

namespace VideoCore {

enum ImageFlagBits : u32 {
    Empty = 0,
    MaybeCpuDirty = 1 << 0, ///< The page this image is in was touched before the image address
    CpuDirty = 1 << 1,      ///< Contents have been modified from the CPU
    GpuDirty = 1 << 2, ///< Contents have been modified from the GPU (valid data in buffer cache)
    Dirty = MaybeCpuDirty | CpuDirty | GpuDirty,
    GpuModified = 1 << 3, ///< Contents have been modified from the GPU
    Registered = 1 << 6,  ///< True when the image is registered
    Picked = 1 << 7,      ///< Temporary flag to mark the image as picked
};
DECLARE_ENUM_FLAG_OPERATORS(ImageFlagBits)

struct UniqueImage {
    explicit UniqueImage();
    explicit UniqueImage(vk::Device device, VmaAllocator allocator);
    ~UniqueImage();

    UniqueImage(const UniqueImage&) = delete;
    UniqueImage& operator=(const UniqueImage&) = delete;

    UniqueImage(UniqueImage&& other)
        : allocator{std::exchange(other.allocator, VK_NULL_HANDLE)},
          allocation{std::exchange(other.allocation, VK_NULL_HANDLE)},
          image{std::exchange(other.image, VK_NULL_HANDLE)} {}
    UniqueImage& operator=(UniqueImage&& other) {
        image = std::exchange(other.image, VK_NULL_HANDLE);
        allocator = std::exchange(other.allocator, VK_NULL_HANDLE);
        allocation = std::exchange(other.allocation, VK_NULL_HANDLE);
        return *this;
    }

    void Create(const vk::ImageCreateInfo& image_ci);

    operator vk::Image() const {
        return image;
    }

private:
    vk::Device device;
    VmaAllocator allocator;
    VmaAllocation allocation;
    vk::Image image{};
};

constexpr Common::SlotId NULL_IMAGE_ID{0};

struct Image {
    Image(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler, const ImageInfo& info);
    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    Image(Image&&) = default;
    Image& operator=(Image&&) = default;

    [[nodiscard]] bool Overlaps(VAddr overlap_cpu_addr, size_t overlap_size) const noexcept {
        const VAddr overlap_end = overlap_cpu_addr + overlap_size;
        const auto image_addr = info.guest_address;
        const auto image_end = info.guest_address + info.guest_size;
        return image_addr < overlap_end && overlap_cpu_addr < image_end;
    }

    ImageViewId FindView(const ImageViewInfo& info) const {
        const auto it = std::ranges::find(image_view_infos, info);
        if (it == image_view_infos.end()) {
            return {};
        }
        return image_view_ids[std::distance(image_view_infos.begin(), it)];
    }

    void AssociateDepth(ImageId image_id) {
        depth_id = image_id;
    }

    boost::container::small_vector<vk::ImageMemoryBarrier2, 32> GetBarriers(
        vk::ImageLayout dst_layout, vk::Flags<vk::AccessFlagBits2> dst_mask,
        vk::PipelineStageFlags2 dst_stage, std::optional<SubresourceRange> subres_range);
    void Transit(vk::ImageLayout dst_layout, vk::Flags<vk::AccessFlagBits2> dst_mask,
                 std::optional<SubresourceRange> range, vk::CommandBuffer cmdbuf = {});
    void Upload(vk::Buffer buffer, u64 offset);

    void CopyImage(const Image& src_image);
    void CopyImageWithBuffer(Image& src_image, vk::Buffer buffer, u64 offset);
    void CopyMip(const Image& src_image, u32 mip, u32 slice);

    bool IsTracked() {
        return track_addr != 0 && track_addr_end != 0;
    }

    const Vulkan::Instance* instance;
    Vulkan::Scheduler* scheduler;
    ImageInfo info;
    UniqueImage image;
    vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eColor;
    ImageFlagBits flags = ImageFlagBits::Dirty;
    VAddr track_addr = 0;
    VAddr track_addr_end = 0;
    std::vector<ImageViewInfo> image_view_infos;
    std::vector<ImageViewId> image_view_ids;
    ImageId depth_id{};

    // Resource state tracking
    struct {
        u32 texture : 1;
        u32 storage : 1;
        u32 render_target : 1;
        u32 depth_target : 1;
        u32 stencil : 1;
        u32 vo_surface : 1;
    } usage{};
    vk::ImageUsageFlags usage_flags;
    vk::FormatFeatureFlags2 format_features;
    struct State {
        vk::Flags<vk::PipelineStageFlagBits2> pl_stage = vk::PipelineStageFlagBits2::eAllCommands;
        vk::Flags<vk::AccessFlagBits2> access_mask = vk::AccessFlagBits2::eNone;
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    };
    State last_state{};
    std::vector<State> subresource_states{};
    boost::container::small_vector<u64, 14> mip_hashes{};
    u64 tick_accessed_last{0};
    u64 hash{0};

    struct {
        union {
            struct {
                u32 is_bound : 1;      // the image is bound to a descriptor set
                u32 is_target : 1;     // the image is bound as color/depth target
                u32 needs_rebind : 1;  // the image needs to be rebound
                u32 force_general : 1; // the image needs to be used in general layout
            };
            u32 raw{};
        };

        void Reset() {
            raw = 0u;
        }
    } binding{};
};

} // namespace VideoCore
