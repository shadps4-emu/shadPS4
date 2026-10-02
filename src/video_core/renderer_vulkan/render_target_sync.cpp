// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "video_core/bruno_diag.h"
#include "video_core/renderer_vulkan/render_target_sync.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_runtime.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/texture_cache.h"

namespace Vulkan {

RenderTargetSync::RenderTargetSync(const Instance& instance_, Scheduler& scheduler_,
                                   Runtime& runtime_, VideoCore::TextureCache& texture_cache_)
    : instance{instance_}, scheduler{scheduler_}, runtime{runtime_}, texture_cache{texture_cache_} {
}

RenderTargetSync::~RenderTargetSync() = default;

void RenderTargetSync::RecordRtWrite(VAddr addr, VideoCore::ImageId id) {
    if (auto it = pending_rt_writes_.find(addr);
        (it == pending_rt_writes_.end() || it->second != id) && BrunoDiag::On()) {
        const auto& i = texture_cache.GetImage(id);
        LOG_INFO(Render_Vulkan, "[BRUNO-EV] RT_WRITE addr={:#x} id={} {}x{} s={} fmt={}", addr,
                 id.index, i.info.size.width, i.info.size.height, i.info.num_samples,
                 vk::to_string(i.info.pixel_format));
    }
    pending_rt_writes_[addr] = id;
    // New RT content at this address — old dedup is stale.
    pending_rt_copied_.erase(addr);
}

void RenderTargetSync::CopyFromLastRt(VAddr addr, VideoCore::ImageId tex_id, u32 copy_w,
                                      u32 copy_h) {
    auto it = pending_rt_writes_.find(addr);
    if (it == pending_rt_writes_.end())
        return;

    VideoCore::ImageId rt_id = it->second;
    auto& rt_image = texture_cache.GetImage(rt_id);

    // Skip if RT is smaller than texture in either dimension — copying would
    // read/write out of bounds of the RT image (invalid Vulkan usage, can hang the GPU).
    if (rt_image.info.size.width < copy_w || rt_image.info.size.height < copy_h) {
        LOG_WARNING(Render_Vulkan,
                    "[CopyFromLastRt] RT smaller than texture: rt={}x{} tex={}x{} @ {:#x}",
                    rt_image.info.size.width, rt_image.info.size.height, copy_w, copy_h, addr);
        return;
    }
    if (rt_id == tex_id)
        return;

    // Dedup: each tex_id only pulls once per submit from this addr's RT.
    auto& copied = pending_rt_copied_[addr];
    if (!copied.insert(tex_id).second)
        return;

    auto& tex_image = texture_cache.GetImage(tex_id);
    if (BrunoDiag::On()) {
        LOG_INFO(Render_Vulkan,
                 "[BRUNO-EV] RT_TO_ALIAS addr={:#x} rt={} ({}x{} s={}) -> tex={} ({}x{} s={} "
                 "fmt={} flags={:#x}) copy={}x{} tile rt={} tex={} pitch rt={} tex={}",
                 addr, rt_id.index, rt_image.info.size.width, rt_image.info.size.height,
                 rt_image.info.num_samples, tex_id.index, tex_image.info.size.width,
                 tex_image.info.size.height, tex_image.info.num_samples,
                 vk::to_string(tex_image.info.pixel_format), u32(tex_image.flags), copy_w, copy_h,
                 u32(rt_image.info.tile_mode), u32(tex_image.info.tile_mode), rt_image.info.pitch,
                 tex_image.info.pitch);
    }
    // A freshly created alias has every dirty bit set, so the next RefreshImage would upload
    // guest memory over the render target contents we are about to copy. Guest memory never
    // received those contents (the RT lives on the host GPU), so that upload replaces the
    // current frame with stale data. God of War III re-creates its post-process input view
    // almost every frame (its height changes), which made the artifact come and go.
    // The RT copy is the authoritative data for such an image, so drop its dirty state.
    // An alias that is only CpuDirty was written through guest memory after the RT
    // (e.g. by StorageImageSync) and must still be refreshed from there.
    static const bool keep_stale_upload = BrunoDiag::Flag("BRUNO_NOFIX1");
    // Only for a row-prefix view of the RT: same layout (width, pitch, tiling, bpp, samples)
    // and no more rows than the RT. Smaller aliases with a different width are unrelated
    // images that merely share the address; their real contents are in guest memory.
    const auto& ri = rt_image.info;
    const auto& ti = tex_image.info;
    const bool is_row_prefix_view =
        ti.size.width == ri.size.width && ti.pitch == ri.pitch && ti.tile_mode == ri.tile_mode &&
        ti.num_bits == ri.num_bits && ti.num_samples == ri.num_samples &&
        ti.size.height <= ri.size.height && ti.resources.levels == 1 && ri.resources.levels == 1;
    const bool is_new_alias = is_row_prefix_view &&
                              (tex_image.flags & VideoCore::ImageFlagBits::Dirty) ==
                                  VideoCore::ImageFlagBits::Dirty;
    if (BrunoDiag::On()) {
        LOG_INFO(Render_Vulkan, "[BRUNO-EV]   fix1: row_prefix={} new_alias={} applied={}",
                 is_row_prefix_view, is_new_alias, is_new_alias && !keep_stale_upload);
    }
    CopyRtToAlias(rt_image, tex_image);
    if (is_new_alias && !keep_stale_upload) {
        tex_image.flags &= ~VideoCore::ImageFlagBits::Dirty;
    }
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
    if (!page_it)
        return;

    for (VideoCore::ImageId alias_id : *page_it) {
        if (alias_id == rt_id)
            continue;
        auto& alias_image = texture_cache.GetImage(alias_id);
        if (alias_image.info.guest_address != addr)
            continue;
        if (alias_image.info.props.is_depth)
            continue;
        if (rt_image.info.size.width < alias_image.info.size.width)
            continue;
        if (rt_image.info.size.height < alias_image.info.size.height)
            continue;

        // Skip aliases that are themselves pending RTs — avoids RT↔RT feedback loops.
        auto pend_it = pending_rt_writes_.find(addr);
        if (pend_it != pending_rt_writes_.end() && pend_it->second == alias_id)
            continue;

        CopyRtToAlias(rt_image, alias_image);
    }
}

void RenderTargetSync::CopyRtToAlias(VideoCore::Image& rt_image, VideoCore::Image& alias_image) {
    const u32 copy_w = alias_image.info.size.width;
    const u32 copy_h = alias_image.info.size.height;

    if (rt_image.info.num_samples != alias_image.info.num_samples) {
        VideoCore::UniqueImage temp{instance.GetDevice(), instance.GetAllocator()};
        temp.Create({
            .flags =
                vk::ImageCreateFlagBits::eMutableFormat | vk::ImageCreateFlagBits::eExtendedUsage,
            .imageType = vk::ImageType::e2D,
            .format = rt_image.info.pixel_format,
            .extent = {copy_w, copy_h, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
        });

        bool needs_flush =
            runtime.Transit(&rt_image, vk::ImageLayout::eTransferSrcOptimal,
                            vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferRead);
        if (needs_flush) {
            runtime.FlushBarriers();
        }

        scheduler.EndRendering();
        auto cmdbuf = scheduler.CommandBuffer();
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
        needs_flush =
            runtime.Transit(&alias_image, vk::ImageLayout::eTransferDstOptimal,
                            vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferWrite);
        if (needs_flush) {
            runtime.FlushBarriers();
        }
        cmdbuf = scheduler.CommandBuffer();
        const vk::ImageCopy copy_region = {
            .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            .srcOffset = {0, 0, 0},
            .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            .dstOffset = {0, 0, 0},
            .extent = {copy_w, copy_h, 1},
        };
        cmdbuf.copyImage(temp, vk::ImageLayout::eTransferSrcOptimal, alias_image.GetImage(),
                         vk::ImageLayout::eTransferDstOptimal, copy_region);

        runtime.Transit(&rt_image, vk::ImageLayout::eColorAttachmentOptimal,
                        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                        vk::AccessFlagBits2::eColorAttachmentWrite);
        runtime.Transit(&alias_image, vk::ImageLayout::eShaderReadOnlyOptimal,
                        vk::PipelineStageFlagBits2::eFragmentShader |
                            vk::PipelineStageFlagBits2::eComputeShader,
                        vk::AccessFlagBits2::eShaderRead);
        runtime.FlushBarriers();
        scheduler.DeferOperation([temp = std::move(temp)]() mutable { temp.Destroy(); });
        return;
    }

    bool needs_flush =
        runtime.Transit(&rt_image, vk::ImageLayout::eTransferSrcOptimal,
                        vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferRead);
    needs_flush |=
        runtime.Transit(&alias_image, vk::ImageLayout::eTransferDstOptimal,
                        vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferWrite);
    if (needs_flush) {
        runtime.FlushBarriers();
    }
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
    runtime.Transit(&rt_image, vk::ImageLayout::eColorAttachmentOptimal,
                    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                    vk::AccessFlagBits2::eColorAttachmentWrite);
    runtime.Transit(&alias_image, vk::ImageLayout::eShaderReadOnlyOptimal,
                    vk::PipelineStageFlagBits2::eFragmentShader |
                        vk::PipelineStageFlagBits2::eComputeShader,
                    vk::AccessFlagBits2::eShaderRead);
    runtime.FlushBarriers();
}

} // namespace Vulkan