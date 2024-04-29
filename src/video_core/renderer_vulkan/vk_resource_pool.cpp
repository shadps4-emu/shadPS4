// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstddef>
#include <optional>
#include <unordered_map>
#include "common/assert.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_master_semaphore.h"
#include "video_core/renderer_vulkan/vk_resource_pool.h"

namespace Vulkan {

ResourcePool::ResourcePool(MasterSemaphore* master_semaphore_, std::size_t grow_step_)
    : master_semaphore{master_semaphore_}, grow_step{grow_step_} {}

std::size_t ResourcePool::CommitResource() {
    u64 gpu_tick = master_semaphore->KnownGpuTick();
    const auto search = [this, gpu_tick](std::size_t begin,
                                         std::size_t end) -> std::optional<std::size_t> {
        for (std::size_t iterator = begin; iterator < end; ++iterator) {
            if (gpu_tick >= ticks[iterator]) {
                ticks[iterator] = master_semaphore->CurrentTick();
                return iterator;
            }
        }
        return std::nullopt;
    };

    // Try to find a free resource from the hinted position to the end.
    auto found = search(hint_iterator, ticks.size());
    if (!found) {
        // Refresh semaphore to query updated results
        master_semaphore->Refresh();
        gpu_tick = master_semaphore->KnownGpuTick();
        found = search(hint_iterator, ticks.size());
    }
    if (!found) {
        // Search from beginning to the hinted position.
        found = search(0, hint_iterator);
        if (!found) {
            // Both searches failed, the pool is full; handle it.
            const std::size_t free_resource = ManageOverflow();

            ticks[free_resource] = master_semaphore->CurrentTick();
            found = free_resource;
        }
    }

    // Free iterator is hinted to the resource after the one that's been commited.
    hint_iterator = (*found + 1) % ticks.size();
    return *found;
}

std::size_t ResourcePool::ManageOverflow() {
    const std::size_t old_capacity = ticks.size();
    ticks.resize(old_capacity + grow_step);
    Allocate(old_capacity, old_capacity + grow_step);
    return old_capacity;
}

constexpr std::size_t COMMAND_BUFFER_POOL_SIZE = 4;

CommandPool::CommandPool(const Instance& instance, MasterSemaphore* master_semaphore)
    : ResourcePool{master_semaphore, COMMAND_BUFFER_POOL_SIZE}, instance{instance} {
    const vk::CommandPoolCreateInfo pool_create_info = {
        .flags = vk::CommandPoolCreateFlagBits::eTransient |
                 vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = instance.GetGraphicsQueueFamilyIndex(),
    };
    const vk::Device device = instance.GetDevice();
    cmd_pool = device.createCommandPoolUnique(pool_create_info);
    if (instance.HasDebuggingToolAttached()) {
        SetObjectName(device, *cmd_pool, "CommandPool");
    }
}

CommandPool::~CommandPool() = default;

void CommandPool::Allocate(std::size_t begin, std::size_t end) {
    cmd_buffers.resize(end);

    const vk::CommandBufferAllocateInfo buffer_alloc_info = {
        .commandPool = *cmd_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = COMMAND_BUFFER_POOL_SIZE,
    };

    const vk::Device device = instance.GetDevice();
    const auto result =
        device.allocateCommandBuffers(&buffer_alloc_info, cmd_buffers.data() + begin);
    ASSERT(result == vk::Result::eSuccess);

    if (instance.HasDebuggingToolAttached()) {
        for (std::size_t i = begin; i < end; ++i) {
            SetObjectName(device, cmd_buffers[i], "CommandPool: Command Buffer {}", i);
        }
    }
}

vk::CommandBuffer CommandPool::Commit() {
    const std::size_t index = CommitResource();
    return cmd_buffers[index];
}

constexpr u32 DESCRIPTOR_SET_BATCH = 32;

DescriptorHeap::DescriptorHeap(const Instance& instance, MasterSemaphore* master_semaphore,
                               std::span<const vk::DescriptorSetLayoutBinding> bindings,
                               u32 descriptor_heap_count_)
    : ResourcePool{master_semaphore, DESCRIPTOR_SET_BATCH}, device{instance.GetDevice()},
      descriptor_heap_count{descriptor_heap_count_} {
    // Create descriptor set layout.
    const vk::DescriptorSetLayoutCreateInfo layout_ci = {
        .bindingCount = static_cast<u32>(bindings.size()),
        .pBindings = bindings.data(),
    };
    descriptor_set_layout = device.createDescriptorSetLayoutUnique(layout_ci);
    if (instance.HasDebuggingToolAttached()) {
        SetObjectName(device, *descriptor_set_layout, "DescriptorSetLayout");
    }

    // Build descriptor set pool counts.
    std::unordered_map<vk::DescriptorType, u16> descriptor_type_counts;
    for (const auto& binding : bindings) {
        descriptor_type_counts[binding.descriptorType] += binding.descriptorCount;
    }
    for (const auto& [type, count] : descriptor_type_counts) {
        auto& pool_size = pool_sizes.emplace_back();
        pool_size.descriptorCount = count * descriptor_heap_count;
        pool_size.type = type;
    }

    // Create descriptor pool
    AppendDescriptorPool();
}

DescriptorHeap::~DescriptorHeap() = default;

void DescriptorHeap::Allocate(std::size_t begin, std::size_t end) {
    ASSERT(end - begin == DESCRIPTOR_SET_BATCH);
    descriptor_sets.resize(end);
    hashes.resize(end);

    std::array<vk::DescriptorSetLayout, DESCRIPTOR_SET_BATCH> layouts;
    layouts.fill(*descriptor_set_layout);

    u32 current_pool = 0;
    vk::DescriptorSetAllocateInfo alloc_info = {
        .descriptorPool = *pools[current_pool],
        .descriptorSetCount = DESCRIPTOR_SET_BATCH,
        .pSetLayouts = layouts.data(),
    };

    // Attempt to allocate the descriptor set batch. If the pool has run out of space, use a new
    // one.
    while (true) {
        const auto result =
            device.allocateDescriptorSets(&alloc_info, descriptor_sets.data() + begin);
        if (result == vk::Result::eSuccess) {
            break;
        }
        if (result == vk::Result::eErrorOutOfPoolMemory) {
            current_pool++;
            if (current_pool == pools.size()) {
                LOG_INFO(Render_Vulkan, "Run out of pools, creating new one!");
                AppendDescriptorPool();
            }
            alloc_info.descriptorPool = *pools[current_pool];
        }
    }
}

vk::DescriptorSet DescriptorHeap::Commit() {
    const std::size_t index = CommitResource();
    return descriptor_sets[index];
}

void DescriptorHeap::AppendDescriptorPool() {
    const vk::DescriptorPoolCreateInfo pool_info = {
        .flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        .maxSets = descriptor_heap_count,
        .poolSizeCount = static_cast<u32>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };
    auto& pool = pools.emplace_back();
    pool = device.createDescriptorPoolUnique(pool_info);
}

} // namespace Vulkan
