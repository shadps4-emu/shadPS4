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

    static const auto scope_loc = GPU_SCOPE_LOCATION("Guest Frame", MarkersPallete::GpuMarkerColor);
    new (profiler_scope)
        tracy::VkCtxScope{instance.GetProfilerContext(), &scope_loc, current_cmdbuf, true};
}

void Scheduler::SubmitExecution(vk::Semaphore signal_semaphore, vk::Semaphore wait_semaphore) {
    const u64 signal_value = master_semaphore.NextTick();

    profiler_scope->~VkCtxScope();
    TracyVkCollect(instance.GetProfilerContext(), current_cmdbuf);

    std::scoped_lock lk{submit_mutex};
    master_semaphore.SubmitWork(current_cmdbuf, wait_semaphore, signal_semaphore, signal_value);
    master_semaphore.Refresh();
    AllocateWorkerCommandBuffers();
}

} // namespace Vulkan
