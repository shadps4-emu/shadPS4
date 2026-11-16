// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <utility>
#include <boost/container/static_vector.hpp>

#include "shader_recompiler/resource.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_pipeline_cache.h"
#include "video_core/renderer_vulkan/vk_pipeline_common.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"

namespace Vulkan {

Pipeline::Pipeline(const Instance& instance_, Scheduler& scheduler_, DescriptorHeap& desc_heap_,
                   const Shader::Profile& profile_, vk::PipelineCache pipeline_cache,
                   bool is_compute_ /*= false*/)
    : instance{instance_}, scheduler{scheduler_}, desc_heap{desc_heap_}, profile{profile_},
      is_compute{is_compute_} {}

Pipeline::~Pipeline() = default;

DescriptorTemplateEntries Pipeline::MakeTemplateEntries(
    std::span<const vk::DescriptorSetLayoutBinding> bindings) {
    DescriptorTemplateEntries entries;
    size_t offset = 0;
    for (const auto& binding : bindings) {
        entries.push_back({
            .dstBinding = binding.binding,
            .dstArrayElement = 0,
            .descriptorCount = binding.descriptorCount,
            .descriptorType = binding.descriptorType,
            .offset = offset,
            .stride = sizeof(DescriptorPayload),
        });
        offset += binding.descriptorCount * sizeof(DescriptorPayload);
    }
    return entries;
}

void Pipeline::CreateDescriptorTemplate(
    std::span<const vk::DescriptorUpdateTemplateEntry> entries) {
    num_descriptors = 0;
    for (const auto& entry : entries) {
        num_descriptors += entry.descriptorCount;
    }
    if (entries.empty()) {
        return;
    }
    const vk::DescriptorUpdateTemplateCreateInfo template_ci = {
        .descriptorUpdateEntryCount = static_cast<u32>(entries.size()),
        .pDescriptorUpdateEntries = entries.data(),
        .templateType = uses_push_descriptors ? vk::DescriptorUpdateTemplateType::ePushDescriptors
                                              : vk::DescriptorUpdateTemplateType::eDescriptorSet,
        .descriptorSetLayout = uses_push_descriptors ? vk::DescriptorSetLayout{} : *desc_layout,
        .pipelineBindPoint =
            IsCompute() ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics,
        .pipelineLayout = *pipeline_layout,
        .set = 0,
    };
    auto [result, descriptor_template] =
        instance.GetDevice().createDescriptorUpdateTemplateUnique(template_ci);
    ASSERT_MSG(result == vk::Result::eSuccess, "Failed to create descriptor update template: {}",
               vk::to_string(result));
    desc_template = std::move(descriptor_template);
}

void Pipeline::BindResources(std::span<const DescriptorPayload> payload,
                             const Shader::PushData& push_data) const {
    const auto cmdbuf = scheduler.CommandBuffer();
    const auto bind_point =
        IsCompute() ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics;

    const auto stage_flags = IsCompute() ? vk::ShaderStageFlagBits::eCompute : AllGraphicsStageBits;
    cmdbuf.pushConstants(*pipeline_layout, stage_flags, 0u, sizeof(push_data), &push_data);

    if (num_descriptors == 0) {
        return;
    }

    // Layouts built without live sharps (preloaded pipelines) can disagree with the draw about
    // array sizes. Pad with null descriptors or truncate so the template never reads past the data.
    boost::container::small_vector<DescriptorPayload, 64> adjusted;
    if (payload.size() != num_descriptors) [[unlikely]] {
        static bool warned = false;
        if (!std::exchange(warned, true)) {
            LOG_WARNING(Render_Vulkan,
                        "Descriptor payload size {} doesn't match pipeline layout ({})",
                        payload.size(), num_descriptors);
        }
        adjusted.assign(payload.begin(),
                        payload.begin() + std::min<size_t>(payload.size(), num_descriptors));
        adjusted.resize(num_descriptors, DescriptorPayload{vk::DescriptorImageInfo{}});
        payload = adjusted;
    }

    if (uses_push_descriptors) {
        cmdbuf.pushDescriptorSetWithTemplateKHR(*desc_template, *pipeline_layout, 0,
                                                payload.data());
        return;
    }

    const auto desc_set = desc_heap.Commit(*desc_layout);
    instance.GetDevice().updateDescriptorSetWithTemplate(desc_set, *desc_template, payload.data());
    cmdbuf.bindDescriptorSets(bind_point, *pipeline_layout, 0, desc_set, {});
}

std::string Pipeline::GetDebugString() const {
    std::string stage_desc;
    for (const auto& stage : stages) {
        if (stage) {
            const auto shader_name = PipelineCache::GetShaderName(stage->hw_stage, stage->pgm_hash);
            if (stage_desc.empty()) {
                stage_desc = shader_name;
            } else {
                stage_desc = fmt::format("{},{}", stage_desc, shader_name);
            }
        }
    }
    return stage_desc;
}

} // namespace Vulkan
