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
    boost::container::static_vector<vk::BufferView, 8> buffer_views;
    boost::container::static_vector<vk::DescriptorBufferInfo, 32> buffer_infos;
    boost::container::small_vector<vk::WriteDescriptorSet, 16> set_writes;
    boost::container::small_vector<vk::BufferMemoryBarrier2, 16> buffer_barriers;
    Shader::PushData push_data{};
    Shader::Backend::Bindings binding{};

    image_infos.clear();

    info->PushUd(binding, push_data);
    for (const auto& desc : info->buffers) {
        bool is_storage = true;
        if (desc.is_gds_buffer) {
            auto* vk_buffer = buffer_cache.GetGdsBuffer();
            buffer_infos.emplace_back(vk_buffer->Handle(), 0, vk_buffer->SizeBytes());
        } else {
            const auto vsharp = desc.GetSharp(*info);
            is_storage = desc.IsStorage(vsharp);
            const VAddr address = vsharp.base_address;
            // Most of the time when a metadata is updated with a shader it gets cleared. It means
            // we can skip the whole dispatch and update the tracked state instead. Also, it is not
            // intended to be consumed and in such rare cases (e.g. HTile introspection, CRAA) we
            // will need its full emulation anyways. For cases of metadata read a warning will be
            // logged.
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
            const u32 size = vsharp.GetSize();
            const u32 alignment =
                is_storage ? instance.StorageMinAlignment() : instance.UniformMinAlignment();
            const auto [vk_buffer, offset] =
                buffer_cache.ObtainBuffer(address, size, desc.is_written);
            const u32 offset_aligned = Common::AlignDown(offset, alignment);
            const u32 adjust = offset - offset_aligned;
            ASSERT(adjust % 4 == 0);
            push_data.AddOffset(binding.buffer, adjust);
            buffer_infos.emplace_back(vk_buffer->Handle(), offset_aligned, size + adjust);
        }
        set_writes.push_back({
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = binding.unified++,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = is_storage ? vk::DescriptorType::eStorageBuffer
                                         : vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &buffer_infos.back(),
        });
        ++binding.buffer;
    }

    const auto null_buffer_view =
        instance.IsNullDescriptorSupported() ? VK_NULL_HANDLE : buffer_cache.NullBufferView();
    for (const auto& desc : info->texture_buffers) {
        const auto vsharp = desc.GetSharp(*info);
        vk::BufferView& buffer_view = buffer_views.emplace_back(null_buffer_view);
        const u32 size = vsharp.GetSize();
        if (vsharp.GetDataFmt() != AmdGpu::DataFormat::FormatInvalid && size != 0) {
            const VAddr address = vsharp.base_address;
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
            const u32 alignment = instance.TexelBufferMinAlignment();
            const auto [vk_buffer, offset] =
                buffer_cache.ObtainBuffer(address, size, desc.is_written, true);
            const u32 fmt_stride = AmdGpu::NumBits(vsharp.GetDataFmt()) >> 3;
            ASSERT_MSG(fmt_stride == vsharp.GetStride(),
                       "Texel buffer stride must match format stride");
            const u32 offset_aligned = Common::AlignDown(offset, alignment);
            const u32 adjust = offset - offset_aligned;
            ASSERT(adjust % fmt_stride == 0);
            push_data.AddOffset(binding.buffer, adjust / fmt_stride);
            buffer_view = vk_buffer->View(offset_aligned, size + adjust, desc.is_written,
                                          vsharp.GetDataFmt(), vsharp.GetNumberFmt());
            if (auto barrier =
                    vk_buffer->GetBarrier(desc.is_written ? vk::AccessFlagBits2::eShaderWrite
                                                          : vk::AccessFlagBits2::eShaderRead,
                                          vk::PipelineStageFlagBits2::eComputeShader)) {
                buffer_barriers.emplace_back(*barrier);
            }
            if (desc.is_written) {
                texture_cache.InvalidateMemoryFromGPU(address, size);
            }
        }
        set_writes.push_back({
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = binding.unified++,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = desc.is_written ? vk::DescriptorType::eStorageTexelBuffer
                                              : vk::DescriptorType::eUniformTexelBuffer,
            .pTexelBufferView = &buffer_view,
        });
        ++binding.buffer;
    }

    BindTextures(texture_cache, *info, binding, set_writes);

    for (const auto& sampler : info->samplers) {
        const auto ssharp = sampler.GetSharp(*info);
        if (ssharp.force_degamma) {
            LOG_WARNING(Render_Vulkan, "Texture requires gamma correction");
        }
        const auto vk_sampler = texture_cache.GetSampler(ssharp);
        image_infos.emplace_back(vk_sampler, VK_NULL_HANDLE, vk::ImageLayout::eGeneral);
        set_writes.push_back({
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = binding.unified++,
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

    if (!buffer_barriers.empty()) {
        const auto dependencies = vk::DependencyInfo{
            .dependencyFlags = vk::DependencyFlagBits::eByRegion,
            .bufferMemoryBarrierCount = u32(buffer_barriers.size()),
            .pBufferMemoryBarriers = buffer_barriers.data(),
        };
        scheduler.EndRendering();
        cmdbuf.pipelineBarrier2(dependencies);
    }

    if (uses_push_descriptors) {
        cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, *pipeline_layout, 0,
                                    set_writes);
    } else {
        const auto desc_set = desc_heap.Commit(*desc_layout);
        for (auto& set_write : set_writes) {
            set_write.dstSet = desc_set;
        }
        instance.GetDevice().updateDescriptorSets(set_writes, {});
        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipeline_layout, 0, desc_set,
                                  {});
    }

    cmdbuf.pushConstants(*pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0u, sizeof(push_data),
                         &push_data);
    return true;
}

} // namespace Vulkan
