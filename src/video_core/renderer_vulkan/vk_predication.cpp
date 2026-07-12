// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/memory.h"
#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_platform.h"
#include "video_core/renderer_vulkan/vk_predication.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"

#include "video_core/host_shaders/occlusion_predicate_comp.h"

namespace Vulkan {

static constexpr u64 OcclusionCounterValidMask = 0x8000000000000000ULL;

struct ReducePushConstants {
    u32 src_index;
    u32 count;
    u32 dst_index;
    u32 combine;
};

/// The predicate-value readback exists only to feed the PredValue debug traces; skip its GPU
/// copy and deferred host read entirely unless debug logging for this class is enabled.
static bool TraceEnabled() {
    const auto logger = Common::Log::ALL_LOGGERS[Common::Log::Class::Render_Vulkan];
    return logger && logger->should_log(spdlog::level::debug);
}

static vk::BufferMemoryBarrier2 MakeBufferBarrier(
    vk::Buffer buffer, vk::PipelineStageFlags2 src_stage, vk::AccessFlags2 src_access,
    vk::PipelineStageFlags2 dst_stage, vk::AccessFlags2 dst_access, vk::DeviceSize offset = 0,
    vk::DeviceSize size = VK_WHOLE_SIZE) {
    return {
        .srcStageMask = src_stage,
        .srcAccessMask = src_access,
        .dstStageMask = dst_stage,
        .dstAccessMask = dst_access,
        .buffer = buffer,
        .offset = offset,
        .size = size,
    };
}

static void RecordBufferBarrier(vk::CommandBuffer cmdbuf, const vk::BufferMemoryBarrier2& barrier) {
    cmdbuf.pipelineBarrier2(vk::DependencyInfo{
        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &barrier,
    });
}

static vk::BufferUsageFlags PredicateBufferUsage(const Instance& instance) {
    vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eStorageBuffer |
                                 vk::BufferUsageFlagBits::eTransferSrc |
                                 vk::BufferUsageFlagBits::eTransferDst;
    if (instance.IsConditionalRenderingSupported()) {
        usage |= vk::BufferUsageFlagBits::eConditionalRenderingEXT;
    }
    return usage;
}

PredicationManager::PredicationManager(const Instance& instance_, Scheduler& scheduler_,
                                       VideoCore::BufferCache& buffer_cache_)
    : instance{instance_}, scheduler{scheduler_}, buffer_cache{buffer_cache_},
      supported{instance_.IsConditionalRenderingSupported()},
      predicate_buffer{instance_,
                       scheduler_,
                       VideoCore::MemoryUsage::DeviceLocal,
                       0,
                       PredicateBufferUsage(instance_),
                       NumPredicateSlots * sizeof(u32)},
      counter_scratch{instance_,
                      scheduler_,
                      VideoCore::MemoryUsage::DeviceLocal,
                      0,
                      vk::BufferUsageFlagBits::eStorageBuffer |
                          vk::BufferUsageFlagBits::eTransferDst,
                      NumScratchQwords * sizeof(u64)},
      trace_buffer{instance_,
                   scheduler_,
                   VideoCore::MemoryUsage::Download,
                   0,
                   vk::BufferUsageFlagBits::eTransferDst,
                   NumTraceSlots * sizeof(u32)} {
    const auto device = instance.GetDevice();
    const vk::QueryPoolCreateInfo query_pool_info = {
        .queryType = vk::QueryType::eOcclusion,
        .queryCount = QueryPoolSize,
    };
    query_pool = Check(device.createQueryPoolUnique(query_pool_info));
    SetObjectName(device, *query_pool, "ZPASS Query Pool");
    if (instance.IsHostQueryResetSupported()) {
        // Slots are reset up front and again in bulk as they are released, keeping the
        // per-draw acquisition path free of driver calls.
        device.resetQueryPool(*query_pool, 0, QueryPoolSize);
    }
    SetObjectName(device, predicate_buffer.Handle(), "Predicate Buffer");
    SetObjectName(device, counter_scratch.Handle(), "Predicate Counter Scratch");
    SetObjectName(device, trace_buffer.Handle(), "Predicate Trace Buffer");

    const std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {{
        {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        {
            .binding = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
    }};
    const vk::DescriptorSetLayoutCreateInfo desc_layout_ci = {
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
        .bindingCount = static_cast<u32>(bindings.size()),
        .pBindings = bindings.data(),
    };
    reduce_desc_layout = Check(device.createDescriptorSetLayoutUnique(desc_layout_ci));

    const auto module =
        Compile(HostShaders::OCCLUSION_PREDICATE_COMP, vk::ShaderStageFlagBits::eCompute, device);
    SetObjectName(device, module, "Occlusion Predicate Reduce");

    const vk::PushConstantRange push_range = {
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(ReducePushConstants),
    };
    const vk::PipelineLayoutCreateInfo layout_info = {
        .setLayoutCount = 1U,
        .pSetLayouts = &(*reduce_desc_layout),
        .pushConstantRangeCount = 1U,
        .pPushConstantRanges = &push_range,
    };
    reduce_pipeline_layout = Check(device.createPipelineLayoutUnique(layout_info));

    const vk::ComputePipelineCreateInfo pipeline_info = {
        .stage =
            vk::PipelineShaderStageCreateInfo{
                .stage = vk::ShaderStageFlagBits::eCompute,
                .module = module,
                .pName = "main",
            },
        .layout = *reduce_pipeline_layout,
    };
    reduce_pipeline = Check(device.createComputePipelineUnique({}, pipeline_info));
    SetObjectName(device, *reduce_pipeline, "Occlusion Predicate Reduce Pipeline");
    device.destroyShaderModule(module);

    if (!supported) {
        LOG_WARNING(Render_Vulkan,
                    "VK_EXT_conditional_rendering is unavailable; PM4 predication will fail open");
    }
}

PredicationManager::~PredicationManager() = default;

void PredicationManager::ControlZpassCounting() {
    counting_enabled = !counting_enabled;
}

void PredicationManager::ResetZpassCounting() {
    zpass_counter = 0;
    if (!active_queries.empty()) {
        ReleaseQuerySlotsWhenDone(std::move(active_queries));
        active_queries.clear();
    }
    active_poisoned = false;
}

void PredicationManager::DumpZpassCounters(VAddr address, u32 num_counter_pairs) {
    if (address == 0 || num_counter_pairs == 0) {
        return;
    }

    auto record = std::make_shared<DumpRecord>(DumpRecord{
        .address = address,
        .num_counter_pairs = num_counter_pairs,
        .queries = std::move(active_queries),
        .poisoned = active_poisoned,
    });
    active_queries.clear();
    active_poisoned = false;
    records[address] = record;

    scheduler.DeferOperation([this, record] {
        ResolveRecord(*record);
        if (const auto it = records.find(record->address);
            it != records.end() && it->second == record) {
            records.erase(it);
        }
        // Predicate copies can only reference these slots while the record is reachable, so
        // any such copy is recorded by now; free the slots once the work queued so far is done.
        if (!record->queries.empty()) {
            ReleaseQuerySlotsWhenDone(std::move(record->queries));
        }
    });
}

void PredicationManager::EnableFromZpass(VAddr address, u32 num_counter_pairs, bool draw_visible,
                                         bool combine) {
    CloseWindow();
    ++predication_seq;

    if (address == 0) {
        SetStatic(std::nullopt, draw_visible, combine, "null_address");
        return;
    }

    // The predicate covers the window between the begin dump (at address) and the end dump
    // (at address + 8); the end dump captured exactly the queries recorded in between.
    const auto it = records.find(address + sizeof(u64));
    if (it == records.end()) {
        // No live measurement window. If a previous window already resolved, the guest block
        // holds final counters and can be read without waiting; otherwise fail open.
        SetStatic(ReadGuestZpass(address, num_counter_pairs), draw_visible, combine, "guest");
        return;
    }

    const DumpRecord& record = *it->second;
    if (!supported || record.poisoned || record.queries.size() > NumScratchQwords) {
        SetStatic(std::nullopt, draw_visible, combine, "fail_open");
        return;
    }
    if (record.queries.empty()) {
        // An empty window cannot have passed any samples.
        SetStatic(false, draw_visible, combine, "empty");
        return;
    }

    const bool fast_path = BuildFromQueries(record.queries, draw_visible, combine);
    TracePredicate(gpu_slot, address);
    LOG_DEBUG(Render_Vulkan, "PredSet\tseq={}\tpath={}\taddr={:#x}\tqueries={}\tinvert={}",
              predication_seq, fast_path ? "zpass_gpu_fast" : "zpass_gpu_reduce", address,
              record.queries.size(), gpu_inverted);
}

void PredicationManager::EnableFromBool(VAddr address, bool is_64bit, bool draw_visible,
                                        bool combine) {
    CloseWindow();
    ++predication_seq;

    if (!supported || address == 0) {
        SetStatic(std::nullopt, draw_visible, combine, "fail_open");
        return;
    }

    // The boolean may be produced by GPU work queued earlier in this very command stream, so
    // it must be sampled on the GPU timeline rather than at parse time. Obtain the guest
    // buffer before touching the command buffer: the cache lookup may flush the scheduler.
    const u32 width = is_64bit ? sizeof(u64) : sizeof(u32);
    const auto [buffer, offset] = buffer_cache.ObtainBuffer(address, width, false);
    scheduler.EndRendering();
    const auto cmdbuf = scheduler.CommandBuffer();
    const u32 qword = AllocScratchQwords(1);

    if (!is_64bit) {
        // Zero the full qword so the 32-bit copy below leaves the high dword clear.
        cmdbuf.fillBuffer(counter_scratch.Handle(), qword * sizeof(u64), sizeof(u64), 0);
        RecordBufferBarrier(cmdbuf, MakeBufferBarrier(counter_scratch.Handle(),
                                                      vk::PipelineStageFlagBits2::eAllTransfer,
                                                      vk::AccessFlagBits2::eTransferWrite,
                                                      vk::PipelineStageFlagBits2::eAllTransfer,
                                                      vk::AccessFlagBits2::eTransferWrite,
                                                      qword * sizeof(u64), sizeof(u64)));
    }
    if (const auto barrier = buffer->GetBarrier(vk::AccessFlagBits2::eTransferRead,
                                                vk::PipelineStageFlagBits2::eAllTransfer)) {
        RecordBufferBarrier(cmdbuf, *barrier);
    }
    const vk::BufferCopy copy = {
        .srcOffset = offset,
        .dstOffset = qword * sizeof(u64),
        .size = width,
    };
    cmdbuf.copyBuffer(buffer->Handle(), counter_scratch.Handle(), copy);

    bool combine_on_gpu = false;
    const u32 slot = SelectPredicateSlot(cmdbuf, combine, combine_on_gpu);
    ReducePredicate(qword, 1, slot, combine_on_gpu);
    ActivateGpuPredicate(slot, draw_visible);

    TracePredicate(slot, address);
    LOG_DEBUG(Render_Vulkan, "PredSet\tseq={}\tpath=bool_gpu\taddr={:#x}\twidth={}\tinvert={}",
              predication_seq, address, width * 8, gpu_inverted);
}

void PredicationManager::Disable() {
    CloseWindow();
    mode = Mode::Disabled;
    static_visible.reset();
    static_execute = true;
    LOG_DEBUG(Render_Vulkan, "PredClear\tseq={}", predication_seq);
}

bool PredicationManager::ShouldSkipPredicatedPacket() {
    if (mode == Mode::Static && !static_execute) {
        ++skipped_packets;
        return true;
    }
    return false;
}

std::optional<u32> PredicationManager::PrepareDrawQuery() {
    if (!counting_enabled) {
        return std::nullopt;
    }

    if (active_queries.size() >= QueryPoolSize / 2) {
        // A window that never dumps must not exhaust the pool; drop the oldest half. The
        // window can no longer produce an exact count, so its predicate fails open.
        const auto mid = active_queries.begin() + active_queries.size() / 2;
        std::vector<u32> stale(active_queries.begin(), mid);
        active_queries.erase(active_queries.begin(), mid);
        active_poisoned = true;
        ReleaseQuerySlotsWhenDone(std::move(stale));
    }

    const auto slot = AcquireQuerySlot();
    if (!slot) {
        LOG_WARNING(Render_Vulkan, "ZPASS query pool exhausted; predicate will fail open");
        active_poisoned = true;
        return std::nullopt;
    }

    if (!instance.IsHostQueryResetSupported()) {
        // Without host query reset the slot must be reset on the GPU timeline before use, and
        // vkCmdResetQueryPool is invalid while dynamic rendering is active. With host reset,
        // free slots were already reset in bulk when they were released.
        scheduler.EndRendering();
        scheduler.CommandBuffer().resetQueryPool(*query_pool, *slot, 1);
    }
    return slot;
}

void PredicationManager::BeginDraw(vk::CommandBuffer cmdbuf, std::optional<u32> query,
                                   bool packet_predicated) {
    if (packet_predicated && mode == Mode::Gpu) {
        vk::ConditionalRenderingFlagsEXT flags{};
        if (gpu_inverted) {
            flags |= vk::ConditionalRenderingFlagBitsEXT::eInverted;
        }
        const vk::ConditionalRenderingBeginInfoEXT info = {
            .buffer = predicate_buffer.Handle(),
            .offset = gpu_slot * sizeof(u32),
            .flags = flags,
        };
        cmdbuf.beginConditionalRenderingEXT(info);
        ++wrapped_draws;
    }
    if (query) {
        cmdbuf.beginQuery(*query_pool, *query, {});
    }
}

void PredicationManager::EndDraw(vk::CommandBuffer cmdbuf, std::optional<u32> query,
                                 bool packet_predicated) {
    if (query) {
        cmdbuf.endQuery(*query_pool, *query);
        active_queries.emplace_back(*query);
    }
    if (packet_predicated && mode == Mode::Gpu) {
        cmdbuf.endConditionalRenderingEXT();
    }
}

std::optional<u32> PredicationManager::AcquireQuerySlot() {
    for (u32 attempt = 0; attempt < QueryPoolSize; ++attempt) {
        const u32 slot = (query_cursor + attempt) % QueryPoolSize;
        if (!query_busy[slot]) {
            query_busy[slot] = true;
            query_cursor = slot + 1;
            return slot;
        }
    }
    return std::nullopt;
}

void PredicationManager::ReleaseQuerySlotsWhenDone(std::vector<u32>&& slots) {
    scheduler.DeferOperation([this, slots = std::move(slots)]() mutable {
        for (const u32 slot : slots) {
            query_busy[slot] = false;
        }
        if (!instance.IsHostQueryResetSupported()) {
            return;
        }
        // The GPU tick containing the last use of these slots has completed, so they can be
        // host-reset here in contiguous runs instead of one driver call per draw at acquire.
        std::ranges::sort(slots);
        const auto device = instance.GetDevice();
        for (size_t i = 0; i < slots.size();) {
            size_t run = 1;
            while (i + run < slots.size() && slots[i + run] == slots[i] + run) {
                ++run;
            }
            device.resetQueryPool(*query_pool, slots[i], static_cast<u32>(run));
            i += run;
        }
    });
}

void PredicationManager::ResolveRecord(const DumpRecord& record) {
    const auto device = instance.GetDevice();
    u64 samples = 0;
    for (const u32 slot : record.queries) {
        u64 result{};
        const auto vk_result =
            device.getQueryPoolResults(*query_pool, slot, 1, sizeof(result), &result,
                                       sizeof(result), vk::QueryResultFlagBits::e64);
        if (vk_result == vk::Result::eSuccess) {
            samples += result;
        } else {
            LOG_WARNING(Render_Vulkan, "ZPASS query {} unavailable at writeback: {}", slot,
                        vk::to_string(vk_result));
        }
    }
    zpass_counter += samples;
    WriteGuestCounters(record.address, record.num_counter_pairs, zpass_counter);
    LOG_DEBUG(Render_Vulkan, "PredDump\taddr={:#x}\tqueries={}\tsamples={}\tcounter={}",
              record.address, record.queries.size(), samples, zpass_counter);
}

void PredicationManager::WriteGuestCounters(VAddr address, u32 num_counter_pairs,
                                            u64 counter) const {
    const u64 value = counter | OcclusionCounterValidMask;
    auto* memory = Core::Memory::Instance();
    const u64 result_size = u64(num_counter_pairs) * sizeof(u64) * 2;
    if (!memory->IsValidMapping(address, result_size)) {
        LOG_WARNING(Render_Vulkan, "Pixel-pipe stats writeback address is invalid: {:#x}", address);
        return;
    }
    for (u32 i = 0; i < num_counter_pairs; ++i) {
        auto* dst = reinterpret_cast<void*>(address + i * sizeof(u64) * 2);
        if (!memory->TryWriteBacking(dst, &value, sizeof(value))) {
            std::memcpy(dst, &value, sizeof(value));
        }
    }
}

std::optional<bool> PredicationManager::ReadGuestZpass(VAddr address, u32 num_counter_pairs) const {
    auto* memory = Core::Memory::Instance();
    const u64 result_size = u64(num_counter_pairs) * sizeof(u64) * 2;
    if (!memory->IsValidMapping(address, result_size)) {
        return std::nullopt;
    }
    const auto* results = reinterpret_cast<const u64*>(address);
    bool visible = false;
    for (u32 i = 0; i < num_counter_pairs; ++i) {
        const u64 begin = results[i * 2];
        const u64 end = results[i * 2 + 1];
        if ((begin & end & OcclusionCounterValidMask) == 0) {
            return std::nullopt;
        }
        visible |= (begin & ~OcclusionCounterValidMask) != (end & ~OcclusionCounterValidMask);
    }
    return visible;
}

void PredicationManager::SetStatic(std::optional<bool> visible, bool draw_visible, bool combine,
                                   const char* path) {
    if (combine) {
        if (mode == Mode::Gpu && !visible.value_or(true)) {
            // OR-ing a false term leaves the previous GPU predicate as the result.
            gpu_inverted = !draw_visible;
            LOG_DEBUG(Render_Vulkan, "PredSet\tseq={}\tpath={}+gpu\tinvert={}", predication_seq,
                      path, gpu_inverted);
            return;
        }
        if (mode == Mode::Static) {
            if (static_visible.has_value() && visible.has_value()) {
                visible = *static_visible || *visible;
            } else {
                visible.reset();
            }
        }
    }
    mode = Mode::Static;
    static_visible = visible;
    static_execute = visible.has_value() ? (draw_visible ? *visible : !*visible) : true;
    LOG_DEBUG(Render_Vulkan, "PredSet\tseq={}\tpath={}\tvisible={}\texecute={}", predication_seq,
              path, visible.has_value() ? (*visible ? "1" : "0") : "?", static_execute);
}

bool PredicationManager::BuildFromQueries(const std::vector<u32>& queries, bool draw_visible,
                                          bool combine) {
    scheduler.EndRendering();
    const auto cmdbuf = scheduler.CommandBuffer();

    if (queries.size() == 1 && !combine) {
        // Fast path for the canonical one-proxy-draw window: copy the low 32 bits of the query
        // result straight into the predicate slot. A non-zero sample count means visible, so no
        // reduction pass is needed; a count of exactly 2^32 is not reachable in practice.
        const u32 slot = AllocPredicateSlot();
        RecordBufferBarrier(
            cmdbuf,
            MakeBufferBarrier(predicate_buffer.Handle(), vk::PipelineStageFlagBits2::eAllCommands,
                              vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
                              vk::PipelineStageFlagBits2::eAllTransfer,
                              vk::AccessFlagBits2::eTransferWrite));
        cmdbuf.copyQueryPoolResults(*query_pool, queries.front(), 1, predicate_buffer.Handle(),
                                    slot * sizeof(u32), sizeof(u32),
                                    vk::QueryResultFlagBits::eWait);
        RecordBufferBarrier(cmdbuf,
                            MakeBufferBarrier(predicate_buffer.Handle(),
                                              vk::PipelineStageFlagBits2::eAllTransfer,
                                              vk::AccessFlagBits2::eTransferWrite,
                                              vk::PipelineStageFlagBits2::eConditionalRenderingEXT |
                                                  vk::PipelineStageFlagBits2::eAllTransfer,
                                              vk::AccessFlagBits2::eConditionalRenderingReadEXT |
                                                  vk::AccessFlagBits2::eTransferRead));
        ActivateGpuPredicate(slot, draw_visible);
        return true;
    }

    // Copy the query results into the scratch buffer as contiguous runs. The GPU waits for
    // result availability itself (kPredicationZPassHintWait semantics), so the CPU never does.
    std::vector<u32> sorted{queries};
    std::ranges::sort(sorted);
    const u32 count = static_cast<u32>(sorted.size());
    const u32 base = AllocScratchQwords(count);
    u32 dst = base;
    for (u32 i = 0; i < count;) {
        u32 run = 1;
        while (i + run < count && sorted[i + run] == sorted[i] + run) {
            ++run;
        }
        cmdbuf.copyQueryPoolResults(*query_pool, sorted[i], run, counter_scratch.Handle(),
                                    dst * sizeof(u64), sizeof(u64),
                                    vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait);
        dst += run;
        i += run;
    }

    bool combine_on_gpu = false;
    const u32 slot = SelectPredicateSlot(cmdbuf, combine, combine_on_gpu);
    ReducePredicate(base, count, slot, combine_on_gpu);
    ActivateGpuPredicate(slot, draw_visible);
    return false;
}

u32 PredicationManager::SelectPredicateSlot(vk::CommandBuffer cmdbuf, bool combine,
                                            bool& combine_on_gpu) {
    if (combine && mode == Mode::Gpu) {
        combine_on_gpu = true;
        return gpu_slot;
    }
    const u32 slot = AllocPredicateSlot();
    if (combine && mode == Mode::Static && static_visible.has_value()) {
        // Seed the slot with the previous visibility so the reduction ORs into it.
        cmdbuf.fillBuffer(predicate_buffer.Handle(), slot * sizeof(u32), sizeof(u32),
                          *static_visible ? 1u : 0u);
        combine_on_gpu = true;
    }
    return slot;
}

void PredicationManager::ActivateGpuPredicate(u32 slot, bool draw_visible) {
    mode = Mode::Gpu;
    gpu_slot = slot;
    gpu_inverted = !draw_visible;
}

u32 PredicationManager::AllocPredicateSlot() {
    // Slots are recycled ring-style; with 1024 slots and a handful of predications per frame a
    // slot is only reused many frames after its last conditional-rendering read.
    return predicate_cursor++ % NumPredicateSlots;
}

u32 PredicationManager::AllocScratchQwords(u32 count) {
    ASSERT(count <= NumScratchQwords);
    if (scratch_cursor + count > NumScratchQwords) {
        scratch_cursor = 0;
    }
    const u32 base = scratch_cursor;
    scratch_cursor += count;
    return base;
}

void PredicationManager::ReducePredicate(u32 src_index, u32 count, u32 dst_slot,
                                         bool combine_on_gpu) {
    const auto cmdbuf = scheduler.CommandBuffer();

    const std::array<vk::BufferMemoryBarrier2, 2> pre_barriers = {{
        MakeBufferBarrier(counter_scratch.Handle(), vk::PipelineStageFlagBits2::eAllTransfer,
                          vk::AccessFlagBits2::eTransferWrite,
                          vk::PipelineStageFlagBits2::eComputeShader,
                          vk::AccessFlagBits2::eShaderRead),
        MakeBufferBarrier(predicate_buffer.Handle(), vk::PipelineStageFlagBits2::eAllCommands,
                          vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
                          vk::PipelineStageFlagBits2::eComputeShader,
                          vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite),
    }};
    cmdbuf.pipelineBarrier2(vk::DependencyInfo{
        .bufferMemoryBarrierCount = static_cast<u32>(pre_barriers.size()),
        .pBufferMemoryBarriers = pre_barriers.data(),
    });

    const vk::DescriptorBufferInfo scratch_info = {
        .buffer = counter_scratch.Handle(),
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };
    const vk::DescriptorBufferInfo predicate_info = {
        .buffer = predicate_buffer.Handle(),
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };
    const std::array<vk::WriteDescriptorSet, 2> writes = {{
        {
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &scratch_info,
        },
        {
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = 1,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &predicate_info,
        },
    }};
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, *reduce_pipeline);
    cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, *reduce_pipeline_layout, 0,
                                writes);
    const ReducePushConstants push_constants = {
        .src_index = src_index,
        .count = count,
        .dst_index = dst_slot,
        .combine = combine_on_gpu ? 1u : 0u,
    };
    cmdbuf.pushConstants(*reduce_pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0,
                         sizeof(push_constants), &push_constants);
    cmdbuf.dispatch(1, 1, 1);

    RecordBufferBarrier(cmdbuf,
                        MakeBufferBarrier(predicate_buffer.Handle(),
                                          vk::PipelineStageFlagBits2::eComputeShader,
                                          vk::AccessFlagBits2::eShaderWrite,
                                          vk::PipelineStageFlagBits2::eConditionalRenderingEXT |
                                              vk::PipelineStageFlagBits2::eAllTransfer,
                                          vk::AccessFlagBits2::eConditionalRenderingReadEXT |
                                              vk::AccessFlagBits2::eTransferRead));
}

void PredicationManager::TracePredicate(u32 slot, VAddr address) {
    if (!TraceEnabled()) {
        return;
    }
    const u32 trace_slot = trace_cursor++ % NumTraceSlots;
    const vk::BufferCopy copy = {
        .srcOffset = slot * sizeof(u32),
        .dstOffset = trace_slot * sizeof(u32),
        .size = sizeof(u32),
    };
    scheduler.CommandBuffer().copyBuffer(predicate_buffer.Handle(), trace_buffer.Handle(), copy);
    scheduler.DeferOperation([this, trace_slot, seq = predication_seq, address] {
        const u32 value = reinterpret_cast<const u32*>(trace_buffer.mapped_data.data())[trace_slot];
        LOG_DEBUG(Render_Vulkan, "PredValue\tseq={}\taddr={:#x}\tvisible={}", seq, address, value);
    });
}

void PredicationManager::CloseWindow() {
    if (mode == Mode::Disabled) {
        return;
    }
    LOG_DEBUG(Render_Vulkan, "PredEnd\tseq={}\twrapped={}\tskipped={}", predication_seq,
              wrapped_draws, skipped_packets);
    wrapped_draws = 0;
    skipped_packets = 0;
}

} // namespace Vulkan
