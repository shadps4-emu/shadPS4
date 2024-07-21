// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include "common/debug.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"

namespace Vulkan {

Scheduler::Scheduler(const Instance& instance)
    : instance{instance}, master_semaphore{instance}, command_pool{instance, &master_semaphore} {
    profiler_scope = reinterpret_cast<tracy::VkCtxScope*>(std::malloc(sizeof(tracy::VkCtxScope)));
    AllocateWorkerCommandBuffers();
}

Scheduler::~Scheduler() {
    std::free(profiler_scope);
}

void Scheduler::BeginRendering(const RenderState& new_state) {
    if (is_rendering && render_state == new_state) {
        return;
    }
    EndRendering();
    is_rendering = true;
    render_state = new_state;

    const vk::RenderingInfo rendering_info = {
        .renderArea =
            {
                .offset = {0, 0},
                .extent = {render_state.width, render_state.height},
            },
        .layerCount = 1,
        .colorAttachmentCount = render_state.num_color_attachments,
        .pColorAttachments = render_state.color_attachments.data(),
        .pDepthAttachment =
            render_state.num_depth_attachments ? &render_state.depth_attachment : nullptr,
    };

    current_cmdbuf.beginRendering(rendering_info);
}

void Scheduler::EndRendering() {
    if (!is_rendering) {
        return;
    }
    is_rendering = false;
    current_cmdbuf.endRendering();
}

void Scheduler::Flush(vk::Semaphore signal, vk::Semaphore wait) {
    // When flushing, we only send data to the worker thread; no waiting is necessary.
    SubmitExecution(signal, wait);
}

void Scheduler::Finish(vk::Semaphore signal, vk::Semaphore wait) {
    // When finishing, we need to wait for the submission to have executed on the device.
    const u64 presubmit_tick = CurrentTick();
    SubmitExecution(signal, wait);
    Wait(presubmit_tick);
}

void Scheduler::Wait(u64 tick) {
    if (tick >= master_semaphore.CurrentTick()) {
        // Make sure we are not waiting for the current tick without signalling
        Flush();
    }
    master_semaphore.Wait(tick);
}

void Scheduler::AllocateWorkerCommandBuffers() {
    const vk::CommandBufferBeginInfo begin_info = {
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    };

    current_cmdbuf = command_pool.Commit();
    current_cmdbuf.begin(begin_info);

    auto* profiler_ctx = instance.GetProfilerContext();
    if (profiler_ctx) {
        static const auto scope_loc =
            GPU_SCOPE_LOCATION("Guest Frame", MarkersPallete::GpuMarkerColor);
        new (profiler_scope) tracy::VkCtxScope{profiler_ctx, &scope_loc, current_cmdbuf, true};
    }
}

void Scheduler::SubmitExecution(vk::Semaphore signal_semaphore, vk::Semaphore wait_semaphore) {
    std::scoped_lock lk{submit_mutex};
    const u64 signal_value = master_semaphore.NextTick();

    auto* profiler_ctx = instance.GetProfilerContext();
    if (profiler_ctx) {
        profiler_scope->~VkCtxScope();
        TracyVkCollect(profiler_ctx, current_cmdbuf);
    }

    EndRendering();
    master_semaphore.SubmitWork(current_cmdbuf, wait_semaphore, signal_semaphore, signal_value);
    master_semaphore.Refresh();
    AllocateWorkerCommandBuffers();

    // Apply pending operations
    while (!pending_ops.empty() && IsFree(pending_ops.front().gpu_tick)) {
        pending_ops.front().callback();
        pending_ops.pop();
    }
}

} // namespace Vulkan
