// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/backend/bindings.h"
#include "shader_recompiler/info.h"
#include "video_core/renderer_vulkan/vk_common.h"
#include "video_core/texture_cache/texture_cache.h"

namespace VideoCore {
class BufferCache;
} // namespace VideoCore

namespace Vulkan {

static constexpr auto gp_stage_flags = vk::ShaderStageFlagBits::eVertex |
                                       vk::ShaderStageFlagBits::eGeometry |
                                       vk::ShaderStageFlagBits::eFragment;

class Instance;
class Scheduler;
class DescriptorHeap;

class Pipeline {
public:
    Pipeline(const Instance& instance, Scheduler& scheduler, DescriptorHeap& desc_heap,
             vk::PipelineCache pipeline_cache, bool is_compute = false);
    virtual ~Pipeline();

    vk::Pipeline Handle() const noexcept {
        return *pipeline;
    }

    vk::PipelineLayout GetLayout() const noexcept {
        return *pipeline_layout;
    }

    auto GetStages() const {
        if (is_compute) {
            return std::span{stages.cend() - 1, stages.cend()};
        } else {
            return std::span{stages.cbegin(), stages.cend() - 1};
        }
    }

    const Shader::Info& GetStage(Shader::Stage stage) const noexcept {
        return *stages[u32(stage)];
    }

    bool IsCompute() const {
        return is_compute;
    }

    using DescriptorWrites = boost::container::small_vector<vk::WriteDescriptorSet, 16>;
    using BufferBarriers = boost::container::small_vector<vk::BufferMemoryBarrier2, 16>;

    void BindResources(DescriptorWrites& set_writes, const BufferBarriers& buffer_barriers,
                       const Shader::PushData& push_data) const;

protected:
    const Instance& instance;
    Scheduler& scheduler;
    DescriptorHeap& desc_heap;
    vk::UniquePipeline pipeline;
    vk::UniquePipelineLayout pipeline_layout;
    vk::UniqueDescriptorSetLayout desc_layout;
    std::array<const Shader::Info*, Shader::MaxStageTypes> stages{};
    bool uses_push_descriptors{};
    const bool is_compute;
};

} // namespace Vulkan
