// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "common/types.h"
#include "core/address_space.h"
#include "video_core/buffer_cache/buffer.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace Vulkan {

class Instance;
class Scheduler;

struct ZpassPredicate {
    enum class Kind : u8 {
        Unknown,
        Visible,
        Occluded,
        Gpu,
    };
    Kind kind{Kind::Unknown};
    u32 offset{};
};

class OcclusionCounter {
public:
    explicit OcclusionCounter(const Instance& instance, Scheduler& scheduler,
                              const Core::AddressSpace& address_space, u32 num_dbs);
    ~OcclusionCounter();

    bool Dump(VAddr address, u32 num_dbs = 8, u32 stride = 16);
    void BeforeDraw(u32 db_count_control);
    void OnEndRendering();
    void OnFence();

    ZpassPredicate EvaluateZpass(VAddr address, bool draw_if_visible);

    [[nodiscard]] vk::Buffer PredicateBuffer() const noexcept {
        return predicate_buffer.buffer;
    }

private:
    struct Record {
        VAddr address;
        u64 id;
        u64 query_end;
        u64 tick;
        u32 num_dbs;
        u32 stride;
    };

    struct DumpSlot {
        VAddr address{};
        u64 id{};
        u64 query_end{};
        u64 uncounted{};
        u64 job_seq{};
        u64 total{};
        u64 lost{};
        bool resolved{};
    };

    struct Job {
        u64 seq{~0ULL};
        u64 first{};
        u32 count{};
        bool computed{};
    };

    void End();
    u64* Word(VAddr address, u32 db, u32 stride) const;
    DumpSlot& SlotFor(VAddr address);

    void ResolverThread(std::stop_token stoken);
    bool WaitTick(u64 tick, std::stop_token stoken);
    bool Resolve(const Record& record, std::stop_token stoken);

    void CreatePredicatePass();
    void ComputePredicates();

    const Instance& instance;
    Scheduler& scheduler;
    const Core::AddressSpace::BackingPageTable& backing_pages;
    const u32 num_dbs;
    vk::QueryControlFlags query_flags;
    vk::UniqueQueryPool pool;

    u32 window_draws;
    bool active{};
    u64 query_write{};
    std::atomic<u64> query_read{};
    u64 next_dump_id{1};
    u64 uncounted{};

    std::mutex mutex;
    std::condition_variable_any records_cv;
    std::condition_variable_any fence_cv;
    bool is_resolving{false};
    std::vector<Record> records;
    std::vector<DumpSlot> dump_slots;
    u64 hold_begin{~0ULL};
    u64 hold_end{};
    u64 hold_tick{};

    u64 total{};
    u64 lost{};
    std::vector<u64> results;

    bool predicates_enabled{};
    VideoCore::UniqueBuffer predicate_buffer;
    vk::UniqueDescriptorSetLayout predicate_desc_layout;
    vk::UniquePipelineLayout predicate_pipeline_layout;
    vk::UniquePipeline predicate_pipeline;
    std::vector<Job> jobs;
    u64 next_job_seq{};
    u64 computed_seq{};
    std::vector<u32> job_upload;

    std::jthread resolver;
};

} // namespace Vulkan
