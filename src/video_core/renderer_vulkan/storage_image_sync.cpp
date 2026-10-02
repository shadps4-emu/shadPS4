// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <unordered_set>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "video_core/bruno_diag.h"
#include "core/memory.h"
#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/renderer_vulkan/storage_image_sync.h"
#include "video_core/renderer_vulkan/vk_runtime.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/texture_cache.h"

namespace Vulkan {

StorageImageSync::StorageImageSync(Scheduler& scheduler_, Runtime& runtime_,
                                   VideoCore::BufferCache& buffer_cache_,
                                   VideoCore::TextureCache& texture_cache_)
    : scheduler{scheduler_}, runtime{runtime_}, buffer_cache{buffer_cache_},
      texture_cache{texture_cache_} {}

StorageImageSync::~StorageImageSync() = default;

bool StorageImageSync::HasAliasAtAddress(VAddr addr, VideoCore::ImageId self_id) const {
    const u64 page = addr >> VideoCore::TextureCache::Traits::PageBits;
    const auto& page_table = texture_cache.GetPageTable();
    const auto page_it = page_table.find(page);
    if (!page_it) {
        return false;
    }

    for (VideoCore::ImageId other_id : *page_it) {
        if (other_id == self_id) {
            continue;
        }
        auto& other_image = texture_cache.GetImage(other_id);
        if (other_image.info.guest_address == addr) {
            return true;
        }
    }
    return false;
}

void StorageImageSync::Sync(VideoCore::ImageId image_id) {
    const auto& serial = Common::ElfInfo::Instance().GameSerial();
    if (serial == "CUSA11227" || serial == "CUSA12982" || serial == "CUSA00093" ||
        serial == "CUSA00003" || serial == "CUSA01627" || serial == "CUSA01778" ||
        serial == "CUSA03173" || serial == "CUSA00900" || serial == "CUSA00208" ||
        serial == "CUSA01363" || serial == "CUSA01322" || serial == "CUSA003027" ||
        serial == "CUSA00299" || serial == "CUSA00207" || serial == "CUSA03014" ||
        serial == "CUSA03023" || serial == "CUSA03014" || serial == "CUSA00900" ||
        serial == "CUSA03388" || serial == "CUSA01589" || serial == "CUSA01760" ||
        serial == "CUSA07439" || serial == "CUSA07339" || serial == "CUSA08692" ||
        serial == "CUSA08495" || serial == "CUSA50617" || serial == "CUSA18723" ||
        serial == "CUSA28863" || serial == "CUSA00093" || serial == "CUSA00003") {
        return;
    }
    auto& img = texture_cache.GetImage(image_id);
    const VAddr guest_addr = img.info.guest_address;
    if (guest_addr == 0) {
        return;
    }

    const bool disable_alias_check =
        serial == "CUSA01623" || serial == "CUSA01715" || serial == "CUSA01740";

    if (!disable_alias_check && !HasAliasAtAddress(guest_addr, image_id)) {
        return;
    }

    const u32 bpp = img.info.num_bits / 8u;
    const u32 row_length = img.info.pitch ? img.info.pitch : img.info.size.width;
    const u32 download_size = row_length * img.info.size.height * img.info.resources.layers * bpp;
    const u32 write_back_size =
        img.info.props.is_tiled ? std::max(download_size, img.info.guest_size) : download_size;

    LOG_DEBUG(Render_Vulkan,
              "[StorageSync] guest={:#x} {}x{} layers={} bpp={} row_len={} size={} "
              "write_back_size={}",
              guest_addr, img.info.size.width, img.info.size.height, img.info.resources.layers, bpp,
              row_length, download_size, write_back_size);

    if (img.info.size.width >= 960 && img.info.num_bits == 32 && BrunoDiag::On()) {
        LOG_INFO(Render_Vulkan,
                 "[BRUNO-EV] STORAGE_SYNC addr={:#x} id={} {}x{} tiled={} fmt={} write_back={} "
                 "guest_size={}",
                 guest_addr, image_id.index, img.info.size.width, img.info.size.height,
                 u32(img.info.props.is_tiled), vk::to_string(img.info.pixel_format),
                 write_back_size, img.info.guest_size);
    }

    // BRUNO-DIAG: describe each distinct synced image (and its aliases) once.
    {
        static std::unordered_set<u64> seen;
        const auto describe = [](const char* tag, const VideoCore::Image& i) {
            const auto& m = i.info.mips_layout[0];
            LOG_INFO(Render_Vulkan,
                     "[BRUNO] {} guest={:#x} {}x{}x{} fmt={} bits={} samples={} tiled={} "
                     "tile_mode={} array_mode={} pitch={} guest_size={} mips={} layers={} "
                     "mip0(size={} pitch={} height={} off={}) flags={:#x}",
                     tag, i.info.guest_address, i.info.size.width, i.info.size.height,
                     i.info.size.depth, vk::to_string(i.info.pixel_format), i.info.num_bits,
                     i.info.num_samples, u32(i.info.props.is_tiled), u32(i.info.tile_mode),
                     u32(i.info.array_mode), i.info.pitch, i.info.guest_size,
                     i.info.resources.levels, i.info.resources.layers, m.size, m.pitch, m.height,
                     m.offset, u32(i.flags));
        };
        const u64 key = guest_addr ^ (u64(img.info.size.width) << 40) ^
                        (u64(img.info.size.height) << 52) ^ (u64(img.info.tile_mode) << 34);
        if (seen.insert(key).second) {
            describe("SYNC ", img);
            const u64 page = guest_addr >> VideoCore::TextureCache::Traits::PageBits;
            if (const auto page_it = texture_cache.GetPageTable().find(page); page_it) {
                for (VideoCore::ImageId other_id : *page_it) {
                    if (other_id == image_id) {
                        continue;
                    }
                    auto& other = texture_cache.GetImage(other_id);
                    if (other.info.guest_address == guest_addr) {
                        describe("ALIAS", other);
                    }
                }
            }
        }
    }

    const auto download = runtime.GetStagingPool().Request(
        std::max<u64>(write_back_size, img.info.guest_size), VideoCore::MemoryType::HostCached);
    if (!download.mapped) {
        LOG_ERROR(Render_Vulkan,
                  "[StorageSync] Staging map failed for {}B — download SKIPPED, "
                  "texture corruption likely",
                  write_back_size);
        return;
    }

    boost::container::small_vector<vk::BufferImageCopy, 6> regions;
    const u32 layer_size = row_length * img.info.size.height * bpp;
    const u32 layers = img.info.resources.layers;
    for (u32 layer = 0; layer < layers; ++layer) {
        regions.push_back({
            .bufferOffset = layer * layer_size,
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

    texture_cache.GetTileManager().TileImage(img, regions, download.buffer, download.offset);

    scheduler.Finish();
    download.Invalidate();

    texture_cache.InvalidateMemory(guest_addr, img.info.guest_size,
                                   /*exclude_image_id=*/image_id);
    Core::Memory::Instance()->TryWriteBacking(std::bit_cast<u8*>(guest_addr), download.mapped,
                                              write_back_size);
    buffer_cache.MarkRegionAsCpuModified(guest_addr, write_back_size);
}

} // namespace Vulkan
