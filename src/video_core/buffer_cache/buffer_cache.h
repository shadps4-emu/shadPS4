// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <deque>
#include <memory>
#include <vector>
#include <boost/container/small_vector.hpp>

#include "common/assert.h"
#include "common/enum.h"
#include "common/multi_level_page_table.h"
#include "common/types.h"
#include "video_core/buffer_cache/buffer.h"
#include "video_core/buffer_cache/fault_manager.h"
#include "video_core/buffer_cache/range_set.h"
#include "video_core/renderer_vulkan/vk_semaphore.h"
#include "video_core/texture_cache/image.h"

namespace AmdGpu {
struct Liverpool;
}

namespace Core {
class MemoryManager;
}

namespace Vulkan {
class GraphicsPipeline;
struct SubmitInfo;
class Runtime;
class StagingBufferPool;
struct StagingBufferRef;
} // namespace Vulkan

VK_DEFINE_HANDLE(VmaPool)

namespace VideoCore {

class TextureCache;
class MemoryTracker;
class PageManager;

struct BlockTableTraits {
    using Entry = VmaAllocation;
    static constexpr size_t ADDRESS_SPACE_BITS = 40;
    static constexpr size_t L1_BITS = 14;
    static constexpr size_t PAGE_BITS = 14;
    static constexpr bool NULL_CHECK = true;
};

enum class ObtainBufferFlags {
    None = 0,
    IsWritten = 1 << 0,
    IsTexelBuffer = 1 << 1,
    IgnoreStreamBuffer = 1 << 2,
    InvalidateTextureCache = 1 << 3,
};
DECLARE_ENUM_FLAG_OPERATORS(ObtainBufferFlags)


class ResidencyBitset {
    static constexpr u64 WORD_BITS = 64;
    static constexpr u64 LEAF_WORDS = 64;
    static constexpr u64 LEAF_BITS = WORD_BITS * LEAF_WORDS;
    static constexpr u64 LEAF_SHIFT = 12;
    static_assert(LEAF_BITS == (u64{1} << LEAF_SHIFT));

    struct Leaf {
        std::array<u64, LEAF_WORDS> words{};
        u32 count{}; 
    };

public:
    ResidencyBitset() = default;

    explicit ResidencyBitset(u64 first_block, u64 num_blocks)
        : base{first_block}, size{num_blocks}, leaves((num_blocks + LEAF_BITS - 1) >> LEAF_SHIFT) {}

    [[nodiscard]] u64 FirstBlock() const noexcept {
        return base;
    }

    [[nodiscard]] u64 EndBlock() const noexcept {
        return base + size;
    }

    [[nodiscard]] bool Test(u64 block) const noexcept {
        DEBUG_ASSERT(block >= base && block < base + size);
        const u64 pos = block - base;
        const Leaf* leaf = leaves[pos >> LEAF_SHIFT].get();
        if (!leaf) {
            return false;
        }
        const u64 bit = pos & (LEAF_BITS - 1);
        return (leaf->words[bit / WORD_BITS] >> (bit % WORD_BITS)) & 1;
    }

    [[nodiscard]] u64 FindNextGap(u64 from, u64 end) const noexcept {
        return FindNext<false>(from, end);
    }

    /// Returns the first resident block in [from, end), or `end` if none of them is.
    [[nodiscard]] u64 FindNextResident(u64 from, u64 end) const noexcept {
        return FindNext<true>(from, end);
    }

    [[nodiscard]] bool IsResident(u64 first, u64 end) const noexcept {
        return FindNext<false>(first, end) == end;
    }

    /// Calls func(gap_start, gap_end) for every maximal run of non-resident blocks in the range.
    /// The set must not be modified from inside the callback.
    template <typename Func>
    void ForEachGap(u64 first, u64 end, Func&& func) const {
        u64 pos = first;
        while (pos < end) {
            const u64 gap_start = FindNext<false>(pos, end);
            if (gap_start >= end) {
                return;
            }
            const u64 gap_end = FindNext<true>(gap_start, end);
            func(gap_start, gap_end);
            pos = gap_end;
        }
    }

    /// Calls func(run_start, run_end) for every maximal run of resident blocks in the range.
    /// The set must not be modified from inside the callback.
    template <typename Func>
    void ForEachResident(u64 first, u64 end, Func&& func) const {
        u64 pos = first;
        while (pos < end) {
            const u64 run_start = FindNext<true>(pos, end);
            if (run_start >= end) {
                return;
            }
            const u64 run_end = FindNext<false>(run_start, end);
            func(run_start, run_end);
            pos = run_end;
        }
    }

    /// Marks every block in [first, end) as resident.
    void Set(u64 first, u64 end) {
        Modify<true>(first, end);
    }

    /// Marks every block in [first, end) as not resident.
    void Clear(u64 first, u64 end) {
        Modify<false>(first, end);
    }

    /// Marks every block that is resident in other as resident. Other must lie inside this set.
    void Merge(const ResidencyBitset& other) {
        other.ForEachResident(other.FirstBlock(), other.EndBlock(),
                              [this](u64 start, u64 end) { Set(start, end); });
    }

private:
    template <bool Value>
    [[nodiscard]] u64 FindNext(u64 from, u64 end) const noexcept {
        DEBUG_ASSERT(from >= base && end <= base + size);
        u64 pos = from - base;
        const u64 stop = end - base;
        while (pos < stop) {
            const u64 leaf_index = pos >> LEAF_SHIFT;
            const u64 leaf_begin = leaf_index << LEAF_SHIFT;
            const u64 leaf_end = std::min(stop, leaf_begin + LEAF_BITS);
            const Leaf* leaf = leaves[leaf_index].get();
            if (!leaf) {
                if constexpr (Value) {
                    pos = leaf_end;
                    continue;
                } else {
                    return pos + base;
                }
            }
            if constexpr (!Value) {
                if (leaf->count == LEAF_BITS) {
                    pos = leaf_end;
                    continue;
                }
            }
            const u64 first_word = (pos - leaf_begin) / WORD_BITS;
            const u64 last_word = (leaf_end - leaf_begin - 1) / WORD_BITS;
            u64 mask = ~u64{0} << ((pos - leaf_begin) % WORD_BITS);
            for (u64 w = first_word; w <= last_word; ++w, mask = ~u64{0}) {
                u64 word = leaf->words[w];
                if constexpr (!Value) {
                    word = ~word;
                }
                word &= mask;
                if (word != 0) {
                    const u64 found = leaf_begin + w * WORD_BITS + std::countr_zero(word);
                    // The match may lie in the tail of the last word, past the requested range.
                    return found < stop ? found + base : end;
                }
            }
            pos = leaf_end;
        }
        return end;
    }

    template <bool Value>
    void Modify(u64 first, u64 end) {
        DEBUG_ASSERT(first >= base && end <= base + size);
        u64 pos = first - base;
        const u64 stop = end - base;
        while (pos < stop) {
            const u64 leaf_index = pos >> LEAF_SHIFT;
            const u64 leaf_begin = leaf_index << LEAF_SHIFT;
            const u64 leaf_end = std::min(stop, leaf_begin + LEAF_BITS);
            std::unique_ptr<Leaf>& slot = leaves[leaf_index];
            if constexpr (Value) {
                if (!slot) {
                    slot = std::make_unique<Leaf>();
                }
            } else {
                if (!slot) {
                    pos = leaf_end;
                    continue;
                }
            }
            Leaf& leaf = *slot;
            const u64 lo = pos - leaf_begin;
            const u64 hi = leaf_end - leaf_begin;
            const u64 first_word = lo / WORD_BITS;
            const u64 last_word = (hi - 1) / WORD_BITS;
            for (u64 w = first_word; w <= last_word; ++w) {
                u64 mask = ~u64{0};
                if (w == first_word) {
                    mask &= ~u64{0} << (lo % WORD_BITS);
                }
                if (w == last_word && hi % WORD_BITS != 0) {
                    mask &= (u64{1} << (hi % WORD_BITS)) - 1;
                }
                u64& word = leaf.words[w];
                const u64 old_word = word;
                word = Value ? (old_word | mask) : (old_word & ~mask);
                leaf.count += static_cast<u32>(std::popcount(word));
                leaf.count -= static_cast<u32>(std::popcount(old_word));
            }
            if constexpr (!Value) {
                if (leaf.count == 0) {
                    slot.reset();
                }
            }
            pos = leaf_end;
        }
    }

    u64 base{};
    u64 size{};
    std::vector<std::unique_ptr<Leaf>> leaves;
};

class BufferCache {
    static constexpr u64 ADDRESS_SPACE_BITS = 40;
    static constexpr u64 ARENA_PAGE_BITS = 32;
    static constexpr u64 ARENA_PAGE_SIZE = u64{1} << ARENA_PAGE_BITS;
    static constexpr u64 NUM_ARENA_PAGES = u64{1} << (ADDRESS_SPACE_BITS - ARENA_PAGE_BITS);
    static constexpr u64 MIN_BLOCK_SIZE = 16_KB;
    static constexpr u64 STREAM_THRESHOLD = 16_KB;

public:
    explicit BufferCache(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler,
                         Vulkan::Runtime& runtime, AmdGpu::Liverpool* liverpool,
                         TextureCache& texture_cache, PageManager& tracker);
    ~BufferCache();

    /// Returns a pointer to GDS device local buffer.
    [[nodiscard]] const Buffer* GetGdsBuffer() const noexcept {
        return &gds_buffer;
    }

    [[nodiscard]] const Buffer* GetNullIndexBuffer() const noexcept {
        return null_index_buffer.get();
    }

    /// Retrieves the device local DBA page table buffer.
    [[nodiscard]] Buffer* GetBdaPageTableBuffer() noexcept {
        return bda_pagetable_buffer.get();
    }

    /// Retrieves the fault buffer.
    [[nodiscard]] Buffer* GetFaultBuffer() noexcept {
        return fault_manager->GetFaultBuffer();
    }

    /// Retrieves the stream buffer.
    [[nodiscard]] StreamBuffer& GetStreamBuffer() noexcept {
        return stream_buffer;
    }

    /// Returns minimum granularity of a sparse memory bind.
    u32 GetSparsePageShift() const noexcept {
        return block_shift;
    }

    void TickFrame();

    /// Invalidates any buffer in the logical page range.
    void InvalidateMemory(VAddr device_addr, u64 size, bool assume_locks = false);

    /// Unmark a region as modified from the GPU.
    void UnmarkRegionAsGpuModified(VAddr device_addr, u64 size, bool is_write = false);

    /// Flushes any GPU modified buffer in the logical page range back to CPU memory.
    void ReadMemory(VAddr device_addr, u64 size, bool is_write = false, bool async = false,
                    bool assume_locks = false);

    /// Flushes GPU modified ranges of the uncovered part of the edge pages of an image.
    void ReadEdgeImagePages(const Image& image);

    /// Finds a buffer for the specified region.
    [[nodiscard]] std::pair<const Buffer*, u64> ObtainBuffer(
        VAddr device_addr, u32 size, ObtainBufferFlags flags = ObtainBufferFlags::None);

    /// Attempts to obtain a buffer without modifying the cache contents.
    [[nodiscard]] std::pair<const Buffer*, u64> ObtainBufferForImage(VAddr device_addr, u32 size);

    /// Return true when a region is modified from the CPU
    [[nodiscard]] bool IsRegionCpuModified(VAddr addr, size_t size);

    /// Return true when a region is modified from the GPU
    [[nodiscard]] bool IsRegionGpuModified(VAddr addr, size_t size);

    /// Mark region as modified from the GPU
    void MarkRegionAsGpuModified(VAddr addr, size_t size);

    /// Synchronizes all buffers needed for DMA.
    void SynchronizeDmaBuffers();

    void FlushPendingPageTable();

    void SubmitPendingArenaBinds(Vulkan::SubmitInfo& info);

private:
    struct Arena : public Buffer {
        explicit Arena(const Vulkan::Instance& instance, VAddr cpu_addr_, u64 size_bytes_,
                       u32 block_shift)
            : Buffer{instance, cpu_addr_, size_bytes_, MemoryType::Sparse},
              residency{cpu_addr_ >> block_shift, size_bytes_ >> block_shift} {}

        ResidencyBitset residency;
    };

    struct ArenaBinds {
        const Buffer* arena;
        boost::container::small_vector<vk::SparseMemoryBind, 32> binds;
    };

    /// Blocks that were marked resident but don't have memory yet.
    struct PendingPages {
        Arena* arena;
        u64 first_block;
        u64 end_block;
    };

    /// BDA page table entries that were not written to the GPU yet.
    struct PendingTableWrite {
        u64 first_block;
        u64 end_block;
        u64 bda_base; ///< Device address of the arena minus its guest address.
    };

    ArenaBinds* BindsForArena(const Buffer* arena) {
        auto it = std::ranges::find(pending_binds, arena, &ArenaBinds::arena);
        if (it != pending_binds.end()) {
            return std::addressof(*it);
        }
        return &pending_binds.emplace_back(arena);
    }

    /// Appends a sparse bind, extending the previous one when both are contiguous.
    static void AppendBind(ArenaBinds& binds, u64 resource_offset, u64 size,
                           vk::DeviceMemory memory, u64 memory_offset);

    Arena* GetArena(u64 first_block, u64 last_block);

    /// Marks all blocks in the range resident. This only queues the work, memory is allocated
    /// by AllocatePendingPages and the page table is written by FlushPendingPageTable.
    void EnsureResident(Arena* arena, u64 first_block, u64 last_block);

    /// Queues the blocks [first_block, end_block) of the arena for allocation and page table
    /// writes.
    void QueueResidentBlocks(Arena* arena, u64 first_block, u64 end_block);

    /// Allocates all queued blocks with one VMA call and records the sparse binds for them.
    void AllocatePendingPages();

    /// Returns the VMA allocation backing a block, or null if it has none (yet).
    [[nodiscard]] VmaAllocation BlockAllocation(u64 block) const;

    void DownloadMemory(const Buffer* arena, VAddr device_addr, u64 size, bool is_write,
                        bool async);

    bool SynchronizeMemory(const Buffer* arena, VAddr device_addr, u32 size, bool is_written,
                           bool is_texel_buffer);

    bool SynchronizeMemoryFromImage(const Buffer* arena, VAddr device_addr, u32 size);

    const Vulkan::Instance& instance;
    Vulkan::Scheduler& scheduler;
    Vulkan::Runtime& runtime;
    Vulkan::StagingBufferPool& staging_pool;
    AmdGpu::Liverpool* liverpool;
    Core::MemoryManager* memory;
    TextureCache& texture_cache;
    std::unique_ptr<MemoryTracker> memory_tracker;

    StreamBuffer stream_buffer;
    Buffer gds_buffer;
    std::unique_ptr<Buffer> null_index_buffer;
    RangeSet gpu_modified_ranges;

    std::unique_ptr<FaultManager> fault_manager;
    std::unique_ptr<Buffer> bda_pagetable_buffer;
    bool fault_process_pending{};

    std::array<Arena*, NUM_ARENA_PAGES> address_space{};
    std::deque<Arena> arenas;
    std::vector<ArenaBinds> pending_binds;
    Vulkan::Semaphore memory_semaphore;

    /// Every resident block owns one allocation in this pool, which is what allows blocks to be
    /// freed individually. Indexed by block number, null for blocks without memory.
    VmaPool sparse_pool{};
    Common::MultiLevelPageTable<BlockTableTraits> block_allocations;

    std::vector<PendingPages> pending_pages;
    u64 pending_page_count{};
    std::vector<PendingTableWrite> pending_table_writes;
    // Scratch space for AllocatePendingPages, kept around to avoid reallocating on every submit.
    std::vector<VmaAllocation> page_handles;
    std::vector<VmaAllocationInfo> page_infos;

    u32 arena_memory_type_index{};
    u32 block_size{};
    u32 block_shift{};
    u32 blocks_per_arena_page{};
    u32 blocks_per_arena_page_shift{};
};

} // namespace VideoCore
