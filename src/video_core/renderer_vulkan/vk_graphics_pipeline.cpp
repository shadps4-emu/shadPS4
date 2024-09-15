// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>

#include "common/alignment.h"
#include "common/assert.h"
#include "video_core/amdgpu/resource.h"
#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/renderer_vulkan/vk_graphics_pipeline.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/texture_cache.h"

namespace Vulkan {

GraphicsPipeline::GraphicsPipeline(const Instance& instance_, Scheduler& scheduler_,
                                   DescriptorHeap& desc_heap_, const GraphicsPipelineKey& key_,
                                   vk::PipelineCache pipeline_cache,
                                   std::span<const Shader::Info*, MaxShaderStages> infos,
                                   std::span<const vk::ShaderModule> modules)
    : instance{instance_}, scheduler{scheduler_}, desc_heap{desc_heap_}, key{key_} {
    const vk::Device device = instance.GetDevice();
    std::ranges::copy(infos, stages.begin());
    BuildDescSetLayout();

    const vk::PushConstantRange push_constants = {
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = sizeof(Shader::PushData),
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
    const auto& vs_info = stages[u32(Shader::Stage::Vertex)];
    for (const auto& input : vs_info->vs_inputs) {
        if (input.instance_step_rate == Shader::Info::VsInput::InstanceIdType::OverStepRate0 ||
            input.instance_step_rate == Shader::Info::VsInput::InstanceIdType::OverStepRate1) {
            // Skip attribute binding as the data will be pulled by shader
            continue;
        }

        const auto buffer = vs_info->ReadUd<AmdGpu::Buffer>(input.sgpr_base, input.dword_offset);
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
        .primitiveRestartEnable = key.enable_primitive_restart != 0,
    };
    ASSERT_MSG(!key.enable_primitive_restart || key.primitive_restart_index == 0xFFFF,
               "Primitive restart index other than 0xFFFF is not supported yet");

    const vk::PipelineRasterizationStateCreateInfo raster_state = {
        .depthClampEnable = false,
        .rasterizerDiscardEnable = false,
        .polygonMode = LiverpoolToVK::PolygonMode(key.polygon_mode),
        .cullMode = LiverpoolToVK::CullMode(key.cull_mode),
        .frontFace = key.front_face == Liverpool::FrontFace::Clockwise
                         ? vk::FrontFace::eClockwise
                         : vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = bool(key.depth_bias_enable),
        .lineWidth = 1.0f,
    };

    const vk::PipelineMultisampleStateCreateInfo multisampling = {
        .rasterizationSamples =
            LiverpoolToVK::NumSamples(key.num_samples, instance.GetFramebufferSampleCounts()),
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
        .pNext = instance.IsDepthClipControlSupported() ? &clip_control : nullptr,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    boost::container::static_vector<vk::DynamicState, 14> dynamic_states = {
        vk::DynamicState::eViewport,           vk::DynamicState::eScissor,
        vk::DynamicState::eBlendConstants,     vk::DynamicState::eDepthBounds,
        vk::DynamicState::eDepthBias,          vk::DynamicState::eStencilReference,
        vk::DynamicState::eStencilCompareMask, vk::DynamicState::eStencilWriteMask,
    };

    if (instance.IsColorWriteEnableSupported()) {
        dynamic_states.push_back(vk::DynamicState::eColorWriteEnableEXT);
        dynamic_states.push_back(vk::DynamicState::eColorWriteMaskEXT);
    }
    if (instance.IsVertexInputDynamicState()) {
        dynamic_states.push_back(vk::DynamicState::eVertexInputEXT);
    }

    const vk::PipelineDynamicStateCreateInfo dynamic_info = {
        .dynamicStateCount = static_cast<u32>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
    };

    const vk::PipelineDepthStencilStateCreateInfo depth_info = {
        .depthTestEnable = key.depth_stencil.depth_enable,
        .depthWriteEnable = key.depth_stencil.depth_write_enable,
        .depthCompareOp = LiverpoolToVK::CompareOp(key.depth_stencil.depth_func),
        .depthBoundsTestEnable = key.depth_stencil.depth_bounds_enable,
        .stencilTestEnable = key.depth_stencil.stencil_enable,
        .front{
            .failOp = LiverpoolToVK::StencilOp(key.stencil.stencil_fail_front),
            .passOp = LiverpoolToVK::StencilOp(key.stencil.stencil_zpass_front),
            .depthFailOp = LiverpoolToVK::StencilOp(key.stencil.stencil_zfail_front),
            .compareOp = LiverpoolToVK::CompareOp(key.depth_stencil.stencil_ref_func),
        },
        .back{
            .failOp = LiverpoolToVK::StencilOp(key.depth_stencil.backface_enable
                                                   ? key.stencil.stencil_fail_back.Value()
                                                   : key.stencil.stencil_fail_front.Value()),
            .passOp = LiverpoolToVK::StencilOp(key.depth_stencil.backface_enable
                                                   ? key.stencil.stencil_zpass_back.Value()
                                                   : key.stencil.stencil_zpass_front.Value()),
            .depthFailOp = LiverpoolToVK::StencilOp(key.depth_stencil.backface_enable
                                                        ? key.stencil.stencil_zfail_back.Value()
                                                        : key.stencil.stencil_zfail_front.Value()),
            .compareOp = LiverpoolToVK::CompareOp(key.depth_stencil.backface_enable
                                                      ? key.depth_stencil.stencil_bf_func.Value()
                                                      : key.depth_stencil.stencil_ref_func.Value()),
        },
    };

    auto stage = u32(Shader::Stage::Vertex);
    boost::container::static_vector<vk::PipelineShaderStageCreateInfo, MaxShaderStages>
        shader_stages;
    if (infos[stage]) {
        shader_stages.emplace_back(vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = modules[stage],
            .pName = "main",
        });
    }
    stage = u32(Shader::Stage::Fragment);
    if (infos[stage]) {
        shader_stages.emplace_back(vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = modules[stage],
            .pName = "main",
        });
    }

    const auto it = std::ranges::find(key.color_formats, vk::Format::eUndefined);
    const u32 num_color_formats = std::distance(key.color_formats.begin(), it);
    const vk::PipelineRenderingCreateInfoKHR pipeline_rendering_ci = {
        .colorAttachmentCount = num_color_formats,
        .pColorAttachmentFormats = key.color_formats.data(),
        .depthAttachmentFormat = key.depth_format,
        .stencilAttachmentFormat = key.stencil_format,
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
        .stageCount = static_cast<u32>(shader_stages.size()),
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
    for (const auto* stage : stages) {
        if (!stage) {
            continue;
        }
        for (const auto& buffer : stage->buffers) {
            const auto sharp = buffer.GetSharp(*stage);
            bindings.push_back({
                .binding = binding++,
                .descriptorType = buffer.IsStorage(sharp) ? vk::DescriptorType::eStorageBuffer
                                                          : vk::DescriptorType::eUniformBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            });
        }
        for (const auto& tex_buffer : stage->texture_buffers) {
            bindings.push_back({
                .binding = binding++,
                .descriptorType = tex_buffer.is_written ? vk::DescriptorType::eStorageTexelBuffer
                                                        : vk::DescriptorType::eUniformTexelBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            });
        }
        for (const auto& image : stage->images) {
            bindings.push_back({
                .binding = binding++,
                .descriptorType = image.is_storage ? vk::DescriptorType::eStorageImage
                                                   : vk::DescriptorType::eSampledImage,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            });
        }
        for (const auto& sampler : stage->samplers) {
            bindings.push_back({
                .binding = binding++,
                .descriptorType = vk::DescriptorType::eSampler,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            });
        }
    }
    uses_push_descriptors = binding < instance.MaxPushDescriptors();
    const auto flags = uses_push_descriptors
                           ? vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR
                           : vk::DescriptorSetLayoutCreateFlagBits{};
    const vk::DescriptorSetLayoutCreateInfo desc_layout_ci = {
        .flags = flags,
        .bindingCount = static_cast<u32>(bindings.size()),
        .pBindings = bindings.data(),
    };
    desc_layout = instance.GetDevice().createDescriptorSetLayoutUnique(desc_layout_ci);
}

void GraphicsPipeline::BindResources(const Liverpool::Regs& regs,
                                     VideoCore::BufferCache& buffer_cache,
                                     VideoCore::TextureCache& texture_cache) const {
    // Bind resource buffers and textures.
    boost::container::static_vector<vk::BufferView, 8> buffer_views;
    boost::container::static_vector<vk::DescriptorBufferInfo, 32> buffer_infos;
    boost::container::static_vector<vk::DescriptorImageInfo, 32> image_infos;
    boost::container::small_vector<vk::WriteDescriptorSet, 16> set_writes;
    boost::container::small_vector<vk::BufferMemoryBarrier2, 16> buffer_barriers;
    Shader::PushData push_data{};
    u32 binding{};

    for (const auto* stage : stages) {
        if (!stage) {
            continue;
        }
        if (stage->uses_step_rates) {
            push_data.step0 = regs.vgt_instance_step_rate_0;
            push_data.step1 = regs.vgt_instance_step_rate_1;
        }
        for (const auto& buffer : stage->buffers) {
            const auto vsharp = buffer.GetSharp(*stage);
            const bool is_storage = buffer.IsStorage(vsharp);
            if (vsharp) {
                const VAddr address = vsharp.base_address;
                if (texture_cache.IsMeta(address)) {
                    LOG_WARNING(Render_Vulkan, "Unexpected metadata read by a PS shader (buffer)");
                }
                const u32 size = vsharp.GetSize();
                const u32 alignment =
                    is_storage ? instance.StorageMinAlignment() : instance.UniformMinAlignment();
                const auto [vk_buffer, offset] =
                    buffer_cache.ObtainBuffer(address, size, buffer.is_written);
                const u32 offset_aligned = Common::AlignDown(offset, alignment);
                const u32 adjust = offset - offset_aligned;
                if (adjust != 0) {
                    ASSERT(adjust % 4 == 0);
                    push_data.AddOffset(binding, adjust);
                }
                buffer_infos.emplace_back(vk_buffer->Handle(), offset_aligned, size + adjust);
            } else {
                buffer_infos.emplace_back(VK_NULL_HANDLE, 0, VK_WHOLE_SIZE);
            }
            set_writes.push_back({
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = binding++,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = is_storage ? vk::DescriptorType::eStorageBuffer
                                             : vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &buffer_infos.back(),
            });
        }

        for (const auto& desc : stage->texture_buffers) {
            const auto vsharp = desc.GetSharp(*stage);
            vk::BufferView& buffer_view = buffer_views.emplace_back(VK_NULL_HANDLE);
            const u32 size = vsharp.GetSize();
            if (vsharp.GetDataFmt() != AmdGpu::DataFormat::FormatInvalid && size != 0) {
                const VAddr address = vsharp.base_address;
                const u32 alignment = instance.TexelBufferMinAlignment();
                const auto [vk_buffer, offset] =
                    buffer_cache.ObtainBuffer(address, size, desc.is_written, true);
                const u32 fmt_stride = AmdGpu::NumBits(vsharp.GetDataFmt()) >> 3;
                ASSERT_MSG(fmt_stride == vsharp.GetStride(),
                           "Texel buffer stride must match format stride");
                const u32 offset_aligned = Common::AlignDown(offset, alignment);
                const u32 adjust = offset - offset_aligned;
                if (adjust != 0) {
                    ASSERT(adjust % fmt_stride == 0);
                    push_data.AddOffset(binding, adjust / fmt_stride);
                }
                buffer_view = vk_buffer->View(offset_aligned, size + adjust, desc.is_written,
                                              vsharp.GetDataFmt(), vsharp.GetNumberFmt());
                const auto dst_access = desc.is_written ? vk::AccessFlagBits2::eShaderWrite
                                                        : vk::AccessFlagBits2::eShaderRead;
                if (auto barrier = vk_buffer->GetBarrier(
                        dst_access, vk::PipelineStageFlagBits2::eVertexShader)) {
                    buffer_barriers.emplace_back(*barrier);
                }
                if (desc.is_written) {
                    texture_cache.MarkWritten(address, size);
                }
            }
            set_writes.push_back({
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = binding++,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = desc.is_written ? vk::DescriptorType::eStorageTexelBuffer
                                                  : vk::DescriptorType::eUniformTexelBuffer,
                .pTexelBufferView = &buffer_view,
            });
        }

        boost::container::static_vector<AmdGpu::Image, 32> tsharps;
        for (const auto& image_desc : stage->images) {
            const auto tsharp = image_desc.GetSharp(*stage);
            if (tsharp.GetDataFmt() != AmdGpu::DataFormat::FormatInvalid) {
                tsharps.emplace_back(tsharp);
                VideoCore::ImageInfo image_info{tsharp, image_desc.is_depth};
                VideoCore::ImageViewInfo view_info{tsharp, image_desc.is_storage};
                const auto& image_view = texture_cache.FindTexture(image_info, view_info);
                const auto& image = texture_cache.GetImage(image_view.image_id);
                image_infos.emplace_back(VK_NULL_HANDLE, *image_view.image_view, image.layout);
            } else {
                image_infos.emplace_back(VK_NULL_HANDLE, VK_NULL_HANDLE, vk::ImageLayout::eGeneral);
            }
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
        for (const auto& sampler : stage->samplers) {
            auto ssharp = sampler.GetSharp(*stage);
            if (ssharp.force_degamma) {
                LOG_WARNING(Render_Vulkan, "Texture requires gamma correction");
            }
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

    if (!set_writes.empty()) {
        if (uses_push_descriptors) {
            cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0,
                                        set_writes);
        } else {
            const auto desc_set = desc_heap.Commit(*desc_layout);
            for (auto& set_write : set_writes) {
                set_write.dstSet = desc_set;
            }
            instance.GetDevice().updateDescriptorSets(set_writes, {});
            cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0,
                                      desc_set, {});
        }
    }
    cmdbuf.pushConstants(*pipeline_layout,
                         vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0U,
                         sizeof(push_data), &push_data);
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, Handle());
}

} // namespace Vulkan
