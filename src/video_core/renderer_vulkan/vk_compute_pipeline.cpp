// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/container/small_vector.hpp>
#include "common/alignment.h"
#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/renderer_vulkan/vk_compute_pipeline.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/texture_cache.h"

namespace Vulkan {

ComputePipeline::ComputePipeline(const Instance& instance_, Scheduler& scheduler_,
                                 vk::PipelineCache pipeline_cache, u64 compute_key_,
                                 const Program* program)
    : instance{instance_}, scheduler{scheduler_}, compute_key{compute_key_},
      info{&program->pgm.info} {
    const vk::PipelineShaderStageCreateInfo shader_ci = {
        .stage = vk::ShaderStageFlagBits::eCompute,
        .module = program->module,
        .pName = "main",
    };

    u32 binding{};
    boost::container::small_vector<vk::DescriptorSetLayoutBinding, 32> bindings;
    for (const auto& buffer : info->buffers) {
        bindings.push_back({
            .binding = binding++,
            .descriptorType = buffer.is_storage ? vk::DescriptorType::eStorageBuffer
                                                : vk::DescriptorType::eUniformBuffer,
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

    const vk::DescriptorSetLayoutCreateInfo desc_layout_ci = {
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
        .bindingCount = static_cast<u32>(bindings.size()),
        .pBindings = bindings.data(),
    };
    desc_layout = instance.GetDevice().createDescriptorSetLayoutUnique(desc_layout_ci);

    const vk::DescriptorSetLayout set_layout = *desc_layout;
    const vk::PipelineLayoutCreateInfo layout_info = {
        .setLayoutCount = 1U,
        .pSetLayouts = &set_layout,
        .pushConstantRangeCount = 1U,
        .pPushConstantRanges = &push_constants,
    };
    pipeline_layout = instance.GetDevice().createPipelineLayoutUnique(layout_info);

    const vk::ComputePipelineCreateInfo compute_pipeline_ci = {
        .stage = shader_ci,
        .layout = *pipeline_layout,
    };
    auto result =
        instance.GetDevice().createComputePipelineUnique(pipeline_cache, compute_pipeline_ci);
    if (result.result == vk::Result::eSuccess) {
        pipeline = std::move(result.value);
    } else {
        UNREACHABLE_MSG("Graphics pipeline creation failed!");
    }
}

ComputePipeline::~ComputePipeline() = default;

bool ComputePipeline::BindResources(VideoCore::BufferCache& buffer_cache,
                                    VideoCore::TextureCache& texture_cache) const {
    // Bind resource buffers and textures.
    boost::container::static_vector<vk::DescriptorBufferInfo, 16> buffer_infos;
    boost::container::static_vector<vk::DescriptorImageInfo, 16> image_infos;
    boost::container::small_vector<vk::WriteDescriptorSet, 16> set_writes;
    Shader::PushData push_data{};
    u32 binding{};

    for (const auto& buffer : info->buffers) {
        const auto vsharp = buffer.GetVsharp(*info);
        const VAddr address = vsharp.base_address;
        // Most of the time when a metadata is updated with a shader it gets cleared. It means we
        // can skip the whole dispatch and update the tracked state instead. Also, it is not
        // intended to be consumed and in such rare cases (e.g. HTile introspection, CRAA) we will
        // need its full emulation anyways. For cases of metadata read a warning will be logged.
        if (buffer.is_storage) {
            if (texture_cache.TouchMeta(address, true)) {
                LOG_WARNING(Render_Vulkan, "Metadata update skipped");
                return false;
            }
        } else {
            if (texture_cache.IsMeta(address)) {
                LOG_WARNING(Render_Vulkan, "Unexpected metadata read by a CS shader (buffer)");
            }
        }
        const u32 size = vsharp.GetSize();
        if (buffer.is_written) {
            texture_cache.InvalidateMemory(address, size, true);
        }
        const u32 alignment =
            buffer.is_storage ? instance.StorageMinAlignment() : instance.UniformMinAlignment();
        const auto [vk_buffer, offset] =
            buffer_cache.ObtainBuffer(address, size, buffer.is_written);
        const u32 offset_aligned = Common::AlignDown(offset, alignment);
        const u32 adjust = offset - offset_aligned;
        if (adjust != 0) {
            ASSERT(adjust % 4 == 0);
            push_data.AddOffset(binding, adjust);
        }
        buffer_infos.emplace_back(vk_buffer->Handle(), offset_aligned, size + adjust);
        set_writes.push_back({
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = binding++,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = buffer.is_storage ? vk::DescriptorType::eStorageBuffer
                                                : vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &buffer_infos.back(),
        });
    }

    for (const auto& image_desc : info->images) {
        const auto tsharp =
            info->ReadUd<AmdGpu::Image>(image_desc.sgpr_base, image_desc.dword_offset);
        VideoCore::ImageInfo image_info{tsharp};
        VideoCore::ImageViewInfo view_info{tsharp, image_desc.is_storage};
        const auto& image_view = texture_cache.FindTexture(image_info, view_info);
        const auto& image = texture_cache.GetImage(image_view.image_id);
        image_infos.emplace_back(VK_NULL_HANDLE, *image_view.image_view, image.layout);
        set_writes.push_back({
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = binding++,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = image_desc.is_storage ? vk::DescriptorType::eStorageImage
                                                    : vk::DescriptorType::eSampledImage,
            .pImageInfo = &image_infos.back(),
        });

        if (texture_cache.IsMeta(tsharp.Address())) {
            LOG_WARNING(Render_Vulkan, "Unexpected metadata read by a CS shader (texture)");
        }
    }
    for (const auto& sampler : info->samplers) {
        const auto ssharp = sampler.GetSsharp(*info);
        const auto vk_sampler = texture_cache.GetSampler(ssharp);
        image_infos.emplace_back(vk_sampler, VK_NULL_HANDLE, vk::ImageLayout::eGeneral);
        set_writes.push_back({
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = binding++,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eSampler,
            .pImageInfo = &image_infos.back(),
        });
    }

    if (set_writes.empty()) {
        return false;
    }

    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.pushConstants(*pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0u, sizeof(push_data),
                         &push_data);
    cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, *pipeline_layout, 0, set_writes);
    return true;
}

} // namespace Vulkan
