// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/container/small_vector.hpp>

#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/renderer_vulkan/vk_compute_pipeline.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/texture_cache.h"

namespace Vulkan {

ComputePipeline::ComputePipeline(const Instance& instance_, Scheduler& scheduler_,
                                 DescriptorHeap& desc_heap_, vk::PipelineCache pipeline_cache,
                                 u64 compute_key_, const Shader::Info& info_,
                                 vk::ShaderModule module)
    : Pipeline{instance_, scheduler_, desc_heap_, pipeline_cache}, compute_key{compute_key_},
      info{&info_} {
    const vk::PipelineShaderStageCreateInfo shader_ci = {
        .stage = vk::ShaderStageFlagBits::eCompute,
        .module = module,
        .pName = "main",
    };

    u32 binding{};
    boost::container::small_vector<vk::DescriptorSetLayoutBinding, 32> bindings;
    for (const auto& buffer : info->buffers) {
        const auto sharp = buffer.GetSharp(*info);
        bindings.push_back({
            .binding = binding++,
            .descriptorType = buffer.IsStorage(sharp) ? vk::DescriptorType::eStorageBuffer
                                                      : vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        });
    }
    for (const auto& tex_buffer : info->texture_buffers) {
        bindings.push_back({
            .binding = binding++,
            .descriptorType = tex_buffer.is_written ? vk::DescriptorType::eStorageTexelBuffer
                                                    : vk::DescriptorType::eUniformTexelBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        });
    }
    for (const auto& image : info->images) {
        bindings.push_back({
            .binding = binding++,
            .descriptorType = image.is_storage ? vk::DescriptorType::eStorageImage
                                               : vk::DescriptorType::eSampledImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        });
    }
    for (const auto& sampler : info->samplers) {
        bindings.push_back({
            .binding = binding++,
            .descriptorType = vk::DescriptorType::eSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        });
    }

    const vk::PushConstantRange push_constants = {
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(Shader::PushData),
    };

    uses_push_descriptors = binding < instance.MaxPushDescriptors();
    const auto flags = uses_push_descriptors
                           ? vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR
                           : vk::DescriptorSetLayoutCreateFlagBits{};
    const vk::DescriptorSetLayoutCreateInfo desc_layout_ci = {
        .flags = flags,
        .bindingCount = static_cast<u32>(bindings.size()),
        .pBindings = bindings.data(),
    };
    auto [descriptor_set_result, descriptor_set] =
        instance.GetDevice().createDescriptorSetLayoutUnique(desc_layout_ci);
    ASSERT_MSG(descriptor_set_result == vk::Result::eSuccess,
               "Failed to create compute descriptor set layout: {}",
               vk::to_string(descriptor_set_result));
    desc_layout = std::move(descriptor_set);

    const vk::DescriptorSetLayout set_layout = *desc_layout;
    const vk::PipelineLayoutCreateInfo layout_info = {
        .setLayoutCount = 1U,
        .pSetLayouts = &set_layout,
        .pushConstantRangeCount = 1U,
        .pPushConstantRanges = &push_constants,
    };
    auto [layout_result, layout] = instance.GetDevice().createPipelineLayoutUnique(layout_info);
    ASSERT_MSG(layout_result == vk::Result::eSuccess,
               "Failed to create compute pipeline layout: {}", vk::to_string(layout_result));
    pipeline_layout = std::move(layout);

    const vk::ComputePipelineCreateInfo compute_pipeline_ci = {
        .stage = shader_ci,
        .layout = *pipeline_layout,
    };
    auto [pipeline_result, pipe] =
        instance.GetDevice().createComputePipelineUnique(pipeline_cache, compute_pipeline_ci);
    ASSERT_MSG(pipeline_result == vk::Result::eSuccess, "Failed to create compute pipeline: {}",
               vk::to_string(pipeline_result));
    pipeline = std::move(pipe);
}

ComputePipeline::~ComputePipeline() = default;

bool ComputePipeline::BindResources(VideoCore::BufferCache& buffer_cache,
                                    VideoCore::TextureCache& texture_cache) const {
    // Bind resource buffers and textures.
    boost::container::small_vector<vk::WriteDescriptorSet, 16> set_writes;
    BufferBarriers buffer_barriers;
    Shader::PushData push_data{};
    Shader::Backend::Bindings binding{};

    info->PushUd(binding, push_data);

    buffer_infos.clear();
    buffer_views.clear();
    image_infos.clear();

    // Most of the time when a metadata is updated with a shader it gets cleared. It means
    // we can skip the whole dispatch and update the tracked state instead. Also, it is not
    // intended to be consumed and in such rare cases (e.g. HTile introspection, CRAA) we
    // will need its full emulation anyways. For cases of metadata read a warning will be logged.
    for (const auto& desc : info->texture_buffers) {
        const VAddr address = desc.GetSharp(*info).base_address;
        if (desc.is_written) {
            if (texture_cache.TouchMeta(address, true)) {
                LOG_TRACE(Render_Vulkan, "Metadata update skipped");
                return false;
            }
        } else {
            if (texture_cache.IsMeta(address)) {
                LOG_WARNING(Render_Vulkan, "Unexpected metadata read by a CS shader (buffer)");
            }
        }
    }

    BindBuffers(buffer_cache, texture_cache, *info, binding, push_data, set_writes,
                buffer_barriers);

    BindTextures(texture_cache, *info, binding, set_writes);

    if (set_writes.empty()) {
        return false;
    }

    const auto cmdbuf = scheduler.CommandBuffer();
    if (!buffer_barriers.empty()) {
        const auto dependencies = vk::DependencyInfo{
            .dependencyFlags = vk::DependencyFlagBits::eByRegion,
            .bufferMemoryBarrierCount = u32(buffer_barriers.size()),
            .pBufferMemoryBarriers = buffer_barriers.data(),
        };
        scheduler.EndRendering();
        cmdbuf.pipelineBarrier2(dependencies);
    }

    cmdbuf.pushConstants(*pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0u, sizeof(push_data),
                         &push_data);

    // Bind descriptor set.
    if (uses_push_descriptors) {
        cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, *pipeline_layout, 0,
                                    set_writes);
        return true;
    }
    const auto desc_set = desc_heap.Commit(*desc_layout);
    for (auto& set_write : set_writes) {
        set_write.dstSet = desc_set;
    }
    instance.GetDevice().updateDescriptorSets(set_writes, {});
    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipeline_layout, 0, desc_set, {});

    return true;
}

} // namespace Vulkan
