// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "core/memory.h"
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
      liverpool{liverpool_}, memory{Core::Memory::Instance()},
      pipeline_cache{instance, scheduler, liverpool},
      vertex_index_buffer{instance, scheduler, VertexIndexFlags, 64_MB} {
    if (!Config::nullGpu()) {
        liverpool->BindRasterizer(this);
    }

    memory->SetInstance(&instance);
}

Rasterizer::~Rasterizer() = default;

void Rasterizer::Draw(bool is_indexed) {
    const auto cmdbuf = scheduler.CommandBuffer();
    const auto& regs = liverpool->regs;
    const u32 num_indices = SetupIndexBuffer(is_indexed);
    const auto& image_view = texture_cache.RenderTarget(regs.color_buffers[0]);
    const GraphicsPipeline* pipeline = pipeline_cache.GetPipeline();
    pipeline->BindResources(memory);

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

    UpdateDynamicState();

    cmdbuf.beginRendering(rendering_info);
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->Handle());
    if (is_indexed) {
        cmdbuf.drawIndexed(num_indices, regs.num_instances.NumInstances(), 0, 0, 0);
    } else {
        cmdbuf.draw(regs.num_indices, regs.num_instances.NumInstances(), 0, 0);
    }
    cmdbuf.endRendering();
}

u32 Rasterizer::SetupIndexBuffer(bool& is_indexed) {
    // Emulate QuadList primitive type with CPU made index buffer.
    const auto& regs = liverpool->regs;
    if (liverpool->regs.primitive_type == Liverpool::PrimitiveType::QuadList) {
        ASSERT_MSG(!is_indexed, "Using QuadList primitive with indexed draw");
        is_indexed = true;

        // Emit indices.
        const u32 index_size = 3 * regs.num_indices;
        const auto [data, offset, _] = vertex_index_buffer.Map(index_size);
        LiverpoolToVK::EmitQuadToTriangleListIndices(data, regs.num_indices);
        vertex_index_buffer.Commit(index_size);

        // Bind index buffer.
        const auto cmdbuf = scheduler.CommandBuffer();
        cmdbuf.bindIndexBuffer(vertex_index_buffer.Handle(), offset, vk::IndexType::eUint16);
        return index_size / sizeof(u16);
    }
    if (!is_indexed) {
        return 0;
    }

    const VAddr index_address = regs.index_base_address.Address();
    const auto [buffer, offset] = memory->GetVulkanBuffer(index_address);
    const vk::IndexType index_type =
        regs.index_buffer_type.index_type == Liverpool::IndexType::Index16 ? vk::IndexType::eUint16
                                                                           : vk::IndexType::eUint32;
    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.bindIndexBuffer(buffer, offset, index_type);
    return regs.num_indices;
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
