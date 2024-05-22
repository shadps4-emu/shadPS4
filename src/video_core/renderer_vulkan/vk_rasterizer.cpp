// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/texture_cache/image_view.h"
#include "video_core/texture_cache/texture_cache.h"

namespace Vulkan {

static constexpr vk::BufferUsageFlags VertexIndexFlags = vk::BufferUsageFlagBits::eVertexBuffer |
                                                         vk::BufferUsageFlagBits::eIndexBuffer |
                                                         vk::BufferUsageFlagBits::eTransferDst;

Rasterizer::Rasterizer(const Instance& instance_, Scheduler& scheduler_,
                       VideoCore::TextureCache& texture_cache_, AmdGpu::Liverpool* liverpool_)
    : instance{instance_}, scheduler{scheduler_}, texture_cache{texture_cache_},
      liverpool{liverpool_}, pipeline_cache{instance, scheduler, liverpool},
      vertex_index_buffer{instance, scheduler, VertexIndexFlags, 64_MB} {
    if (!Config::nullGpu()) {
        liverpool->BindRasterizer(this);
    }
}

Rasterizer::~Rasterizer() = default;

void Rasterizer::DrawIndex() {
    const auto cmdbuf = scheduler.CommandBuffer();
    auto& regs = liverpool->regs;

    static bool first_time = true;
    if (first_time) {
        first_time = false;
        return;
    }

    UpdateDynamicState();

    pipeline_cache.BindPipeline();

    const u32 pitch = regs.color_buffers[0].Pitch();
    const u32 height = regs.color_buffers[0].Height();
    const u32 tile_max = regs.color_buffers[0].slice.tile_max;
    auto& image_view = texture_cache.RenderTarget(regs.color_buffers[0].Address(), pitch);

    const vk::RenderingAttachmentInfo color_info = {
        .imageView = *image_view.image_view,
        .imageLayout = vk::ImageLayout::eGeneral,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
    };

    // TODO: Don't restart renderpass every draw
    const vk::RenderingInfo rendering_info = {
        .renderArea = {.offset = {0, 0}, .extent = {1920, 1080}},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_info,
    };

    cmdbuf.beginRendering(rendering_info);
    cmdbuf.bindIndexBuffer(vertex_index_buffer.Handle(), 0, vk::IndexType::eUint32);
    cmdbuf.bindVertexBuffers(0, vertex_index_buffer.Handle(), vk::DeviceSize(0));
    cmdbuf.draw(regs.num_indices, regs.num_instances.NumInstances(), 0, 0);
    cmdbuf.endRendering();
}

void Rasterizer::UpdateDynamicState() {
    UpdateViewportScissorState();
}

void Rasterizer::UpdateViewportScissorState() {
    auto& regs = liverpool->regs;

    const auto cmdbuf = scheduler.CommandBuffer();
    const vk::Viewport viewport{
        .x = regs.viewports[0].xoffset - regs.viewports[0].xscale,
        .y = regs.viewports[0].yoffset - regs.viewports[0].yscale,
        .width = regs.viewports[0].xscale * 2.0f,
        .height = regs.viewports[0].yscale * 2.0f,
        .minDepth = regs.viewports[0].zoffset - regs.viewports[0].zscale,
        .maxDepth = regs.viewports[0].zscale + regs.viewports[0].zoffset,
    };
    const vk::Rect2D scissor{
        .offset = {regs.screen_scissor.top_left_x, regs.screen_scissor.top_left_y},
        .extent = {regs.screen_scissor.GetWidth(), regs.screen_scissor.GetHeight()},
    };
    cmdbuf.setViewport(0, viewport);
    cmdbuf.setScissor(0, scissor);
}

void Rasterizer::UpdateDepthStencilState() {
    auto& depth = liverpool->regs.depth_control;

    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.setDepthBoundsTestEnable(depth.depth_bounds_enable);
}

} // namespace Vulkan
