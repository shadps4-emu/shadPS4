// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/debug.h"
#include "common/singleton.h"
#include "core/debug_state.h"
#include "core/devtools/layer.h"
#include "core/file_format/splash.h"
#include "core/libraries/system/systemservice.h"
#include "imgui/renderer/imgui_core.h"
#include "sdl_window.h"
#include "video_core/renderer_vulkan/vk_presenter.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"
#include "video_core/texture_cache/image.h"

#include "video_core/host_shaders/fs_tri_vert.h"
#include "video_core/host_shaders/post_process_frag.h"

#include <vk_mem_alloc.h>

namespace Vulkan {

bool CanBlitToSwapchain(const vk::PhysicalDevice physical_device, vk::Format format) {
    const vk::FormatProperties props{physical_device.getFormatProperties(format)};
    return static_cast<bool>(props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst);
}

[[nodiscard]] vk::ImageSubresourceLayers MakeImageSubresourceLayers() {
    return vk::ImageSubresourceLayers{
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
}

[[nodiscard]] vk::ImageBlit MakeImageBlit(s32 frame_width, s32 frame_height, s32 dst_width,
                                          s32 dst_height, s32 offset_x, s32 offset_y) {
    return vk::ImageBlit{
        .srcSubresource = MakeImageSubresourceLayers(),
        .srcOffsets =
            std::array{
                vk::Offset3D{
                    .x = 0,
                    .y = 0,
                    .z = 0,
                },
                vk::Offset3D{
                    .x = frame_width,
                    .y = frame_height,
                    .z = 1,
                },
            },
        .dstSubresource = MakeImageSubresourceLayers(),
        .dstOffsets =
            std::array{
                vk::Offset3D{
                    .x = offset_x,
                    .y = offset_y,
                    .z = 0,
                },
                vk::Offset3D{
                    .x = offset_x + dst_width,
                    .y = offset_y + dst_height,
                    .z = 1,
                },
            },
    };
}

[[nodiscard]] vk::ImageBlit MakeImageBlitStretch(s32 frame_width, s32 frame_height,
                                                 s32 swapchain_width, s32 swapchain_height) {
    return MakeImageBlit(frame_width, frame_height, swapchain_width, swapchain_height, 0, 0);
}

static vk::Rect2D FitImage(s32 frame_width, s32 frame_height, s32 swapchain_width,
                           s32 swapchain_height) {
    float frame_aspect = static_cast<float>(frame_width) / frame_height;
    float swapchain_aspect = static_cast<float>(swapchain_width) / swapchain_height;

    u32 dst_width = swapchain_width;
    u32 dst_height = swapchain_height;

    if (frame_aspect > swapchain_aspect) {
        dst_height = static_cast<s32>(swapchain_width / frame_aspect);
    } else {
        dst_width = static_cast<s32>(swapchain_height * frame_aspect);
    }

    const s32 offset_x = (swapchain_width - dst_width) / 2;
    const s32 offset_y = (swapchain_height - dst_height) / 2;

    return vk::Rect2D{{offset_x, offset_y}, {dst_width, dst_height}};
}

[[nodiscard]] vk::ImageBlit MakeImageBlitFit(s32 frame_width, s32 frame_height, s32 swapchain_width,
                                             s32 swapchain_height) {
    const auto& dst_rect = FitImage(frame_width, frame_height, swapchain_width, swapchain_height);

    return MakeImageBlit(frame_width, frame_height, dst_rect.extent.width, dst_rect.extent.height,
                         dst_rect.offset.x, dst_rect.offset.y);
}

static vk::Format FormatToUnorm(vk::Format fmt) {
    switch (fmt) {
    case vk::Format::eR8G8B8A8Srgb:
        return vk::Format::eR8G8B8A8Unorm;
    case vk::Format::eB8G8R8A8Srgb:
        return vk::Format::eB8G8R8A8Unorm;
    default:
        UNREACHABLE();
    }
}

void Presenter::CreatePostProcessPipeline() {
    static const std::array pp_shaders{
        HostShaders::FS_TRI_VERT,
        HostShaders::POST_PROCESS_FRAG,
    };

    boost::container::static_vector<vk::DescriptorSetLayoutBinding, 2> bindings{
        {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
        },
    };

    const vk::DescriptorSetLayoutCreateInfo desc_layout_ci = {
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
        .bindingCount = static_cast<u32>(bindings.size()),
        .pBindings = bindings.data(),
    };
    auto desc_layout_result = instance.GetDevice().createDescriptorSetLayoutUnique(desc_layout_ci);
    ASSERT_MSG(desc_layout_result.result == vk::Result::eSuccess,
               "Failed to create descriptor set layout: {}",
               vk::to_string(desc_layout_result.result));
    pp_desc_set_layout = std::move(desc_layout_result.value);

    const vk::PushConstantRange push_constants = {
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = sizeof(PostProcessSettings),
    };

    const auto& vs_module =
        Vulkan::Compile(pp_shaders[0], vk::ShaderStageFlagBits::eVertex, instance.GetDevice());
    ASSERT(vs_module);
    Vulkan::SetObjectName(instance.GetDevice(), vs_module, "fs_tri.vert");

    const auto& fs_module =
        Vulkan::Compile(pp_shaders[1], vk::ShaderStageFlagBits::eFragment, instance.GetDevice());
    ASSERT(fs_module);
    Vulkan::SetObjectName(instance.GetDevice(), fs_module, "post_process.frag");

    const std::array shaders_ci{
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = vs_module,
            .pName = "main",
        },
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = fs_module,
            .pName = "main",
        },
    };

    const vk::DescriptorSetLayout set_layout = *pp_desc_set_layout;
    const vk::PipelineLayoutCreateInfo layout_info = {
        .setLayoutCount = 1U,
        .pSetLayouts = &set_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constants,
    };
    auto [layout_result, layout] = instance.GetDevice().createPipelineLayoutUnique(layout_info);
    ASSERT_MSG(layout_result == vk::Result::eSuccess, "Failed to create pipeline layout: {}",
               vk::to_string(layout_result));
    pp_pipeline_layout = std::move(layout);

    const std::array pp_color_formats{
        vk::Format::eB8G8R8A8Unorm, // swapchain.GetSurfaceFormat().format,
    };
    const vk::PipelineRenderingCreateInfoKHR pipeline_rendering_ci = {
        .colorAttachmentCount = 1u,
        .pColorAttachmentFormats = pp_color_formats.data(),
    };

    const vk::PipelineVertexInputStateCreateInfo vertex_input_info = {
        .vertexBindingDescriptionCount = 0u,
        .vertexAttributeDescriptionCount = 0u,
    };

    const vk::PipelineInputAssemblyStateCreateInfo input_assembly = {
        .topology = vk::PrimitiveTopology::eTriangleList,
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

    const vk::PipelineRasterizationStateCreateInfo raster_state = {
        .depthClampEnable = false,
        .rasterizerDiscardEnable = false,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = false,
        .lineWidth = 1.0f,
    };

    const vk::PipelineMultisampleStateCreateInfo multisampling = {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
    };

    const std::array attachments{
        vk::PipelineColorBlendAttachmentState{
            .blendEnable = false,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        },
    };

    const vk::PipelineColorBlendStateCreateInfo color_blending = {
        .logicOpEnable = false,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .blendConstants = std::array{1.0f, 1.0f, 1.0f, 1.0f},
    };

    const std::array dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    const vk::PipelineDynamicStateCreateInfo dynamic_info = {
        .dynamicStateCount = static_cast<u32>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
    };

    const vk::GraphicsPipelineCreateInfo pipeline_info = {
        .pNext = &pipeline_rendering_ci,
        .stageCount = static_cast<u32>(shaders_ci.size()),
        .pStages = shaders_ci.data(),
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_info,
        .pRasterizationState = &raster_state,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_info,
        .layout = *pp_pipeline_layout,
    };

    auto result = instance.GetDevice().createGraphicsPipelineUnique(
        /*pipeline_cache*/ {}, pipeline_info);
    if (result.result == vk::Result::eSuccess) {
        pp_pipeline = std::move(result.value);
    } else {
        UNREACHABLE_MSG("Post process pipeline creation failed!");
    }

    // Once pipeline is compiled, we don't need the shader module anymore
    instance.GetDevice().destroyShaderModule(vs_module);
    instance.GetDevice().destroyShaderModule(fs_module);

    // Create sampler resource
    const vk::SamplerCreateInfo sampler_ci = {
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .addressModeU = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV = vk::SamplerAddressMode::eClampToEdge,
    };
    auto [sampler_result, smplr] = instance.GetDevice().createSamplerUnique(sampler_ci);
    ASSERT_MSG(sampler_result == vk::Result::eSuccess, "Failed to create sampler: {}",
               vk::to_string(sampler_result));
    pp_sampler = std::move(smplr);
}

Presenter::Presenter(Frontend::WindowSDL& window_, AmdGpu::Liverpool* liverpool_)
    : window{window_}, liverpool{liverpool_},
      instance{window, Config::getGpuId(), Config::vkValidationEnabled(),
               Config::vkCrashDiagnosticEnabled()},
      draw_scheduler{instance}, present_scheduler{instance}, flip_scheduler{instance},
      swapchain{instance, window},
      rasterizer{std::make_unique<Rasterizer>(instance, draw_scheduler, liverpool)},
      texture_cache{rasterizer->GetTextureCache()} {
    const u32 num_images = swapchain.GetImageCount();
    const vk::Device device = instance.GetDevice();

    // Create presentation frames.
    present_frames.resize(num_images);
    for (u32 i = 0; i < num_images; i++) {
        Frame& frame = present_frames[i];
        auto [fence_result, fence] =
            device.createFence({.flags = vk::FenceCreateFlagBits::eSignaled});
        ASSERT_MSG(fence_result == vk::Result::eSuccess, "Failed to create present done fence: {}",
                   vk::to_string(fence_result));
        frame.present_done = fence;
        free_queue.push(&frame);
    }

    CreatePostProcessPipeline();

    // Setup ImGui
    ImGui::Core::Initialize(instance, window, num_images,
                            FormatToUnorm(swapchain.GetSurfaceFormat().format));
    ImGui::Layer::AddLayer(Common::Singleton<Core::Devtools::Layer>::Instance());
}

Presenter::~Presenter() {
    ImGui::Layer::RemoveLayer(Common::Singleton<Core::Devtools::Layer>::Instance());
    draw_scheduler.Finish();
    const vk::Device device = instance.GetDevice();
    for (auto& frame : present_frames) {
        vmaDestroyImage(instance.GetAllocator(), frame.image, frame.allocation);
        device.destroyImageView(frame.image_view);
        device.destroyFence(frame.present_done);
    }
    ImGui::Core::Shutdown(device);
}

void Presenter::RecreateFrame(Frame* frame, u32 width, u32 height) {
    const vk::Device device = instance.GetDevice();
    if (frame->image_view) {
        device.destroyImageView(frame->image_view);
    }
    if (frame->image) {
        vmaDestroyImage(instance.GetAllocator(), frame->image, frame->allocation);
    }

    const vk::Format format = swapchain.GetSurfaceFormat().format;
    const vk::ImageCreateInfo image_info = {
        .flags = vk::ImageCreateFlagBits::eMutableFormat,
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst |
                 vk::ImageUsageFlagBits::eTransferSrc,
    };

    const VmaAllocationCreateInfo alloc_info = {
        .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .requiredFlags = 0,
        .preferredFlags = 0,
        .pool = VK_NULL_HANDLE,
        .pUserData = nullptr,
    };

    VkImage unsafe_image{};
    VkImageCreateInfo unsafe_image_info = static_cast<VkImageCreateInfo>(image_info);

    VkResult result = vmaCreateImage(instance.GetAllocator(), &unsafe_image_info, &alloc_info,
                                     &unsafe_image, &frame->allocation, nullptr);
    if (result != VK_SUCCESS) [[unlikely]] {
        LOG_CRITICAL(Render_Vulkan, "Failed allocating texture with error {}",
                     vk::to_string(vk::Result{result}));
        UNREACHABLE();
    }
    frame->image = vk::Image{unsafe_image};

    const vk::ImageViewCreateInfo view_info = {
        .image = frame->image,
        .viewType = vk::ImageViewType::e2D,
        .format = FormatToUnorm(format),
        .subresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    auto [view_result, view] = device.createImageView(view_info);
    ASSERT_MSG(view_result == vk::Result::eSuccess, "Failed to create frame image view: {}",
               vk::to_string(view_result));
    frame->image_view = view;
    frame->width = width;
    frame->height = height;
}

bool Presenter::ShowSplash(Frame* frame /*= nullptr*/) {
    const auto* splash = Common::Singleton<Splash>::Instance();
    if (splash->GetImageData().empty()) {
        return false;
    }

    if (!Libraries::SystemService::IsSplashVisible()) {
        return false;
    }

    draw_scheduler.EndRendering();
    const auto cmdbuf = draw_scheduler.CommandBuffer();

    if (!frame) {
        if (!splash_img.has_value()) {
            VideoCore::ImageInfo info{};
            info.pixel_format = vk::Format::eR8G8B8A8Srgb;
            info.type = vk::ImageType::e2D;
            info.size =
                VideoCore::Extent3D{splash->GetImageInfo().width, splash->GetImageInfo().height, 1};
            info.pitch = splash->GetImageInfo().width;
            info.guest_address = VAddr(splash->GetImageData().data());
            info.guest_size_bytes = splash->GetImageData().size();
            info.mips_layout.emplace_back(splash->GetImageData().size(),
                                          splash->GetImageInfo().width,
                                          splash->GetImageInfo().height, 0);
            splash_img.emplace(instance, present_scheduler, info);
            texture_cache.RefreshImage(*splash_img);

            splash_img->Transit(vk::ImageLayout::eTransferSrcOptimal,
                                vk::AccessFlagBits2::eTransferRead, {}, cmdbuf);
        }

        frame = GetRenderFrame();
    }

    const auto frame_subresources = vk::ImageSubresourceRange{
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    const auto pre_barrier =
        vk::ImageMemoryBarrier2{.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
                                .srcAccessMask = vk::AccessFlagBits2::eTransferRead,
                                .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
                                .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
                                .oldLayout = vk::ImageLayout::eUndefined,
                                .newLayout = vk::ImageLayout::eTransferDstOptimal,
                                .image = frame->image,
                                .subresourceRange{frame_subresources}};

    cmdbuf.pipelineBarrier2(vk::DependencyInfo{
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &pre_barrier,
    });

    cmdbuf.blitImage(splash_img->image, vk::ImageLayout::eTransferSrcOptimal, frame->image,
                     vk::ImageLayout::eTransferDstOptimal,
                     MakeImageBlitFit(splash->GetImageInfo().width, splash->GetImageInfo().height,
                                      frame->width, frame->height),
                     vk::Filter::eLinear);

    const auto post_barrier =
        vk::ImageMemoryBarrier2{.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
                                .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
                                .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
                                .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                                .newLayout = vk::ImageLayout::eGeneral,
                                .image = frame->image,
                                .subresourceRange{frame_subresources}};

    cmdbuf.pipelineBarrier2(vk::DependencyInfo{
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &post_barrier,
    });

    // Flush frame creation commands.
    frame->ready_semaphore = draw_scheduler.GetMasterSemaphore()->Handle();
    frame->ready_tick = draw_scheduler.CurrentTick();
    SubmitInfo info{};
    draw_scheduler.Flush(info);

    Present(frame);
    return true;
}

Frame* Presenter::PrepareFrameInternal(VideoCore::ImageId image_id, bool is_eop) {
    // Request a free presentation frame.
    Frame* frame = GetRenderFrame();

    // EOP flips are triggered from GPU thread so use the drawing scheduler to record
    // commands. Otherwise we are dealing with a CPU flip which could have arrived
    // from any guest thread. Use a separate scheduler for that.
    auto& scheduler = is_eop ? draw_scheduler : flip_scheduler;
    scheduler.EndRendering();
    const auto cmdbuf = scheduler.CommandBuffer();

    const auto frame_subresources = vk::ImageSubresourceRange{
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    const auto pre_barrier =
        vk::ImageMemoryBarrier2{.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
                                .srcAccessMask = vk::AccessFlagBits2::eTransferRead,
                                .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
                                .oldLayout = vk::ImageLayout::eUndefined,
                                .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                .image = frame->image,
                                .subresourceRange{frame_subresources}};

    cmdbuf.pipelineBarrier2(vk::DependencyInfo{
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &pre_barrier,
    });

    if (image_id != VideoCore::NULL_IMAGE_ID) {
        auto& image = texture_cache.GetImage(image_id);
        image.Transit(vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits2::eShaderRead, {},
                      cmdbuf);

        static vk::DescriptorImageInfo image_info{
            .sampler = *pp_sampler,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        };

        VideoCore::ImageViewInfo info{};
        info.format = image.info.pixel_format;
        if (auto view = image.FindView(info)) {
            image_info.imageView = *texture_cache.GetImageView(view).image_view;
        } else {
            image_info.imageView = *texture_cache.RegisterImageView(image_id, info).image_view;
        }

        static const std::array set_writes{
            vk::WriteDescriptorSet{
                .dstSet = VK_NULL_HANDLE,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &image_info,
            },
        };

        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *pp_pipeline);

        const auto& dst_rect =
            FitImage(image.info.size.width, image.info.size.height, frame->width, frame->height);

        const std::array viewports = {
            vk::Viewport{
                .x = 1.0f * dst_rect.offset.x,
                .y = 1.0f * dst_rect.offset.y,
                .width = 1.0f * dst_rect.extent.width,
                .height = 1.0f * dst_rect.extent.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            },
        };

        cmdbuf.setViewport(0, viewports);
        cmdbuf.setScissor(0, {dst_rect});

        cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, *pp_pipeline_layout, 0,
                                    set_writes);
        cmdbuf.pushConstants(*pp_pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0,
                             sizeof(PostProcessSettings), &pp_settings);

        const std::array attachments = {vk::RenderingAttachmentInfo{
            .imageView = frame->image_view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
        }};

        vk::RenderingInfo rendering_info{
            .renderArea =
                vk::Rect2D{
                    .offset = {0, 0},
                    .extent = {frame->width, frame->height},
                },
            .layerCount = 1,
            .colorAttachmentCount = attachments.size(),
            .pColorAttachments = attachments.data(),
        };
        cmdbuf.beginRendering(rendering_info);
        cmdbuf.draw(3, 1, 0, 0);
        cmdbuf.endRendering();
    }

    const auto post_barrier =
        vk::ImageMemoryBarrier2{.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
                                .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
                                .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                .newLayout = vk::ImageLayout::eGeneral,
                                .image = frame->image,
                                .subresourceRange{frame_subresources}};

    cmdbuf.pipelineBarrier2(vk::DependencyInfo{
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &post_barrier,
    });

    // Flush frame creation commands.
    frame->ready_semaphore = scheduler.GetMasterSemaphore()->Handle();
    frame->ready_tick = scheduler.CurrentTick();
    SubmitInfo info{};
    scheduler.Flush(info);
    return frame;
}

void Presenter::Present(Frame* frame) {
    // Recreate the swapchain if the window was resized.
    if (window.GetWidth() != swapchain.GetExtent().width ||
        window.GetHeight() != swapchain.GetExtent().height) {
        swapchain.Recreate(window.GetWidth(), window.GetHeight());
    }

    if (!swapchain.AcquireNextImage()) {
        swapchain.Recreate(window.GetWidth(), window.GetHeight());
    }

    ImGui::Core::NewFrame();

    const vk::Image swapchain_image = swapchain.Image();

    auto& scheduler = present_scheduler;
    const auto cmdbuf = scheduler.CommandBuffer();

    ImGui::Core::Render(cmdbuf, frame);

    {
        auto* profiler_ctx = instance.GetProfilerContext();
        TracyVkNamedZoneC(profiler_ctx, renderer_gpu_zone, cmdbuf, "Host frame",
                          MarkersPalette::GpuMarkerColor, profiler_ctx != nullptr);

        const vk::Extent2D extent = swapchain.GetExtent();
        const std::array pre_barriers{
            vk::ImageMemoryBarrier{
                .srcAccessMask = vk::AccessFlagBits::eNone,
                .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
                .oldLayout = vk::ImageLayout::eUndefined,
                .newLayout = vk::ImageLayout::eTransferDstOptimal,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = swapchain_image,
                .subresourceRange{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = VK_REMAINING_ARRAY_LAYERS,
                },
            },
            vk::ImageMemoryBarrier{
                .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
                .dstAccessMask = vk::AccessFlagBits::eTransferRead,
                .oldLayout = vk::ImageLayout::eGeneral,
                .newLayout = vk::ImageLayout::eTransferSrcOptimal,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = frame->image,
                .subresourceRange{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = VK_REMAINING_ARRAY_LAYERS,
                },
            },
        };
        const vk::ImageMemoryBarrier post_barrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eMemoryRead,
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::ePresentSrcKHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = swapchain_image,
            .subresourceRange{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS,
            },
        };

        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
                               vk::PipelineStageFlagBits::eTransfer,
                               vk::DependencyFlagBits::eByRegion, {}, {}, pre_barriers);

        cmdbuf.blitImage(
            frame->image, vk::ImageLayout::eTransferSrcOptimal, swapchain_image,
            vk::ImageLayout::eTransferDstOptimal,
            MakeImageBlitStretch(frame->width, frame->height, extent.width, extent.height),
            vk::Filter::eLinear);

        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                               vk::PipelineStageFlagBits::eAllCommands,
                               vk::DependencyFlagBits::eByRegion, {}, {}, post_barrier);

        if (profiler_ctx) {
            TracyVkCollect(profiler_ctx, cmdbuf);
        }
    }

    // Flush vulkan commands.
    SubmitInfo info{};
    info.AddWait(swapchain.GetImageAcquiredSemaphore());
    info.AddWait(frame->ready_semaphore, frame->ready_tick);
    info.AddSignal(swapchain.GetPresentReadySemaphore());
    info.AddSignal(frame->present_done);
    scheduler.Flush(info);

    // Present to swapchain.
    std::scoped_lock submit_lock{Scheduler::submit_mutex};
    if (!swapchain.Present()) {
        swapchain.Recreate(window.GetWidth(), window.GetHeight());
    }

    // Free the frame for reuse
    std::scoped_lock fl{free_mutex};
    free_queue.push(frame);
    free_cv.notify_one();

    DebugState.IncFlipFrameNum();
}

Frame* Presenter::GetRenderFrame() {
    // Wait for free presentation frames
    Frame* frame;
    {
        std::unique_lock lock{free_mutex};
        free_cv.wait(lock, [this] { return !free_queue.empty(); });
        LOG_DEBUG(Render_Vulkan, "Got render frame, remaining {}", free_queue.size() - 1);

        // Take the frame from the queue
        frame = free_queue.front();
        free_queue.pop();
    }

    const vk::Device device = instance.GetDevice();
    vk::Result result{};

    const auto wait = [&]() {
        result = device.waitForFences(frame->present_done, false, std::numeric_limits<u64>::max());
        return result;
    };

    // Wait for the presentation to be finished so all frame resources are free
    while (wait() != vk::Result::eSuccess) {
        ASSERT_MSG(result != vk::Result::eErrorDeviceLost,
                   "Device lost during waiting for a frame");
        // Retry if the waiting times out
        if (result == vk::Result::eTimeout) {
            continue;
        }
    }

    // Reset fence for next queue submission.
    device.resetFences(frame->present_done);

    // If the window dimensions changed, recreate this frame
    if (frame->width != window.GetWidth() || frame->height != window.GetHeight()) {
        RecreateFrame(frame, window.GetWidth(), window.GetHeight());
    }

    return frame;
}

} // namespace Vulkan
