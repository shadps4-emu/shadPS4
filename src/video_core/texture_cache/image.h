// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"
#include "core/libraries/videoout/buffer.h"
#include "video_core/renderer_vulkan/vk_common.h"
#include "video_core/texture_cache/types.h"

namespace Vulkan {
class Instance;
class Scheduler;
} // namespace Vulkan

VK_DEFINE_HANDLE(VmaAllocation)
VK_DEFINE_HANDLE(VmaAllocator)

namespace VideoCore {

enum ImageFlagBits : u32 {
    CpuModified = 1 << 2, ///< Contents have been modified from the CPU
    GpuModified = 1 << 3, ///< Contents have been modified from the GPU
    Tracked = 1 << 4,     ///< Writes and reads are being hooked from the CPU
    Registered = 1 << 6,  ///< True when the image is registered
    Picked = 1 << 7,      ///< Temporary flag to mark the image as picked
};
DECLARE_ENUM_FLAG_OPERATORS(ImageFlagBits)

struct ImageInfo {
    ImageInfo() = default;
    explicit ImageInfo(const Libraries::VideoOut::BufferAttributeGroup& group) noexcept;

    bool is_tiled = false;
    vk::Format pixel_format = vk::Format::eUndefined;
    vk::ImageType type = vk::ImageType::e1D;
    SubresourceExtent resources;
    Extent3D size{1, 1, 1};
    u32 pitch = 0;
    u32 guest_size_bytes = 0;
};

struct Handle {
    VmaAllocation allocation;
    VkImage image;

    Handle() = default;

    Handle(Handle&& other)
        : image{std::exchange(other.image, VK_NULL_HANDLE)},
          allocation{std::exchange(other.allocation, VK_NULL_HANDLE)} {}

    Handle& operator=(Handle&& other) {
        image = std::exchange(other.image, VK_NULL_HANDLE);
        allocation = std::exchange(other.allocation, VK_NULL_HANDLE);
        return *this;
    }
};

struct UniqueImage {
    explicit UniqueImage(vk::Device device, VmaAllocator allocator);
    ~UniqueImage();

    UniqueImage(const UniqueImage&) = delete;
    UniqueImage& operator=(const UniqueImage&) = delete;

    UniqueImage(UniqueImage&& other) : image{std::exchange(other.image, VK_NULL_HANDLE)} {}
    UniqueImage& operator=(UniqueImage&& other) {
        image = std::exchange(other.image, VK_NULL_HANDLE);
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

struct Image {
    explicit Image(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
                   const ImageInfo& info, VAddr cpu_addr);
    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    Image(Image&&) = default;
    Image& operator=(Image&&) = default;

    [[nodiscard]] bool Overlaps(VAddr overlap_cpu_addr, size_t overlap_size) const noexcept {
        const VAddr overlap_end = overlap_cpu_addr + overlap_size;
        return cpu_addr < overlap_end && overlap_cpu_addr < cpu_addr_end;
    }

    const Vulkan::Instance* instance;
    Vulkan::Scheduler* scheduler;
    ImageInfo info;
    UniqueImage image;
    vk::ImageAspectFlags aspect_mask;
    ImageFlagBits flags = ImageFlagBits::CpuModified;
    VAddr cpu_addr = 0;
    VAddr cpu_addr_end = 0;
};

} // namespace VideoCore
