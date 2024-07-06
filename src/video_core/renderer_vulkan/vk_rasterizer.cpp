// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/debug.h"
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
      vertex_index_buffer{instance, scheduler, VertexIndexFlags, 3_GB, BufferType::Upload} {
    if (!Config::nullGpu()) {
        liverpool->BindRasterizer(this);
    }

    memory->SetInstance(&instance);
}

Rasterizer::~Rasterizer() = default;

void Rasterizer::Draw(bool is_indexed, u32 index_offset) {
    RENDERER_TRACE;

    const auto cmdbuf = scheduler.CommandBuffer();
    const auto& regs = liverpool->regs;
    const u32 num_indices = SetupIndexBuffer(is_indexed, index_offset);
    const GraphicsPipeline* pipeline = pipeline_cache.GetGraphicsPipeline();
    if (!pipeline) {
        return;
    }

    try {
        pipeline->BindResources(memory, vertex_index_buffer, texture_cache);
    } catch (...) {
        UNREACHABLE();
    }

    BeginRendering();
    UpdateDynamicState(*pipeline);

    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->Handle());

    const u32 step_rates[] = {
        regs.vgt_instance_step_rate_0,
        regs.vgt_instance_step_rate_1,
    };
    cmdbuf.pushConstants(pipeline->GetLayout(), vk::ShaderStageFlagBits::eVertex, 0u,
                         sizeof(step_rates), &step_rates);
    if (is_indexed) {
        cmdbuf.drawIndexed(num_indices, regs.num_instances.NumInstances(), 0, 0, 0);
    } else {
        const u32 num_vertices = regs.primitive_type == AmdGpu::Liverpool::PrimitiveType::RectList
                                     ? 4
                                     : regs.num_indices;
        cmdbuf.draw(num_vertices, regs.num_instances.NumInstances(), 0, 0);
    }
}

void Rasterizer::DispatchDirect() {
    RENDERER_TRACE;

    const auto cmdbuf = scheduler.CommandBuffer();
    const auto& cs_program = liverpool->regs.cs_program;
    const ComputePipeline* pipeline = pipeline_cache.GetComputePipeline();
    if (!pipeline) {
        return;
    }

    try {
        const auto has_resources =
            pipeline->BindResources(memory, vertex_index_buffer, texture_cache);
        if (!has_resources) {
            return;
        }
    } catch (...) {
        UNREACHABLE();
    }

    scheduler.EndRendering();
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline->Handle());
    cmdbuf.dispatch(cs_program.dim_x, cs_program.dim_y, cs_program.dim_z);
}

void Rasterizer::BeginRendering() {
    const auto& regs = liverpool->regs;
    RenderState state;

    for (auto col_buf_id = 0u; col_buf_id < Liverpool::NumColorBuffers; ++col_buf_id) {
        const auto& col_buf = regs.color_buffers[col_buf_id];
        if (!col_buf) {
            continue;
        }

        // If the color buffer is still bound but rendering to it is disabled by the target mask,
        // we need to prevent the render area from being affected by unbound render target extents.
        if (!regs.color_target_mask.GetMask(col_buf_id)) {
            continue;
        }

        const auto& hint = liverpool->last_cb_extent[col_buf_id];
        const auto& image_view = texture_cache.RenderTarget(col_buf, hint);
        const auto& image = texture_cache.GetImage(image_view.image_id);
        state.width = std::min<u32>(state.width, image.info.size.width);
        state.height = std::min<u32>(state.height, image.info.size.height);

        const bool is_clear = texture_cache.IsMetaCleared(col_buf.CmaskAddress());
        state.color_attachments[state.num_color_attachments++] = {
            .imageView = *image_view.image_view,
            .imageLayout = vk::ImageLayout::eGeneral,
            .loadOp = is_clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue =
                is_clear ? LiverpoolToVK::ColorBufferClearValue(col_buf) : vk::ClearValue{},
        };
        texture_cache.TouchMeta(col_buf.CmaskAddress(), false);
    }

    if (regs.depth_buffer.z_info.format != Liverpool::DepthBuffer::ZFormat::Invald &&
        regs.depth_buffer.Address() != 0) {
        const auto htile_address = regs.depth_htile_data_base.GetAddress();
        const bool is_clear = regs.depth_render_control.depth_clear_enable ||
                              texture_cache.IsMetaCleared(htile_address);
        const auto& hint = liverpool->last_db_extent;
        const auto& image_view = texture_cache.DepthTarget(regs.depth_buffer, htile_address, hint,
                                                           regs.depth_control.depth_write_enable);
        const auto& image = texture_cache.GetImage(image_view.image_id);
        state.width = std::min<u32>(state.width, image.info.size.width);
        state.height = std::min<u32>(state.height, image.info.size.height);
        state.depth_attachment = {
            .imageView = *image_view.image_view,
            .imageLayout = image.layout,
            .loadOp = is_clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
            .storeOp = is_clear ? vk::AttachmentStoreOp::eNone : vk::AttachmentStoreOp::eStore,
            .clearValue = vk::ClearValue{.depthStencil = {.depth = regs.depth_clear,
                                                          .stencil = regs.stencil_clear}},
        };
        texture_cache.TouchMeta(htile_address, false);
        state.num_depth_attachments++;
    }
    scheduler.BeginRendering(state);
}

u32 Rasterizer::SetupIndexBuffer(bool& is_indexed, u32 index_offset) {
    // Emulate QuadList primitive type with CPU made index buffer.
    const auto& regs = liverpool->regs;
    if (liverpool->regs.primitive_type == Liverpool::PrimitiveType::QuadList) {
        // ASSERT_MSG(!is_indexed, "Using QuadList primitive with indexed draw");
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
    const u32 index_buffer_size = (index_offset + regs.num_indices) * index_size;
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

    boost::container::static_vector<vk::Viewport, Liverpool::NumViewports> viewports;
    boost::container::static_vector<vk::Rect2D, Liverpool::NumViewports> scissors;

    const float reduce_z =
        regs.clipper_control.clip_space == AmdGpu::Liverpool::ClipSpace::MinusWToW ? 1.0f : 0.0f;
    for (u32 i = 0; i < Liverpool::NumViewports; i++) {
        const auto& vp = regs.viewports[i];
        const auto& vp_d = regs.viewport_depths[i];
        if (vp.xscale == 0) {
            continue;
        }
        viewports.push_back({
            .x = vp.xoffset - vp.xscale,
            .y = vp.yoffset - vp.yscale,
            .width = vp.xscale * 2.0f,
            .height = vp.yscale * 2.0f,
            .minDepth = vp.zoffset - vp.zscale * reduce_z,
            .maxDepth = vp.zscale + vp.zoffset,
        });
    }
    const auto& sc = regs.screen_scissor;
    scissors.push_back({
        .offset = {sc.top_left_x, sc.top_left_y},
        .extent = {sc.GetWidth(), sc.GetHeight()},
    });
    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.setViewport(0, viewports);
    cmdbuf.setScissor(0, scissors);
}

void Rasterizer::UpdateDepthStencilState() {
    auto& depth = liverpool->regs.depth_control;

    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.setDepthBoundsTestEnable(depth.depth_bounds_enable);
}

} // namespace Vulkan
