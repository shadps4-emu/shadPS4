// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <xxhash.h>
#include "common/assert.h"
#include "video_core/page_manager.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/texture_cache.h"
#include "video_core/texture_cache/tile_manager.h"

namespace VideoCore {

static constexpr u64 StreamBufferSize = 512_MB;
static constexpr u64 PageShift = 12;

TextureCache::TextureCache(const Vulkan::Instance& instance_, Vulkan::Scheduler& scheduler_,
                           BufferCache& buffer_cache_, PageManager& tracker_)
    : instance{instance_}, scheduler{scheduler_}, buffer_cache{buffer_cache_}, tracker{tracker_},
      staging{instance, scheduler, MemoryUsage::Upload, StreamBufferSize},
      tile_manager{instance, scheduler} {
    ImageInfo info;
    info.pixel_format = vk::Format::eR8G8B8A8Unorm;
    info.type = vk::ImageType::e2D;
    const ImageId null_id = slot_images.insert(instance, scheduler, info);
    ASSERT(null_id.index == 0);

    ImageViewInfo view_info;
    void(slot_image_views.insert(instance, view_info, slot_images[null_id], null_id));
}

TextureCache::~TextureCache() = default;

void TextureCache::InvalidateMemory(VAddr address, size_t size) {
    std::unique_lock lock{mutex};
    ForEachImageInRegion(address, size, [&](ImageId image_id, Image& image) {
        // Ensure image is reuploaded when accessed again.
        image.flags |= ImageFlagBits::CpuModified;
        // Untrack image, so the range is unprotected and the guest can write freely.
        UntrackImage(image, image_id);
    });
}

void TextureCache::UnmapMemory(VAddr cpu_addr, size_t size) {
    std::scoped_lock lk{mutex};

    boost::container::small_vector<ImageId, 16> deleted_images;
    ForEachImageInRegion(cpu_addr, size, [&](ImageId id, Image&) { deleted_images.push_back(id); });
    for (const ImageId id : deleted_images) {
        Image& image = slot_images[id];
        if (True(image.flags & ImageFlagBits::Tracked)) {
            UntrackImage(image, id);
        }
        // TODO: Download image data back to host.
        UnregisterImage(id);
        DeleteImage(id);
    }
}

ImageId TextureCache::FindImage(const ImageInfo& info, bool refresh_on_create) {
    if (info.guest_address == 0) [[unlikely]] {
        return NULL_IMAGE_VIEW_ID;
    }

    std::unique_lock lock{mutex};
    boost::container::small_vector<ImageId, 2> image_ids;
    ForEachImageInRegion(
        info.guest_address, info.guest_size_bytes, [&](ImageId image_id, Image& image) {
            // Address and width must match.
            if (image.cpu_addr != info.guest_address || image.info.size.width != info.size.width) {
                return;
            }
            if (info.IsDepthStencil() != image.info.IsDepthStencil() &&
                info.pixel_format != vk::Format::eR32Sfloat) {
                return;
            }
            image_ids.push_back(image_id);
        });

    // ASSERT_MSG(image_ids.size() <= 1, "Overlapping images not allowed!");

    ImageId image_id{};
    if (image_ids.empty()) {
        image_id = slot_images.insert(instance, scheduler, info);
        RegisterImage(image_id);
    } else {
        image_id = image_ids[image_ids.size() > 1 ? 1 : 0];
    }

    Image& image = slot_images[image_id];
    if (True(image.flags & ImageFlagBits::CpuModified) && refresh_on_create) {
        RefreshImage(image);
        TrackImage(image, image_id);
    }

    return image_id;
}

ImageView& TextureCache::RegisterImageView(ImageId image_id, const ImageViewInfo& view_info) {
    Image& image = slot_images[image_id];
    if (const ImageViewId view_id = image.FindView(view_info); view_id) {
        return slot_image_views[view_id];
    }

    // All tiled images are created with storage usage flag. This makes set of formats (e.g. sRGB)
    // impossible to use. However, during view creation, if an image isn't used as storage we can
    // temporary remove its storage bit.
    std::optional<vk::ImageUsageFlags> usage_override;
    if (!image.info.usage.storage) {
        usage_override = image.usage & ~vk::ImageUsageFlagBits::eStorage;
    }

    const ImageViewId view_id =
        slot_image_views.insert(instance, view_info, image, image_id, usage_override);
    image.image_view_infos.emplace_back(view_info);
    image.image_view_ids.emplace_back(view_id);
    return slot_image_views[view_id];
}

ImageView& TextureCache::FindTexture(const ImageInfo& info, const ImageViewInfo& view_info) {
    const ImageId image_id = FindImage(info);
    Image& image = slot_images[image_id];
    auto& usage = image.info.usage;

    if (view_info.is_storage) {
        image.Transit(vk::ImageLayout::eGeneral,
                      vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite);
        usage.storage = true;
    } else {
        const auto new_layout = image.info.IsDepthStencil()
                                    ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                    : vk::ImageLayout::eShaderReadOnlyOptimal;
        image.Transit(new_layout, vk::AccessFlagBits::eShaderRead);
        usage.texture = true;
    }

    // These changes are temporary and should be removed once texture cache will handle subresources
    // merging
    auto view_info_tmp = view_info;
    if (view_info_tmp.range.base.level > image.info.resources.levels - 1 ||
        view_info_tmp.range.base.layer > image.info.resources.layers - 1 ||
        view_info_tmp.range.extent.levels > image.info.resources.levels ||
        view_info_tmp.range.extent.layers > image.info.resources.layers) {

        LOG_DEBUG(Render_Vulkan,
                  "Subresource range ({}~{},{}~{}) exceeds base image extents ({},{})",
                  view_info_tmp.range.base.level, view_info_tmp.range.extent.levels,
                  view_info_tmp.range.base.layer, view_info_tmp.range.extent.layers,
                  image.info.resources.levels, image.info.resources.layers);

        view_info_tmp.range.base.level =
            std::min(view_info_tmp.range.base.level, image.info.resources.levels - 1);
        view_info_tmp.range.base.layer =
            std::min(view_info_tmp.range.base.layer, image.info.resources.layers - 1);
        view_info_tmp.range.extent.levels =
            std::min(view_info_tmp.range.extent.levels, image.info.resources.levels);
        view_info_tmp.range.extent.layers =
            std::min(view_info_tmp.range.extent.layers, image.info.resources.layers);
    }

    return RegisterImageView(image_id, view_info_tmp);
}

ImageView& TextureCache::FindRenderTarget(const ImageInfo& image_info,
                                          const ImageViewInfo& view_info) {
    const ImageId image_id = FindImage(image_info);
    Image& image = slot_images[image_id];
    image.flags &= ~ImageFlagBits::CpuModified;

    image.Transit(vk::ImageLayout::eColorAttachmentOptimal,
                  vk::AccessFlagBits::eColorAttachmentWrite |
                      vk::AccessFlagBits::eColorAttachmentRead);

    // Register meta data for this color buffer
    if (!(image.flags & ImageFlagBits::MetaRegistered)) {
        if (image_info.meta_info.cmask_addr) {
            surface_metas.emplace(
                image_info.meta_info.cmask_addr,
                MetaDataInfo{.type = MetaDataInfo::Type::CMask, .is_cleared = true});
            image.info.meta_info.cmask_addr = image_info.meta_info.cmask_addr;
            image.flags |= ImageFlagBits::MetaRegistered;
        }

        if (image_info.meta_info.fmask_addr) {
            surface_metas.emplace(
                image_info.meta_info.fmask_addr,
                MetaDataInfo{.type = MetaDataInfo::Type::FMask, .is_cleared = true});
            image.info.meta_info.fmask_addr = image_info.meta_info.fmask_addr;
            image.flags |= ImageFlagBits::MetaRegistered;
        }
    }

    // Update tracked image usage
    image.info.usage.render_target = true;

    return RegisterImageView(image_id, view_info);
}

ImageView& TextureCache::FindDepthTarget(const ImageInfo& image_info,
                                         const ImageViewInfo& view_info) {
    const ImageId image_id = FindImage(image_info, false);
    Image& image = slot_images[image_id];
    image.flags &= ~ImageFlagBits::CpuModified;

    const auto new_layout = view_info.is_storage ? vk::ImageLayout::eDepthStencilAttachmentOptimal
                                                 : vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    image.Transit(new_layout, vk::AccessFlagBits::eDepthStencilAttachmentWrite |
                                  vk::AccessFlagBits::eDepthStencilAttachmentRead);

    // Register meta data for this depth buffer
    if (!(image.flags & ImageFlagBits::MetaRegistered)) {
        if (image_info.meta_info.htile_addr) {
            surface_metas.emplace(
                image_info.meta_info.htile_addr,
                MetaDataInfo{.type = MetaDataInfo::Type::HTile, .is_cleared = true});
            image.info.meta_info.htile_addr = image_info.meta_info.htile_addr;
            image.flags |= ImageFlagBits::MetaRegistered;
        }
    }

    // Update tracked image usage
    image.info.usage.depth_target = true;

    return RegisterImageView(image_id, view_info);
}

void TextureCache::RefreshImage(Image& image) {
    // Mark image as validated.
    image.flags &= ~ImageFlagBits::CpuModified;

    scheduler.EndRendering();

    const auto cmdbuf = scheduler.CommandBuffer();
    image.Transit(vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eTransferWrite);

    vk::Buffer buffer{staging.Handle()};
    u32 offset{0};

    auto upload_buffer = tile_manager.TryDetile(image);
    if (upload_buffer) {
        buffer = *upload_buffer;
    } else {
        // Upload data to the staging buffer.
        offset = staging.Copy(image.info.guest_address, image.info.guest_size_bytes, 16);
    }

    const auto& num_layers = image.info.resources.layers;
    const auto& num_mips = image.info.resources.levels;
    ASSERT(num_mips == image.info.mips_layout.size());

    boost::container::small_vector<vk::BufferImageCopy, 14> image_copy{};
    for (u32 m = 0; m < num_mips; m++) {
        const u32 width = std::max(image.info.size.width >> m, 1u);
        const u32 height = std::max(image.info.size.height >> m, 1u);
        const u32 depth =
            image.info.props.is_volume ? std::max(image.info.size.depth >> m, 1u) : 1u;
        const auto& [_, mip_pitch, mip_height, mip_ofs] = image.info.mips_layout[m];

        image_copy.push_back({
            .bufferOffset = offset + mip_ofs * num_layers,
            .bufferRowLength = static_cast<uint32_t>(mip_pitch),
            .bufferImageHeight = static_cast<uint32_t>(mip_height),
            .imageSubresource{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = m,
                .baseArrayLayer = 0,
                .layerCount = num_layers,
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, depth},
        });
    }

    cmdbuf.copyBufferToImage(buffer, image.image, vk::ImageLayout::eTransferDstOptimal, image_copy);
}

vk::Sampler TextureCache::GetSampler(const AmdGpu::Sampler& sampler) {
    const u64 hash = XXH3_64bits(&sampler, sizeof(sampler));
    const auto [it, new_sampler] = samplers.try_emplace(hash, instance, sampler);
    return it->second.Handle();
}

void TextureCache::RegisterImage(ImageId image_id) {
    Image& image = slot_images[image_id];
    ASSERT_MSG(False(image.flags & ImageFlagBits::Registered),
               "Trying to register an already registered image");
    image.flags |= ImageFlagBits::Registered;
    ForEachPage(image.cpu_addr, image.info.guest_size_bytes,
                [this, image_id](u64 page) { page_table[page].push_back(image_id); });
}

void TextureCache::UnregisterImage(ImageId image_id) {
    Image& image = slot_images[image_id];
    ASSERT_MSG(True(image.flags & ImageFlagBits::Registered),
               "Trying to unregister an already registered image");
    image.flags &= ~ImageFlagBits::Registered;
    ForEachPage(image.cpu_addr, image.info.guest_size_bytes, [this, image_id](u64 page) {
        const auto page_it = page_table.find(page);
        if (page_it == nullptr) {
            UNREACHABLE_MSG("Unregistering unregistered page=0x{:x}", page << PageShift);
            return;
        }
        auto& image_ids = *page_it;
        const auto vector_it = std::ranges::find(image_ids, image_id);
        if (vector_it == image_ids.end()) {
            ASSERT_MSG(false, "Unregistering unregistered image in page=0x{:x}", page << PageShift);
            return;
        }
        image_ids.erase(vector_it);
    });
    slot_images.erase(image_id);
}

void TextureCache::TrackImage(Image& image, ImageId image_id) {
    if (True(image.flags & ImageFlagBits::Tracked)) {
        return;
    }
    image.flags |= ImageFlagBits::Tracked;
    tracker.UpdatePagesCachedCount(image.cpu_addr, image.info.guest_size_bytes, 1);
}

void TextureCache::UntrackImage(Image& image, ImageId image_id) {
    if (False(image.flags & ImageFlagBits::Tracked)) {
        return;
    }
    image.flags &= ~ImageFlagBits::Tracked;
    tracker.UpdatePagesCachedCount(image.cpu_addr, image.info.guest_size_bytes, -1);
}

void TextureCache::DeleteImage(ImageId image_id) {
    Image& image = slot_images[image_id];
    ASSERT_MSG(False(image.flags & ImageFlagBits::Tracked), "Image was not untracked");
    ASSERT_MSG(False(image.flags & ImageFlagBits::Registered), "Image was not unregistered");

    // Remove any registered meta areas.
    const auto& meta_info = image.info.meta_info;
    if (meta_info.cmask_addr) {
        surface_metas.erase(meta_info.cmask_addr);
    }
    if (meta_info.fmask_addr) {
        surface_metas.erase(meta_info.fmask_addr);
    }
    if (meta_info.htile_addr) {
        surface_metas.erase(meta_info.htile_addr);
    }

    // Reclaim image and any image views it references.
    scheduler.DeferOperation([this, image_id] {
        Image& image = slot_images[image_id];
        for (const ImageViewId image_view_id : image.image_view_ids) {
            slot_image_views.erase(image_view_id);
        }
        slot_images.erase(image_id);
    });
}

} // namespace VideoCore
