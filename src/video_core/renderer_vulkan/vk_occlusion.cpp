// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <functional>

#include "common/logging/log.h"
#include "common/thread.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_occlusion.h"
#include "video_core/renderer_vulkan/vk_platform.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"

#include "video_core/host_shaders/occlusion_predicate_comp.h"

namespace Vulkan {

namespace {

constexpr u32 NumQueries = 4096;
constexpr u32 MaxWindowDraws = 128;
constexpr u64 WaitSliceNs = 5'000'000;
constexpr u64 ValidBit = 1ULL << 63;
constexpr u32 DumpStride = 16;
constexpr u32 ZpassIncrementDisable = 1;
constexpr u32 DumpSlotBits = 12;
constexpr u32 NumDumpSlots = 1U << DumpSlotBits;

constexpr u32 NumJobs = 4096;
constexpr u32 ResultsOffset = 0;
constexpr u32 JobsOffset = ResultsOffset + NumQueries * 2 * sizeof(u32);
constexpr u32 PredsOffset = JobsOffset + NumJobs * 2 * sizeof(u32);
constexpr u32 PredicateBufferSize = PredsOffset + NumJobs * 2 * sizeof(u32);
constexpr u32 PredicateGroupSize = 64;

struct PredicatePushConstants {
    u32 job_count;
    u32 out_start;
    u32 num_queries;
    u32 num_preds;
    u32 jobs_base;
    u32 preds_base;
};

} // Anonymous namespace

OcclusionCounter::OcclusionCounter(const Instance& instance_, Scheduler& scheduler_,
                                   const Core::AddressSpace& address_space, u32 num_dbs_)
    : instance{instance_}, scheduler{scheduler_}, backing_pages{address_space.BackingPages()},
      num_dbs{num_dbs_}, window_draws{MaxWindowDraws},
      predicate_buffer{instance_.GetDevice(), instance_.GetAllocator()} {
    if (!instance.IsHostQueryResetSupported()) {
        return;
    }
    const vk::Device device = instance.GetDevice();
    auto [result, query_pool] = device.createQueryPoolUnique({
        .queryType = vk::QueryType::eOcclusion,
        .queryCount = NumQueries,
    });
    if (result != vk::Result::eSuccess) {
        return;
    }
    pool = std::move(query_pool);
    if (instance.IsOcclusionQueryPreciseSupported()) {
        query_flags = vk::QueryControlFlagBits::ePrecise;
    }
    device.resetQueryPool(*pool, 0, NumQueries);
    results.resize(NumQueries * 2);
    dump_slots.resize(NumDumpSlots);
    if (instance.IsConditionalRenderingSupported()) {
        CreatePredicatePass();
    }
    resolver = std::jthread(std::bind_front(&OcclusionCounter::ResolverThread, this));
}

OcclusionCounter::~OcclusionCounter() {
    if (resolver.joinable()) {
        resolver.request_stop();
        records_cv.notify_all();
        fence_cv.notify_all();
        resolver.join();
    }
}

void OcclusionCounter::CreatePredicatePass() {
    const vk::Device device = instance.GetDevice();
    vk::BufferCreateInfo buffer_ci = {
        .size = PredicateBufferSize,
        .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst |
                 vk::BufferUsageFlagBits::eConditionalRenderingEXT,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    predicate_buffer.Create(buffer_ci, VideoCore::MemoryType::DeviceLocal, nullptr);
    SetObjectName(device, predicate_buffer.buffer, "Occlusion Predicates");

    const vk::DescriptorSetLayoutBinding binding = {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
    };
    predicate_desc_layout = Check(device.createDescriptorSetLayoutUnique({
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
        .bindingCount = 1,
        .pBindings = &binding,
    }));
    const vk::PushConstantRange push_range = {
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(PredicatePushConstants),
    };
    predicate_pipeline_layout = Check(device.createPipelineLayoutUnique({
        .setLayoutCount = 1,
        .pSetLayouts = &*predicate_desc_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_range,
    }));
    const vk::ShaderModule module = CompileSPV(OCCLUSION_PREDICATE_COMP, device);
    predicate_pipeline = Check(device.createComputePipelineUnique(
        {}, {
                .stage =
                    {
                        .stage = vk::ShaderStageFlagBits::eCompute,
                        .module = module,
                        .pName = "main",
                    },
                .layout = *predicate_pipeline_layout,
            }));
    device.destroyShaderModule(module);

    jobs.resize(NumJobs);
    job_upload.reserve(NumJobs * 2);
    predicates_enabled = true;
}

OcclusionCounter::DumpSlot& OcclusionCounter::SlotFor(VAddr address) {
    const u64 hash = (address >> 3) * 0x9E3779B97F4A7C15ULL;
    return dump_slots[hash >> (64 - DumpSlotBits)];
}

bool OcclusionCounter::Dump(VAddr address, u32 dbs, u32 stride) {
    if (!pool) {
        return false;
    }
    End();
    {
        std::scoped_lock lk{mutex};
        const u64 id = next_dump_id++;
        records.push_back({
            .address = address,
            .id = id,
            .query_end = query_write,
            .tick = scheduler.CurrentTick(),
            .num_dbs = dbs ? dbs : num_dbs,
            .stride = stride ? stride : DumpStride,
        });
        u64 job_seq = ~0ULL;
        if (predicates_enabled && (address & 8) != 0) {
            const DumpSlot& begin = SlotFor(address - 8);
            if (begin.address == address - 8 && begin.query_end < query_write) {
                job_seq = next_job_seq++;
                jobs[job_seq % NumJobs] = {
                    .seq = job_seq,
                    .first = begin.query_end,
                    .count = static_cast<u32>(query_write - begin.query_end),
                };
            }
        }
        SlotFor(address) = {
            .address = address,
            .id = id,
            .query_end = query_write,
            .uncounted = uncounted,
            .job_seq = job_seq,
        };
    }
    records_cv.notify_one();
    window_draws = 0;
    return true;
}

void OcclusionCounter::BeforeDraw(u32 db_count_control) {
    if (!pool) {
        return;
    }
    if (db_count_control & ZpassIncrementDisable) {
        End();
        return;
    }
    if (window_draws >= MaxWindowDraws) {
        End();
        ++uncounted;
        return;
    }
    ++window_draws;
    if (active) {
        return;
    }
    if (query_write - query_read.load(std::memory_order_acquire) == NumQueries) {
        LOG_TRACE(Render_Vulkan, "Occlusion query pool exhausted ({}/{}), skipping query",
                  NumQueries, NumQueries);
        ++uncounted;
        return;
    }
    scheduler.CommandBuffer().beginQuery(*pool, query_write % NumQueries, query_flags);
    active = true;
}

void OcclusionCounter::OnEndRendering() {
    End();
}

void OcclusionCounter::End() {
    if (!active) {
        return;
    }
    scheduler.CommandBuffer().endQuery(*pool, query_write % NumQueries);
    ++query_write;
    active = false;
}

ZpassPredicate OcclusionCounter::EvaluateZpass(VAddr address, bool draw_if_visible) {
    if (!pool) {
        return {};
    }
    u64 job_seq;
    {
        std::scoped_lock lk{mutex};
        const DumpSlot& begin = SlotFor(address);
        const DumpSlot& end = SlotFor(address + 8);
        if (begin.address != address || end.address != address + 8 || end.id <= begin.id ||
            end.uncounted != begin.uncounted) {
            LOG_TRACE(Render_Vulkan,
                      "EvaluateZpass: query slot mismatch or uncounted query for addr={:#x}",
                      address);
            return {};
        }
        if (end.resolved) {
            if (!begin.resolved || end.lost != begin.lost) {
                return {};
            }
            return {end.total != begin.total ? ZpassPredicate::Kind::Visible
                                             : ZpassPredicate::Kind::Occluded};
        }
        if (end.query_end == begin.query_end) {
            LOG_TRACE(Render_Vulkan,
                      "EvaluateZpass: empty query range for addr={:#x}, returning occluded",
                      address);
            return {ZpassPredicate::Kind::Occluded};
        }
        job_seq = end.job_seq;
    }
    if (!predicates_enabled || job_seq == ~0ULL) {
        return {};
    }
    const Job& job = jobs[job_seq % NumJobs];
    if (job.seq != job_seq) {
        return {};
    }
    if (job_seq >= computed_seq) {
        ComputePredicates();
    }
    if (!job.computed) {
        return {};
    }
    const u32 word = static_cast<u32>(job_seq % NumJobs) * 2 + (draw_if_visible ? 0 : 1);
    return {ZpassPredicate::Kind::Gpu, PredsOffset + word * static_cast<u32>(sizeof(u32))};
}

void OcclusionCounter::ComputePredicates() {
    const u64 start = std::max(computed_seq, next_job_seq > NumJobs ? next_job_seq - NumJobs : 0);
    const u32 job_count = static_cast<u32>(next_job_seq - start);
    computed_seq = next_job_seq;
    if (job_count == 0) {
        return;
    }

    scheduler.EndRendering();

    u64 copy_first = ~0ULL;
    for (u64 seq = start; seq < next_job_seq; ++seq) {
        copy_first = std::min(copy_first, jobs[seq % NumJobs].first);
    }
    const u64 copy_end = query_write;
    {
        std::scoped_lock lk{mutex};
        copy_first = std::max(copy_first, query_read.load(std::memory_order_acquire));
        if (copy_first < copy_end) {
            hold_begin = std::min(hold_begin, copy_first);
            hold_end = std::max(hold_end, copy_end);
            hold_tick = scheduler.CurrentTick();
        }
    }

    job_upload.clear();
    for (u64 seq = start; seq < next_job_seq; ++seq) {
        Job& job = jobs[seq % NumJobs];
        job.computed = job.first >= copy_first;
        job_upload.push_back(static_cast<u32>(job.first % NumQueries));
        job_upload.push_back(job.computed ? job.count : 0);
    }

    const vk::Buffer buffer = predicate_buffer.buffer;
    const auto cmdbuf = scheduler.CommandBuffer();
    const vk::MemoryBarrier2 to_transfer = {
        .srcStageMask = vk::PipelineStageFlagBits2::eAllCommands,
        .srcAccessMask = vk::AccessFlagBits2::eMemoryWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
    };
    cmdbuf.pipelineBarrier2({
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &to_transfer,
    });
    for (u64 query = copy_first; query < copy_end;) {
        const u32 slot = static_cast<u32>(query % NumQueries);
        const u32 count = static_cast<u32>(std::min<u64>(copy_end - query, NumQueries - slot));
        cmdbuf.copyQueryPoolResults(*pool, slot, count, buffer,
                                    ResultsOffset + slot * 2 * sizeof(u32), 2 * sizeof(u32),
                                    vk::QueryResultFlagBits::eWithAvailability);
        query += count;
    }
    cmdbuf.updateBuffer(buffer, JobsOffset, job_upload.size() * sizeof(u32), job_upload.data());
    const vk::MemoryBarrier2 to_compute = {
        .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eComputeShader,
        .dstAccessMask =
            vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite,
    };
    cmdbuf.pipelineBarrier2({
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &to_compute,
    });

    cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, *predicate_pipeline);
    const vk::DescriptorBufferInfo buffer_info = {
        .buffer = buffer,
        .offset = 0,
        .range = PredicateBufferSize,
    };
    const vk::WriteDescriptorSet write = {
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .pBufferInfo = &buffer_info,
    };
    cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, *predicate_pipeline_layout, 0,
                                write);
    const PredicatePushConstants push = {
        .job_count = job_count,
        .out_start = static_cast<u32>(start % NumJobs),
        .num_queries = NumQueries,
        .num_preds = NumJobs,
        .jobs_base = JobsOffset / sizeof(u32),
        .preds_base = PredsOffset / sizeof(u32),
    };
    cmdbuf.pushConstants(*predicate_pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0,
                         sizeof(push), &push);
    cmdbuf.dispatch((job_count + PredicateGroupSize - 1) / PredicateGroupSize, 1, 1);

    const vk::MemoryBarrier2 to_predicate = {
        .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
        .srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eConditionalRenderingEXT,
        .dstAccessMask = vk::AccessFlagBits2::eConditionalRenderingReadEXT,
    };
    cmdbuf.pipelineBarrier2({
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &to_predicate,
    });
}

u64* OcclusionCounter::Word(VAddr address, u32 db, u32 stride) const {
    const VAddr word = address + db * stride;
    const auto* entry = backing_pages.find(word >> Core::AddressSpace::Traits::PAGE_BITS);
    if (!entry || !*entry) {
        return nullptr;
    }
    const VAddr offset = word & ((VAddr{1} << Core::AddressSpace::Traits::PAGE_BITS) - 1);
    return reinterpret_cast<u64*>(*entry + offset);
}

void OcclusionCounter::ResolverThread(std::stop_token stoken) {
    Common::SetCurrentThreadName("shadPS4:OcclusionResolver");
    std::vector<Record> batch;
    while (!stoken.stop_requested()) {
        {
            std::unique_lock lk{mutex};
            if (!records_cv.wait(lk, stoken, [this] { return !records.empty(); })) {
                return;
            }
            batch.swap(records);
            is_resolving = true;
        }
        for (const Record& record : batch) {
            if (!WaitTick(record.tick, stoken) || !Resolve(record, stoken)) {
                return;
            }
        }
        batch.clear();
        {
            std::unique_lock lk{mutex};
            is_resolving = false;
        }
        fence_cv.notify_all();
    }
}

bool OcclusionCounter::WaitTick(u64 tick, std::stop_token stoken) {
    const vk::Device device = instance.GetDevice();
    const vk::Semaphore semaphore = scheduler.GetWorkSemaphore()->Handle();
    const vk::SemaphoreWaitInfo wait_info = {
        .semaphoreCount = 1,
        .pSemaphores = &semaphore,
        .pValues = &tick,
    };
    while (device.waitSemaphores(wait_info, WaitSliceNs) == vk::Result::eTimeout) {
        if (stoken.stop_requested()) {
            return false;
        }
    }
    return true;
}

bool OcclusionCounter::Resolve(const Record& record, std::stop_token stoken) {
    const vk::Device device = instance.GetDevice();
    const u64 first = query_read.load(std::memory_order_relaxed);
    bool complete = true;
    for (u64 query = first; query < record.query_end;) {
        const u32 slot = query % NumQueries;
        const u32 count =
            static_cast<u32>(std::min<u64>(record.query_end - query, NumQueries - slot));
        const vk::Result result = device.getQueryPoolResults(
            *pool, slot, count, count * 2 * sizeof(u64), results.data(), 2 * sizeof(u64),
            vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWithAvailability);
        const bool read = result == vk::Result::eSuccess || result == vk::Result::eNotReady;
        for (u32 i = 0; i < count; ++i) {
            if (read && results[2 * i + 1] != 0) {
                total += results[2 * i];
            } else {
                complete = false;
                ++lost;
            }
        }
        query += count;
    }

    {
        std::unique_lock lk{mutex};
        while (first < record.query_end && first < hold_end && record.query_end > hold_begin) {
            const u64 tick = hold_tick;
            lk.unlock();
            if (!WaitTick(tick, stoken)) {
                return false;
            }
            lk.lock();
            if (hold_tick == tick) {
                hold_begin = ~0ULL;
                hold_end = 0;
            }
        }
        for (u64 query = first; query < record.query_end;) {
            const u32 slot = query % NumQueries;
            const u32 count =
                static_cast<u32>(std::min<u64>(record.query_end - query, NumQueries - slot));
            device.resetQueryPool(*pool, slot, count);
            query += count;
        }
        query_read.store(record.query_end, std::memory_order_release);
        DumpSlot& slot = SlotFor(record.address);
        if (slot.address == record.address && slot.id == record.id) {
            slot.total = total;
            slot.lost = lost;
            slot.resolved = true;
        }
    }

    if (!complete) {
        return true;
    }
    for (u32 db = 0; db < record.num_dbs; ++db) {
        if (u64* word = Word(record.address, db, record.stride)) {
            std::atomic_ref<u64>{*word}.store(ValidBit | (total + db) / record.num_dbs,
                                              std::memory_order_release);
        }
    }
    return true;
}

void OcclusionCounter::OnFence() {
    if (!pool) {
        return;
    }
    {
        std::scoped_lock lk{mutex};
        if (records.empty() && !is_resolving) {
            return;
        }
    }
    End();
    scheduler.Flush();
    std::unique_lock lk{mutex};
    fence_cv.wait(lk, [this] { return records.empty() && !is_resolving; });
}

} // namespace Vulkan
