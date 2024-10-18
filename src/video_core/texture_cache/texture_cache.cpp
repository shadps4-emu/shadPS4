// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <optional>
#include <xxhash.h>
#include "common/assert.h"
#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/page_manager.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/host_compatibility.h"
#include "video_core/texture_cache/texture_cache.h"
#include "video_core/texture_cache/tile_manager.h"

namespace VideoCore {

static constexpr u64 PageShift = 12;
static constexpr u64 NumFramesBeforeRemoval = 32;

TextureCache::TextureCache(const Vulkan::Instance& instance_, Vulkan::Scheduler& scheduler_,
                           BufferCache& buffer_cache_, PageManager& tracker_)
    : instance{instance_}, scheduler{scheduler_}, buffer_cache{buffer_cache_}, tracker{tracker_},
      tile_manager{instance, scheduler} {
    ImageInfo info{};
    info.pixel_format = vk::Format::eR8G8B8A8Unorm;
    info.type = vk::ImageType::e2D;
    info.tiling_idx = u32(AmdGpu::TilingMode::Texture_MicroTiled);
    info.num_bits = 32;
    info.UpdateSize();
    const ImageId null_id = slot_images.insert(instance, scheduler, info);
    ASSERT(null_id.index == 0);
    const vk::Image& null_image = slot_images[null_id].image;
    Vulkan::SetObjectName(instance.GetDevice(), null_image, "Null Image");
    slot_images[null_id].flags = ImageFlagBits::Tracked;

    ImageViewInfo view_info;
    const auto null_view_id =
        slot_image_views.insert(instance, view_info, slot_images[null_id], null_id);
    ASSERT(null_view_id.index == 0);
    const vk::ImageView& null_image_view = slot_image_views[null_view_id].image_view.get();
    Vulkan::SetObjectName(instance.GetDevice(), null_image_view, "Null Image View");
}

TextureCache::~TextureCache() = default;

void TextureCache::InvalidateMemory(VAddr address, size_t size) {
    std::scoped_lock lock{mutex};
    ForEachImageInRegion(address, size, [&](ImageId image_id, Image& image) {
        // Ensure image is reuploaded when accessed again.
        image.flags |= ImageFlagBits::CpuDirty;
        // Untrack image, so the range is unprotected and the guest can write freely.
        UntrackImage(image_id);
    });
}

void TextureCache::InvalidateMemoryFromGPU(VAddr address, size_t max_size) {
    std::scoped_lock lock{mutex};
    ForEachImageInRegion(address, max_size, [&](ImageId image_id, Image& image) {
        // Only consider images that match base address.
        // TODO: Maybe also consider subresources
        if (image.info.guest_address != address) {
            return;
        }
        // Ensure image is reuploaded when accessed again.
        image.flags |= ImageFlagBits::GpuDirty;
    });
}

void TextureCache::UnmapMemory(VAddr cpu_addr, size_t size) {
    std::scoped_lock lk{mutex};

    boost::container::small_vector<ImageId, 16> deleted_images;
    ForEachImageInRegion(cpu_addr, size, [&](ImageId id, Image&) { deleted_images.push_back(id); });
    for (const ImageId id : deleted_images) {
        // TODO: Download image data back to host.
        FreeImage(id);
    }
}

ImageId TextureCache::ResolveDepthOverlap(const ImageInfo& requested_info, ImageId cache_image_id) {
    const auto& cache_info = slot_images[cache_image_id].info;

    const bool was_bound_as_texture =
        !cache_info.usage.depth_target && (cache_info.usage.texture || cache_info.usage.storage);
    if (requested_info.usage.depth_target && was_bound_as_texture) {
        auto new_image_id = slot_images.insert(instance, scheduler, requested_info);
        RegisterImage(new_image_id);

        // TODO: perform a depth copy here

        FreeImage(cache_image_id);
        return new_image_id;
    }

    const bool should_bind_as_texture =
        !requested_info.usage.depth_target &&
        (requested_info.usage.texture || requested_info.usage.storage);
    if (cache_info.usage.depth_target && should_bind_as_texture) {
        if (cache_info.resources == requested_info.resources) {
            return cache_image_id;
        } else {
            UNREACHABLE();
        }
    }

    return {};
}

ImageId TextureCache::ResolveOverlap(const ImageInfo& image_info, ImageId cache_image_id,
                                     ImageId merged_image_id) {
    auto& tex_cache_image = slot_images[cache_image_id];

    if (image_info.guest_address == tex_cache_image.info.guest_address) { // Equal address
        if (image_info.size != tex_cache_image.info.size) {
            // Very likely this kind of overlap is caused by allocation from a pool. We can assume
            // it is safe to delete the image if it wasn't accessed in some amount of frames.
            if (scheduler.CurrentTick() - tex_cache_image.tick_accessed_last >
                NumFramesBeforeRemoval) {

                FreeImage(cache_image_id);
            }
            return merged_image_id;
        }

        if (auto depth_image_id = ResolveDepthOverlap(image_info, cache_image_id)) {
            return depth_image_id;
        }

        if (image_info.pixel_format != tex_cache_image.info.pixel_format ||
            image_info.guest_size_bytes <= tex_cache_image.info.guest_size_bytes) {
            auto result_id = merged_image_id ? merged_image_id : cache_image_id;
            const auto& result_image = slot_images[result_id];
            return IsVulkanFormatCompatible(image_info.pixel_format, result_image.info.pixel_format)
                       ? result_id
                       : ImageId{};
        }

        ImageId new_image_id{};
        if (image_info.type == tex_cache_image.info.type) {
            new_image_id = ExpandImage(image_info, cache_image_id);
        } else {
            UNREACHABLE();
        }
        return new_image_id;
    }

    // Right overlap, the image requested is a possible subresource of the image from cache.
    if (image_info.guest_address > tex_cache_image.info.guest_address) {
        // Should be handled by view. No additional actions needed.
    } else {
        // Left overlap, the image from cache is a possible subresource of the image requested
        if (!merged_image_id) {
            // We need to have a larger, already allocated image to copy this one into
            return {};
        }

        if (tex_cache_image.info.IsMipOf(image_info)) {
            tex_cache_image.Transit(vk::ImageLayout::eTransferSrcOptimal,
                                    vk::AccessFlagBits2::eTransferRead, {});

            const auto num_mips_to_copy = tex_cache_image.info.resources.levels;
            ASSERT(num_mips_to_copy == 1);

            auto& merged_image = slot_images[merged_image_id];
            merged_image.CopyMip(tex_cache_image, image_info.resources.levels - 1);

            FreeImage(cache_image_id);
        }
    }

    return merged_image_id;
}

ImageId TextureCache::ExpandImage(const ImageInfo& info, ImageId image_id) {
    const auto new_image_id = slot_images.insert(instance, scheduler, info);
    RegisterImage(new_image_id);

    auto& src_image = slot_images[image_id];
    auto& new_image = slot_images[new_image_id];

    src_image.Transit(vk::ImageLayout::eTransferSrcOptimal, vk::AccessFlagBits2::eTransferRead, {});
    new_image.CopyImage(src_image);

    if (True(src_image.flags & ImageFlagBits::Bound)) {
        src_image.flags |= ImageFlagBits::NeedsRebind;
    }

    FreeImage(image_id);

    TrackImage(new_image_id);
    new_image.flags &= ~ImageFlagBits::Dirty;
    return new_image_id;
}

ImageId TextureCache::FindImage(const ImageInfo& info, FindFlags flags) {
    if (info.guest_address == 0) [[unlikely]] {
        return NULL_IMAGE_VIEW_ID;
    }

    std::scoped_lock lock{mutex};
    boost::container::small_vector<ImageId, 8> image_ids;
    ForEachImageInRegion(info.guest_address, info.guest_size_bytes,
                         [&](ImageId image_id, Image& image) { image_ids.push_back(image_id); });

    ImageId image_id{};

    // Check for a perfect match first
    for (const auto& cache_id : image_ids) {
        auto& cache_image = slot_images[cache_id];
        if (cache_image.info.guest_address != info.guest_address) {
            continue;
        }
        if (False(flags & FindFlags::RelaxSize) &&
            cache_image.info.guest_size_bytes != info.guest_size_bytes) {
            continue;
        }
        if (False(flags & FindFlags::RelaxDim) && cache_image.info.size != info.size) {
            continue;
        }
        if (False(flags & FindFlags::RelaxFmt) &&
            !IsVulkanFormatCompatible(info.pixel_format, cache_image.info.pixel_format)) {
            continue;
        }
        ASSERT(cache_image.info.type == info.type || True(flags & FindFlags::RelaxFmt));
        image_id = cache_id;
    }

    if (True(flags & FindFlags::NoCreate) && !image_id) {
        return {};
    }

    // Try to resolve overlaps (if any)
    if (!image_id) {
        for (const auto& cache_id : image_ids) {
            const auto& merged_info = image_id ? slot_images[image_id].info : info;
            image_id = ResolveOverlap(merged_info, cache_id, image_id);
        }
    }

    if (image_id) {
        Image& image_resoved = slot_images[image_id];

        if (image_resoved.info.resources < info.resources) {
            // The image was clearly picked up wrong.
            FreeImage(image_id);
            image_id = {};
            LOG_WARNING(Render_Vulkan, "Image overlap resolve failed");
        }
    }
    // Create and register a new image
    if (!image_id) {
        image_id = slot_images.insert(instance, scheduler, info);
        RegisterImage(image_id);
    }

    Image& image = slot_images[image_id];
    image.tick_accessed_last = scheduler.CurrentTick();

    return image_id;
}

ImageView& TextureCache::RegisterImageView(ImageId image_id, const ImageViewInfo& view_info) {
    Image& image = slot_images[image_id];
    if (const ImageViewId view_id = image.FindView(view_info); view_id) {
        return slot_image_views[view_id];
    }

    const ImageViewId view_id = slot_image_views.insert(instance, view_info, image, image_id);
    image.image_view_infos.emplace_back(view_info);
    image.image_view_ids.emplace_back(view_id);
    return slot_image_views[view_id];
}

ImageView& TextureCache::FindTexture(ImageId image_id, const ImageViewInfo& view_info) {
    Image& image = slot_images[image_id];
    UpdateImage(image_id);
    auto& usage = image.info.usage;

    if (view_info.is_storage) {
        image.Transit(vk::ImageLayout::eGeneral,
                      vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite,
                      view_info.range);
        usage.storage = true;
    } else {
        const auto new_layout = image.info.IsDepthStencil()
                                    ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                    : vk::ImageLayout::eShaderReadOnlyOptimal;
        image.Transit(new_layout, vk::AccessFlagBits2::eShaderRead, view_info.range);
        usage.texture = true;
    }

    return RegisterImageView(image_id, view_info);
}

ImageView& TextureCache::FindRenderTarget(const ImageInfo& image_info,
                                          const ImageViewInfo& view_info) {
    const ImageId image_id = FindImage(image_info);
    Image& image = slot_images[image_id];
    image.flags |= ImageFlagBits::GpuModified;
    UpdateImage(image_id);

    image.Transit(vk::ImageLayout::eColorAttachmentOptimal,
                  vk::AccessFlagBits2::eColorAttachmentWrite |
                      vk::AccessFlagBits2::eColorAttachmentRead,
                  view_info.range);

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
    const ImageId image_id = FindImage(image_info);
    Image& image = slot_images[image_id];
    image.flags |= ImageFlagBits::GpuModified;
    image.flags &= ~ImageFlagBits::Dirty;
    image.aspect_mask = vk::ImageAspectFlagBits::eDepth;

    const bool has_stencil = image_info.usage.stencil;
    if (has_stencil) {
        image.aspect_mask |= vk::ImageAspectFlagBits::eStencil;
    }

    const auto new_layout = view_info.is_storage
                                ? has_stencil ? vk::ImageLayout::eDepthStencilAttachmentOptimal
                                              : vk::ImageLayout::eDepthAttachmentOptimal
                            : has_stencil ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                          : vk::ImageLayout::eDepthReadOnlyOptimal;
    image.Transit(new_layout,
                  vk::AccessFlagBits2::eDepthStencilAttachmentWrite |
                      vk::AccessFlagBits2::eDepthStencilAttachmentRead,
                  view_info.range);

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
    image.info.usage.stencil = has_stencil;

    return RegisterImageView(image_id, view_info);
}

void TextureCache::RefreshImage(Image& image, Vulkan::Scheduler* custom_scheduler /*= nullptr*/) {
    if (False(image.flags & ImageFlagBits::Dirty)) {
        return;
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
        const auto& [mip_size, mip_pitch, mip_height, mip_ofs] = image.info.mips_layout[m];

        // Protect GPU modified resources from accidental CPU reuploads.
        const bool is_gpu_modified = True(image.flags & ImageFlagBits::GpuModified);
        const bool is_gpu_dirty = True(image.flags & ImageFlagBits::GpuDirty);
        if (is_gpu_modified && !is_gpu_dirty) {
            const u8* addr = std::bit_cast<u8*>(image.info.guest_address);
            const u64 hash = XXH3_64bits(addr + mip_ofs, mip_size);
            if (image.mip_hashes[m] == hash) {
                continue;
            }
            image.mip_hashes[m] = hash;
        }

        image_copy.push_back({
            .bufferOffset = mip_ofs * num_layers,
            .bufferRowLength = static_cast<u32>(mip_pitch),
            .bufferImageHeight = static_cast<u32>(mip_height),
            .imageSubresource{
                .aspectMask = image.aspect_mask & ~vk::ImageAspectFlagBits::eStencil,
                .mipLevel = m,
                .baseArrayLayer = 0,
                .layerCount = num_layers,
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, depth},
        });
    }

    if (image_copy.empty()) {
        return;
    }

    auto* sched_ptr = custom_scheduler ? custom_scheduler : &scheduler;
    sched_ptr->EndRendering();

    const auto cmdbuf = sched_ptr->CommandBuffer();
    image.Transit(vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits2::eTransferWrite, {},
                  cmdbuf);

    const VAddr image_addr = image.info.guest_address;
    const size_t image_size = image.info.guest_size_bytes;
    const auto [vk_buffer, buf_offset] = buffer_cache.ObtainViewBuffer(image_addr, image_size);
    // The obtained buffer may be written by a shader so we need to emit a barrier to prevent RAW
    // hazard
    if (auto barrier = vk_buffer->GetBarrier(vk::AccessFlagBits2::eTransferRead,
                                             vk::PipelineStageFlagBits2::eTransfer)) {
        const auto dependencies = vk::DependencyInfo{
            .dependencyFlags = vk::DependencyFlagBits::eByRegion,
            .bufferMemoryBarrierCount = 1,
            .pBufferMemoryBarriers = &barrier.value(),
        };
        cmdbuf.pipelineBarrier2(dependencies);
    }

    const auto [buffer, offset] = tile_manager.TryDetile(vk_buffer->Handle(), buf_offset, image);
    for (auto& copy : image_copy) {
        copy.bufferOffset += offset;
    }

    cmdbuf.copyBufferToImage(buffer, image.image, vk::ImageLayout::eTransferDstOptimal, image_copy);
    image.flags &= ~ImageFlagBits::Dirty;
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
}

void TextureCache::TrackImage(ImageId image_id) {
    auto& image = slot_images[image_id];
    if (True(image.flags & ImageFlagBits::Tracked)) {
        return;
    }
    image.flags |= ImageFlagBits::Tracked;
    tracker.UpdatePagesCachedCount(image.cpu_addr, image.info.guest_size_bytes, 1);
}

void TextureCache::UntrackImage(ImageId image_id) {
    auto& image = slot_images[image_id];
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
