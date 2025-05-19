// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <shared_mutex>
#include <boost/container/small_vector.hpp>
#include "common/div_ceil.h"
#include "common/slot_vector.h"
#include "common/types.h"
#include "video_core/buffer_cache/buffer.h"
#include "video_core/buffer_cache/memory_tracker_base.h"
#include "video_core/buffer_cache/range_set.h"
#include "video_core/multi_level_page_table.h"

namespace AmdGpu {
struct Liverpool;
}

namespace Shader {
namespace Gcn {
struct FetchShaderData;
}
struct Info;
} // namespace Shader

namespace Vulkan {
class GraphicsPipeline;
}

namespace VideoCore {

using BufferId = Common::SlotId;

static constexpr BufferId NULL_BUFFER_ID{0};

class TextureCache;

class BufferCache {
public:
    static constexpr u32 CACHING_PAGEBITS = 14;
    static constexpr u64 CACHING_PAGESIZE = u64{1} << CACHING_PAGEBITS;
    static constexpr u64 DEVICE_PAGESIZE = 16_KB;
    static constexpr u64 CACHING_NUMPAGES = u64{1} << (40 - CACHING_PAGEBITS);

    static constexpr u64 BDA_PAGETABLE_SIZE = CACHING_NUMPAGES * sizeof(vk::DeviceAddress);
    static constexpr u64 FAULT_BUFFER_SIZE = CACHING_NUMPAGES / 8; // Bit per page

    struct PageData {
        BufferId buffer_id{};
    };

    struct Traits {
        using Entry = PageData;
        static constexpr size_t AddressSpaceBits = 40;
        static constexpr size_t FirstLevelBits = 16;
        static constexpr size_t PageBits = CACHING_PAGEBITS;
    };
    using PageTable = MultiLevelPageTable<Traits>;

    struct OverlapResult {
        boost::container::small_vector<BufferId, 16> ids;
        VAddr begin;
        VAddr end;
        bool has_stream_leap = false;
    };

public:
    explicit BufferCache(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
                         Vulkan::Rasterizer& rasterizer_, AmdGpu::Liverpool* liverpool,
                         TextureCache& texture_cache, PageManager& tracker);
    ~BufferCache();

    /// Returns a pointer to GDS device local buffer.
    [[nodiscard]] const Buffer* GetGdsBuffer() const noexcept {
        return &gds_buffer;
    }

    /// Retrieves the host visible device local stream buffer.
    [[nodiscard]] StreamBuffer& GetStreamBuffer() noexcept {
        return stream_buffer;
    }

    /// Retrieves the device local DBA page table buffer.
    [[nodiscard]] Buffer* GetBdaPageTableBuffer() noexcept {
        return &bda_pagetable_buffer;
    }

    /// Retrieves the fault buffer.
    [[nodiscard]] Buffer* GetFaultBuffer() noexcept {
        return &fault_buffer;
    }

    /// Retrieves the buffer with the specified id.
    [[nodiscard]] Buffer& GetBuffer(BufferId id) {
        return slot_buffers[id];
    }

    /// Invalidates any buffer in the logical page range.
    void InvalidateMemory(VAddr device_addr, u64 size, bool unmap);

    /// Binds host vertex buffers for the current draw.
    void BindVertexBuffers(const Vulkan::GraphicsPipeline& pipeline);

    /// Bind host index buffer for the current draw.
    void BindIndexBuffer(u32 index_offset);

    /// Writes a value to GPU buffer. (uses command buffer to temporarily store the data)
    void InlineData(VAddr address, const void* value, u32 num_bytes, bool is_gds);

    /// Writes a value to GPU buffer. (uses staging buffer to temporarily store the data)
    void WriteData(VAddr address, const void* value, u32 num_bytes, bool is_gds);

    /// Obtains a buffer for the specified region.
    [[nodiscard]] std::pair<Buffer*, u32> ObtainBuffer(VAddr gpu_addr, u32 size, bool is_written,
                                                       bool is_texel_buffer = false,
                                                       BufferId buffer_id = {});

    /// Attempts to obtain a buffer without modifying the cache contents.
    [[nodiscard]] std::pair<Buffer*, u32> ObtainViewBuffer(VAddr gpu_addr, u32 size,
                                                           bool prefer_gpu);

    /// Return true when a region is registered on the cache
    [[nodiscard]] bool IsRegionRegistered(VAddr addr, size_t size);

    /// Return true when a CPU region is modified from the CPU
    [[nodiscard]] bool IsRegionCpuModified(VAddr addr, size_t size);

    /// Return true when a CPU region is modified from the GPU
    [[nodiscard]] bool IsRegionGpuModified(VAddr addr, size_t size);

    /// Return buffer id for the specified region
    BufferId FindBuffer(VAddr device_addr, u32 size);

    /// Processes the fault buffer.
    void ProcessFaultBuffer();

    /// Synchronizes all buffers in the specified range.
    void SynchronizeBuffersInRange(VAddr device_addr, u64 size);

    /// Synchronizes all buffers neede for DMA.
    void SynchronizeDmaBuffers();

    /// Record memory barrier. Used for buffers when accessed via BDA.
    void MemoryBarrier();

private:
    template <typename Func>
    void ForEachBufferInRange(VAddr device_addr, u64 size, Func&& func) {
        buffer_ranges.ForEachInRange(device_addr, size,
                                     [&](u64 page_start, u64 page_end, BufferId id) {
                                         Buffer& buffer = slot_buffers[id];
                                         func(id, buffer);
                                     });
    }

    void DownloadBufferMemory(Buffer& buffer, VAddr device_addr, u64 size);

    [[nodiscard]] OverlapResult ResolveOverlaps(VAddr device_addr, u32 wanted_size);

    void JoinOverlap(BufferId new_buffer_id, BufferId overlap_id, bool accumulate_stream_score);

    BufferId CreateBuffer(VAddr device_addr, u32 wanted_size);

    void Register(BufferId buffer_id);

    void Unregister(BufferId buffer_id);

    template <bool insert>
    void ChangeRegister(BufferId buffer_id);

    void SynchronizeBuffer(Buffer& buffer, VAddr device_addr, u32 size, bool is_texel_buffer);

    bool SynchronizeBufferFromImage(Buffer& buffer, VAddr device_addr, u32 size);

    void InlineDataBuffer(Buffer& buffer, VAddr address, const void* value, u32 num_bytes);

    void WriteDataBuffer(Buffer& buffer, VAddr address, const void* value, u32 num_bytes);

    void DeleteBuffer(BufferId buffer_id);

    const Vulkan::Instance& instance;
    Vulkan::Scheduler& scheduler;
    Vulkan::Rasterizer& rasterizer;
    AmdGpu::Liverpool* liverpool;
    TextureCache& texture_cache;
    PageManager& tracker;
    StreamBuffer staging_buffer;
    StreamBuffer stream_buffer;
    StreamBuffer download_buffer;
    Buffer gds_buffer;
    Buffer bda_pagetable_buffer;
    Buffer fault_buffer;
    std::shared_mutex slot_buffers_mutex;
    Common::SlotVector<Buffer> slot_buffers;
    RangeSet gpu_modified_ranges;
    SplitRangeMap<BufferId> buffer_ranges;
    MemoryTracker memory_tracker;
    PageTable page_table;
    vk::UniqueDescriptorSetLayout fault_process_desc_layout;
    vk::UniquePipeline fault_process_pipeline;
    vk::UniquePipelineLayout fault_process_pipeline_layout;
};

} // namespace VideoCore
