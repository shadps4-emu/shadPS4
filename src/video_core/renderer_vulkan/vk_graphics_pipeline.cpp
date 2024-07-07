// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
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

    const vk::PushConstantRange push_constants = {
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = 2 * sizeof(u32),
    };

    const vk::DescriptorSetLayout set_layout = *desc_layout;
    const vk::PipelineLayoutCreateInfo layout_info = {
        .setLayoutCount = 1U,
        .pSetLayouts = &set_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constants,
    };
    pipeline_layout = instance.GetDevice().createPipelineLayoutUnique(layout_info);

    boost::container::static_vector<vk::VertexInputBindingDescription, 32> bindings;
    boost::container::static_vector<vk::VertexInputAttributeDescription, 32> attributes;
    const auto& vs_info = stages[0];
    for (const auto& input : vs_info.vs_inputs) {
        if (input.instance_step_rate == Shader::Info::VsInput::InstanceIdType::OverStepRate0 ||
            input.instance_step_rate == Shader::Info::VsInput::InstanceIdType::OverStepRate1) {
            // Skip attribute binding as the data will be pulled by shader
            continue;
        }

        const auto buffer = vs_info.ReadUd<AmdGpu::Buffer>(input.sgpr_base, input.dword_offset);
        attributes.push_back({
            .location = input.binding,
            .binding = input.binding,
            .format = LiverpoolToVK::SurfaceFormat(buffer.GetDataFmt(), buffer.GetNumberFmt()),
            .offset = 0,
        });
        bindings.push_back({
            .binding = input.binding,
            .stride = buffer.GetStride(),
            .inputRate = input.instance_step_rate == Shader::Info::VsInput::None
                             ? vk::VertexInputRate::eVertex
                             : vk::VertexInputRate::eInstance,
        });
    }

    const vk::PipelineVertexInputStateCreateInfo vertex_input_info = {
        .vertexBindingDescriptionCount = static_cast<u32>(bindings.size()),
        .pVertexBindingDescriptions = bindings.data(),
        .vertexAttributeDescriptionCount = static_cast<u32>(attributes.size()),
        .pVertexAttributeDescriptions = attributes.data(),
    };

    if (key.prim_type == Liverpool::PrimitiveType::RectList && !IsEmbeddedVs()) {
        LOG_WARNING(Render_Vulkan,
                    "Rectangle List primitive type is only supported for embedded VS");
    }

    const vk::PipelineInputAssemblyStateCreateInfo input_assembly = {
        .topology = LiverpoolToVK::PrimitiveType(key.prim_type),
        .primitiveRestartEnable = false,
    };

    const vk::PipelineRasterizationStateCreateInfo raster_state = {
        .depthClampEnable = false,
        .rasterizerDiscardEnable = false,
        .polygonMode = LiverpoolToVK::PolygonMode(key.polygon_mode),
        .cullMode = vk::CullModeFlagBits::eNone /*LiverpoolToVK::CullMode(key.cull_mode)*/,
        .frontFace = key.front_face == Liverpool::FrontFace::Clockwise
                         ? vk::FrontFace::eClockwise
                         : vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = bool(key.depth_bias_enable),
        .depthBiasConstantFactor = key.depth_bias_const_factor,
        .depthBiasClamp = key.depth_bias_clamp,
        .depthBiasSlopeFactor = key.depth_bias_slope_factor,
        .lineWidth = 1.0f,
    };

    const vk::PipelineMultisampleStateCreateInfo multisampling = {
        .rasterizationSamples = LiverpoolToVK::NumSamples(key.num_samples),
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

    const vk::PipelineViewportDepthClipControlCreateInfoEXT clip_control = {
        .negativeOneToOne = key.clip_space == Liverpool::ClipSpace::MinusWToW,
    };

    const vk::PipelineViewportStateCreateInfo viewport_info = {
        .pNext = &clip_control,
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
        .minDepthBounds = key.depth_bounds_min,
        .maxDepthBounds = key.depth_bounds_max,
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
        .depthAttachmentFormat = key.depth_format,
        .stencilAttachmentFormat = vk::Format::eUndefined,
    };

    std::array<vk::PipelineColorBlendAttachmentState, Liverpool::NumColorBuffers> attachments;
    for (u32 i = 0; i < num_color_formats; i++) {
        const auto& control = key.blend_controls[i];
        const auto src_color = LiverpoolToVK::BlendFactor(control.color_src_factor);
        const auto dst_color = LiverpoolToVK::BlendFactor(control.color_dst_factor);
        const auto color_blend = LiverpoolToVK::BlendOp(control.color_func);
        attachments[i] = vk::PipelineColorBlendAttachmentState{
            .blendEnable = control.enable,
            .srcColorBlendFactor = src_color,
            .dstColorBlendFactor = dst_color,
            .colorBlendOp = color_blend,
            .srcAlphaBlendFactor = control.separate_alpha_blend
                                       ? LiverpoolToVK::BlendFactor(control.alpha_src_factor)
                                       : src_color,
            .dstAlphaBlendFactor = control.separate_alpha_blend
                                       ? LiverpoolToVK::BlendFactor(control.alpha_dst_factor)
                                       : dst_color,
            .alphaBlendOp = control.separate_alpha_blend
                                ? LiverpoolToVK::BlendOp(control.alpha_func)
                                : color_blend,
            .colorWriteMask =
                instance.IsColorWriteEnableSupported()
                    ? vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
                    : key.write_masks[i],
        };

        // On GCN GPU there is an additional mask which allows to control color components exported
        // from a pixel shader. A situation possible, when the game may mask out the alpha channel,
        // while it is still need to be used in blending ops. For such cases, HW will default alpha
        // to 1 and perform the blending, while shader normally outputs 0 in the last component.
        // Unfortunatelly, Vulkan doesn't provide any control on blend inputs, so below we detecting
        // such cases and override alpha value in order to emulate HW behaviour.
        const auto has_alpha_masked_out =
            (key.cb_shader_mask.GetMask(i) & Liverpool::ColorBufferMask::ComponentA) == 0;
        const auto has_src_alpha_in_src_blend = src_color == vk::BlendFactor::eSrcAlpha ||
                                                src_color == vk::BlendFactor::eOneMinusSrcAlpha;
        const auto has_src_alpha_in_dst_blend = dst_color == vk::BlendFactor::eSrcAlpha ||
                                                dst_color == vk::BlendFactor::eOneMinusSrcAlpha;
        if (has_alpha_masked_out && has_src_alpha_in_src_blend) {
            attachments[i].srcColorBlendFactor = src_color == vk::BlendFactor::eSrcAlpha
                                                     ? vk::BlendFactor::eOne
                                                     : vk::BlendFactor::eZero; // 1-A
        }
        if (has_alpha_masked_out && has_src_alpha_in_dst_blend) {
            attachments[i].dstColorBlendFactor = dst_color == vk::BlendFactor::eSrcAlpha
                                                     ? vk::BlendFactor::eOne
                                                     : vk::BlendFactor::eZero; // 1-A
        }
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
                .descriptorType = image.is_storage ? vk::DescriptorType::eStorageImage
                                                   : vk::DescriptorType::eSampledImage,
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
    BindVertexBuffers(staging);

    // Bind resource buffers and textures.
    boost::container::static_vector<vk::DescriptorBufferInfo, 16> buffer_infos;
    boost::container::static_vector<vk::DescriptorImageInfo, 32> image_infos;
    boost::container::small_vector<vk::WriteDescriptorSet, 16> set_writes;
    u32 binding{};

    for (const auto& stage : stages) {
        for (const auto& buffer : stage.buffers) {
            const auto vsharp = buffer.GetVsharp(stage);
            const VAddr address = vsharp.base_address;
            const u32 size = vsharp.GetSize();
            const u32 offset = staging.Copy(address, size,
                                            buffer.is_storage ? instance.StorageMinAlignment()
                                                              : instance.UniformMinAlignment());
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

            if (texture_cache.IsMeta(address)) {
                LOG_WARNING(Render_Vulkan, "Unexpected metadata read by a PS shader (buffer)");
            }
        }

        boost::container::static_vector<AmdGpu::Image, 16> tsharps;
        for (const auto& image_desc : stage.images) {
            const auto& tsharp = tsharps.emplace_back(
                stage.ReadUd<AmdGpu::Image>(image_desc.sgpr_base, image_desc.dword_offset));
            const auto& image_view = texture_cache.FindImageView(tsharp, image_desc.is_storage);
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
                LOG_WARNING(Render_Vulkan, "Unexpected metadata read by a PS shader (texture)");
            }
        }
        for (const auto& sampler : stage.samplers) {
            auto ssharp = stage.ReadUd<AmdGpu::Sampler>(sampler.sgpr_base, sampler.dword_offset);
            if (sampler.disable_aniso) {
                const auto& tsharp = tsharps[sampler.associated_image];
                if (tsharp.base_level == 0 && tsharp.last_level == 0) {
                    ssharp.max_aniso.Assign(AmdGpu::AnisoRatio::One);
                }
            }
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
        const auto cmdbuf = scheduler.CommandBuffer();
        cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0,
                                    set_writes);
    }
}

void GraphicsPipeline::BindVertexBuffers(StreamBuffer& staging) const {
    const auto& vs_info = stages[0];
    if (vs_info.vs_inputs.empty()) {
        return;
    }

    std::array<vk::Buffer, MaxVertexBufferCount> host_buffers;
    std::array<vk::DeviceSize, MaxVertexBufferCount> host_offsets;
    boost::container::static_vector<AmdGpu::Buffer, MaxVertexBufferCount> guest_buffers;

    struct BufferRange {
        VAddr base_address;
        VAddr end_address;
        u64 offset; // offset in the mapped memory

        size_t GetSize() const {
            return end_address - base_address;
        }
    };

    // Calculate buffers memory overlaps
    boost::container::static_vector<BufferRange, MaxVertexBufferCount> ranges{};
    for (const auto& input : vs_info.vs_inputs) {
        if (input.instance_step_rate == Shader::Info::VsInput::InstanceIdType::OverStepRate0 ||
            input.instance_step_rate == Shader::Info::VsInput::InstanceIdType::OverStepRate1) {
            continue;
        }

        const auto& buffer = vs_info.ReadUd<AmdGpu::Buffer>(input.sgpr_base, input.dword_offset);
        if (buffer.GetSize() == 0) {
            continue;
        }
        guest_buffers.emplace_back(buffer);
        ranges.emplace_back(buffer.base_address, buffer.base_address + buffer.GetSize());
    }
    std::ranges::sort(ranges, [](const BufferRange& lhv, const BufferRange& rhv) {
        return lhv.base_address < rhv.base_address;
    });

    boost::container::static_vector<BufferRange, MaxVertexBufferCount> ranges_merged{ranges[0]};
    for (auto range : ranges) {
        auto& prev_range = ranges_merged.back();
        if (prev_range.end_address < range.base_address) {
            ranges_merged.emplace_back(range);
        } else {
            prev_range.end_address = std::max(prev_range.end_address, range.end_address);
        }
    }

    // Map buffers
    for (auto& range : ranges_merged) {
        range.offset = staging.Copy(range.base_address, range.GetSize(), 4);
    }

    // Bind vertex buffers
    const size_t num_buffers = guest_buffers.size();
    for (u32 i = 0; i < num_buffers; ++i) {
        const auto& buffer = guest_buffers[i];
        const auto& host_buffer = std::ranges::find_if(
            ranges_merged.cbegin(), ranges_merged.cend(), [&](const BufferRange& range) {
                return (buffer.base_address >= range.base_address &&
                        buffer.base_address < range.end_address);
            });
        assert(host_buffer != ranges_merged.cend());

        host_buffers[i] = staging.Handle();
        host_offsets[i] = host_buffer->offset + buffer.base_address - host_buffer->base_address;
    }

    if (num_buffers > 0) {
        const auto cmdbuf = scheduler.CommandBuffer();
        cmdbuf.bindVertexBuffers(0, num_buffers, host_buffers.data(), host_offsets.data());
    }
}

} // namespace Vulkan
