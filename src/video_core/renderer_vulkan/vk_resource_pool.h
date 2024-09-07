// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <deque>
#include <vector>
#include <boost/container/static_vector.hpp>
#include <tsl/robin_map.h>

#include "common/types.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace Vulkan {

class Instance;
class MasterSemaphore;

/**
 * Handles a pool of resources protected by fences. Manages resource overflow allocating more
 * resources.
 */
class ResourcePool {
public:
    explicit ResourcePool() = default;
    explicit ResourcePool(MasterSemaphore* master_semaphore, std::size_t grow_step);
    virtual ~ResourcePool() = default;

    ResourcePool& operator=(ResourcePool&&) noexcept = default;
    ResourcePool(ResourcePool&&) noexcept = default;

    ResourcePool& operator=(const ResourcePool&) = default;
    ResourcePool(const ResourcePool&) = default;

protected:
    std::size_t CommitResource();

    /// Called when a chunk of resources have to be allocated.
    virtual void Allocate(std::size_t begin, std::size_t end) = 0;

private:
    /// Manages pool overflow allocating new resources.
    std::size_t ManageOverflow();

protected:
    MasterSemaphore* master_semaphore{nullptr};
    std::size_t grow_step = 0;     ///< Number of new resources created after an overflow
    std::size_t hint_iterator = 0; ///< Hint to where the next free resources is likely to be found
    std::vector<u64> ticks;        ///< Ticks for each resource
};

class CommandPool final : public ResourcePool {
public:
    explicit CommandPool(const Instance& instance, MasterSemaphore* master_semaphore);
    ~CommandPool() override;

    void Allocate(std::size_t begin, std::size_t end) override;

    vk::CommandBuffer Commit();

private:
    const Instance& instance;
    vk::UniqueCommandPool cmd_pool;
    std::vector<vk::CommandBuffer> cmd_buffers;
};

class DescriptorHeap final {
    static constexpr u32 DescriptorSetBatch = 32;

public:
    explicit DescriptorHeap(const Instance& instance, MasterSemaphore* master_semaphore,
                            std::span<const vk::DescriptorPoolSize> pool_sizes,
                            u32 descriptor_heap_count = 1024);
    ~DescriptorHeap();

    vk::DescriptorSet Commit(vk::DescriptorSetLayout set_layout);

private:
    void CreateDescriptorPool();

private:
    vk::Device device;
    MasterSemaphore* master_semaphore;
    u32 descriptor_heap_count;
    std::span<const vk::DescriptorPoolSize> pool_sizes;
    vk::DescriptorPool curr_pool;
    std::deque<std::pair<vk::DescriptorPool, u64>> pending_pools;
    using DescSetBatch = boost::container::static_vector<vk::DescriptorSet, DescriptorSetBatch>;
    tsl::robin_map<u64, DescSetBatch> descriptor_sets;
};

} // namespace Vulkan
