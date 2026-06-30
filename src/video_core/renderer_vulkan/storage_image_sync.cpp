// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/memory.h"
#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/renderer_vulkan/storage_image_sync.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/texture_cache.h"

namespace Vulkan {

StorageImageSync::StorageImageSync(Scheduler& scheduler_, VideoCore::BufferCache& buffer_cache_,
                                   VideoCore::TextureCache& texture_cache_)
    : scheduler{scheduler_}, buffer_cache{buffer_cache_}, texture_cache{texture_cache_} {}

StorageImageSync::~StorageImageSync() = default;

void StorageImageSync::Sync(VideoCore::ImageId image_id) {
    auto& img = texture_cache.GetImage(image_id);
    const VAddr guest_addr = img.info.guest_address;
    if (guest_addr == 0)
        return;

    const u32 bpp = img.info.num_bits / 8u;
    const u32 row_length = img.info.pitch ? img.info.pitch : img.info.size.width;
    const u32 download_size = row_length * img.info.size.height * img.info.resources.layers * bpp;

    LOG_DEBUG(Render_Vulkan, "[StorageSync] guest={:#x} {}x{} layers={} bpp={} row_len={} size={}",
              guest_addr, img.info.size.width, img.info.size.height, img.info.resources.layers, bpp,
              row_length, download_size);

    // Transit to transfer-src for the copy.
    img.Transit(vk::ImageLayout::eTransferSrcOptimal, vk::AccessFlagBits2::eTransferRead, {});

    scheduler.EndRendering();
    auto cmdbuf = scheduler.CommandBuffer();

    // Map staging buffer and record copy.
    auto& download_buf = buffer_cache.GetUtilityBuffer(VideoCore::MemoryUsage::Download);
    const auto [data, offset] = download_buf.Map(download_size);
    if (!data) {
        LOG_ERROR(Render_Vulkan,
                  "[StorageSync] StreamBuffer Map failed for {}B — download SKIPPED, "
                  "texture corruption likely",
                  download_size);
        return;
    }

    boost::container::small_vector<vk::BufferImageCopy, 6> regions;
    const u32 layer_size = row_length * img.info.size.height * bpp;
    const u32 layers = img.info.resources.layers;
    for (u32 layer = 0; layer < layers; ++layer) {
        regions.push_back({
            .bufferOffset = layer * layer_size + offset,
            .bufferRowLength = row_length,
            .bufferImageHeight = 0,
            .imageSubresource{
                .aspectMask = img.aspect_mask & ~vk::ImageAspectFlagBits::eStencil,
                .mipLevel = 0,
                .baseArrayLayer = layer,
                .layerCount = 1,
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {img.info.size.width, img.info.size.height, 1},
        });
    }
    download_buf.Commit();

    cmdbuf.copyImageToBuffer(img.GetImage(), vk::ImageLayout::eTransferSrcOptimal,
                             download_buf.Handle(), regions);

    // Re-tile: CopyImageToBuffer always produces linear data, but guest memory
    // must hold tiled data so downstream detile works correctly.
    auto& tile_manager = texture_cache.GetTileManager();
    auto [tiled_buffer, tiled_offset] =
        tile_manager.TileLinearBuffer(download_buf.Handle(), offset, img.info);
    if (tiled_buffer != download_buf.Handle()) {
        // Tiler wrote to a scratch buffer; copy back to download buffer.
        const vk::BufferCopy tile_copy = {
            .srcOffset = tiled_offset,
            .dstOffset = offset,
            .size = download_size,
        };
        scheduler.CommandBuffer().copyBuffer(tiled_buffer, download_buf.Handle(), tile_copy);
    }

    // Submit, wait for GPU, then write to guest memory.
    scheduler.EndRendering();
    const u64 tick = scheduler.CurrentTick();
    scheduler.Wait(tick);

    // Mark consumers dirty before writing to guest (hash check needs old hash).
    texture_cache.InvalidateMemory(guest_addr, img.info.guest_size,
                                   /*exclude_image_id=*/image_id);
    Core::Memory::Instance()->TryWriteBacking(std::bit_cast<u8*>(guest_addr), data, download_size);
    // Notify buffer cache that guest memory has new data so SynchronizeBuffer picks it up.
    buffer_cache.MarkRegionAsCpuModified(guest_addr, download_size);
}

} // namespace Vulkan
