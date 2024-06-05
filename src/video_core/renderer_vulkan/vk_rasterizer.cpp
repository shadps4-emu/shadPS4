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

static constexpr vk::BufferUsageFlags VertexIndexFlags =
    vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer |
    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer |
    vk::BufferUsageFlagBits::eStorageBuffer;

Rasterizer::Rasterizer(const Instance& instance_, Scheduler& scheduler_,
                       VideoCore::TextureCache& texture_cache_, AmdGpu::Liverpool* liverpool_)
    : instance{instance_}, scheduler{scheduler_}, texture_cache{texture_cache_},
      liverpool{liverpool_}, memory{Core::Memory::Instance()},
      pipeline_cache{instance, scheduler, liverpool},
      vertex_index_buffer{instance, scheduler, VertexIndexFlags, 32_MB} {
    if (!Config::nullGpu()) {
        liverpool->BindRasterizer(this);
    }

    memory->SetInstance(&instance);
}

Rasterizer::~Rasterizer() = default;

void Rasterizer::Draw(bool is_indexed, u32 index_offset) {
    const auto cmdbuf = scheduler.CommandBuffer();
    const auto& regs = liverpool->regs;
    const u32 num_indices = SetupIndexBuffer(is_indexed, index_offset);
    const GraphicsPipeline* pipeline = pipeline_cache.GetGraphicsPipeline();
    pipeline->BindResources(memory, vertex_index_buffer, texture_cache);

    boost::container::static_vector<vk::RenderingAttachmentInfo, Liverpool::NumColorBuffers>
        color_attachments{};
    for (auto col_buf_id = 0u; col_buf_id < Liverpool::NumColorBuffers; ++col_buf_id) {
        const auto& col_buf = regs.color_buffers[col_buf_id];
        if (!col_buf) {
            continue;
        }

        const auto& hint = liverpool->last_cb_extent[col_buf_id];
        const auto& image_view = texture_cache.RenderTarget(col_buf, hint);

        color_attachments.push_back({
            .imageView = *image_view.image_view,
            .imageLayout = vk::ImageLayout::eGeneral,
            .loadOp = vk::AttachmentLoadOp::eLoad,
            .storeOp = vk::AttachmentStoreOp::eStore,
        });
    }

    // TODO: Don't restart renderpass every draw
    const auto& scissor = regs.screen_scissor;
    const vk::RenderingInfo rendering_info = {
        .renderArea =
            {
                .offset = {scissor.top_left_x, scissor.top_left_y},
                .extent = {scissor.GetWidth(), scissor.GetHeight()},
            },
        .layerCount = 1,
        .colorAttachmentCount = static_cast<u32>(color_attachments.size()),
        .pColorAttachments = color_attachments.data(),
    };

    UpdateDynamicState(*pipeline);

    cmdbuf.beginRendering(rendering_info);
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->Handle());
    if (is_indexed) {
        cmdbuf.drawIndexed(num_indices, regs.num_instances.NumInstances(), 0, 0, 0);
    } else {
        const u32 num_vertices = pipeline->IsEmbeddedVs() ? 4 : regs.num_indices;
        cmdbuf.draw(num_vertices, regs.num_instances.NumInstances(), 0, 0);
    }
    cmdbuf.endRendering();
}

void Rasterizer::DispatchDirect() {
    const auto cmdbuf = scheduler.CommandBuffer();
    const auto& cs_program = liverpool->regs.cs_program;
    const ComputePipeline* pipeline = pipeline_cache.GetComputePipeline();
    pipeline->BindResources(memory, vertex_index_buffer, texture_cache);

    cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline->Handle());
    cmdbuf.dispatch(cs_program.dim_x, cs_program.dim_y, cs_program.dim_z);
}

u32 Rasterizer::SetupIndexBuffer(bool& is_indexed, u32 index_offset) {
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
        return regs.num_indices;
    }

    // Figure out index type and size.
    const bool is_index16 = regs.index_buffer_type.index_type == Liverpool::IndexType::Index16;
    const vk::IndexType index_type = is_index16 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;
    const u32 index_size = is_index16 ? sizeof(u16) : sizeof(u32);

    // Upload index data to stream buffer.
    const auto index_address = regs.index_base_address.Address<const void*>();
    const u32 index_buffer_size = regs.num_indices * index_size;
    const auto [data, offset, _] = vertex_index_buffer.Map(index_buffer_size);
    std::memcpy(data, index_address, index_buffer_size);
    vertex_index_buffer.Commit(index_buffer_size);

    // Bind index buffer.
    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.bindIndexBuffer(vertex_index_buffer.Handle(), offset + index_offset * index_size,
                           index_type);
    return regs.num_indices;
}

void Rasterizer::UpdateDynamicState(const GraphicsPipeline& pipeline) {
    UpdateViewportScissorState();

    auto& regs = liverpool->regs;
    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.setBlendConstants(&regs.blend_constants.red);

    if (instance.IsColorWriteEnableSupported()) {
        const auto& write_masks = pipeline.GetWriteMasks();
        std::array<vk::Bool32, Liverpool::NumColorBuffers> write_ens{};
        std::transform(write_masks.cbegin(), write_masks.cend(), write_ens.begin(),
                       [](auto in) { return in ? vk::True : vk::False; });

        cmdbuf.setColorWriteEnableEXT(write_ens);
        cmdbuf.setColorWriteMaskEXT(0, write_masks);
    }
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
