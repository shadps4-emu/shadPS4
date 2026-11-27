// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/profile.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/renderer_vulkan/vk_common.h"

#include <span>
#include <boost/container/small_vector.hpp>

namespace Shader {
struct Info;
struct PushData;
} // namespace Shader

namespace Vulkan {

static constexpr auto AllGraphicsStageBits =
    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eTessellationControl |
    vk::ShaderStageFlagBits::eTessellationEvaluation | vk::ShaderStageFlagBits::eGeometry |
    vk::ShaderStageFlagBits::eFragment;

class Instance;
class Scheduler;
class DescriptorHeap;

union DescriptorPayload {
    DescriptorPayload(const vk::DescriptorBufferInfo& buffer_) : buffer{buffer_} {}
    DescriptorPayload(const vk::DescriptorImageInfo& image_) : image{image_} {}

    vk::DescriptorBufferInfo buffer;
    vk::DescriptorImageInfo image;
};
static_assert(sizeof(vk::DescriptorBufferInfo) == sizeof(vk::DescriptorImageInfo));
static_assert(sizeof(DescriptorPayload) == sizeof(vk::DescriptorBufferInfo));
static_assert(std::is_trivially_copyable_v<DescriptorPayload>);

using DescriptorTemplateEntries =
    boost::container::small_vector<vk::DescriptorUpdateTemplateEntry, 32>;

class Pipeline {
public:
    Pipeline(const Instance& instance, Scheduler& scheduler, DescriptorHeap& desc_heap,
             const Shader::Profile& profile, vk::PipelineCache pipeline_cache,
             bool is_compute = false);
    virtual ~Pipeline();

    vk::Pipeline Handle() const noexcept {
        return *pipeline;
    }

    vk::PipelineLayout GetLayout() const noexcept {
        return *pipeline_layout;
    }

    auto GetStages() const {
        static_assert(static_cast<u32>(Shader::SwStage::Compute) == Shader::MaxStageTypes - 1);
        if (is_compute) {
            return std::span{stages.cend() - 1, stages.cend()};
        } else {
            return std::span{stages.cbegin(), stages.cend() - 1};
        }
    }

    const Shader::Info& GetStage(Shader::SwStage stage) const noexcept {
        return *stages[u32(stage)];
    }

    bool IsCompute() const {
        return is_compute;
    }

    void BindResources(std::span<const DescriptorPayload> payload,
                       const Shader::PushData& push_data) const;

protected:
    [[nodiscard]] std::string GetDebugString() const;

    static DescriptorTemplateEntries MakeTemplateEntries(
        std::span<const vk::DescriptorSetLayoutBinding> bindings);
    void CreateDescriptorTemplate(std::span<const vk::DescriptorUpdateTemplateEntry> entries);

    const Instance& instance;
    Scheduler& scheduler;
    DescriptorHeap& desc_heap;
    const Shader::Profile& profile;
    vk::UniquePipeline pipeline;
    vk::UniquePipelineLayout pipeline_layout;
    vk::UniqueDescriptorSetLayout desc_layout;
    vk::UniqueDescriptorUpdateTemplate desc_template;
    u32 num_descriptors{};
    std::array<const Shader::Info*, Shader::MaxStageTypes> stages{};
    bool uses_push_descriptors{};
    bool is_compute;
};

} // namespace Vulkan
