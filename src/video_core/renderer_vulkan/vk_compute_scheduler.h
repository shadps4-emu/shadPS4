// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <queue>
#include <vector>

#include "common/unique_function.h"
#include "video_core/renderer_vulkan/vk_master_semaphore.h"
#include "video_core/renderer_vulkan/vk_resource_pool.h"

namespace Vulkan {

class Instance;
class Scheduler;

/// Scheduler for async compute operations on a dedicated compute queue
class ComputeScheduler {
public:
    explicit ComputeScheduler(const Instance& instance);
    ~ComputeScheduler();

    /// Sends the current execution context to the GPU and increments the timeline semaphore.
    void Flush();

    /// Sends the current execution context to the GPU and waits for it to complete.
    void Finish();

    /// Waits for the given tick to trigger on the GPU.
    void Wait(u64 tick);

    /// Returns the current command buffer.
    vk::CommandBuffer CommandBuffer() const {
        return current_cmdbuf;
    }

    /// Returns the current command buffer tick.
    [[nodiscard]] u64 CurrentTick() const noexcept {
        return master_semaphore.CurrentTick();
    }

    /// Returns true when a tick has been triggered by the GPU.
    [[nodiscard]] bool IsFree(u64 tick) noexcept {
        if (master_semaphore.IsFree(tick)) {
            return true;
        }
        master_semaphore.Refresh();
        return master_semaphore.IsFree(tick);
    }

    /// Returns the master timeline semaphore.
    [[nodiscard]] MasterSemaphore* GetMasterSemaphore() noexcept {
        return &master_semaphore;
    }

    /// Defers an operation until the gpu has reached the current cpu tick.
    void DeferOperation(Common::UniqueFunction<void>&& func) {
        pending_ops.emplace(std::move(func), CurrentTick());
    }

    /// Attempts to execute operations whose tick the GPU has caught up with.
    void PopPendingOperations();

    /// Inserts a barrier to wait for the graphics queue to finish before compute.
    void WaitForGraphics(Scheduler& graphics_scheduler);

    /// Signals the graphics queue that compute has finished.
    void SignalGraphics(Scheduler& graphics_scheduler);

    /// Returns true if this scheduler uses a dedicated compute queue
    bool IsDedicated() const {
        return is_dedicated;
    }

private:
    void AllocateWorkerCommandBuffers();
    void SubmitExecution();

private:
    const Instance& instance;
    MasterSemaphore master_semaphore;
    CommandPool command_pool;
    vk::CommandBuffer current_cmdbuf;
    vk::Queue compute_queue;

    struct PendingOp {
        Common::UniqueFunction<void> callback;
        u64 gpu_tick;
    };
    std::queue<PendingOp> pending_ops;

    bool is_dedicated{false};
    std::mutex submit_mutex;

    // Semaphores to wait on for the next submission
    std::vector<vk::Semaphore> wait_semaphores;
    std::vector<u64> wait_values;
};

} // namespace Vulkan
