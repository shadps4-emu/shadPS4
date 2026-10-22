// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <magic_enum/magic_enum.hpp>
#include <xxhash.h>

#include "common/alignment.h"
#include "core/debug_state.h"
#include "core/memory.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/buffer_cache/buffer.h"
#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/buffer_cache/memory_tracker.h"
#include "video_core/buffer_cache/region_definitions.h"
#include "video_core/renderer_vulkan/vk_graphics_pipeline.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_runtime.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/texture_cache.h"

#include <vk_mem_alloc.h>

namespace VideoCore {

static constexpr size_t GDS_BUFFER_SIZE = 64_KB;
static constexpr size_t NULL_INDEX_BUFFER_SIZE = 16;
static constexpr size_t STREAM_BUFFER_SIZE = 128_MB;
constexpr std::optional<u32> CmaskColorExpandedValue(u32 num_samples) {
    switch (num_samples) {
    case 1:
        return 0xFFFFFFFF;
    case 2:
        return 0xDDDDDDDD;
    case 4:
        return 0xEEEEEEEE;
    case 8:
        return 0xFFFFFFFF;
    default:
        return 0;
    }
}
static_assert(CmaskColorExpandedValue(1) == 0xFFFFFFFF);
static_assert(CmaskColorExpandedValue(2) == 0xDDDDDDDD);
static_assert(CmaskColorExpandedValue(4) == 0xEEEEEEEE);
static_assert(CmaskColorExpandedValue(8) == 0xFFFFFFFF);

static constexpr auto ARENA_USAGE =
    vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst |
    vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
    vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
    vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;

std::optional<u32> FindMemoryType(const vk::PhysicalDeviceMemoryProperties& properties,
                                  vk::MemoryPropertyFlags wanted, u32 memory_type_bits) {
    for (u32 i = 0; i < properties.memoryTypeCount; ++i) {
        if (((memory_type_bits >> i) & 1) == 0) {
            continue;
        }
        const auto flags = properties.memoryTypes[i].propertyFlags;
        if ((flags & wanted) == wanted) {
            return i;
        }
    }
    return std::nullopt;
}

BufferCache::BufferCache(const Vulkan::Instance& instance_, Vulkan::Scheduler& scheduler_,
                         Vulkan::Runtime& runtime_, AmdGpu::Liverpool* liverpool_,
                         TextureCache& texture_cache_, PageManager& tracker)
    : instance{instance_}, scheduler{scheduler_}, runtime{runtime_},
      staging_pool{runtime_.GetStagingPool()}, liverpool{liverpool_},
      memory{Core::Memory::Instance()}, texture_cache{texture_cache_},
      memory_tracker{std::make_unique<MemoryTracker>(tracker)},
      stream_buffer{instance, scheduler, MemoryType::Stream, STREAM_BUFFER_SIZE},
      gds_buffer{instance, 0, GDS_BUFFER_SIZE, MemoryType::Stream, "GDS Buffer"},
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
    blocks_per_arena_page_shift = ARENA_PAGE_BITS - block_shift;
    arena_memory_type_index =
        FindMemoryType(instance.GetMemoryProperties(), vk::MemoryPropertyFlagBits::eDeviceLocal,
                       reqs.memoryTypeBits)
            .value();

    static_assert(BlockTableTraits::ADDRESS_SPACE_BITS == ADDRESS_SPACE_BITS);
    static_assert((u64{1} << BlockTableTraits::PAGE_BITS) == MIN_BLOCK_SIZE);

    // Sparse block memory is sub-allocated from a dedicated pool. Every block gets an allocation
    // handle of its own, but they all share a few large VkDeviceMemory blocks (which VMA grows
    // geometrically since no block size is given). Only buffers are bound to this memory, so the
    // buffer image granularity can be ignored.
    const VmaPoolCreateInfo pool_info = {
        .memoryTypeIndex = arena_memory_type_index,
        .flags = VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
        .blockSize = 0,
        .minBlockCount = 0,
        .maxBlockCount = 0,
        .priority = 0.5f,
        .minAllocationAlignment = block_size,
        .pMemoryAllocateNext = nullptr,
    };
    const VkResult pool_result = vmaCreatePool(instance.GetAllocator(), &pool_info, &sparse_pool);
    ASSERT_MSG(pool_result == VK_SUCCESS, "Failed to create sparse block pool: {}",
               vk::to_string(vk::Result{pool_result}));

    const u64 bda_pagetable_size =
        (blocks_per_arena_page * NUM_ARENA_PAGES) * sizeof(vk::DeviceAddress);
    fault_manager = std::make_unique<FaultManager>(instance, scheduler, *this, block_shift,
                                                   blocks_per_arena_page * NUM_ARENA_PAGES);
    bda_pagetable_buffer = std::make_unique<Buffer>(
        instance, 0, bda_pagetable_size, MemoryType::DeviceLocal, "BDA Page Table Buffer");
    runtime.FillBuffer(bda_pagetable_buffer.get(), 0u, bda_pagetable_size, 0u);
    if (!instance.IsMaintenance6Supported()) {
        null_index_buffer = std::make_unique<Buffer>(instance, 0, NULL_INDEX_BUFFER_SIZE,
                                                     MemoryType::DeviceLocal, "Null Index Buffer");
        runtime.FillBuffer(null_index_buffer.get(), 0u, NULL_INDEX_BUFFER_SIZE, 0u);
    }
}

BufferCache::~BufferCache() {
    // Hand every block back to VMA. The arenas are destroyed first so that the buffers go away
    // before the memory bound to them does.
    std::vector<VmaAllocation> allocations;
    for (const Arena& arena : arenas) {
        const ResidencyBitset& residency = arena.residency;
        residency.ForEachResident(
            residency.FirstBlock(), residency.EndBlock(), [&](u64 first_block, u64 end_block) {
                for (u64 block = first_block; block < end_block; ++block) {
                    if (const VmaAllocation allocation = BlockAllocation(block)) {
                        allocations.push_back(allocation);
                    }
                }
            });
    }
    arenas.clear();
    vmaFreeMemoryPages(instance.GetAllocator(), allocations.size(), allocations.data());
    vmaDestroyPool(instance.GetAllocator(), sparse_pool);
}

void BufferCache::TickFrame() {
    if (std::exchange(fault_process_pending, false)) {
        fault_manager->ProcessFaultBuffer();
    }
}

void BufferCache::InvalidateMemory(VAddr device_addr, u64 size, bool assume_locks) {
    memory_tracker->InvalidateRegion(device_addr, size, [this, device_addr, size, assume_locks] {
        ReadMemory(device_addr, size, true, false, assume_locks);
    });
}

void BufferCache::UnmarkRegionAsGpuModified(VAddr device_addr, u64 size, bool is_write) {
    memory_tracker->UnmarkRegionAsGpuModified(device_addr, size, is_write);
    gpu_modified_ranges.Subtract(device_addr, size);
}

void BufferCache::ReadMemory(VAddr device_addr, u64 size, bool is_write, bool async,
                             bool assume_locks) {
    const auto flush_request = [this, device_addr, size, is_write, async] {
        const u32 first_block = device_addr >> block_shift;
        const u32 last_block = (device_addr + size - 1) >> block_shift;
        const auto* arena = GetArena(first_block, last_block);

        // GPU-modified ranges come as many small scattered islands,
        // so the download is widened to a window around the request
        constexpr u64 WindowSize = 512_KB;
        const VAddr arena_end = arena->cpu_addr + arena->size_bytes;
        const VAddr window_start =
            std::max<VAddr>(Common::AlignDown(device_addr, WindowSize), arena->cpu_addr);
        const VAddr window_end = std::min<VAddr>(
            std::max<VAddr>(window_start + WindowSize, device_addr + size), arena_end);
        DownloadMemory(arena, window_start, window_end - window_start, async);
        if (is_write) {
            memory_tracker->MarkRegionAsCpuModified(device_addr, size);
        }
    };
    if (assume_locks) {
        flush_request();
    } else {
        liverpool->SendCommand<true>(std::move(flush_request));
    }
}

void BufferCache::DownloadMemory(const Buffer* arena, VAddr device_addr, u64 size, bool async) {
    boost::container::small_vector<vk::BufferCopy, 1> copies;
    u64 total_size_bytes = 0;
    const VAddr arena_base = arena->cpu_addr;
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
    const auto download =
        staging_pool.Request(total_size_bytes, VideoCore::MemoryType::HostCached, 16, async);
    for (auto& copy : copies) {
        copy.dstOffset += download.offset;
    }
    runtime.CopyBuffer(arena, download.buffer, copies);

    const auto write_data = [this, arena_base, device_addr, size, download, async,
                             copies = std::move(copies)] {
        download.Invalidate();
        for (const auto& copy : copies) {
            auto* dst_addr = std::bit_cast<u8*>(arena_base + copy.srcOffset);
            memory->TryWriteBacking(dst_addr, download.mapped + (copy.dstOffset - download.offset),
                                    copy.size);
        }
        memory_tracker->UnmarkRegionAsGpuModified(device_addr, size, false);

        if (async) {
            staging_pool.FreeDeferred(download);
        }
    };

    if (async) {
        scheduler.DeferOperation(std::move(write_data));
    } else {
        scheduler.Finish();
        write_data();
    }
}

std::pair<const Buffer*, u64> BufferCache::ObtainBuffer(VAddr device_addr, u32 size,
                                                        ObtainBufferFlags flags) {
    const bool is_written = True(flags & ObtainBufferFlags::IsWritten);
    const bool is_texel_buffer = True(flags & ObtainBufferFlags::IsTexelBuffer);
    const bool skip_stream_buffer = True(flags & ObtainBufferFlags::IgnoreStreamBuffer);
    // For read-only buffers use device local stream buffer to reduce renderpass breaks.
    if (!is_written && !skip_stream_buffer && size <= STREAM_THRESHOLD &&
        !IsRegionGpuModified(device_addr, size)) {
        const auto [data, offset] = stream_buffer.Map(size, instance.UniformMinAlignment());
        memory->CopySparseMemory(device_addr, data, size);
        stream_buffer.Commit();
        return {&stream_buffer, offset};
    }
    const u64 first_block = device_addr >> block_shift;
    const u64 last_block = (device_addr + size - 1) >> block_shift;
    auto* arena = GetArena(first_block, last_block);
    EnsureResident(arena, first_block, last_block);
    if (is_written) {
        texture_cache.MarkAsMaybeGpuDirty(device_addr, size);
    }
    SynchronizeMemory(arena, device_addr, size, is_written, is_texel_buffer);
    if (is_written) {
        gpu_modified_ranges.Add(device_addr, size);
    }
    return {arena, arena->Offset(device_addr)};
}

std::pair<const Buffer*, u64> BufferCache::ObtainBufferForImage(VAddr device_addr, u32 size) {
    if (IsRegionGpuModified(device_addr, size)) {
        return ObtainBuffer(device_addr, size);
    }
    const auto staging = staging_pool.Request(size, VideoCore::MemoryType::HostUncached,
                                              instance.StorageMinAlignment());
    memory->CopySparseMemory(device_addr, staging.mapped, staging.size);
    staging.Flush();
    return {staging.buffer, staging.offset};
}

bool BufferCache::IsRegionCpuModified(VAddr addr, size_t size) {
    return memory_tracker->IsRegionCpuModified(addr, size);
}

bool BufferCache::IsRegionGpuModified(VAddr addr, size_t size) {
    return memory_tracker->IsRegionGpuModified(addr, size);
}

void BufferCache::MarkRegionAsGpuModified(VAddr addr, size_t size) {
    texture_cache.MarkAsMaybeGpuDirty(addr, size);
    memory_tracker->MarkRegionAsGpuModified(addr, size);
    gpu_modified_ranges.Add(addr, size);
}

void BufferCache::ReadEdgeImagePages(const Image& image) {
    // May happen that after downloading the image and invalidating region,
    // that there were GPU modified ranges that are lost due to CPU reuploading.
    // This doesn't change tracker state and it is spected to call DownloadImageMemory after this.
    const VAddr image_addr = image.info.guest_address;
    const u64 image_size = image.info.guest_size;
    const VAddr image_end = image_addr + image_size;
    const VAddr page_start = PageManager::GetPageAddr(image_addr);
    const VAddr page_end = PageManager::GetNextPageAddr(image_end - 1);
    boost::container::small_vector<vk::BufferCopy, 2> copies;
    u64 total_size_bytes = 0;
    const auto [buffer, offset] = ObtainBuffer(page_start, page_end - page_start);
    const auto add_download = [&](VAddr start, VAddr end) {
        const u64 new_offset = start - buffer->CpuAddr();
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
    gpu_modified_ranges.ForEachInRange(page_start, image_addr - page_start, add_download);
    gpu_modified_ranges.ForEachInRange(image_end, page_end - image_end, add_download);
    gpu_modified_ranges.Subtract(page_start, page_end - page_start);
    if (total_size_bytes == 0) {
        return;
    }
    const auto download =
        staging_pool.Request(total_size_bytes, VideoCore::MemoryType::HostCached, 16, true);
    for (auto& copy : copies) {
        // Modify copies to have the staging offset in mind
        copy.dstOffset += download.offset;
    }
    scheduler.EndRendering();
    runtime.CopyBuffer(buffer, download.buffer, copies);
    scheduler.DeferOperation(
        [this, base = buffer->CpuAddr(), download, copies = std::move(copies)]() {
            download.Invalidate();
            auto* memory = Core::Memory::Instance();
            for (const auto& copy : copies) {
                const VAddr copy_device_addr = base + copy.srcOffset;
                const u64 dst_offset = copy.dstOffset - download.offset;
                memory->TryWriteBacking(std::bit_cast<u8*>(copy_device_addr),
                                        download.mapped + dst_offset, copy.size);
            }
            staging_pool.FreeDeferred(download);
        });
}

void BufferCache::SynchronizeDmaBuffers() {
    fault_process_pending = true;
    // Shaders using DMA read the BDA page table, so queued entries have to be in it by now.
    FlushPendingPageTable();

    // SynchronizeMemory takes a 32 bit size, so runs longer than 1 GiB are split up.
    const u64 chunk_blocks = u64{1_GB} >> block_shift;
    for (Arena& arena : arenas) {
        // Arenas that were migrated into a bigger one are stale and have no blocks left.
        const ResidencyBitset& residency = arena.residency;
        residency.ForEachResident(
            residency.FirstBlock(), residency.EndBlock(), [&](u64 first_block, u64 end_block) {
                for (u64 block = first_block; block < end_block; block += chunk_blocks) {
                    const u64 chunk_end = std::min(end_block, block + chunk_blocks);
                    SynchronizeMemory(&arena, block << block_shift,
                                      static_cast<u32>((chunk_end - block) << block_shift), false,
                                      false);
                }
            });
    }
}

BufferCache::Arena* BufferCache::GetArena(u64 first_block, u64 last_block) {
    const u64 first_page = first_block >> blocks_per_arena_page_shift;
    const u64 last_page = last_block >> blocks_per_arena_page_shift;
    ASSERT_MSG(last_page - first_page <= 1,
               "Buffer request cannot span more than two VA arena pages");

    Arena* first_arena = address_space[first_page];
    Arena* last_arena = address_space[last_page];
    if (first_arena == last_arena) {
        if (!first_arena) {
            const u64 base_block = Common::AlignDownPow2<u64>(first_block, blocks_per_arena_page);
            const u64 num_pages = last_page - first_page + 1;
            Arena* new_arena = &arenas.emplace_back(instance, base_block << block_shift,
                                                    num_pages << ARENA_PAGE_BITS, block_shift);
            address_space[first_page] = new_arena;
            address_space[last_page] = new_arena;
        }
        return address_space[first_page];
    }

    LOG_WARNING(Render, "Migrating arena");

    // Blocks that were only queued so far need their memory before they can be rebound.
    AllocatePendingPages();

    const u64 first_addr = first_arena ? first_arena->cpu_addr : (first_page << ARENA_PAGE_BITS);
    const u64 first_size = first_arena ? first_arena->size_bytes : ARENA_PAGE_SIZE;
    const u64 last_size = last_arena ? last_arena->size_bytes : ARENA_PAGE_SIZE;
    const u64 total_size = first_size + last_size;

    Arena* new_arena = &arenas.emplace_back(instance, first_addr, total_size, block_shift);
    ArenaBinds* bind = BindsForArena(new_arena);
    const auto migrate = [&](Arena* old_arena) {
        if (!old_arena) {
            return;
        }
        ResidencyBitset& old_residency = old_arena->residency;
        old_residency.ForEachResident(
            old_residency.FirstBlock(), old_residency.EndBlock(), [&](u64 start, u64 end) {
                for (u64 block = start; block < end; ++block) {
                    const VmaAllocation allocation = BlockAllocation(block);
                    ASSERT_MSG(allocation, "Resident block {} has no memory", block);
                    VmaAllocationInfo info;
                    vmaGetAllocationInfo(instance.GetAllocator(), allocation, &info);
                    AppendBind(*bind, (block << block_shift) - new_arena->cpu_addr, block_size,
                               vk::DeviceMemory{info.deviceMemory}, info.offset);
                }
            });
        new_arena->residency.Merge(old_residency);
        old_residency = ResidencyBitset{};
    };
    migrate(first_arena);
    migrate(last_arena);

    u64 base_page = first_addr >> ARENA_PAGE_BITS;
    for (u32 page = 0; page < (first_size >> ARENA_PAGE_BITS); ++page) {
        address_space[base_page + page] = new_arena;
    }
    base_page = last_page;
    for (u32 page = 0; page < (last_size >> ARENA_PAGE_BITS); ++page) {
        address_space[base_page + page] = new_arena;
    }
    return new_arena;
}

void BufferCache::EnsureResident(Arena* arena, u64 first_block, u64 last_block) {
    ResidencyBitset& residency = arena->residency;
    const u64 end_block = last_block + 1;

    // Hot path, this runs for every buffer of every draw. Nearly always the whole range is
    // already resident, which costs a handful of bit tests and no allocation.
    if (residency.FindNextGap(first_block, end_block) == end_block) [[likely]] {
        return;
    }

    boost::container::small_vector<std::pair<u64, u64>, 8> gaps;
    residency.ForEachGap(first_block, end_block,
                         [&](u64 start, u64 end) { gaps.emplace_back(start, end); });
    for (const auto& [start, end] : gaps) {
        LOG_INFO(Render, "Making range start={}, end={} resident", start, end);
        residency.Set(start, end);
        QueueResidentBlocks(arena, start, end);
    }
}

void BufferCache::QueueResidentBlocks(Arena* arena, u64 first_block, u64 end_block) {
    // Neighbouring requests are usually contiguous, so grow the previous entries when possible.
    if (!pending_pages.empty() && pending_pages.back().arena == arena &&
        pending_pages.back().end_block == first_block) {
        pending_pages.back().end_block = end_block;
    } else {
        pending_pages.push_back({arena, first_block, end_block});
    }
    pending_page_count += end_block - first_block;

    const u64 bda_base = arena->BufferDeviceAddress() - arena->cpu_addr;
    if (!pending_table_writes.empty() && pending_table_writes.back().bda_base == bda_base &&
        pending_table_writes.back().end_block == first_block) {
        pending_table_writes.back().end_block = end_block;
    } else {
        pending_table_writes.push_back({first_block, end_block, bda_base});
    }
}

void BufferCache::FlushPendingPageTable() {
    if (pending_table_writes.empty()) {
        return;
    }

    u64 total_blocks{};
    for (const auto& write : pending_table_writes) {
        total_blocks += write.end_block - write.first_block;
    }

    const auto staging =
        staging_pool.Request(total_blocks * sizeof(vk::DeviceAddress), MemoryType::HostUncached);
    auto* entries = reinterpret_cast<vk::DeviceAddress*>(staging.mapped);

    boost::container::small_vector<vk::BufferCopy, 8> copies;
    u64 src_offset = staging.offset;
    for (const auto& write : pending_table_writes) {
        for (u64 block = write.first_block; block < write.end_block; ++block) {
            *(entries++) = write.bda_base + (block << block_shift);
        }
        const u64 dst_offset = write.first_block * sizeof(vk::DeviceAddress);
        const u64 size = (write.end_block - write.first_block) * sizeof(vk::DeviceAddress);
        // Source data is laid out back to back, so only the destination has to be contiguous.
        if (!copies.empty() && copies.back().dstOffset + copies.back().size == dst_offset) {
            copies.back().size += size;
        } else {
            copies.emplace_back(src_offset, dst_offset, size);
        }
        src_offset += size;
    }

    staging.Flush();
    runtime.CopyBuffer(staging.buffer, bda_pagetable_buffer.get(), copies);
    pending_table_writes.clear();
}

void BufferCache::AllocatePendingPages() {
    if (pending_pages.empty()) {
        return;
    }

    const size_t count = pending_page_count;
    page_handles.resize(count);
    page_infos.resize(count);

    // All pages share the same parameters. VMA reads a single requirements and create info
    // struct and applies them to every allocation of the call.
    const VkMemoryRequirements requirements = {
        .size = block_size,
        .alignment = block_size,
        .memoryTypeBits = 1u << arena_memory_type_index,
    };
    const VmaAllocationCreateInfo create_info = {
        .flags = 0,
        .usage = VMA_MEMORY_USAGE_UNKNOWN,
        .requiredFlags = 0,
        .preferredFlags = 0,
        .memoryTypeBits = 0,
        .pool = sparse_pool,
        .pUserData = nullptr,
        .priority = 0.0f,
    };
    const VkResult result =
        vmaAllocateMemoryPages(instance.GetAllocator(), &requirements, &create_info, count,
                               page_handles.data(), page_infos.data());
    ASSERT_MSG(result == VK_SUCCESS, "Failed to allocate {} sparse blocks: {}", count,
               vk::to_string(vk::Result{result}));

    size_t index = 0;
    for (const PendingPages& range : pending_pages) {
        ArenaBinds* binds = BindsForArena(range.arena);
        for (u64 block = range.first_block; block < range.end_block; ++block, ++index) {
            const VmaAllocationInfo& info = page_infos[index];
            block_allocations[block] = page_handles[index];
            AppendBind(*binds, (block << block_shift) - range.arena->cpu_addr, block_size,
                       vk::DeviceMemory{info.deviceMemory}, info.offset);
        }
    }
    pending_pages.clear();
    pending_page_count = 0;

    // Don't keep a huge scratch allocation alive after an unusually large request.
    constexpr size_t MAX_SCRATCH_PAGES = 1 << 14;
    if (count > MAX_SCRATCH_PAGES) {
        std::vector<VmaAllocation>().swap(page_handles);
        std::vector<VmaAllocationInfo>().swap(page_infos);
    }
}

VmaAllocation BufferCache::BlockAllocation(u64 block) const {
    const VmaAllocation* slot = block_allocations.find(block);
    return slot ? *slot : VmaAllocation{};
}

void BufferCache::AppendBind(ArenaBinds& binds, u64 resource_offset, u64 size,
                             vk::DeviceMemory memory, u64 memory_offset) {
    if (!binds.binds.empty()) {
        vk::SparseMemoryBind& last = binds.binds.back();
        if (last.memory == memory && last.resourceOffset + last.size == resource_offset &&
            last.memoryOffset + last.size == memory_offset) {
            last.size += size;
            return;
        }
    }
    binds.binds.push_back(vk::SparseMemoryBind{
        .resourceOffset = resource_offset,
        .size = size,
        .memory = memory,
        .memoryOffset = memory_offset,
    });
}

bool BufferCache::SynchronizeMemory(const Buffer* arena, VAddr device_addr, u32 size,
                                    bool is_written, bool is_texel_buffer) {
    boost::container::small_vector<vk::BufferCopy, 4> copies;
    size_t total_size_bytes{};
    memory_tracker->ForEachUploadRange(device_addr, size, is_written, [&](u64 addr, u64 size) {
        copies.emplace_back(total_size_bytes, addr, size);
        total_size_bytes += size;
    });
    if (!copies.empty()) {
        const auto staging = staging_pool.Request(total_size_bytes, MemoryType::HostUncached);
        for (auto& copy : copies) {
            memory->CopySparseMemory(copy.dstOffset, staging.mapped + copy.srcOffset, copy.size);
            copy.srcOffset += staging.offset;
            copy.dstOffset -= arena->cpu_addr;
        }
        staging.Flush();
        runtime.CopyBuffer(staging.buffer, arena, copies);
    }
    if (is_texel_buffer && !is_written) {
        return SynchronizeMemoryFromImage(arena, device_addr, size);
    }
    return false;
}

bool BufferCache::SynchronizeMemoryFromImage(const Buffer* arena, VAddr device_addr, u32 size) {
    if (const auto meta = texture_cache.GetMetaInfo(device_addr)) {
        if (meta->type == TextureCache::MetaType::HTile) {
            static constexpr u32 ZmaskUncompressed = 0xf;
            runtime.FillBuffer(arena, arena->Offset(device_addr), size, ZmaskUncompressed);
            return true;
        } else if (meta->type == TextureCache::MetaType::CMask) {
            const auto expanded_value = CmaskColorExpandedValue(meta->num_samples);
            if (!expanded_value) {
                LOG_WARNING(Render_Vulkan,
                            "Unsupported CMask sample count: address={:#x}, size={:#x}, "
                            "samples={}, owner={}:{}",
                            device_addr, size, meta->num_samples, meta->owner_id.index,
                            meta->owner_uid);
                return false;
            }
            runtime.FillBuffer(arena, arena->Offset(device_addr), size, *expanded_value);
            return true;
        } else {
            LOG_WARNING(Render_Vulkan, "Unhandled metadata type {}",
                        magic_enum::enum_name(meta->type));
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
    const u64 arena_offset = arena->Offset(device_addr);
    boost::container::small_vector<vk::BufferImageCopy, 8> buffer_copies;
    for (u32 mip = 0; mip < image.info.resources.levels; mip++) {
        const auto& mip_info = image.info.mips_layout[mip];
        const u32 width = std::max(image.info.size.width >> mip, 1u);
        const u32 height = std::max(image.info.size.height >> mip, 1u);
        const u32 depth = std::max(image.info.size.depth >> mip, 1u);
        if (arena_offset + mip_info.offset + mip_info.size > arena->size_bytes) {
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
    }
    if (buffer_copies.empty()) {
        return false;
    }
    auto& tile_manager = texture_cache.GetTileManager();
    tile_manager.TileImage(image, buffer_copies, arena, arena_offset);
    return true;
}

void BufferCache::SubmitPendingArenaBinds(Vulkan::SubmitInfo& info) {
    AllocatePendingPages();
    if (pending_binds.empty()) {
        return;
    }

    std::vector<vk::SparseBufferMemoryBindInfo> buffer_binds;
    buffer_binds.reserve(pending_binds.size());

    for (const auto& binds : pending_binds) {
        buffer_binds.emplace_back(vk::SparseBufferMemoryBindInfo{
            .buffer = binds.arena->Handle(),
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
