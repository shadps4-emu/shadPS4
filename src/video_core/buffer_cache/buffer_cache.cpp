// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <magic_enum/magic_enum.hpp>
#include "common/alignment.h"
#include "core/memory.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/buffer_cache/barrier_batch.h"
#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/buffer_cache/memory_tracker.h"
#include "video_core/renderer_vulkan/vk_graphics_pipeline.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/texture_cache.h"

#include <vk_mem_alloc.h>

namespace VideoCore {

static constexpr size_t DataShareBufferSize = 64_KB;
static constexpr size_t StagingBufferSize = 512_MB;
static constexpr size_t DownloadBufferSize = 32_MB;
static constexpr size_t UboStreamBufferSize = 64_MB;
static constexpr size_t DeviceBufferSize = 128_MB;

static constexpr auto ARENA_USAGE =
    vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst |
    vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
    vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
    vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;

BufferCache::BufferCache(const Vulkan::Instance& instance_, Vulkan::Scheduler& scheduler_,
                         AmdGpu::Liverpool* liverpool_, TextureCache& texture_cache_,
                         BarrierBatch& barriers_, PageManager& tracker)
    : instance{instance_}, scheduler{scheduler_}, liverpool{liverpool_},
      memory{Core::Memory::Instance()}, texture_cache{texture_cache_}, barriers{barriers_},
      memory_tracker{std::make_unique<MemoryTracker>(tracker)},
      staging_buffer{instance, scheduler, MemoryUsage::Upload, StagingBufferSize},
      stream_buffer{instance, scheduler, MemoryUsage::Stream, UboStreamBufferSize},
      download_buffer{instance, scheduler, MemoryUsage::Download, DownloadBufferSize},
      device_buffer{instance, scheduler, MemoryUsage::DeviceLocal, DeviceBufferSize},
      gds_buffer{instance, scheduler,           MemoryUsage::Stream, 0,
                 AllFlags, DataShareBufferSize, "GDS Buffer"},
      memory_semaphore{instance} {
    const vk::BufferCreateInfo probe_ci = {
        .flags =
            vk::BufferCreateFlagBits::eSparseBinding | vk::BufferCreateFlagBits::eSparseResidency,
        .size = ARENA_PAGE_SIZE,
        .usage = ARENA_USAGE,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    const vk::DeviceBufferMemoryRequirements req_info = {
        .pCreateInfo = &probe_ci,
    };
    const auto device = instance.GetDevice();
    const auto reqs = device.getBufferMemoryRequirements(req_info).memoryRequirements;
    block_size = Common::AlignUp(std::max<u64>(reqs.alignment, MIN_BLOCK_SIZE), reqs.alignment);
    ASSERT_MSG(std::popcount(block_size) == 1, "Sparse block size {} is not a power of 2",
               block_size);
    block_shift = std::bit_width(block_size) - 1;
    blocks_per_arena_page = ARENA_PAGE_SIZE / block_size;
    blocks_per_arena_page_shift = std::bit_width(blocks_per_arena_page) - 1;
    arena_memory_type_bits = reqs.memoryTypeBits;

    const u64 bda_pagetable_size =
        (blocks_per_arena_page * NUM_ARENA_PAGES) * sizeof(vk::DeviceAddress);
    fault_manager = std::make_unique<FaultManager>(instance, scheduler, *this, block_shift,
                                                   blocks_per_arena_page * NUM_ARENA_PAGES);
    bda_pagetable_buffer =
        std::make_unique<Buffer>(instance, scheduler, MemoryUsage::DeviceLocal, 0, AllFlags,
                                 bda_pagetable_size, "BDA Page Table Buffer");
    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.fillBuffer(bda_pagetable_buffer->Handle(), 0u, bda_pagetable_size, 0u);
    const vk::MemoryBarrier2 post_barrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eAllCommands,
        .dstAccessMask = vk::AccessFlagBits2::eMemoryRead,
    };
    cmdbuf.pipelineBarrier2(vk::DependencyInfo{
        .dependencyFlags = vk::DependencyFlagBits::eByRegion,
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &post_barrier,
    });

    scheduler.SetSubmitCallback(
        [this](Vulkan::SubmitInfo& info) { SubmitPendingArenaBinds(info); });
}

BufferCache::~BufferCache() {
    const auto device = instance.GetDevice();
    for (const auto& arena : arenas) {
        device.destroyBuffer(arena.buffer);
    }
}

void BufferCache::InvalidateMemory(VAddr device_addr, u64 size) {
    memory_tracker->InvalidateRegion(
        device_addr, size, [this, device_addr, size] { ReadMemory(device_addr, size, true); });
}

void BufferCache::ReadMemory(VAddr device_addr, u64 size, bool is_write) {
    liverpool->SendCommand<true>([this, device_addr, size, is_write] {
        const u32 first_block = device_addr >> block_shift;
        const u32 last_block = (device_addr + size - 1) >> block_shift;
        const Arena* arena = GetArena(first_block, last_block);

        // GPU-modified ranges come as many small scattered islands,
        // so the download is widened to a window around the request
        constexpr u64 WindowSize = 512_KB;
        const VAddr arena_start = arena->base_address;
        const VAddr arena_end = arena_start + (arena->num_pages << ARENA_PAGE_BITS);
        const VAddr window_start =
            std::max<VAddr>(Common::AlignDown(device_addr, WindowSize), arena_start);
        const VAddr window_end = std::min<VAddr>(
            std::max<VAddr>(window_start + WindowSize, device_addr + size), arena_end);
        DownloadMemory(arena, window_start, window_end - window_start);
        if (is_write) {
            memory_tracker->MarkRegionAsCpuModified(device_addr, size);
        }
    });
}

void BufferCache::DownloadMemory(const Arena* arena, VAddr device_addr, u64 size) {
    boost::container::small_vector<vk::BufferCopy, 1> copies;
    u64 total_size_bytes = 0;
    const VAddr arena_base = arena->base_address;
    memory_tracker->ForEachDownloadRange<false>(device_addr, size, [&](u64 address, u64 size) {
        const auto add_download = [&](VAddr start, VAddr end) {
            const u64 new_offset = start - arena_base;
            const u64 new_size = end - start;
            copies.push_back(vk::BufferCopy{
                .srcOffset = new_offset,
                .dstOffset = total_size_bytes,
                .size = new_size,
            });
            // Align up to avoid cache conflicts
            constexpr u64 align = 64ULL;
            constexpr u64 mask = ~(align - 1ULL);
            total_size_bytes += (new_size + align - 1) & mask;
        };
        gpu_modified_ranges.ForEachInRange(address, size, add_download);
        gpu_modified_ranges.Subtract(address, size);
    });
    if (total_size_bytes == 0) {
        return;
    }
    const auto [download, offset] = download_buffer.Map(total_size_bytes);
    for (auto& copy : copies) {
        copy.dstOffset += offset;
    }
    download_buffer.Commit();
    scheduler.EndRendering();
    const vk::MemoryBarrier2 pre_barrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eAllCommands,
        .srcAccessMask = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .dstAccessMask = vk::AccessFlagBits2::eTransferRead,
    };
    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.pipelineBarrier2(vk::DependencyInfo{
        .dependencyFlags = vk::DependencyFlagBits::eByRegion,
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &pre_barrier,
    });
    cmdbuf.copyBuffer(arena->buffer, download_buffer.Handle(), copies);
    scheduler.Finish();
    for (const auto& copy : copies) {
        auto* dst_addr = std::bit_cast<u8*>(arena_base + copy.srcOffset);
        memory->TryWriteBacking(dst_addr, download + (copy.dstOffset - offset), copy.size);
    }
    memory_tracker->UnmarkRegionAsGpuModified(device_addr, size);
}

std::pair<vk::Buffer, u64> BufferCache::ObtainBuffer(VAddr device_addr, u32 size, bool is_written,
                                                     bool is_texel_buffer) {
    // For read-only buffers use device local stream buffer to reduce renderpass breaks.
    if (!is_written && size <= STREAM_THRESHOLD && !IsRegionGpuModified(device_addr, size)) {
        const u64 offset = stream_buffer.Copy(device_addr, size, instance.UniformMinAlignment());
        return {stream_buffer.Handle(), offset};
    }
    const u64 first_block = device_addr >> block_shift;
    const u64 last_block = (device_addr + size - 1) >> block_shift;
    const Arena* arena = GetArena(first_block, last_block);
    EnsureResident(arena, first_block, last_block);
    SynchronizeMemory(arena, device_addr, size, is_written, is_texel_buffer);
    if (is_written) {
        gpu_modified_ranges.Add(device_addr, size);
    }
    return {arena->buffer, device_addr - arena->base_address};
}

std::pair<vk::Buffer, u64> BufferCache::ObtainBufferForImage(VAddr device_addr, u32 size) {
    if (IsRegionGpuModified(device_addr, size)) {
        return ObtainBuffer(device_addr, size, false);
    }
    const auto offset = staging_buffer.Copy(device_addr, size, instance.StorageMinAlignment());
    return {staging_buffer.Handle(), offset};
}

bool BufferCache::IsRegionCpuModified(VAddr addr, size_t size) {
    return memory_tracker->IsRegionCpuModified(addr, size);
}

bool BufferCache::IsRegionGpuModified(VAddr addr, size_t size) {
    return memory_tracker->IsRegionGpuModified(addr, size);
}

void BufferCache::ProcessFaultBuffer() {
    fault_manager->ProcessFaultBuffer();
}

void BufferCache::SynchronizeDmaBuffers() {
    for (const auto& range : resident_ranges) {
        const u64 page = range.start >> (ARENA_PAGE_BITS - block_shift);
        const VAddr device_addr = range.start << block_shift;
        const u64 size = (range.end - range.start) << block_shift;
        SynchronizeMemory(address_space[page], device_addr, size, false, false);
    }
}

BufferCache::Arena* BufferCache::CreateArena(u64 base_block, u64 num_pages) {
    const vk::BufferCreateInfo buffer_ci = {
        .flags =
            vk::BufferCreateFlagBits::eSparseBinding | vk::BufferCreateFlagBits::eSparseResidency,
        .size = num_pages * ARENA_PAGE_SIZE,
        .usage = ARENA_USAGE,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    const auto device = instance.GetDevice();
    const auto result = device.createBuffer(buffer_ci);
    if (result.result != vk::Result::eSuccess) {
        UNREACHABLE_MSG("Failed to create arena: base_block={}, num_pages={}, result={}",
                        base_block, num_pages, vk::to_string(result.result));
        return nullptr;
    }

    const vk::BufferDeviceAddressInfo addr_info = {
        .buffer = result.value,
    };

    auto* arena = &arenas.emplace_back();
    arena->buffer = result.value;
    arena->bda = device.getBufferAddress(addr_info);
    arena->base_address = base_block << block_shift;
    arena->num_pages = num_pages;

    LOG_INFO(Render, "Creating new arena base_address={:#x}, num_pages={}", arena->base_address,
             arena->num_pages);

    return arena;
}

const BufferCache::Arena* BufferCache::GetArena(u64 first_block, u64 last_block) {
    const u64 first_page = first_block >> blocks_per_arena_page_shift;
    const u64 last_page = last_block >> blocks_per_arena_page_shift;
    ASSERT_MSG(last_page - first_page <= 1,
               "Buffer request cannot span more than two VA arena pages");

    const Arena* first_arena = address_space[first_page];
    const Arena* last_arena = address_space[last_page];
    if (first_arena == last_arena) {
        if (!first_arena) {
            const u64 base_block = Common::AlignDownPow2<u64>(first_block, blocks_per_arena_page);
            const u64 num_pages = last_page - first_page + 1;
            const Arena* new_arena = CreateArena(base_block, num_pages);
            address_space[first_page] = new_arena;
            address_space[last_page] = new_arena;
        }
        return address_space[first_page];
    }

    LOG_WARNING(Render, "Migrating arena");

    const u64 base_block = first_arena->base_address >> ARENA_PAGE_BITS;
    const u64 num_pages = first_arena->num_pages + last_arena->num_pages;
    const u64 end_block = base_block + (num_pages << blocks_per_arena_page_shift);
    Arena* new_arena = CreateArena(base_block, num_pages);
    auto* bind = BindsForArena(new_arena);
    resident_ranges.ForEachInRange(base_block, end_block, [&](const Backing& backing) {
        const u64 start = std::max(base_block, backing.start);
        const u64 end = std::min(end_block, backing.end);
        bind->binds.push_back(vk::SparseMemoryBind{
            .resourceOffset = (start - base_block) << block_shift,
            .size = (end - start) << block_shift,
            .memory = backing.memory,
            .memoryOffset = backing.offset + ((start - backing.start) << block_shift),
        });
    });

    u64 base_page = first_arena->base_address >> ARENA_PAGE_BITS;
    for (u32 page = 0; page < first_arena->num_pages; ++page) {
        address_space[base_page + page] = new_arena;
    }
    base_page = last_arena->base_address >> ARENA_PAGE_BITS;
    for (u32 page = 0; page < last_arena->num_pages; ++page) {
        address_space[base_page + page] = new_arena;
    }
    return new_arena;
}

void BufferCache::EnsureResident(const Arena* arena, u64 first_block, u64 last_block) {
    ArenaBinds* bind{};
    u32 resident_blocks{};
    u32 existing_binds{};
    resident_ranges.ForEachGap(first_block, last_block + 1, [&](u64 start, u64 end) {
        if (!bind) {
            bind = BindsForArena(arena);
            existing_binds = bind->binds.size();
        }
        resident_blocks += end - start;
        auto& binds = bind->binds;
        binds.push_back(vk::SparseMemoryBind{
            .resourceOffset = start,
            .size = end - start,
        });
    });

    if (!bind || bind->binds.size() == existing_binds) {
        return;
    }

    const vk::MemoryRequirements memory_reqs = {
        .size = resident_blocks << block_shift,
        .alignment = block_size,
        .memoryTypeBits = arena_memory_type_bits,
    };
    const VkMemoryRequirements memory_reqs_unsafe = static_cast<VkMemoryRequirements>(memory_reqs);
    const VmaAllocationCreateInfo alloc_ci = {
        .flags = VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT,
        .usage = VMA_MEMORY_USAGE_UNKNOWN,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    VmaAllocation allocation{};
    VmaAllocationInfo alloc_info{};
    const auto result = vmaAllocateMemory(instance.GetAllocator(), &memory_reqs_unsafe, &alloc_ci,
                                          &allocation, &alloc_info);
    ASSERT_MSG(result == VK_SUCCESS, "Unable to allocate backing memory with error {}",
               vk::to_string(vk::Result{result}));

    boost::container::small_vector<vk::BufferCopy, 8> copies;
    auto [staging, offset] = staging_buffer.Map(resident_blocks * sizeof(vk::DeviceAddress));

    u64 memory_offset = alloc_info.offset;
    auto& binds = bind->binds;
    auto* bda_addrs = reinterpret_cast<vk::DeviceAddress*>(staging);
    for (u32 i = existing_binds; i < binds.size(); ++i) {
        Backing backing;
        backing.start = binds[i].resourceOffset;
        backing.end = binds[i].resourceOffset + binds[i].size;
        backing.memory = alloc_info.deviceMemory;
        backing.offset = memory_offset;
        resident_ranges.Add(backing);

        LOG_INFO(Render, "Making range start={}, end={} resident", backing.start, backing.end);

        binds[i].resourceOffset <<= block_shift;
        binds[i].resourceOffset -= arena->base_address;
        binds[i].size <<= block_shift;
        binds[i].memory = alloc_info.deviceMemory;
        binds[i].memoryOffset = memory_offset;
        memory_offset += binds[i].size;

        for (u32 block = 0; block < binds[i].size; block += block_size) {
            *(bda_addrs++) = arena->bda + binds[i].resourceOffset + block;
        }
        const u64 copy_size = (backing.end - backing.start) * sizeof(vk::DeviceAddress);
        copies.emplace_back(offset, backing.start * sizeof(vk::DeviceAddress), copy_size);
        offset += copy_size;
    }

    staging_buffer.Commit();
    scheduler.EndRendering();
    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.copyBuffer(staging_buffer.Handle(), bda_pagetable_buffer->Handle(), copies);
    const vk::MemoryBarrier2 post_barrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eAllCommands,
        .dstAccessMask = vk::AccessFlagBits2::eMemoryRead,
    };
    cmdbuf.pipelineBarrier2(vk::DependencyInfo{
        .dependencyFlags = vk::DependencyFlagBits::eByRegion,
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &post_barrier,
    });
}

bool BufferCache::SynchronizeMemory(const Arena* arena, VAddr device_addr, u32 size,
                                    bool is_written, bool is_texel_buffer) {
    boost::container::small_vector<vk::BufferCopy, 4> copies;
    size_t total_size_bytes = 0;
    vk::Buffer src_buffer = VK_NULL_HANDLE;
    memory_tracker->ForEachUploadRange(
        device_addr, size, is_written,
        [&](u64 addr, u64 size) {
            copies.emplace_back(total_size_bytes, addr, size);
            total_size_bytes += size;
        },
        [&] { src_buffer = UploadCopies(arena, copies, total_size_bytes); });

    if (src_buffer) {
        for (const auto& copy : copies) {
            barriers.IsRegionAccessed(arena->base_address + copy.dstOffset, size, true);
        }
        barriers.FlushBarriers(scheduler);
        scheduler.EndRendering();
        const auto cmdbuf = scheduler.CommandBuffer();
        cmdbuf.copyBuffer(src_buffer, arena->buffer, copies);
        for (const auto& copy : copies) {
            barriers.AccessMemory(arena->base_address + copy.dstOffset, copy.size,
                                  vk::PipelineStageFlagBits2::eTransfer,
                                  vk::AccessFlagBits2::eTransferWrite);
        }
    }
    if (is_texel_buffer && !is_written) {
        return SynchronizeMemoryFromImage(arena, device_addr, size);
    }
    return false;
}

vk::Buffer BufferCache::UploadCopies(const Arena* arena, std::span<vk::BufferCopy> copies,
                                     size_t total_size_bytes) {
    if (copies.empty()) {
        return VK_NULL_HANDLE;
    }
    const auto [staging, offset] = staging_buffer.Map(total_size_bytes);
    if (staging) {
        for (auto& copy : copies) {
            memory->CopySparseMemory(copy.dstOffset, staging + copy.srcOffset, copy.size);
            copy.srcOffset += offset;
            copy.dstOffset -= arena->base_address;
        }
        staging_buffer.Commit();
        return staging_buffer.Handle();
    } else {
        // For large one time transfers use a temporary host buffer.
        auto temp_buffer =
            std::make_unique<Buffer>(instance, scheduler, MemoryUsage::Upload, 0,
                                     vk::BufferUsageFlagBits::eTransferSrc, total_size_bytes);
        const vk::Buffer src_buffer = temp_buffer->Handle();
        u8* const staging = temp_buffer->mapped_data.data();
        for (auto& copy : copies) {
            memory->CopySparseMemory(copy.dstOffset, staging + copy.srcOffset, copy.size);
            copy.dstOffset -= arena->base_address;
        }
        scheduler.DeferOperation([buffer = std::move(temp_buffer)]() mutable { buffer.reset(); });
        return src_buffer;
    }
}

bool BufferCache::SynchronizeMemoryFromImage(const Arena* arena, VAddr device_addr, u32 size) {
    if (auto type = texture_cache.IsMeta(device_addr)) {
        if (*type == TextureCache::MetaType::HTile) {
            static constexpr u32 ZmaskUncompressed = 0xf;
            const u64 offset = device_addr - arena->base_address;
            barriers.IsRegionAccessedAndFlush(device_addr, size, scheduler);
            const auto cmdbuf = scheduler.CommandBuffer();
            cmdbuf.fillBuffer(arena->buffer, device_addr - arena->base_address, size,
                              ZmaskUncompressed);
            barriers.AccessMemory(device_addr, size, vk::PipelineStageFlagBits2::eComputeShader,
                                  vk::AccessFlagBits2::eShaderWrite);
            return true;
        } else {
            LOG_WARNING(Render_Vulkan, "Unhandled metadata type {}", magic_enum::enum_name(*type));
        }
    }
    const ImageId image_id = texture_cache.FindImageFromRange(device_addr, size);
    if (!image_id) {
        return false;
    }
    Image& image = texture_cache.GetImage(image_id);
    ASSERT_MSG(device_addr == image.info.guest_address,
               "Texel buffer aliases image subresources {:x} : {:x}", device_addr,
               image.info.guest_address);
    const u64 arena_size = arena->num_pages << ARENA_PAGE_BITS;
    const u64 arena_offset = image.info.guest_address - arena->base_address;
    boost::container::small_vector<vk::BufferImageCopy, 8> buffer_copies;
    u32 copy_size = 0;
    for (u32 mip = 0; mip < image.info.resources.levels; mip++) {
        const auto& mip_info = image.info.mips_layout[mip];
        const u32 width = std::max(image.info.size.width >> mip, 1u);
        const u32 height = std::max(image.info.size.height >> mip, 1u);
        const u32 depth = std::max(image.info.size.depth >> mip, 1u);
        if (arena_offset + mip_info.offset + mip_info.size > arena_size) {
            break;
        }
        buffer_copies.push_back(vk::BufferImageCopy{
            .bufferOffset = mip_info.offset,
            .bufferRowLength = mip_info.pitch,
            .bufferImageHeight = mip_info.height,
            .imageSubresource{
                .aspectMask = image.aspect_mask & ~vk::ImageAspectFlagBits::eStencil,
                .mipLevel = mip,
                .baseArrayLayer = 0,
                .layerCount = image.info.resources.layers,
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, depth},
        });
        copy_size += mip_info.size;
    }
    if (copy_size == 0) {
        return false;
    }
    barriers.IsRegionAccessedAndFlush(device_addr, copy_size, scheduler);
    auto& tile_manager = texture_cache.GetTileManager();
    tile_manager.TileImage(image, buffer_copies, arena->buffer, arena_offset, copy_size);
    barriers.AccessMemory(device_addr, copy_size, vk::PipelineStageFlagBits2::eComputeShader,
                          vk::AccessFlagBits2::eShaderWrite);
    return true;
}

void BufferCache::SubmitPendingArenaBinds(Vulkan::SubmitInfo& info) {
    if (pending_binds.empty()) {
        return;
    }

    std::vector<vk::SparseBufferMemoryBindInfo> buffer_binds;
    buffer_binds.reserve(pending_binds.size());

    for (const auto& binds : pending_binds) {
        buffer_binds.emplace_back(vk::SparseBufferMemoryBindInfo{
            .buffer = binds.arena->buffer,
            .bindCount = static_cast<u32>(binds.binds.size()),
            .pBinds = binds.binds.data(),
        });
    }

    const u64 signal_tick = memory_semaphore.NextTick();
    const auto signal_sema = memory_semaphore.Handle();

    const vk::TimelineSemaphoreSubmitInfo timeline_si = {
        .signalSemaphoreValueCount = 1u,
        .pSignalSemaphoreValues = &signal_tick,
    };

    const vk::BindSparseInfo sparse_info = {
        .pNext = &timeline_si,
        .bufferBindCount = static_cast<u32>(buffer_binds.size()),
        .pBufferBinds = buffer_binds.data(),
        .signalSemaphoreCount = 1u,
        .pSignalSemaphores = &signal_sema,
    };

    info.AddWait(signal_sema, signal_tick);
    auto submit_result = instance.GetGraphicsQueue().bindSparse(sparse_info);
    ASSERT_MSG(submit_result != vk::Result::eErrorDeviceLost, "Device lost during submit");

    pending_binds.clear();
}

} // namespace VideoCore
