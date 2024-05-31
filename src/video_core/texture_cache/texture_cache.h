// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/container/small_vector.hpp>
#include <boost/icl/interval_map.hpp>
#include <tsl/robin_map.h>

#include "video_core/amdgpu/resource.h"
#include "video_core/renderer_vulkan/vk_stream_buffer.h"
#include "video_core/texture_cache/image.h"
#include "video_core/texture_cache/image_view.h"
#include "video_core/texture_cache/sampler.h"
#include "video_core/texture_cache/slot_vector.h"

namespace Core::Libraries::VideoOut {
struct BufferAttributeGroup;
}

namespace VideoCore {

class TextureCache {
    // This is the page shift for adding images into the hash map. It isn't related to
    // the page size of the guest or the host and is chosen for convenience. A number too
    // small will increase the number of hash map lookups per image, while too large will
    // increase the number of images per page.
    static constexpr u64 PageBits = 20;
    static constexpr u64 PageMask = (1ULL << PageBits) - 1;

public:
    explicit TextureCache(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler);
    ~TextureCache();

    /// Invalidates any image in the logical page range.
    void OnCpuWrite(VAddr address);

    /// Retrieves the image handle of the image with the provided attributes and address.
    Image& FindImage(const ImageInfo& info, VAddr cpu_address);

    /// Retrieves an image view with the properties of the specified image descriptor.
    ImageView& FindImageView(const AmdGpu::Image& image);

    /// Retrieves the render target with specified properties
    ImageView& RenderTarget(const AmdGpu::Liverpool::ColorBuffer& buffer);

    /// Reuploads image contents.
    void RefreshImage(Image& image);

    /// Retrieves the sampler that matches the provided S# descriptor.
    vk::Sampler GetSampler(const AmdGpu::Sampler& sampler);

private:
    /// Iterate over all page indices in a range
    template <typename Func>
    static void ForEachPage(PAddr addr, size_t size, Func&& func) {
        static constexpr bool RETURNS_BOOL = std::is_same_v<std::invoke_result<Func, u64>, bool>;
        const u64 page_end = (addr + size - 1) >> PageBits;
        for (u64 page = addr >> PageBits; page <= page_end; ++page) {
            if constexpr (RETURNS_BOOL) {
                if (func(page)) {
                    break;
                }
            } else {
                func(page);
            }
        }
    }

    template <typename Func>
    void ForEachImageInRegion(VAddr cpu_addr, size_t size, Func&& func) {
        using FuncReturn = typename std::invoke_result<Func, ImageId, Image&>::type;
        static constexpr bool BOOL_BREAK = std::is_same_v<FuncReturn, bool>;
        boost::container::small_vector<ImageId, 32> images;
        ForEachPage(cpu_addr, size, [this, &images, cpu_addr, size, func](u64 page) {
            const auto it = page_table.find(page);
            if (it == page_table.end()) {
                if constexpr (BOOL_BREAK) {
                    return false;
                } else {
                    return;
                }
            }
            for (const ImageId image_id : it->second) {
                Image& image = slot_images[image_id];
                if (image.flags & ImageFlagBits::Picked) {
                    continue;
                }
                image.flags |= ImageFlagBits::Picked;
                images.push_back(image_id);
                if constexpr (BOOL_BREAK) {
                    if (func(image_id, image)) {
                        return true;
                    }
                } else {
                    func(image_id, image);
                }
            }
            if constexpr (BOOL_BREAK) {
                return false;
            }
        });
        for (const ImageId image_id : images) {
            slot_images[image_id].flags &= ~ImageFlagBits::Picked;
        }
    }

    /// Create an image from the given parameters
    [[nodiscard]] ImageId InsertImage(const ImageInfo& info, VAddr cpu_addr);

    /// Register image in the page table
    void RegisterImage(ImageId image);

    /// Unregister image from the page table
    void UnregisterImage(ImageId image);

    /// Track CPU reads and writes for image
    void TrackImage(Image& image, ImageId image_id);

    /// Stop tracking CPU reads and writes for image
    void UntrackImage(Image& image, ImageId image_id);

    /// Increase/decrease the number of surface in pages touching the specified region
    void UpdatePagesCachedCount(VAddr addr, u64 size, s32 delta);

private:
    const Vulkan::Instance& instance;
    Vulkan::Scheduler& scheduler;
    Vulkan::StreamBuffer staging;
    SlotVector<Image> slot_images;
    SlotVector<ImageView> slot_image_views;
    tsl::robin_map<u64, Sampler> samplers;
    tsl::robin_pg_map<u64, std::vector<ImageId>> page_table;
    boost::icl::interval_map<VAddr, s32> cached_pages;
    std::mutex mutex;
#ifdef _WIN64
    void* veh_handle{};
#endif
    std::mutex m_page_table;
};

} // namespace VideoCore
