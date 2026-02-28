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

    // Flush graphics to ensure all pending work is submitted
    // We must flush BEFORE checking the tick, since pending commands
    // in the command buffer don't change the tick until submitted.
    graphics_scheduler.Flush();

    // After flush, CurrentTick is N+1, the signaled tick is N
    const auto graphics_tick = graphics_scheduler.CurrentTick() - 1;

    // If we've already synced with this tick, skip adding another wait
    if (graphics_tick <= last_graphics_sync_tick) {
        return;
    }

    const auto graphics_sem = graphics_scheduler.GetTimelineSemaphore();

    std::lock_guard<std::mutex> lk{submit_mutex};
    wait_semaphores.push_back(graphics_sem);
    wait_values.push_back(graphics_tick);
    last_graphics_sync_tick = graphics_tick;
}

void ComputeScheduler::SignalGraphics(Scheduler& graphics_scheduler) {
    if (!is_dedicated) {
        return;
    }

    // If the command buffer is empty (no dispatches since last flush), skip
    if (!has_pending_work) {
        return;
    }

    // RAW Hazard Prevention:
    // Graphics must wait for Compute to finish before reading results.
    Flush();
    has_pending_work = false;

    const auto compute_sem = master_semaphore.Handle();
    const auto signal_value = master_semaphore.CurrentTick() - 1;

    graphics_scheduler.Wait(compute_sem, signal_value);
}

void ComputeScheduler::OnComputeDispatch(Scheduler& graphics_scheduler) {
    // This is called before every dispatch.
    // We can use it to potentially flush if the command buffer is getting too large,
    // or to ensure some baseline sync if needed.
    // For now, we don't force flushes here to allow batching.
}

void ComputeScheduler::AllocateWorkerCommandBuffers() {
    const vk::CommandBufferBeginInfo begin_info = {
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    };

    current_cmdbuf = command_pool.Commit();
    Check(current_cmdbuf.begin(begin_info));
}

void ComputeScheduler::SubmitExecution() {
    std::lock_guard<std::mutex> lk{submit_mutex};

    Check(current_cmdbuf.end());

    const u64 signal_value = master_semaphore.NextTick();
    const vk::Semaphore timeline = master_semaphore.Handle();

    // Build wait semaphore infos using synchronization2
    std::vector<vk::SemaphoreSubmitInfo> wait_infos;
    wait_infos.reserve(wait_semaphores.size());
    for (size_t i = 0; i < wait_semaphores.size(); ++i) {
        ASSERT_MSG(wait_values[i] > 0,
                   "Invalid wait value {} at index {} in compute submit", wait_values[i], i);
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

    // Reset graphics sync tracking so the next compute batch can sync with new graphics work
    last_graphics_sync_tick = 0;

    master_semaphore.Refresh();
    AllocateWorkerCommandBuffers();

    PopPendingOperations();
}

} // namespace Vulkan
