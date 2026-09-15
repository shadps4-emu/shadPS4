// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <deque>
#include <boost/container/small_vector.hpp>

#include "common/interval_set.h"
#include "common/types.h"
#include "video_core/buffer_cache/buffer.h"
#include "video_core/buffer_cache/fault_manager.h"
#include "video_core/buffer_cache/range_set.h"
#include "video_core/renderer_vulkan/vk_semaphore.h"

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
} // namespace Vulkan

namespace VideoCore {

class TextureCache;
class MemoryTracker;
class PageManager;

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

    /// Retrieves the device local DBA page table buffer.
    [[nodiscard]] Buffer* GetBdaPageTableBuffer() noexcept {
        return bda_pagetable_buffer.get();
    }

    /// Retrieves the fault buffer.
    [[nodiscard]] Buffer* GetFaultBuffer() noexcept {
        return fault_manager->GetFaultBuffer();
    }

    /// Retrieves the stream buffer.
    StreamBuffer& GetStreamBuffer() noexcept {
        return stream_buffer;
    }

    /// Returns minimum granularity of a sparse memory bind.
    u32 GetSparsePageShift() const noexcept {
        return block_shift;
    }

    /// Invalidates any buffer in the logical page range.
    void InvalidateMemory(VAddr device_addr, u64 size, bool assume_locks = false);

    /// Flushes any GPU modified buffer in the logical page range back to CPU memory.
    void ReadMemory(VAddr device_addr, u64 size, bool is_write = false, bool assume_locks = false);

    /// Finds a buffer for the specified region.
    [[nodiscard]] std::pair<const Buffer*, u64> ObtainBuffer(VAddr device_addr, u32 size,
                                                             bool is_written,
                                                             bool is_texel_buffer = false);

    /// Attempts to obtain a buffer without modifying the cache contents.
    [[nodiscard]] std::pair<const Buffer*, u64> ObtainBufferForImage(VAddr device_addr, u32 size);

    /// Return true when a region is modified from the CPU
    [[nodiscard]] bool IsRegionCpuModified(VAddr addr, size_t size);

    /// Return true when a region is modified from the GPU
    [[nodiscard]] bool IsRegionGpuModified(VAddr addr, size_t size);

    /// Processes the fault buffer.
    void ProcessFaultBuffer();

    /// Synchronizes all buffers needed for DMA.
    void SynchronizeDmaBuffers();

    /// Commits pending sparse buffer memory binds. Must be called before every scheduler submit.
    void SubmitPendingArenaBinds(Vulkan::SubmitInfo& info);

    /// Flushes pending synchronization requests
    void FlushSyncBatch(bool from_scheduler = false);

    class SyncBatchList {
    public:
        struct Range {
            u64 lo;
            u64 hi;
            bool written;
        };

        const Range* begin() const {
            return v_.data();
        }
        const Range* end() const {
            return v_.data() + v_.size();
        }

        void Clear() {
            v_.clear();
        }
        bool Empty() const {
            return v_.empty();
        }
        std::size_t Size() const {
            return v_.size();
        }

        void Add(u64 lo, u64 hi, bool written) {
            if (lo >= hi) {
                return;
            }

            auto i = std::ranges::lower_bound(v_, lo, {}, &Range::hi);
            auto k = i;
            while (k != v_.end() && k->lo <= hi) {
                ++k;
            }
            const std::size_t first = i - v_.begin(), last = k - v_.begin();

            scratch_.clear();
            if (i != k && i->lo < lo) {
                scratch_.emplace_back(i->lo, lo, i->written);
            }
            u64 cur = lo;
            for (auto it = i; it != k; ++it) {
                if (!it->written) {
                    continue;
                }
                const u64 a = std::max(it->lo, lo), b = std::min(it->hi, hi);
                if (a >= b) {
                    continue;
                }
                if (cur < a) {
                    scratch_.emplace_back(cur, a, written);
                }
                scratch_.emplace_back(a, b, true);
                cur = b;
            }
            if (cur < hi) {
                scratch_.emplace_back(cur, hi, written);
            }
            if (i != k && (k - 1)->hi > hi) {
                scratch_.emplace_back(hi, (k - 1)->hi, (k - 1)->written);
            }

            Coalesce(scratch_);
            v_.erase(v_.begin() + first, v_.begin() + last);
            v_.insert(v_.begin() + first, scratch_.begin(), scratch_.end());
        }

        bool Overlaps(u64 lo, u64 hi) const {
            if (lo >= hi) {
                return false;
            }
            auto i = std::ranges::upper_bound(v_, lo, {}, &Range::hi);
            return i != v_.end() && i->lo < hi;
        }

        bool OverlapsWritten(u64 lo, u64 hi) const {
            if (lo >= hi) {
                return false;
            }
            auto it = std::ranges::upper_bound(v_, lo, {}, &Range::hi);
            for (; it != v_.end() && it->lo < hi; ++it) {
                if (it->written) {
                    return true;
                }
            }
            return false;
        }

        const Range* At(u64 x) const {
            auto it = std::ranges::upper_bound(v_, x, {}, &Range::lo);
            if (it == v_.begin()) {
                return nullptr;
            }
            --it;
            return (it->lo <= x && x < it->hi) ? &*it : nullptr;
        }

    private:
        static void Coalesce(std::vector<Range>& out) {
            std::size_t w = 0;
            for (std::size_t r = 1; r < out.size(); ++r) {
                if (out[w].hi == out[r].lo && out[w].written == out[r].written) {
                    out[w].hi = out[r].hi;
                } else {
                    out[++w] = out[r];
                }
            }
            if (!out.empty()) {
                out.resize(w + 1);
            }
        }

        std::vector<Range> v_;
        std::vector<Range> scratch_;
    };

    SyncBatchList sync_batch{};
    u32 num_flushes_per_frame{};

private:
    struct ArenaBinds {
        const Buffer* arena;
        boost::container::small_vector<vk::SparseMemoryBind, 32> binds;
    };

    ArenaBinds* BindsForArena(const Buffer* arena) {
        auto it = std::ranges::find(pending_binds, arena, &ArenaBinds::arena);
        if (it != pending_binds.end()) {
            return std::addressof(*it);
        }
        return &pending_binds.emplace_back(arena);
    }

    const Buffer* GetArena(u64 first_block, u64 last_block);

    void EnsureResident(const Buffer* arena, u64 first_block, u64 last_block);

    void DownloadMemory(const Buffer* arena, VAddr device_addr, u64 size);

    bool SynchronizeMemory(const Buffer* arena, VAddr device_addr, u32 size, bool is_written,
                           bool is_texel_buffer);

    const Buffer* UploadCopies(const Buffer* arena, std::span<vk::BufferCopy> copies,
                               size_t total_size_bytes);

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
    RangeSet gpu_modified_ranges;

    std::unique_ptr<FaultManager> fault_manager;
    std::unique_ptr<Buffer> bda_pagetable_buffer;

    std::array<const Buffer*, NUM_ARENA_PAGES> address_space{};
    std::deque<Buffer> arenas;
    std::vector<ArenaBinds> pending_binds;
    Vulkan::Semaphore memory_semaphore;

    struct Backing : public Interval {
        vk::DeviceMemory memory;
        u64 offset;
        constexpr bool CanMergeWith(const Backing& other) const noexcept {
            return memory == other.memory && offset + (end - start) == other.offset;
        }
        constexpr Backing SubRange(u64 a, u64 b) const noexcept {
            return {{a, b}, memory, offset + (a - start)};
        }
    };
    IntervalList<Backing> resident_ranges;

    u32 arena_memory_type_index{};
    u32 block_size{};
    u32 block_shift{};
    u32 blocks_per_arena_page{};
    u32 blocks_per_arena_page_shift{};
};

} // namespace VideoCore
