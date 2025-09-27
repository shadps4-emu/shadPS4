// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"
#include "video_core/texture_cache/blit_helper.h"
#include "video_core/texture_cache/image.h"

#include "video_core/host_shaders/color_to_ms_depth_frag.h"
#include "video_core/host_shaders/fs_tri_vert.h"
#include "video_core/host_shaders/ms_image_blit_frag.h"

namespace VideoCore {

static vk::SampleCountFlagBits ToSampleCount(u32 num_samples) {
    switch (num_samples) {
    case 1:
        return vk::SampleCountFlagBits::e1;
    case 2:
        return vk::SampleCountFlagBits::e2;
    case 4:
        return vk::SampleCountFlagBits::e4;
    case 8:
        return vk::SampleCountFlagBits::e8;
    case 16:
        return vk::SampleCountFlagBits::e16;
    default:
        UNREACHABLE_MSG("Unknown samples count = {}", num_samples);
    }
}

BlitHelper::BlitHelper(const Vulkan::Instance& instance_, Vulkan::Scheduler& scheduler_)
    : instance{instance_}, scheduler{scheduler_} {
    CreateShaders();
    CreatePipelineLayouts();
}

BlitHelper::~BlitHelper() = default;

void BlitHelper::ReinterpretColorAsMsDepth(u32 width, u32 height, u32 num_samples,
                                           vk::Format pixel_format, vk::Image source,
                                           vk::Image dest) {
    const vk::ImageViewUsageCreateInfo color_usage_ci{.usage = vk::ImageUsageFlagBits::eSampled};
    const vk::ImageViewCreateInfo color_view_ci = {
        .pNext = &color_usage_ci,
        .image = source,
        .viewType = vk::ImageViewType::e2D,
        .format = pixel_format,
        .subresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0U,
            .levelCount = 1U,
            .baseArrayLayer = 0U,
            .layerCount = 1U,
        },
    };
    const auto [color_view_result, color_view] =
        instance.GetDevice().createImageView(color_view_ci);
    ASSERT_MSG(color_view_result == vk::Result::eSuccess, "Failed to create image view: {}",
               vk::to_string(color_view_result));
    const vk::ImageViewUsageCreateInfo depth_usage_ci{
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment};
    const vk::ImageViewCreateInfo depth_view_ci = {
        .pNext = &depth_usage_ci,
        .image = dest,
        .viewType = vk::ImageViewType::e2D,
        .format = pixel_format,
        .subresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eDepth,
            .baseMipLevel = 0U,
            .levelCount = 1U,
            .baseArrayLayer = 0U,
            .layerCount = 1U,
        },
    };
    const auto [depth_view_result, depth_view] =
        instance.GetDevice().createImageView(depth_view_ci);
    ASSERT_MSG(depth_view_result == vk::Result::eSuccess, "Failed to create image view: {}",
               vk::to_string(depth_view_result));
    scheduler.DeferOperation([device = instance.GetDevice(), color_view, depth_view] {
        device.destroyImageView(color_view);
        device.destroyImageView(depth_view);
    });

    Vulkan::RenderState state{};
    state.has_depth = true;
    state.width = width;
    state.height = height;
    state.depth_attachment = vk::RenderingAttachmentInfo{
        .imageView = depth_view,
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eDontCare,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = vk::ClearValue{.depthStencil = {.depth = 0.f}},
    };
    scheduler.BeginRendering(state);

    const auto cmdbuf = scheduler.CommandBuffer();
    const vk::DescriptorImageInfo image_info = {
        .sampler = VK_NULL_HANDLE,
        .imageView = color_view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };
    const vk::WriteDescriptorSet texture_write = {
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = 0U,
        .dstArrayElement = 0U,
        .descriptorCount = 1U,
        .descriptorType = vk::DescriptorType::eSampledImage,
        .pImageInfo = &image_info,
    };
    cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, *single_texture_pl_layout, 0U,
                                texture_write);

    const MsPipelineKey key{num_samples, vk::Format::eUndefined, pixel_format, false};
    auto it = std::ranges::find(color_to_ms_depth_pl, key, &MsPipeline::first);
    if (it == color_to_ms_depth_pl.end()) {
        CreateColorToMSDepthPipeline(key);
        it = --color_to_ms_depth_pl.end();
    }
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *it->second);

    const vk::Viewport viewport = {
        .x = 0,
        .y = 0,
        .width = float(state.width),
        .height = float(state.height),
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };
    cmdbuf.setViewportWithCount(viewport);

    const vk::Rect2D scissor = {
        .offset = {0, 0},
        .extent = {state.width, state.height},
    };
    cmdbuf.setScissorWithCount(scissor);

    cmdbuf.draw(3, 1, 0, 0);

    scheduler.GetDynamicState().Invalidate();
}

void BlitHelper::BlitNonMsImageMsImage(u32 width, u32 height, u32 num_samples,
                                       vk::Format pixel_format, bool is_depth, bool src_msaa,
                                       vk::Image source, vk::Image dest) {
    const vk::ImageViewUsageCreateInfo non_ms_usage_ci{.usage = vk::ImageUsageFlagBits::eSampled};
    const vk::ImageViewCreateInfo non_ms_view_ci = {
        .pNext = &non_ms_usage_ci,
        .image = source,
        .viewType = vk::ImageViewType::e2D,
        .format = pixel_format,
        .subresourceRange{
            .aspectMask =
                is_depth ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0U,
            .levelCount = 1U,
            .baseArrayLayer = 0U,
            .layerCount = 1U,
        },
    };
    const auto [non_ms_view_result, non_ms_view] =
        instance.GetDevice().createImageView(non_ms_view_ci);
    ASSERT_MSG(non_ms_view_result == vk::Result::eSuccess, "Failed to create image view: {}",
               vk::to_string(non_ms_view_result));

    const vk::ImageViewUsageCreateInfo ms_usage_ci{
        .usage = is_depth ? vk::ImageUsageFlagBits::eDepthStencilAttachment
                          : vk::ImageUsageFlagBits::eColorAttachment};
    const vk::ImageViewCreateInfo ms_view_ci = {
        .pNext = &ms_usage_ci,
        .image = dest,
        .viewType = vk::ImageViewType::e2D,
        .format = pixel_format,
        .subresourceRange{
            .aspectMask =
                is_depth ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0U,
            .levelCount = 1U,
            .baseArrayLayer = 0U,
            .layerCount = 1U,
        },
    };
    const auto [ms_view_result, ms_view] = instance.GetDevice().createImageView(ms_view_ci);
    ASSERT_MSG(ms_view_result == vk::Result::eSuccess, "Failed to create image view: {}",
               vk::to_string(ms_view_result));
    scheduler.DeferOperation([device = instance.GetDevice(), non_ms_view, ms_view] {
        device.destroyImageView(non_ms_view);
        device.destroyImageView(ms_view);
    });

    Vulkan::RenderState state{};
    state.width = width;
    state.height = height;
    if (is_depth) {
        state.has_depth = true;
        state.depth_attachment = vk::RenderingAttachmentInfo{
            .imageView = ms_view,
            .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eDontCare,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = vk::ClearValue{.depthStencil = {.depth = 0.f}},
        };
    } else {
        state.color_attachments[state.num_color_attachments++] = vk::RenderingAttachmentInfo{
            .imageView = ms_view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eDontCare,
            .storeOp = vk::AttachmentStoreOp::eStore,
        };
    }
    scheduler.BeginRendering(state);

    const auto cmdbuf = scheduler.CommandBuffer();
    const vk::DescriptorImageInfo image_info = {
        .sampler = VK_NULL_HANDLE,
        .imageView = non_ms_view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };
    const vk::WriteDescriptorSet texture_write = {
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = 0U,
        .dstArrayElement = 0U,
        .descriptorCount = 1U,
        .descriptorType = vk::DescriptorType::eSampledImage,
        .pImageInfo = &image_info,
    };
    cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, *single_texture_pl_layout, 0U,
                                texture_write);

    const auto color_format = is_depth ? vk::Format::eUndefined : pixel_format;
    const auto depth_format = is_depth ? pixel_format : vk::Format::eUndefined;
    const MsPipelineKey key{num_samples, color_format, depth_format, src_msaa};
    auto it = std::ranges::find(non_ms_color_to_ms_color_pl, key, &MsPipeline::first);
    if (it == non_ms_color_to_ms_color_pl.end()) {
        CreateNonMsImageToMsImagePipeline(key);
        it = --non_ms_color_to_ms_color_pl.end();
    }
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *it->second);

    const vk::Viewport viewport = {
        .x = 0,
        .y = 0,
        .width = float(state.width),
        .height = float(state.height),
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };
    cmdbuf.setViewportWithCount(viewport);

    const vk::Rect2D scissor = {
        .offset = {0, 0},
        .extent = {state.width, state.height},
    };
    cmdbuf.setScissorWithCount(scissor);

    cmdbuf.draw(3, 1, 0, 0);

    scheduler.GetDynamicState().Invalidate();
}

void BlitHelper::CreateShaders() {
    const auto device = instance.GetDevice();
    fs_tri_vertex =
        Vulkan::Compile(HostShaders::FS_TRI_VERT, vk::ShaderStageFlagBits::eVertex, device);
    fs_tri_layer_vertex = Vulkan::Compile(
        HostShaders::FS_TRI_VERT, vk::ShaderStageFlagBits::eVertex, device, {"INSTANCE_AS_LAYER"});
    color_to_ms_depth_frag = Vulkan::Compile(HostShaders::COLOR_TO_MS_DEPTH_FRAG,
                                             vk::ShaderStageFlagBits::eFragment, device);
    non_ms_color_to_ms_color_frag = Vulkan::Compile(HostShaders::MS_IMAGE_BLIT_FRAG,
                                                    vk::ShaderStageFlagBits::eFragment, device);
    non_ms_depth_to_ms_depth_frag = Vulkan::Compile(
        HostShaders::MS_IMAGE_BLIT_FRAG, vk::ShaderStageFlagBits::eFragment, device, {"IS_DEPTH"});
    ms_color_to_non_ms_color_frag = Vulkan::Compile(
        HostShaders::MS_IMAGE_BLIT_FRAG, vk::ShaderStageFlagBits::eFragment, device, {"SRC_MSAA"});
    ms_depth_to_non_ms_depth_frag =
        Vulkan::Compile(HostShaders::MS_IMAGE_BLIT_FRAG, vk::ShaderStageFlagBits::eFragment, device,
                        {"IS_DEPTH", "SRC_MSAA"});
}

void BlitHelper::CreatePipelineLayouts() {
    const vk::DescriptorSetLayoutBinding texture_binding = {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eSampledImage,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };
    const vk::DescriptorSetLayoutCreateInfo desc_layout_ci = {
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
        .bindingCount = 1U,
        .pBindings = &texture_binding,
    };
    auto [desc_layout_result, desc_layout] =
        instance.GetDevice().createDescriptorSetLayoutUnique(desc_layout_ci);
    single_texture_descriptor_set_layout = std::move(desc_layout);
    const vk::DescriptorSetLayout set_layout = *single_texture_descriptor_set_layout;
    const vk::PipelineLayoutCreateInfo layout_info = {
        .setLayoutCount = 1U,
        .pSetLayouts = &set_layout,
        .pushConstantRangeCount = 0U,
        .pPushConstantRanges = nullptr,
    };
    auto [layout_result, pipeline_layout] =
        instance.GetDevice().createPipelineLayoutUnique(layout_info);
    ASSERT_MSG(layout_result == vk::Result::eSuccess,
               "Failed to create graphics pipeline layout: {}", vk::to_string(layout_result));
    Vulkan::SetObjectName(instance.GetDevice(), *pipeline_layout, "Single texture pipeline layout");
    single_texture_pl_layout = std::move(pipeline_layout);
}

void BlitHelper::CreateColorToMSDepthPipeline(const MsPipelineKey& key) {
    const vk::PipelineInputAssemblyStateCreateInfo input_assembly = {
        .topology = vk::PrimitiveTopology::eTriangleList,
    };
    const vk::PipelineMultisampleStateCreateInfo multisampling = {
        .rasterizationSamples = ToSampleCount(key.num_samples),
    };
    const vk::PipelineDepthStencilStateCreateInfo depth_state = {
        .depthTestEnable = true,
        .depthWriteEnable = true,
        .depthCompareOp = vk::CompareOp::eAlways,
    };
    const std::array dynamic_states = {vk::DynamicState::eViewportWithCount,
                                       vk::DynamicState::eScissorWithCount};
    const vk::PipelineDynamicStateCreateInfo dynamic_info = {
        .dynamicStateCount = static_cast<u32>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
    };

    std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages;
    shader_stages[0] = {
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = fs_tri_vertex,
        .pName = "main",
    };
    shader_stages[1] = {
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = color_to_ms_depth_frag,
        .pName = "main",
    };

    const vk::PipelineRenderingCreateInfo pipeline_rendering_ci = {
        .colorAttachmentCount = 0U,
        .pColorAttachmentFormats = nullptr,
        .depthAttachmentFormat = key.depth_format,
        .stencilAttachmentFormat = vk::Format::eUndefined,
    };

    const vk::PipelineColorBlendStateCreateInfo color_blending{};
    const vk::PipelineViewportStateCreateInfo viewport_info{};
    const vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
    const vk::PipelineRasterizationStateCreateInfo raster_state{.lineWidth = 1.f};

    const vk::GraphicsPipelineCreateInfo pipeline_info = {
        .pNext = &pipeline_rendering_ci,
        .stageCount = static_cast<u32>(shader_stages.size()),
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_info,
        .pRasterizationState = &raster_state,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_state,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_info,
        .layout = *single_texture_pl_layout,
    };

    auto [pipeline_result, pipeline] =
        instance.GetDevice().createGraphicsPipelineUnique(VK_NULL_HANDLE, pipeline_info);
    ASSERT_MSG(pipeline_result == vk::Result::eSuccess, "Failed to create graphics pipeline: {}",
               vk::to_string(pipeline_result));
    Vulkan::SetObjectName(instance.GetDevice(), *pipeline, "Color to MS Depth {}", key.num_samples);

    color_to_ms_depth_pl.emplace_back(key, std::move(pipeline));
}

void BlitHelper::CreateNonMsImageToMsImagePipeline(const MsPipelineKey& key) {
    const bool is_depth = key.depth_format != vk::Format::eUndefined;
    const bool is_color = key.color_format != vk::Format::eUndefined;

    const vk::PipelineInputAssemblyStateCreateInfo input_assembly = {
        .topology = vk::PrimitiveTopology::eTriangleList,
    };
    const vk::PipelineMultisampleStateCreateInfo multisampling = {
        .rasterizationSamples = ToSampleCount(key.src_msaa ? 1 : key.num_samples),
    };
    const vk::PipelineDepthStencilStateCreateInfo depth_state = {
        .depthTestEnable = is_depth,
        .depthWriteEnable = is_depth,
        .depthCompareOp = vk::CompareOp::eAlways,
    };
    const std::array dynamic_states = {vk::DynamicState::eViewportWithCount,
                                       vk::DynamicState::eScissorWithCount};
    const vk::PipelineDynamicStateCreateInfo dynamic_info = {
        .dynamicStateCount = static_cast<u32>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
    };

    std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages;
    shader_stages[0] = {
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = fs_tri_vertex,
        .pName = "main",
    };
    shader_stages[1] = {
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = key.src_msaa
                      ? (is_color ? ms_color_to_non_ms_color_frag : ms_depth_to_non_ms_depth_frag)
                  : is_color ? non_ms_color_to_ms_color_frag
                             : non_ms_depth_to_ms_depth_frag,
        .pName = "main",
    };

    const vk::PipelineRenderingCreateInfo pipeline_rendering_ci = {
        .colorAttachmentCount = is_color,
        .pColorAttachmentFormats = &key.color_format,
        .depthAttachmentFormat = key.depth_format,
        .stencilAttachmentFormat = vk::Format::eUndefined,
    };

    const vk::PipelineColorBlendAttachmentState attachment = {
        .blendEnable = false,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    const vk::PipelineColorBlendStateCreateInfo color_blending = {
        .logicOpEnable = false,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = is_color,
        .pAttachments = &attachment,
    };
    const vk::PipelineViewportStateCreateInfo viewport_info{};
    const vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
    const vk::PipelineRasterizationStateCreateInfo raster_state{.lineWidth = 1.f};

    const vk::GraphicsPipelineCreateInfo pipeline_info = {
        .pNext = &pipeline_rendering_ci,
        .stageCount = static_cast<u32>(shader_stages.size()),
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_info,
        .pRasterizationState = &raster_state,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_state,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_info,
        .layout = *single_texture_pl_layout,
    };

    auto [pipeline_result, pipeline] =
        instance.GetDevice().createGraphicsPipelineUnique(VK_NULL_HANDLE, pipeline_info);
    ASSERT_MSG(pipeline_result == vk::Result::eSuccess, "Failed to create graphics pipeline: {}",
               vk::to_string(pipeline_result));
    Vulkan::SetObjectName(instance.GetDevice(), *pipeline, "Non MS Image to MS Image {}",
                          key.num_samples);

    non_ms_color_to_ms_color_pl.emplace_back(key, std::move(pipeline));
}

} // namespace VideoCore
