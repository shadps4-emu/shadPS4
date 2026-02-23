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

    // WAR Hazard Prevention:
    // We must wait for Graphics to finish using resources before Compute touches them.
    graphics_scheduler.Flush();

    const auto graphics_sem = graphics_scheduler.GetTimelineSemaphore();
    const auto graphics_tick = graphics_scheduler.CurrentTick();

    std::lock_guard<std::mutex> lk{submit_mutex};
    wait_semaphores.push_back(graphics_sem);
    wait_values.push_back(graphics_tick);
}

void ComputeScheduler::SignalGraphics(Scheduler& graphics_scheduler) {
    if (!is_dedicated) {
        return;
    }

    // RAW Hazard Prevention:
    // Graphics must wait for Compute to finish before reading results.
    Flush();

    const auto compute_sem = master_semaphore.Handle();
    const auto signal_value = master_semaphore.CurrentTick();

    // Register the wait on the Graphics Scheduler.
    graphics_scheduler.Wait(compute_sem, signal_value);
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
    const u64 signal_value = master_semaphore.NextTick();

    Check(current_cmdbuf.end());

    const vk::Semaphore timeline = master_semaphore.Handle();

    std::vector<u64> tmp_wait_values = wait_values;
    std::vector<vk::Semaphore> tmp_wait_semaphores = wait_semaphores;

    // Timeline signal info
    const vk::TimelineSemaphoreSubmitInfo timeline_si = {
        .waitSemaphoreValueCount = static_cast<u32>(tmp_wait_values.size()),
        .pWaitSemaphoreValues = tmp_wait_values.data(),
        .signalSemaphoreValueCount = 1,
        .pSignalSemaphoreValues = &signal_value,
    };

    // For compute, we usually wait at the start of the pipe
    std::vector<vk::PipelineStageFlags> wait_stages(tmp_wait_semaphores.size(),
                                                    vk::PipelineStageFlagBits::eComputeShader);

    const vk::SubmitInfo submit_info = {
        .pNext = &timeline_si,
        .waitSemaphoreCount = static_cast<u32>(tmp_wait_semaphores.size()),
        .pWaitSemaphores = tmp_wait_semaphores.data(),
        .pWaitDstStageMask = wait_stages.data(),
        .commandBufferCount = 1U,
        .pCommandBuffers = &current_cmdbuf,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &timeline,
    };

    auto submit_result = compute_queue.submit(submit_info, nullptr);
    if (submit_result == vk::Result::eErrorDeviceLost) {
        LOG_CRITICAL(Render_Vulkan, "Device lost during compute submit!");
    }

    // Clear waits after submission
    wait_semaphores.clear();
    wait_values.clear();

    master_semaphore.Refresh();
    AllocateWorkerCommandBuffers();

    PopPendingOperations();
}

} // namespace Vulkan
