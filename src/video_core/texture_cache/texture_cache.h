// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <unordered_set>
#include <boost/container/small_vector.hpp>
#include <tsl/robin_map.h>

#include "common/lru_cache.h"
#include "common/slot_vector.h"
#include "video_core/amdgpu/resource.h"
#include "video_core/multi_level_page_table.h"
#include "video_core/texture_cache/blit_helper.h"
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
    ExactFmt = 1 << 4,  ///< Require the format to be exactly the same.
};
DECLARE_ENUM_FLAG_OPERATORS(FindFlags)

static constexpr u32 MaxInvalidateDist = 12_MB;

class TextureCache {
    // Default values for garbage collection
    static constexpr s64 DEFAULT_PRESSURE_GC_MEMORY = 1_GB + 512_MB;
    static constexpr s64 DEFAULT_CRITICAL_GC_MEMORY = 3_GB;
    static constexpr s64 TARGET_GC_THRESHOLD = 8_GB;

    struct Traits {
        using Entry = boost::container::small_vector<ImageId, 16>;
        static constexpr size_t AddressSpaceBits = 40;
        static constexpr size_t FirstLevelBits = 10;
        static constexpr size_t PageBits = 20;
    };
    using PageTable = MultiLevelPageTable<Traits>;

public:
    enum class BindingType : u32 {
        Texture,
        Storage,
        RenderTarget,
        DepthTarget,
        VideoOut,
    };

    struct BaseDesc {
        ImageInfo info;
        ImageViewInfo view_info;
        BindingType type{BindingType::Texture};

        BaseDesc() = default;
        BaseDesc(BindingType type_, ImageInfo info_, ImageViewInfo view_info_) noexcept
            : info{std::move(info_)}, view_info{std::move(view_info_)}, type{type_} {}
    };

    struct TextureDesc : public BaseDesc {
        TextureDesc() = default;
        TextureDesc(const AmdGpu::Image& image, const Shader::ImageResource& desc)
            : BaseDesc{desc.is_written ? BindingType::Storage : BindingType::Texture,
                       ImageInfo{image, desc}, ImageViewInfo{image, desc}} {}
    };

    struct RenderTargetDesc : public BaseDesc {
        RenderTargetDesc(const AmdGpu::Liverpool::ColorBuffer& buffer,
                         const AmdGpu::Liverpool::CbDbExtent& hint = {})
            : BaseDesc{BindingType::RenderTarget, ImageInfo{buffer, hint}, ImageViewInfo{buffer}} {}
    };

    struct DepthTargetDesc : public BaseDesc {
        DepthTargetDesc(const AmdGpu::Liverpool::DepthBuffer& buffer,
                        const AmdGpu::Liverpool::DepthView& view,
                        const AmdGpu::Liverpool::DepthControl& ctl, VAddr htile_address,
                        const AmdGpu::Liverpool::CbDbExtent& hint = {}, bool write_buffer = false)
            : BaseDesc{BindingType::DepthTarget,
                       ImageInfo{buffer, view.NumSlices(), htile_address, hint, write_buffer},
                       ImageViewInfo{buffer, view, ctl}} {}
    };

    struct VideoOutDesc : public BaseDesc {
        VideoOutDesc(const Libraries::VideoOut::BufferAttributeGroup& group, VAddr cpu_address)
            : BaseDesc{BindingType::VideoOut, ImageInfo{group, cpu_address}, ImageViewInfo{}} {}
    };

public:
    TextureCache(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
                 BufferCache& buffer_cache, PageManager& tracker);
    ~TextureCache();

    /// Invalidates any image in the logical page range.
    void InvalidateMemory(VAddr addr, size_t size);

    /// Marks an image as dirty if it exists at the provided address.
    void InvalidateMemoryFromGPU(VAddr address, size_t max_size);

    /// Evicts any images that overlap the unmapped range.
    void UnmapMemory(VAddr cpu_addr, size_t size);

    /// Schedules a copy of pending images for download back to CPU memory.
    void ProcessDownloadImages();

    /// Retrieves the image handle of the image with the provided attributes.
    [[nodiscard]] ImageId FindImage(BaseDesc& desc, FindFlags flags = {});

    /// Retrieves an image view with the properties of the specified image id.
    [[nodiscard]] ImageView& FindTexture(ImageId image_id, const BaseDesc& desc);

    /// Retrieves the render target with specified properties
    [[nodiscard]] ImageView& FindRenderTarget(BaseDesc& desc);

    /// Retrieves the depth target with specified properties
    [[nodiscard]] ImageView& FindDepthTarget(BaseDesc& desc);

    /// Updates image contents if it was modified by CPU.
    void UpdateImage(ImageId image_id, Vulkan::Scheduler* custom_scheduler = nullptr) {
        std::scoped_lock lock{mutex};
        Image& image = slot_images[image_id];
        TrackImage(image_id);
        TouchImage(image);
        RefreshImage(image, custom_scheduler);
    }

    [[nodiscard]] std::tuple<ImageId, int, int> ResolveOverlap(const ImageInfo& info,
                                                               BindingType binding,
                                                               ImageId cache_img_id,
                                                               ImageId merged_image_id);

    /// Resolves depth overlap and either re-creates the image or returns existing one
    [[nodiscard]] ImageId ResolveDepthOverlap(const ImageInfo& requested_info, BindingType binding,
                                              ImageId cache_img_id);

    [[nodiscard]] ImageId ExpandImage(const ImageInfo& info, ImageId image_id);

    /// Reuploads image contents.
    void RefreshImage(Image& image, Vulkan::Scheduler* custom_scheduler = nullptr);

    /// Retrieves the sampler that matches the provided S# descriptor.
    [[nodiscard]] vk::Sampler GetSampler(
        const AmdGpu::Sampler& sampler,
        const AmdGpu::Liverpool::BorderColorBufferBase& border_color_base);

    /// Retrieves the image with the specified id.
    [[nodiscard]] Image& GetImage(ImageId id) {
        auto& image = slot_images[id];
        TouchImage(image);
        return image;
    }

    /// Retrieves the image view with the specified id.
    [[nodiscard]] ImageView& GetImageView(ImageId id) {
        auto& view = slot_image_views[id];
        // Maybe this is not needed.
        Image& image = slot_images[view.image_id];
        TouchImage(image);
        return view;
    }

    /// Registers an image view for provided image
    ImageView& RegisterImageView(ImageId image_id, const ImageViewInfo& view_info);

    /// Returns true if the specified address is a metadata surface.
    bool IsMeta(VAddr address) const {
        return surface_metas.contains(address);
    }

    /// Returns true if a slice of the specified metadata surface has been cleared.
    bool IsMetaCleared(VAddr address, u32 slice) const {
        const auto& it = surface_metas.find(address);
        if (it != surface_metas.end()) {
            return it.value().clear_mask & (1u << slice);
        }
        return false;
    }

    /// Clears all slices of the specified metadata surface.
    bool ClearMeta(VAddr address) {
        auto it = surface_metas.find(address);
        if (it != surface_metas.end()) {
            it.value().clear_mask = u32(-1);
            return true;
        }
        return false;
    }

    /// Updates the state of a slice of the specified metadata surface.
    bool TouchMeta(VAddr address, u32 slice, bool is_clear) {
        auto it = surface_metas.find(address);
        if (it != surface_metas.end()) {
            if (is_clear) {
                it.value().clear_mask |= 1u << slice;
            } else {
                it.value().clear_mask &= ~(1u << slice);
            }
            return true;
        }
        return false;
    }

    /// Runs the garbage collector.
    void RunGarbageCollector();

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

    /// Gets or creates a null image for a particular format.
    ImageId GetNullImage(vk::Format format);

    /// Copies image memory back to CPU.
    void DownloadImageMemory(ImageId image_id);

    /// Create an image from the given parameters
    [[nodiscard]] ImageId InsertImage(const ImageInfo& info, VAddr cpu_addr);

    /// Register image in the page table
    void RegisterImage(ImageId image);

    /// Unregister image from the page table
    void UnregisterImage(ImageId image);

    /// Track CPU reads and writes for image
    void TrackImage(ImageId image_id);
    void TrackImageHead(ImageId image_id);
    void TrackImageTail(ImageId image_id);

    /// Stop tracking CPU reads and writes for image
    void UntrackImage(ImageId image_id);
    void UntrackImageHead(ImageId image_id);
    void UntrackImageTail(ImageId image_id);

    void MarkAsMaybeDirty(ImageId image_id, Image& image);

    /// Removes the image and any views/surface metas that reference it.
    void DeleteImage(ImageId image_id);

    /// Touch the image in the LRU cache.
    void TouchImage(const Image& image);

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
    BlitHelper blit_helper;
    TileManager tile_manager;
    Common::SlotVector<Image> slot_images;
    Common::SlotVector<ImageView> slot_image_views;
    tsl::robin_map<u64, Sampler> samplers;
    tsl::robin_map<vk::Format, ImageId> null_images;
    std::unordered_set<ImageId> download_images;
    u64 total_used_memory = 0;
    u64 trigger_gc_memory = 0;
    u64 pressure_gc_memory = 0;
    u64 critical_gc_memory = 0;
    u64 gc_tick = 0;
    Common::LeastRecentlyUsedCache<ImageId, u64> lru_cache;
    PageTable page_table;
    std::mutex mutex;

    struct MetaDataInfo {
        enum class Type {
            CMask,
            FMask,
            HTile,
        };
        Type type;
        u32 clear_mask{u32(-1)};
    };
    tsl::robin_map<VAddr, MetaDataInfo> surface_metas;
};

} // namespace VideoCore
