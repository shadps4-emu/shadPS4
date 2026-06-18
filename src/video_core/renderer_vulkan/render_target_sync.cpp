// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "video_core/renderer_vulkan/render_target_sync.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/texture_cache.h"

namespace Vulkan {

RenderTargetSync::RenderTargetSync(const Instance& instance_, Scheduler& scheduler_,
                                   VideoCore::TextureCache& texture_cache_)
    : instance{instance_}, scheduler{scheduler_}, texture_cache{texture_cache_} {}

RenderTargetSync::~RenderTargetSync() = default;

void RenderTargetSync::RecordRtWrite(VAddr addr, VideoCore::ImageId id) {
    pending_rt_writes_[addr] = id;
    // New RT content at this address — old dedup is stale.
    pending_rt_copied_.erase(addr);
}

void RenderTargetSync::CopyFromLastRt(VAddr addr, VideoCore::ImageId tex_id,
                                       u32 copy_w, u32 copy_h) {
    auto it = pending_rt_writes_.find(addr);
    if (it == pending_rt_writes_.end()) return;

    VideoCore::ImageId rt_id = it->second;
    auto& rt_image = texture_cache.GetImage(rt_id);

    // Only skip if RT is strictly smaller than texture.
    if (rt_image.info.size.width < copy_w && rt_image.info.size.height < copy_h) {
        LOG_WARNING(Render_Vulkan,
                    "[CopyFromLastRt] RT smaller than texture: rt={}x{} tex={}x{} @ {:#x}",
                    rt_image.info.size.width, rt_image.info.size.height, copy_w, copy_h, addr);
        return;
    }
    if (rt_id == tex_id) return;

    // Dedup: each tex_id only pulls once per submit from this addr's RT.
    auto& copied = pending_rt_copied_[addr];
    if (!copied.insert(tex_id).second) return;

    auto& tex_image = texture_cache.GetImage(tex_id);
    CopyRtToAlias(rt_image, tex_image);
}

void RenderTargetSync::PushPendingRtAliases() {
    for (auto& [addr, rt_id] : pending_rt_writes_) {
        PushRtToAliases(addr, rt_id);
    }
    pending_rt_writes_.clear();
}

void RenderTargetSync::ClearRecords() {
    pending_rt_writes_.clear();
    pending_rt_copied_.clear();
}

void RenderTargetSync::Schedule1x1Readback(VideoCore::ImageId image_id) {
    texture_cache.AddDownload(image_id);
}

void RenderTargetSync::PushRtToAliases(VAddr addr, VideoCore::ImageId rt_id) {
    auto& rt_image = texture_cache.GetImage(rt_id);

    const u64 page = addr >> VideoCore::TextureCache::Traits::PageBits;
    const auto& page_table = texture_cache.GetPageTable();
    const auto page_it = page_table.find(page);
    if (!page_it) return;

    for (VideoCore::ImageId alias_id : *page_it) {
        if (alias_id == rt_id) continue;
        auto& alias_image = texture_cache.GetImage(alias_id);
        if (alias_image.info.guest_address != addr) continue;
        if (alias_image.info.props.is_depth) continue;
        if (rt_image.info.size.width < alias_image.info.size.width) continue;
        if (rt_image.info.size.height < alias_image.info.size.height) continue;

        // Skip aliases that are themselves pending RTs — avoids RT↔RT feedback loops.
        auto pend_it = pending_rt_writes_.find(addr);
        if (pend_it != pending_rt_writes_.end() && pend_it->second == alias_id) continue;

        CopyRtToAlias(rt_image, alias_image);
    }
}

void RenderTargetSync::CopyRtToAlias(VideoCore::Image& rt_image,
                                      VideoCore::Image& alias_image) {
    const u32 copy_w = alias_image.info.size.width;
    const u32 copy_h = alias_image.info.size.height;

    if (rt_image.info.num_samples != alias_image.info.num_samples) {
        scheduler.EndRendering();
        auto cmdbuf = scheduler.CommandBuffer();

        VideoCore::UniqueImage temp{instance.GetDevice(), instance.GetAllocator()};
        temp.Create({
            .flags = vk::ImageCreateFlagBits::eMutableFormat |
                     vk::ImageCreateFlagBits::eExtendedUsage,
            .imageType = vk::ImageType::e2D,
            .format = rt_image.info.pixel_format,
            .extent = {copy_w, copy_h, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eTransferSrc |
                     vk::ImageUsageFlagBits::eTransferDst,
        });

        rt_image.Transit(vk::ImageLayout::eTransferSrcOptimal,
                         vk::AccessFlagBits2::eTransferRead, {});
        {
            const vk::ImageMemoryBarrier2 temp_barrier = {
                .srcStageMask = vk::PipelineStageFlagBits2::eNone,
                .srcAccessMask = vk::AccessFlagBits2::eNone,
                .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
                .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
                .oldLayout = vk::ImageLayout::eUndefined,
                .newLayout = vk::ImageLayout::eTransferDstOptimal,
                .image = temp,
                .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
            };
            cmdbuf.pipelineBarrier2(vk::DependencyInfo{.imageMemoryBarrierCount = 1,
                                                       .pImageMemoryBarriers = &temp_barrier});
        }
        const vk::ImageResolve resolve_region = {
            .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            .srcOffset = {0, 0, 0},
            .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            .dstOffset = {0, 0, 0},
            .extent = {copy_w, copy_h, 1},
        };
        cmdbuf.resolveImage(rt_image.GetImage(), vk::ImageLayout::eTransferSrcOptimal, temp,
                            vk::ImageLayout::eTransferDstOptimal, resolve_region);

        {
            const vk::ImageMemoryBarrier2 temp_barrier = {
                .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
                .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
                .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
                .dstAccessMask = vk::AccessFlagBits2::eTransferRead,
                .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                .newLayout = vk::ImageLayout::eTransferSrcOptimal,
                .image = temp,
                .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
            };
            cmdbuf.pipelineBarrier2(vk::DependencyInfo{.imageMemoryBarrierCount = 1,
                                                       .pImageMemoryBarriers = &temp_barrier});
        }
        alias_image.Transit(vk::ImageLayout::eTransferDstOptimal,
                            vk::AccessFlagBits2::eTransferWrite, {});
        const vk::ImageCopy copy_region = {
            .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            .srcOffset = {0, 0, 0},
            .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            .dstOffset = {0, 0, 0},
            .extent = {copy_w, copy_h, 1},
        };
        cmdbuf.copyImage(temp, vk::ImageLayout::eTransferSrcOptimal, alias_image.GetImage(),
                         vk::ImageLayout::eTransferDstOptimal, copy_region);

        rt_image.Transit(vk::ImageLayout::eColorAttachmentOptimal,
                         vk::AccessFlagBits2::eColorAttachmentWrite, {});
        alias_image.Transit(vk::ImageLayout::eShaderReadOnlyOptimal,
                            vk::AccessFlagBits2::eShaderRead, {});
        scheduler.DeferOperation([temp = std::move(temp)]() mutable { temp.Destroy(); });
        return;
    }

    // Same sample count: direct copyImage.
    rt_image.Transit(vk::ImageLayout::eTransferSrcOptimal,
                     vk::AccessFlagBits2::eTransferRead, {});
    alias_image.Transit(vk::ImageLayout::eTransferDstOptimal,
                        vk::AccessFlagBits2::eTransferWrite, {});
    const vk::ImageCopy region = {
        .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .srcOffset = {0, 0, 0},
        .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .dstOffset = {0, 0, 0},
        .extent = {copy_w, copy_h, 1},
    };
    scheduler.CommandBuffer().copyImage(rt_image.GetImage(), vk::ImageLayout::eTransferSrcOptimal,
                                        alias_image.GetImage(),
                                        vk::ImageLayout::eTransferDstOptimal, region);
    rt_image.Transit(vk::ImageLayout::eColorAttachmentOptimal,
                     vk::AccessFlagBits2::eColorAttachmentWrite, {});
    alias_image.Transit(vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits2::eShaderRead,
                        {});
}

} // namespace Vulkan
