// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/memory.h"
#include "video_core/renderer_vulkan/vk_compute_download.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_platform.h"
#include "video_core/texture_cache/image.h"
#include "video_core/texture_cache/texture_cache.h"
#include <vk_mem_alloc.h>

namespace Vulkan {

namespace {
struct PendingDownload {
    vk::Fence fence{};
    VkBuffer staging_buffer{};
    VmaAllocation staging_alloc{};
    VmaAllocator vma_alloc{};
    u8* staging_ptr{};
    VAddr guest_base{};
    u32 img_width{};
    u32 bpp{};
    u32 min_x{};
    u32 min_y{};
    u32 max_x{};
    u32 max_y{};
};
} // anonymous namespace

struct ComputeDownloadManager::Impl {
    const Instance& instance;
    Scheduler& scheduler;
    VideoCore::TextureCache& texture_cache;
    std::vector<PendingDownload> pending_downloads;
};

ComputeDownloadManager::ComputeDownloadManager(const Instance& instance,
                                               Scheduler& scheduler,
                                               VideoCore::TextureCache& texture_cache)
    : m_impl(std::make_unique<Impl>(instance, scheduler, texture_cache)) {}

ComputeDownloadManager::~ComputeDownloadManager() {
    // Clean up any downloads that were never resolved (e.g. compute-only workloads).
    auto device = m_impl->instance.GetDevice();
    for (auto& dl : m_impl->pending_downloads) {
        if (dl.fence) {
            (void)device.waitForFences(dl.fence, true, UINT64_MAX);
            device.destroyFence(dl.fence);
        }
        vmaDestroyBuffer(dl.vma_alloc, dl.staging_buffer, dl.staging_alloc);
    }
}

bool ComputeDownloadManager::IsEnabled() {
    static const bool enabled = [] {
        const auto serial = Common::ElfInfo::Instance().GameSerial();
        const bool ok = serial == "CUSA01623" || serial == "CUSA01715" || serial == "CUSA01740";
        if (ok) {
            LOG_INFO(Render, "[ComputeDownload] Enabled for game {} (God of War III)", serial);
        }
        return ok;
    }();
    return enabled;
}

void ComputeDownloadManager::SyncOne(VideoCore::Image& storage_img, u32 grid_x, u32 grid_y) {
    // Guard checks.
    if (storage_img.info.guest_address == 0) {
        LOG_WARNING(Render, "[ComputeDownload] Storage image has null guest address, skip");
        return;
    }
    if (storage_img.info.num_samples > 1) return; // multisampled — expected, no download
    const u32 bpp = storage_img.info.num_bits / 8u;
    if (bpp == 0 || storage_img.info.size.width == 0 || storage_img.info.size.height == 0) {
        LOG_WARNING(Render, "[ComputeDownload] Invalid dimensions ({}×{} bpp={}), skip",
                    storage_img.info.size.width, storage_img.info.size.height, bpp);
        return;
    }

    // NOTE: We do NOT check whether a texture currently overlaps this storage image.
    // The typical pattern is "compute writes first, textures are created later at the
    // same address" (A/B double-buffer swap). Conservatively always download.
    //
    // This also assumes compute output is consumed by a graphics pipeline, not by
    // another compute dispatch binding a different ImageId at the same guest address.
    // The known CS (ccdebd80f02a77aa) does not exhibit that pattern.

    // Compute conservative dirty rect from dispatch grid dimensions.
    const u32 img_w = static_cast<u32>(storage_img.info.size.width);
    const u32 img_h = static_cast<u32>(storage_img.info.size.height);
    const u32 block_w = std::max<u32>(1, (img_w + grid_x - 1) / grid_x);
    const u32 block_h = std::max<u32>(1, (img_h + grid_y - 1) / grid_y);
    const u32 dirty_w = std::min<u32>(grid_x * block_w, img_w);
    const u32 dirty_h = std::min<u32>(grid_y * block_h, img_h);

    // Allocate staging buffer (VMA, persistently mapped).
    const u32 full_row_bytes = img_w * bpp;
    const u32 download_size = dirty_h * full_row_bytes;

    const VkBufferCreateInfo buffer_ci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = download_size,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VmaAllocationCreateInfo alloc_ci = {};
    alloc_ci.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    alloc_ci.usage = VMA_MEMORY_USAGE_AUTO;

    VmaAllocator vma_alloc = storage_img.backing->image.allocator;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo alloc_info{};
    VkBuffer staging_vk = VK_NULL_HANDLE;
    if (vmaCreateBuffer(vma_alloc, &buffer_ci, &alloc_ci, &staging_vk, &allocation,
                        &alloc_info) != VK_SUCCESS) {
        LOG_ERROR(Render, "[ComputeDownload] VMA staging allocation failed ({} bytes)",
                  download_size);
        return;
    }

    // Record GPU copy into current command buffer.
    const vk::BufferImageCopy copy_region = {
        .bufferOffset = 0,
        .bufferRowLength = img_w,
        .bufferImageHeight = 0,
        .imageSubresource =
            {
                .aspectMask = storage_img.aspect_mask & ~vk::ImageAspectFlagBits::eStencil,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        .imageOffset = {0, 0, 0},
        .imageExtent = {img_w, dirty_h, 1},
    };
    vk::Buffer staging_buffer{staging_vk};
    std::span copies{&copy_region, 1};
    storage_img.Download(copies, staging_buffer, 0, download_size);

    // Submit with fence (non-blocking).
    auto device = m_impl->instance.GetDevice();
    vk::Fence fence = Check(device.createFence({}));
    SubmitInfo info{};
    info.AddSignal(fence);
    m_impl->scheduler.Flush(info);

    // Store pending download.
    m_impl->pending_downloads.push_back({
        .fence = fence,
        .staging_buffer = staging_vk,
        .staging_alloc = allocation,
        .vma_alloc = vma_alloc,
        .staging_ptr = static_cast<u8*>(alloc_info.pMappedData),
        .guest_base = storage_img.info.guest_address,
        .img_width = img_w,
        .bpp = bpp,
        .min_x = 0,
        .min_y = 0,
        .max_x = dirty_w,
        .max_y = dirty_h,
    });

    // Set flags.
    storage_img.flags |= VideoCore::ImageFlagBits::GpuModified |
                         VideoCore::ImageFlagBits::ComputeWritten;
    storage_img.flags &= ~VideoCore::ImageFlagBits::Dirty;

    static constexpr size_t kMaxPendingDownloads = 10;
    if (m_impl->pending_downloads.size() >= kMaxPendingDownloads) {
        static bool overflow_logged = false;
        if (!overflow_logged) {
            LOG_ERROR(Render,
                      "[ComputeDownload] {} pending downloads without resolution — "
                      "no graphics draw between compute dispatches?",
                      m_impl->pending_downloads.size());
            overflow_logged = true;
        }
    }
}

void ComputeDownloadManager::ResolvePending() {
    if (m_impl->pending_downloads.empty()) return;

    auto device = m_impl->instance.GetDevice();
    // Process in reverse order: if multiple dispatches wrote to the same storage image,
    // the last dispatch (freshest data) is resolved first. Earlier downloads for the
    // same image fail validation and are discarded.
    for (size_t i = m_impl->pending_downloads.size(); i > 0; --i) {
        auto& dl = m_impl->pending_downloads[i - 1];

        // Wait for GPU copy to complete (fence is usually already signaled).
        if (dl.fence) {
            if (device.getFenceStatus(dl.fence) != vk::Result::eSuccess) {
                static bool logged = false;
                if (!logged) {
                    LOG_WARNING(Render,
                                "[ComputeDownload] Fence not ready, blocking on waitForFences "
                                "(async overlap insufficient between dispatch and draw)");
                    logged = true;
                }
            }
            (void)device.waitForFences(dl.fence, true, UINT64_MAX);
            device.destroyFence(dl.fence);
            dl.fence = nullptr;
        }

        // Verify the storage image still exists and still needs this download.
        const u64 range_size = static_cast<u64>(dl.max_y) * dl.img_width * dl.bpp;
        bool valid = false;
        m_impl->texture_cache.ForEachImageInRegion(
            dl.guest_base, range_size,
            [&](VideoCore::ImageId, VideoCore::Image& img) {
                if (True(img.flags & VideoCore::ImageFlagBits::ComputeWritten)) {
                    valid = true;
                    return true;
                }
                return false;
            });

        if (!valid) {
            LOG_WARNING(Render,
                        "[ComputeDownload] Discarded: image at guest={:#018x} evicted "
                        "or ComputeWritten cleared before memcpy",
                        dl.guest_base);
            vmaDestroyBuffer(dl.vma_alloc, dl.staging_buffer, dl.staging_alloc);
            continue;
        }

        // Per-row memcpy from staging buffer to guest memory.
        const u32 full_row_bytes = dl.img_width * dl.bpp;
        const u32 rect_w_bytes = (dl.max_x - dl.min_x) * dl.bpp;
        for (u32 row = dl.min_y; row < dl.max_y; ++row) {
            const u32 src_off = row * full_row_bytes + dl.min_x * dl.bpp;
            u8* dst = std::bit_cast<u8*>(dl.guest_base + src_off);
            std::memcpy(dst, dl.staging_ptr + src_off, rect_w_bytes);
        }

        vmaDestroyBuffer(dl.vma_alloc, dl.staging_buffer, dl.staging_alloc);
        m_impl->texture_cache.InvalidateMemoryRange(dl.guest_base, range_size);
    }
    m_impl->pending_downloads.clear();
}

} // namespace Vulkan
