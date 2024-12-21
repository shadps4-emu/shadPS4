// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <boost/container/static_vector.hpp>
#include "common/types.h"
#include "common/unique_function.h"
#include "video_core/renderer_vulkan/vk_master_semaphore.h"
#include "video_core/renderer_vulkan/vk_resource_pool.h"

namespace tracy {
class VkCtxScope;
}

namespace Vulkan {

class Instance;

struct RenderState {
    std::array<vk::RenderingAttachmentInfo, 8> color_attachments{};
    vk::RenderingAttachmentInfo depth_attachment{};
    vk::RenderingAttachmentInfo stencil_attachment{};
    u32 num_color_attachments{};
    bool has_depth{};
    bool has_stencil{};
    u32 width = std::numeric_limits<u32>::max();
    u32 height = std::numeric_limits<u32>::max();

    bool operator==(const RenderState& other) const noexcept {
        return std::memcmp(this, &other, sizeof(RenderState)) == 0;
    }
};

struct SubmitInfo {
    boost::container::static_vector<vk::Semaphore, 3> wait_semas;
    boost::container::static_vector<u64, 3> wait_ticks;
    boost::container::static_vector<vk::Semaphore, 3> signal_semas;
    boost::container::static_vector<u64, 3> signal_ticks;
    vk::Fence fence;

    void AddWait(vk::Semaphore semaphore, u64 tick = 1) {
        wait_semas.emplace_back(semaphore);
        wait_ticks.emplace_back(tick);
    }

    void AddSignal(vk::Semaphore semaphore, u64 tick = 1) {
        signal_semas.emplace_back(semaphore);
        signal_ticks.emplace_back(tick);
    }

    void AddSignal(vk::Fence fence) {
        this->fence = fence;
    }
};

class Scheduler {
public:
    explicit Scheduler(const Instance& instance);
    ~Scheduler();

    /// Sends the current execution context to the GPU
    /// and increments the scheduler timeline semaphore.
    void Flush(SubmitInfo& info);

    /// Sends the current execution context to the GPU and waits for it to complete.
    void Finish();

    /// Waits for the given tick to trigger on the GPU.
    void Wait(u64 tick);

    /// Starts a new rendering scope with provided state.
    void BeginRendering(const RenderState& new_state);

    /// Ends current rendering scope.
    void EndRendering();

    /// Returns the current render state.
    const RenderState& GetRenderState() const {
        return render_state;
    }

    /// Returns the current command buffer.
    vk::CommandBuffer CommandBuffer() const {
        return current_cmdbuf;
    }

    /// Returns the current command buffer tick.
    [[nodiscard]] u64 CurrentTick() const noexcept {
        return master_semaphore.CurrentTick();
    }

    /// Returns true when a tick has been triggered by the GPU.
    [[nodiscard]] bool IsFree(u64 tick) const noexcept {
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

    static std::mutex submit_mutex;

private:
    void AllocateWorkerCommandBuffers();

    void SubmitExecution(SubmitInfo& info);

private:
    const Instance& instance;
    MasterSemaphore master_semaphore;
    CommandPool command_pool;
    vk::CommandBuffer current_cmdbuf;
    std::condition_variable_any event_cv;
    struct PendingOp {
        Common::UniqueFunction<void> callback;
        u64 gpu_tick;
    };
    std::queue<PendingOp> pending_ops;
    RenderState render_state;
    bool is_rendering = false;
    tracy::VkCtxScope* profiler_scope{};
};

} // namespace Vulkan
