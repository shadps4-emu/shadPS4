// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <memory>
#include "video_core/renderer_vulkan/vk_staging_buffer_pool.h"

namespace Vulkan {
class Instance;
class Scheduler;

class DispatchHelper {
public:
    DispatchHelper(const Instance& instance, Scheduler& scheduler, StagingBufferPool& staging_pool);
    vk::DescriptorBufferInfo Prepare(const VideoCore::Buffer* arguments, u64 offset);

private:
    struct Batch;
    const Instance& instance;
    Scheduler& scheduler;
    StagingBufferPool& staging_pool;
    vk::UniqueDescriptorSetLayout descriptor_layout;
    vk::UniquePipelineLayout pipeline_layout;
    vk::UniquePipeline pipeline;
    std::shared_ptr<Batch> batch;
    std::array<u32, 3> max_groups;
    u64 stride;
};
} // namespace Vulkan
