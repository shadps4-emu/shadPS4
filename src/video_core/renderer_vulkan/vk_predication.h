// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "common/types.h"
#include "video_core/buffer_cache/buffer.h"

namespace VideoCore {
class BufferCache;
}

namespace Vulkan {

class Instance;
class Scheduler;

/**
 * Emulates IT_SET_PREDICATION and the pixel-pipe ZPASS statistics it consumes.
 *
 * On real hardware the CP evaluates the predicate when the predicated packets *execute*, long
 * after they were queued (applications never wait for occlusion results before enabling
 * predication). To match that without stalling, the predicate is built on the GPU timeline:
 * per-draw occlusion queries are reduced into a predicate buffer by a small compute pass and
 * predicated draws are wrapped in VK_EXT_conditional_rendering. The CPU never waits for query
 * results. Guest-visible ZPASS counter writeback is resolved asynchronously once the submission
 * that produced the samples completes.
 */
class PredicationManager {
public:
    explicit PredicationManager(const Instance& instance, Scheduler& scheduler,
                                VideoCore::BufferCache& buffer_cache);
    ~PredicationManager();

    /// Handles PIXEL_PIPE_STAT_CONTROL: toggles perfect ZPASS sample counting.
    void ControlZpassCounting();

    /// Handles PIXEL_PIPE_STAT_RESET: clears the accumulated ZPASS sample counter.
    void ResetZpassCounting();

    /// Handles a ZPASS_DONE pixel-pipe stat dump. Captures the queries recorded since the last
    /// dump and schedules the guest counter writeback for when their results are available.
    void DumpZpassCounters(VAddr address, u32 num_counter_pairs);

    /// Enables predication from the ZPASS delta of an occlusion query results block.
    void EnableFromZpass(VAddr address, u32 num_counter_pairs, bool draw_visible, bool combine);

    /// Enables predication from a 32/64-bit boolean in guest memory, read on the GPU timeline.
    void EnableFromBool(VAddr address, bool is_64bit, bool draw_visible, bool combine);

    /// Disables predication; packets execute unconditionally.
    void Disable();

    /// Returns true when predicated packets must be skipped during command processing. Only
    /// possible when the predicate value is already final; GPU-side predicates never skip here.
    bool ShouldSkipPredicatedPacket();

    /// Returns true when predicated draws must be wrapped in conditional rendering.
    bool IsGpuPredicationActive() const {
        return mode == Mode::Gpu;
    }

    /// Acquires and resets an occlusion query slot for the next draw when counting is active.
    std::optional<u32> PrepareDrawQuery();

    /// Begins conditional rendering and/or the draw's occlusion query.
    void BeginDraw(vk::CommandBuffer cmdbuf, std::optional<u32> query, bool packet_predicated);

    /// Ends the draw's occlusion query and/or conditional rendering.
    void EndDraw(vk::CommandBuffer cmdbuf, std::optional<u32> query, bool packet_predicated);

private:
    enum class Mode {
        Disabled, ///< No predication; packets execute normally.
        Static,   ///< Predicate value is final; decision applied while parsing packets.
        Gpu,      ///< Predicate lives in the predicate buffer; draws use conditional rendering.
    };

    /// Queries captured between two pixel-pipe stat dumps, keyed by the dump address.
    struct DumpRecord {
        VAddr address{};
        u32 num_counter_pairs{};
        std::vector<u32> queries;
        bool poisoned{};
    };

    std::optional<u32> AcquireQuerySlot();
    void ReleaseQuerySlotsWhenDone(std::vector<u32>&& slots);
    void ResolveRecord(const DumpRecord& record);
    void WriteGuestCounters(VAddr address, u32 num_counter_pairs, u64 counter) const;
    std::optional<bool> ReadGuestZpass(VAddr address, u32 num_counter_pairs) const;

    void SetStatic(std::optional<bool> visible, bool draw_visible, bool combine, const char* path);
    /// Returns true when the single-query fast path was used (no reduction dispatch).
    bool BuildFromQueries(const std::vector<u32>& queries, bool draw_visible, bool combine);
    /// Picks (and seeds, when combining) the predicate slot the reduction writes into.
    u32 SelectPredicateSlot(vk::CommandBuffer cmdbuf, bool combine, bool& combine_on_gpu);
    void ActivateGpuPredicate(u32 slot, bool draw_visible);
    u32 AllocPredicateSlot();
    u32 AllocScratchQwords(u32 count);
    void ReducePredicate(u32 src_index, u32 count, u32 dst_slot, bool combine_on_gpu);
    void TracePredicate(u32 slot, VAddr address);
    void CloseWindow();

    const Instance& instance;
    Scheduler& scheduler;
    VideoCore::BufferCache& buffer_cache;
    bool supported{};

    // ZPASS sample counting.
    static constexpr u32 QueryPoolSize = 4096;
    vk::UniqueQueryPool query_pool;
    std::array<bool, QueryPoolSize> query_busy{};
    u32 query_cursor{};
    bool counting_enabled{};
    bool active_poisoned{};
    std::vector<u32> active_queries;
    std::unordered_map<VAddr, std::shared_ptr<DumpRecord>> records;
    u64 zpass_counter{};

    // GPU predicate generation.
    static constexpr u32 NumPredicateSlots = 1024;
    static constexpr u32 NumScratchQwords = 8192;
    static constexpr u32 NumTraceSlots = 1024;
    VideoCore::Buffer predicate_buffer;
    VideoCore::Buffer counter_scratch;
    VideoCore::Buffer trace_buffer;
    u32 predicate_cursor{};
    u32 scratch_cursor{};
    u32 trace_cursor{};
    vk::UniqueDescriptorSetLayout reduce_desc_layout;
    vk::UniquePipelineLayout reduce_pipeline_layout;
    vk::UniquePipeline reduce_pipeline;

    // Current predication state.
    Mode mode{Mode::Disabled};
    bool static_execute{true};
    std::optional<bool> static_visible;
    u32 gpu_slot{};
    bool gpu_inverted{};

    // Diagnostics.
    u64 predication_seq{};
    u64 wrapped_draws{};
    u64 skipped_packets{};
};

} // namespace Vulkan
