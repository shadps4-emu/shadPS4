// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/container/small_vector.hpp>
#include <tsl/robin_map.h>

#include "common/slot_vector.h"
#include "video_core/amdgpu/resource.h"
#include "video_core/multi_level_page_table.h"
#include "video_core/texture_cache/image.h"
#include "video_core/texture_cache/image_view.h"
#include "video_core/texture_cache/sampler.h"
#include "video_core/texture_cache/tile_manager.h"

namespace Core::Libraries::VideoOut {
struct BufferAttributeGroup;
}

namespace VideoCore {

class BufferCache;
class PageManager;

enum class FindFlags {
    NoCreate = 1 << 0,  ///< Do not create an image if searching for one fails.
    RelaxDim = 1 << 1,  ///< Do not check the dimentions of image, only address.
    RelaxSize = 1 << 2, ///< Do not check that the size matches exactly.
    RelaxFmt = 1 << 3,  ///< Do not check that format is compatible.
};
DECLARE_ENUM_FLAG_OPERATORS(FindFlags)

static constexpr u32 MaxInvalidateDist = 12_MB;

class TextureCache {
    struct Traits {
        using Entry = boost::container::small_vector<ImageId, 16>;
        static constexpr size_t AddressSpaceBits = 39;
        static constexpr size_t FirstLevelBits = 9;
        static constexpr size_t PageBits = 20;
    };
    using PageTable = MultiLevelPageTable<Traits>;

public:
    explicit TextureCache(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
                          BufferCache& buffer_cache, PageManager& tracker);
    ~TextureCache();

    /// Invalidates any image in the logical page range.
    void InvalidateMemory(VAddr address, size_t size);

    /// Marks an image as dirty if it exists at the provided address.
    void InvalidateMemoryFromGPU(VAddr address, size_t max_size);

    /// Evicts any images that overlap the unmapped range.
    void UnmapMemory(VAddr cpu_addr, size_t size);

    /// Retrieves the image handle of the image with the provided attributes.
    [[nodiscard]] ImageId FindImage(const ImageInfo& info, FindFlags flags = {});

    /// Retrieves an image view with the properties of the specified image id.
    [[nodiscard]] ImageView& FindTexture(ImageId image_id, const ImageViewInfo& view_info);

    /// Retrieves the render target with specified properties
    [[nodiscard]] ImageView& FindRenderTarget(const ImageInfo& image_info,
                                              const ImageViewInfo& view_info);

    /// Retrieves the depth target with specified properties
    [[nodiscard]] ImageView& FindDepthTarget(const ImageInfo& image_info,
                                             const ImageViewInfo& view_info);

    /// Updates image contents if it was modified by CPU.
    void UpdateImage(ImageId image_id, Vulkan::Scheduler* custom_scheduler = nullptr) {
        Image& image = slot_images[image_id];
        TrackImage(image_id);
        RefreshImage(image, custom_scheduler);
    }

    [[nodiscard]] ImageId ResolveOverlap(const ImageInfo& info, ImageId cache_img_id,
                                         ImageId merged_image_id);

    /// Resolves depth overlap and either re-creates the image or returns existing one
    [[nodiscard]] ImageId ResolveDepthOverlap(const ImageInfo& requested_info,
                                              ImageId cache_img_id);

    [[nodiscard]] ImageId ExpandImage(const ImageInfo& info, ImageId image_id);

    /// Reuploads image contents.
    void RefreshImage(Image& image, Vulkan::Scheduler* custom_scheduler = nullptr);

    /// Retrieves the sampler that matches the provided S# descriptor.
    [[nodiscard]] vk::Sampler GetSampler(const AmdGpu::Sampler& sampler);

    /// Retrieves the image with the specified id.
    [[nodiscard]] Image& GetImage(ImageId id) {
        return slot_images[id];
    }

    /// Retrieves the image view with the specified id.
    [[nodiscard]] ImageView& GetImageView(ImageId id) {
        return slot_image_views[id];
    }

    /// Registers an image view for provided image
    ImageView& RegisterImageView(ImageId image_id, const ImageViewInfo& view_info);

    bool IsMeta(VAddr address) const {
        return surface_metas.contains(address);
    }

    bool IsMetaCleared(VAddr address) const {
        const auto& it = surface_metas.find(address);
        if (it != surface_metas.end()) {
            return it.value().is_cleared;
        }
        return false;
    }

    bool TouchMeta(VAddr address, bool is_clear) {
        auto it = surface_metas.find(address);
        if (it != surface_metas.end()) {
            it.value().is_cleared = is_clear;
            return true;
        }
        return false;
    }

    template <typename Func>
    void ForEachImageInRegion(VAddr cpu_addr, size_t size, Func&& func) {
        using FuncReturn = typename std::invoke_result<Func, ImageId, Image&>::type;
        static constexpr bool BOOL_BREAK = std::is_same_v<FuncReturn, bool>;
        boost::container::small_vector<ImageId, 32> images;
        ForEachPage(cpu_addr, size, [this, &images, cpu_addr, size, func](u64 page) {
            const auto it = page_table.find(page);
            if (it == nullptr) {
                if constexpr (BOOL_BREAK) {
                    return false;
                } else {
                    return;
                }
            }
            for (const ImageId image_id : *it) {
                Image& image = slot_images[image_id];
                if (image.flags & ImageFlagBits::Picked) {
                    continue;
                }
                if (!image.Overlaps(cpu_addr, size)) {
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

private:
    /// Iterate over all page indices in a range
    template <typename Func>
    static void ForEachPage(PAddr addr, size_t size, Func&& func) {
        static constexpr bool RETURNS_BOOL = std::is_same_v<std::invoke_result<Func, u64>, bool>;
        const u64 page_end = (addr + size - 1) >> Traits::PageBits;
        for (u64 page = addr >> Traits::PageBits; page <= page_end; ++page) {
            if constexpr (RETURNS_BOOL) {
                if (func(page)) {
                    break;
                }
            } else {
                func(page);
            }
        }
    }

    /// Create an image from the given parameters
    [[nodiscard]] ImageId InsertImage(const ImageInfo& info, VAddr cpu_addr);

    /// Register image in the page table
    void RegisterImage(ImageId image);

    /// Unregister image from the page table
    void UnregisterImage(ImageId image);

    /// Track CPU reads and writes for image
    void TrackImage(ImageId image_id);

    /// Stop tracking CPU reads and writes for image
    void UntrackImage(ImageId image_id);

    /// Removes the image and any views/surface metas that reference it.
    void DeleteImage(ImageId image_id);

    void FreeImage(ImageId image_id) {
        UntrackImage(image_id);
        UnregisterImage(image_id);
        DeleteImage(image_id);
    }

private:
    const Vulkan::Instance& instance;
    Vulkan::Scheduler& scheduler;
    BufferCache& buffer_cache;
    PageManager& tracker;
    TileManager tile_manager;
    Common::SlotVector<Image> slot_images;
    Common::SlotVector<ImageView> slot_image_views;
    tsl::robin_map<u64, Sampler> samplers;
    PageTable page_table;
    std::mutex mutex;

    struct MetaDataInfo {
        enum class Type {
            CMask,
            FMask,
            HTile,
        };
        Type type;
        bool is_cleared;
    };
    tsl::robin_map<VAddr, MetaDataInfo> surface_metas;
};

} // namespace VideoCore
