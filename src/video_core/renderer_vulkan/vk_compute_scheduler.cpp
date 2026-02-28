// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "video_core/renderer_vulkan/vk_compute_scheduler.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"

namespace Vulkan {

ComputeScheduler::ComputeScheduler(const Instance& instance_)
    : instance{instance_}, master_semaphore{instance},
      command_pool{instance, &master_semaphore, instance.GetComputeQueueFamilyIndex()},
      compute_queue{instance.GetComputeQueue()}, is_dedicated{instance.HasDedicatedComputeQueue()} {
    AllocateWorkerCommandBuffers();
    LOG_INFO(Render_Vulkan, "ComputeScheduler initialized (dedicated queue: {})", is_dedicated);
}

ComputeScheduler::~ComputeScheduler() = default;

void ComputeScheduler::Flush() {
    SubmitExecution();
}

void ComputeScheduler::Finish() {
    const u64 presubmit_tick = CurrentTick();
    SubmitExecution();
    Wait(presubmit_tick);
}

void ComputeScheduler::Wait(u64 tick) {
    if (tick >= master_semaphore.CurrentTick()) {
        Flush();
    }
    master_semaphore.Wait(tick);
}

void ComputeScheduler::PopPendingOperations() {
    master_semaphore.Refresh();
    while (!pending_ops.empty() && master_semaphore.IsFree(pending_ops.front().gpu_tick)) {
        pending_ops.front().callback();
        pending_ops.pop();
    }
}

void ComputeScheduler::WaitForGraphics(Scheduler& graphics_scheduler) {
    if (!is_dedicated) {
        // If sharing the queue, standard pipeline barriers handle this.
        graphics_scheduler.EndRendering();
        return;
    }

    // End any active rendering
    graphics_scheduler.EndRendering();

    // The tick we want to wait for is the CURRENT graphics tick minus one (the last submitted)
    // Graphics scheduler's CurrentTick() is always the one it's BUILDING, not the one it just submitted.
    const auto graphics_tick = graphics_scheduler.CurrentTick() - 1;

    // If we've already synced with this tick or a later one, skip adding another wait
    if (graphics_tick <= last_graphics_sync_tick || graphics_tick == 0) {
        return;
    }

    const auto graphics_sem = graphics_scheduler.GetTimelineSemaphore();

    std::lock_guard<std::mutex> lk{submit_mutex};
    
    // Check if we already have a wait for this semaphore in the current batch
    bool already_waiting = false;
    for (size_t i = 0; i < wait_semaphores.size(); ++i) {
        if (wait_semaphores[i] == graphics_sem) {
            wait_values[i] = std::max(wait_values[i], graphics_tick);
            already_waiting = true;
            break;
        }
    }

    if (!already_waiting) {
        wait_semaphores.push_back(graphics_sem);
        wait_values.push_back(graphics_tick);
    }
    
    last_graphics_sync_tick = graphics_tick;
}

void ComputeScheduler::SignalGraphics(Scheduler& graphics_scheduler) {
    if (!is_dedicated || !has_pending_work) {
        return;
    }

    // RAW Hazard Prevention:
    // Graphics must wait for Compute to finish before reading results.
    // We flush all pending compute work to the GPU.
    Flush();

    const auto compute_sem = master_semaphore.Handle();
    // The tick we just submitted in Flush() is CurrentTick() - 1
    const auto signal_value = master_semaphore.CurrentTick() - 1;

    if (signal_value > 0) {
        graphics_scheduler.Wait(compute_sem, signal_value);
    }
}

void ComputeScheduler::OnComputeDispatch(Scheduler& graphics_scheduler) {
    // Mark that we have work that needs to be synced later
    has_pending_work = true;
    
    // Baseline sync: ensure compute is waiting for current graphics state if not already doing so
    WaitForGraphics(graphics_scheduler);
}

void ComputeScheduler::AllocateWorkerCommandBuffers() {
    const vk::CommandBufferBeginInfo begin_info = {
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    };

    current_cmdbuf = command_pool.Commit();
    Check(current_cmdbuf.begin(begin_info));
    has_pending_work = false;
}

void ComputeScheduler::SubmitExecution() {
    std::lock_guard<std::mutex> lk{submit_mutex};

    if (!has_pending_work && wait_semaphores.empty()) {
        // No work to submit and no waits to process
        return;
    }

    Check(current_cmdbuf.end());

    const u64 signal_value = master_semaphore.NextTick();
    const vk::Semaphore timeline = master_semaphore.Handle();

    // Global memory barrier to ensure compute writes are visible to subsequent reads
    // Since we don't have fine-grained resource tracking yet, this is the safest way.
    vk::MemoryBarrier2 memory_barrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
        .srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eAllGraphics | vk::PipelineStageFlagBits2::eComputeShader,
        .dstAccessMask = vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eUniformRead,
    };

    vk::DependencyInfo dependency_info = {
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &memory_barrier,
    };

    // Build wait semaphore infos using synchronization2
    std::vector<vk::SemaphoreSubmitInfo> wait_infos;
    wait_infos.reserve(wait_semaphores.size());
    for (size_t i = 0; i < wait_semaphores.size(); ++i) {
        wait_infos.push_back({
            .semaphore = wait_semaphores[i],
            .value = wait_values[i],
            .stageMask = vk::PipelineStageFlagBits2::eComputeShader,
        });
    }

    // Signal semaphore info
    const vk::SemaphoreSubmitInfo signal_info = {
        .semaphore = timeline,
        .value = signal_value,
        .stageMask = vk::PipelineStageFlagBits2::eComputeShader,
    };

    // Command buffer info
    const vk::CommandBufferSubmitInfo cmdbuf_info = {
        .commandBuffer = current_cmdbuf,
    };

    // Use vkQueueSubmit2 (synchronization2)
    const vk::SubmitInfo2 submit_info = {
        .waitSemaphoreInfoCount = static_cast<u32>(wait_infos.size()),
        .pWaitSemaphoreInfos = wait_infos.data(),
        .commandBufferInfoCount = 1U,
        .pCommandBufferInfos = &cmdbuf_info,
        .signalSemaphoreInfoCount = 1U,
        .pSignalSemaphoreInfos = &signal_info,
    };

    auto submit_result = compute_queue.submit2(submit_info, nullptr);
    ASSERT_MSG(submit_result != vk::Result::eErrorDeviceLost,
               "Device lost during compute submit! signal_value={}", signal_value);

    // Clear waits after submission
    wait_semaphores.clear();
    wait_values.clear();

    master_semaphore.Refresh();
    AllocateWorkerCommandBuffers();

    PopPendingOperations();
}

} // namespace Vulkan
