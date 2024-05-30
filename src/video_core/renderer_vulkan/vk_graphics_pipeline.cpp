// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>

#include "common/assert.h"
#include "core/memory.h"
#include "video_core/amdgpu/resource.h"
#include "video_core/renderer_vulkan/vk_graphics_pipeline.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_stream_buffer.h"
#include "video_core/texture_cache/texture_cache.h"

namespace Vulkan {

GraphicsPipeline::GraphicsPipeline(const Instance& instance_, Scheduler& scheduler_,
                                   const GraphicsPipelineKey& key_,
                                   vk::PipelineCache pipeline_cache,
                                   std::span<const Shader::Info*, MaxShaderStages> infos,
                                   std::array<vk::ShaderModule, MaxShaderStages> modules)
    : instance{instance_}, scheduler{scheduler_}, key{key_} {
    const vk::Device device = instance.GetDevice();
    for (u32 i = 0; i < MaxShaderStages; i++) {
        if (!infos[i]) {
            continue;
        }
        stages[i] = *infos[i];
    }
    BuildDescSetLayout();
    const vk::DescriptorSetLayout set_layout = *desc_layout;
    const vk::PipelineLayoutCreateInfo layout_info = {
        .setLayoutCount = 1U,
        .pSetLayouts = &set_layout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };
    pipeline_layout = instance.GetDevice().createPipelineLayoutUnique(layout_info);

    boost::container::static_vector<vk::VertexInputBindingDescription, 32> bindings;
    boost::container::static_vector<vk::VertexInputAttributeDescription, 32> attributes;
    const auto& vs_info = stages[0];
    for (const auto& input : vs_info.vs_inputs) {
        const auto buffer = vs_info.ReadUd<AmdGpu::Buffer>(input.sgpr_base, input.dword_offset);
        attributes.push_back({
            .location = input.binding,
            .binding = input.binding,
            .format = LiverpoolToVK::SurfaceFormat(buffer.data_format, buffer.num_format),
            .offset = 0,
        });
        bindings.push_back({
            .binding = input.binding,
            .stride = buffer.GetStride(),
            .inputRate = vk::VertexInputRate::eVertex,
        });
    }

    const vk::PipelineVertexInputStateCreateInfo vertex_input_info = {
        .vertexBindingDescriptionCount = static_cast<u32>(bindings.size()),
        .pVertexBindingDescriptions = bindings.data(),
        .vertexAttributeDescriptionCount = static_cast<u32>(attributes.size()),
        .pVertexAttributeDescriptions = attributes.data(),
    };

    ASSERT_MSG(key.prim_type != Liverpool::PrimitiveType::RectList || IsEmbeddedVs(),
               "Rectangle List primitive type is only supported for embedded VS");

    const vk::PipelineInputAssemblyStateCreateInfo input_assembly = {
        .topology = LiverpoolToVK::PrimitiveType(key.prim_type),
        .primitiveRestartEnable = false,
    };

    const vk::PipelineRasterizationStateCreateInfo raster_state = {
        .depthClampEnable = false,
        .rasterizerDiscardEnable = false,
        .polygonMode = LiverpoolToVK::PolygonMode(key.polygon_mode),
        .cullMode = LiverpoolToVK::CullMode(key.cull_mode),
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = false,
        .lineWidth = 1.0f,
    };

    const vk::PipelineMultisampleStateCreateInfo multisampling = {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = false,
    };

    const vk::Viewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = 1.0f,
        .height = 1.0f,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    const vk::Rect2D scissor = {
        .offset = {0, 0},
        .extent = {1, 1},
    };

    const vk::PipelineViewportStateCreateInfo viewport_info = {
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    boost::container::static_vector<vk::DynamicState, 14> dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
        vk::DynamicState::eBlendConstants,
    };

    if (instance.IsColorWriteEnableSupported()) {
        dynamic_states.push_back(vk::DynamicState::eColorWriteEnableEXT);
        dynamic_states.push_back(vk::DynamicState::eColorWriteMaskEXT);
    }

    const vk::PipelineDynamicStateCreateInfo dynamic_info = {
        .dynamicStateCount = static_cast<u32>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
    };

    const vk::PipelineDepthStencilStateCreateInfo depth_info = {
        .depthTestEnable = key.depth.depth_enable,
        .depthWriteEnable = key.depth.depth_write_enable,
        .depthCompareOp = LiverpoolToVK::CompareOp(key.depth.depth_func),
        .depthBoundsTestEnable = key.depth.depth_bounds_enable,
        .stencilTestEnable = key.depth.stencil_enable,
        .front{
            .failOp = LiverpoolToVK::StencilOp(key.stencil.stencil_fail_front),
            .passOp = LiverpoolToVK::StencilOp(key.stencil.stencil_zpass_front),
            .depthFailOp = LiverpoolToVK::StencilOp(key.stencil.stencil_zfail_front),
            .compareOp = LiverpoolToVK::CompareOp(key.depth.stencil_ref_func),
            .compareMask = key.stencil_ref_front.stencil_mask,
            .writeMask = key.stencil_ref_front.stencil_write_mask,
            .reference = key.stencil_ref_front.stencil_test_val,
        },
        .back{
            .failOp = LiverpoolToVK::StencilOp(key.stencil.stencil_fail_back),
            .passOp = LiverpoolToVK::StencilOp(key.stencil.stencil_zpass_back),
            .depthFailOp = LiverpoolToVK::StencilOp(key.stencil.stencil_zfail_back),
            .compareOp = LiverpoolToVK::CompareOp(key.depth.stencil_bf_func),
            .compareMask = key.stencil_ref_back.stencil_mask,
            .writeMask = key.stencil_ref_back.stencil_write_mask,
            .reference = key.stencil_ref_back.stencil_test_val,
        },
    };

    u32 shader_count = 1;
    std::array<vk::PipelineShaderStageCreateInfo, MaxShaderStages> shader_stages;
    shader_stages[0] = vk::PipelineShaderStageCreateInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = modules[0],
        .pName = "main",
    };
    if (modules[4]) {
        shader_stages[1] = vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = modules[4],
            .pName = "main",
        };
        ++shader_count;
    }

    const auto it = std::ranges::find(key.color_formats, vk::Format::eUndefined);
    const u32 num_color_formats = std::distance(key.color_formats.begin(), it);
    const vk::PipelineRenderingCreateInfoKHR pipeline_rendering_ci = {
        .colorAttachmentCount = num_color_formats,
        .pColorAttachmentFormats = key.color_formats.data(),
        .depthAttachmentFormat = key.depth.depth_enable ? key.depth_format : vk::Format::eUndefined,
        .stencilAttachmentFormat = vk::Format::eUndefined,
    };

    std::array<vk::PipelineColorBlendAttachmentState, Liverpool::NumColorBuffers> attachments;
    for (u32 i = 0; i < num_color_formats; i++) {
        const auto& control = key.blend_controls[i];
        attachments[i] = vk::PipelineColorBlendAttachmentState{
            .blendEnable = key.blend_controls[i].enable,
            .srcColorBlendFactor = LiverpoolToVK::BlendFactor(control.color_src_factor),
            .dstColorBlendFactor = LiverpoolToVK::BlendFactor(control.color_dst_factor),
            .colorBlendOp = LiverpoolToVK::BlendOp(control.color_func),
            .srcAlphaBlendFactor = LiverpoolToVK::BlendFactor(control.alpha_src_factor),
            .dstAlphaBlendFactor = LiverpoolToVK::BlendFactor(control.color_dst_factor),
            .alphaBlendOp = LiverpoolToVK::BlendOp(control.alpha_func),
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        };
    }

    const vk::PipelineColorBlendStateCreateInfo color_blending = {
        .logicOpEnable = false,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = num_color_formats,
        .pAttachments = attachments.data(),
        .blendConstants = std::array{1.0f, 1.0f, 1.0f, 1.0f},
    };

    const vk::GraphicsPipelineCreateInfo pipeline_info = {
        .pNext = &pipeline_rendering_ci,
        .stageCount = shader_count,
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_info,
        .pRasterizationState = &raster_state,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_info,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_info,
        .layout = *pipeline_layout,
    };

    auto result = device.createGraphicsPipelineUnique(pipeline_cache, pipeline_info);
    if (result.result == vk::Result::eSuccess) {
        pipeline = std::move(result.value);
    } else {
        UNREACHABLE_MSG("Graphics pipeline creation failed!");
    }
}

GraphicsPipeline::~GraphicsPipeline() = default;

void GraphicsPipeline::BuildDescSetLayout() {
    u32 binding{};
    boost::container::small_vector<vk::DescriptorSetLayoutBinding, 32> bindings;
    for (const auto& stage : stages) {
        for (const auto& buffer : stage.buffers) {
            bindings.push_back({
                .binding = binding++,
                .descriptorType = buffer.is_storage ? vk::DescriptorType::eStorageBuffer
                                                    : vk::DescriptorType::eUniformBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            });
        }
        for (const auto& image : stage.images) {
            bindings.push_back({
                .binding = binding++,
                .descriptorType = vk::DescriptorType::eSampledImage,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            });
        }
        for (const auto& sampler : stage.samplers) {
            bindings.push_back({
                .binding = binding++,
                .descriptorType = vk::DescriptorType::eSampler,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            });
        }
    }
    const vk::DescriptorSetLayoutCreateInfo desc_layout_ci = {
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
        .bindingCount = static_cast<u32>(bindings.size()),
        .pBindings = bindings.data(),
    };
    desc_layout = instance.GetDevice().createDescriptorSetLayoutUnique(desc_layout_ci);
}

void GraphicsPipeline::BindResources(Core::MemoryManager* memory, StreamBuffer& staging,
                                     VideoCore::TextureCache& texture_cache) const {
    static constexpr u64 MinUniformAlignment = 64;

    const auto map_staging = [&](auto src, size_t size) {
        const auto [data, offset, _] = staging.Map(size, MinUniformAlignment);
        std::memcpy(data, reinterpret_cast<const void*>(src), size);
        staging.Commit(size);
        return offset;
    };

    std::array<vk::Buffer, MaxVertexBufferCount> buffers;
    std::array<vk::DeviceSize, MaxVertexBufferCount> offsets;
    VAddr base_address = 0;
    u32 start_offset = 0;

    // Bind vertex buffer.
    const auto& vs_info = stages[0];
    const size_t num_buffers = vs_info.vs_inputs.size();
    for (u32 i = 0; i < num_buffers; ++i) {
        const auto& input = vs_info.vs_inputs[i];
        const auto buffer = vs_info.ReadUd<AmdGpu::Buffer>(input.sgpr_base, input.dword_offset);
        if (i == 0) {
            start_offset = map_staging(buffer.base_address.Value(), buffer.GetSize());
            base_address = buffer.base_address;
        }
        buffers[i] = staging.Handle();
        offsets[i] = start_offset + buffer.base_address - base_address;
    }

    const auto cmdbuf = scheduler.CommandBuffer();
    if (num_buffers > 0) {
        cmdbuf.bindVertexBuffers(0, num_buffers, buffers.data(), offsets.data());
    }

    // Bind resource buffers and textures.
    boost::container::static_vector<vk::DescriptorBufferInfo, 4> buffer_infos;
    boost::container::static_vector<vk::DescriptorImageInfo, 8> image_infos;
    boost::container::small_vector<vk::WriteDescriptorSet, 16> set_writes;
    u32 binding{};

    for (const auto& stage : stages) {
        for (const auto& buffer : stage.buffers) {
            const auto vsharp = stage.ReadUd<AmdGpu::Buffer>(buffer.sgpr_base, buffer.dword_offset);
            const u32 size = vsharp.GetSize();
            const u32 offset = map_staging(vsharp.base_address.Value(), size);
            buffer_infos.emplace_back(staging.Handle(), offset, size);
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

        for (const auto& image : stage.images) {
            const auto tsharp = stage.ReadUd<AmdGpu::Image>(image.sgpr_base, image.dword_offset);
            const auto& image_view = texture_cache.FindImageView(tsharp);
            image_infos.emplace_back(VK_NULL_HANDLE, *image_view.image_view,
                                     vk::ImageLayout::eGeneral);
            set_writes.push_back({
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = binding++,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampledImage,
                .pImageInfo = &image_infos.back(),
            });
        }
        for (const auto& sampler : stage.samplers) {
            const auto ssharp =
                stage.ReadUd<AmdGpu::Sampler>(sampler.sgpr_base, sampler.dword_offset);
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
    }

    if (!set_writes.empty()) {
        cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0,
                                    set_writes);
    }
}

} // namespace Vulkan
