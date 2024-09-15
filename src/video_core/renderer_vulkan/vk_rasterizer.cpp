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
#include "vk_rasterizer.h"

namespace Vulkan {

Rasterizer::Rasterizer(const Instance& instance_, Scheduler& scheduler_,
                       AmdGpu::Liverpool* liverpool_)
    : instance{instance_}, scheduler{scheduler_}, page_manager{this},
      buffer_cache{instance, scheduler, liverpool_, texture_cache, page_manager},
      texture_cache{instance, scheduler, buffer_cache, page_manager}, liverpool{liverpool_},
      memory{Core::Memory::Instance()}, pipeline_cache{instance, scheduler, liverpool} {
    if (!Config::nullGpu()) {
        liverpool->BindRasterizer(this);
    }
    memory->SetRasterizer(this);
    wfi_event = instance.GetDevice().createEventUnique({});
}

Rasterizer::~Rasterizer() = default;

void Rasterizer::CpSync() {
    scheduler.EndRendering();
    auto cmdbuf = scheduler.CommandBuffer();

    const vk::MemoryBarrier ib_barrier{
        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
        .dstAccessMask = vk::AccessFlagBits::eIndirectCommandRead,
    };
    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                           vk::PipelineStageFlagBits::eDrawIndirect,
                           vk::DependencyFlagBits::eByRegion, ib_barrier, {}, {});
}

void Rasterizer::Draw(bool is_indexed, u32 index_offset) {
    RENDERER_TRACE;

    const auto cmdbuf = scheduler.CommandBuffer();
    const auto& regs = liverpool->regs;
    const GraphicsPipeline* pipeline = pipeline_cache.GetGraphicsPipeline();
    if (!pipeline) {
        return;
    }

    try {
        pipeline->BindResources(regs, buffer_cache, texture_cache);
    } catch (...) {
        UNREACHABLE();
    }

    const auto& vs_info = pipeline->GetStage(Shader::Stage::Vertex);
    buffer_cache.BindVertexBuffers(vs_info);
    const u32 num_indices = buffer_cache.BindIndexBuffer(is_indexed, index_offset);

    BeginRendering();
    UpdateDynamicState(*pipeline);

    const auto [vertex_offset, instance_offset] = vs_info.GetDrawOffsets();

    if (is_indexed) {
        cmdbuf.drawIndexed(num_indices, regs.num_instances.NumInstances(), 0, s32(vertex_offset),
                           instance_offset);
    } else {
        const u32 num_vertices = regs.primitive_type == AmdGpu::Liverpool::PrimitiveType::RectList
                                     ? 4
                                     : regs.num_indices;
        cmdbuf.draw(num_vertices, regs.num_instances.NumInstances(), vertex_offset,
                    instance_offset);
    }
}

void Rasterizer::DrawIndirect(bool is_indexed, VAddr address, u32 offset, u32 size) {
    RENDERER_TRACE;

    const auto cmdbuf = scheduler.CommandBuffer();
    const auto& regs = liverpool->regs;
    const GraphicsPipeline* pipeline = pipeline_cache.GetGraphicsPipeline();
    if (!pipeline) {
        return;
    }

    ASSERT_MSG(regs.primitive_type != AmdGpu::Liverpool::PrimitiveType::RectList,
               "Unsupported primitive type for indirect draw");

    try {
        pipeline->BindResources(regs, buffer_cache, texture_cache);
    } catch (...) {
        UNREACHABLE();
    }

    const auto& vs_info = pipeline->GetStage(Shader::Stage::Vertex);
    buffer_cache.BindVertexBuffers(vs_info);
    const u32 num_indices = buffer_cache.BindIndexBuffer(is_indexed, 0);

    BeginRendering();
    UpdateDynamicState(*pipeline);

    const auto [buffer, base] = buffer_cache.ObtainBuffer(address, size, true);
    const auto total_offset = base + offset;

    // We can safely ignore both SGPR UD indices and results of fetch shader parsing, as vertex and
    // instance offsets will be automatically applied by Vulkan from indirect args buffer.

    if (is_indexed) {
        cmdbuf.drawIndexedIndirect(buffer->Handle(), total_offset, 1, 0);
    } else {
        cmdbuf.drawIndirect(buffer->Handle(), total_offset, 1, 0);
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
        const auto has_resources = pipeline->BindResources(buffer_cache, texture_cache);
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

void Rasterizer::DispatchIndirect(VAddr address, u32 offset, u32 size) {
    RENDERER_TRACE;

    const auto cmdbuf = scheduler.CommandBuffer();
    const auto& cs_program = liverpool->regs.cs_program;
    const ComputePipeline* pipeline = pipeline_cache.GetComputePipeline();
    if (!pipeline) {
        return;
    }

    try {
        const auto has_resources = pipeline->BindResources(buffer_cache, texture_cache);
        if (!has_resources) {
            return;
        }
    } catch (...) {
        UNREACHABLE();
    }

    scheduler.EndRendering();
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline->Handle());
    const auto [buffer, base] = buffer_cache.ObtainBuffer(address, size, true);
    const auto total_offset = base + offset;
    cmdbuf.dispatchIndirect(buffer->Handle(), total_offset);
}

u64 Rasterizer::Flush() {
    const u64 current_tick = scheduler.CurrentTick();
    SubmitInfo info{};
    scheduler.Flush(info);
    return current_tick;
}

void Rasterizer::Finish() {
    scheduler.Finish();
}

void Rasterizer::BeginRendering() {
    const auto& regs = liverpool->regs;
    RenderState state;

    if (regs.color_control.degamma_enable) {
        LOG_WARNING(Render_Vulkan, "Color buffers require gamma correction");
    }

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
        VideoCore::ImageInfo image_info{col_buf, hint};
        VideoCore::ImageViewInfo view_info{col_buf, false /*!!image.info.usage.vo_buffer*/};
        const auto& image_view = texture_cache.FindRenderTarget(image_info, view_info);
        const auto& image = texture_cache.GetImage(image_view.image_id);
        state.width = std::min<u32>(state.width, image.info.size.width);
        state.height = std::min<u32>(state.height, image.info.size.height);

        const bool is_clear = texture_cache.IsMetaCleared(col_buf.CmaskAddress());
        state.color_images[state.num_color_attachments] = image.image;
        state.color_attachments[state.num_color_attachments++] = {
            .imageView = *image_view.image_view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = is_clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue =
                is_clear ? LiverpoolToVK::ColorBufferClearValue(col_buf) : vk::ClearValue{},
        };
        texture_cache.TouchMeta(col_buf.CmaskAddress(), false);
    }

    using ZFormat = AmdGpu::Liverpool::DepthBuffer::ZFormat;
    using StencilFormat = AmdGpu::Liverpool::DepthBuffer::StencilFormat;
    if (regs.depth_buffer.Address() != 0 &&
        ((regs.depth_control.depth_enable && regs.depth_buffer.z_info.format != ZFormat::Invalid) ||
         (regs.depth_control.stencil_enable &&
          regs.depth_buffer.stencil_info.format != StencilFormat::Invalid))) {
        const auto htile_address = regs.depth_htile_data_base.GetAddress();
        const bool is_clear = regs.depth_render_control.depth_clear_enable ||
                              texture_cache.IsMetaCleared(htile_address);
        const auto& hint = liverpool->last_db_extent;
        VideoCore::ImageInfo image_info{regs.depth_buffer, regs.depth_view.NumSlices(),
                                        htile_address, hint};
        VideoCore::ImageViewInfo view_info{regs.depth_buffer, regs.depth_view, regs.depth_control};
        const auto& image_view = texture_cache.FindDepthTarget(image_info, view_info);
        const auto& image = texture_cache.GetImage(image_view.image_id);
        state.width = std::min<u32>(state.width, image.info.size.width);
        state.height = std::min<u32>(state.height, image.info.size.height);
        state.depth_image = image.image;
        state.depth_attachment = {
            .imageView = *image_view.image_view,
            .imageLayout = image.layout,
            .loadOp = is_clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
            .storeOp = is_clear ? vk::AttachmentStoreOp::eNone : vk::AttachmentStoreOp::eStore,
            .clearValue = vk::ClearValue{.depthStencil = {.depth = regs.depth_clear,
                                                          .stencil = regs.stencil_clear}},
        };
        texture_cache.TouchMeta(htile_address, false);
        state.has_depth =
            regs.depth_buffer.z_info.format != AmdGpu::Liverpool::DepthBuffer::ZFormat::Invalid;
        state.has_stencil = regs.depth_buffer.stencil_info.format !=
                            AmdGpu::Liverpool::DepthBuffer::StencilFormat::Invalid;
    }
    scheduler.BeginRendering(state);
}

void Rasterizer::InlineDataToGds(u32 gds_offset, u32 value) {
    buffer_cache.InlineDataToGds(gds_offset, value);
}

u32 Rasterizer::ReadDataFromGds(u32 gds_offset) {
    auto* gds_buf = buffer_cache.GetGdsBuffer();
    u32 value;
    std::memcpy(&value, gds_buf->mapped_data.data() + gds_offset, sizeof(u32));
    return value;
}

void Rasterizer::InvalidateMemory(VAddr addr, u64 size) {
    buffer_cache.InvalidateMemory(addr, size);
    texture_cache.InvalidateMemory(addr, size);
}

void Rasterizer::MapMemory(VAddr addr, u64 size) {
    page_manager.OnGpuMap(addr, size);
}

void Rasterizer::UnmapMemory(VAddr addr, u64 size) {
    buffer_cache.InvalidateMemory(addr, size);
    texture_cache.UnmapMemory(addr, size);
    page_manager.OnGpuUnmap(addr, size);
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
    if (regs.depth_control.depth_bounds_enable) {
        cmdbuf.setDepthBounds(regs.depth_bounds_min, regs.depth_bounds_max);
    }
    if (regs.polygon_control.NeedsBias()) {
        if (regs.polygon_control.enable_polygon_offset_front) {
            cmdbuf.setDepthBias(regs.poly_offset.front_offset, regs.poly_offset.depth_bias,
                                regs.poly_offset.front_scale);
        } else {
            cmdbuf.setDepthBias(regs.poly_offset.back_offset, regs.poly_offset.depth_bias,
                                regs.poly_offset.back_scale);
        }
    }
    if (regs.depth_control.stencil_enable) {
        const auto front = regs.stencil_ref_front;
        const auto back = regs.stencil_ref_back;
        if (front.stencil_test_val == back.stencil_test_val) {
            cmdbuf.setStencilReference(vk::StencilFaceFlagBits::eFrontAndBack,
                                       front.stencil_test_val);
        } else {
            cmdbuf.setStencilReference(vk::StencilFaceFlagBits::eFront, front.stencil_test_val);
            cmdbuf.setStencilReference(vk::StencilFaceFlagBits::eBack, back.stencil_test_val);
        }
        if (front.stencil_write_mask == back.stencil_write_mask) {
            cmdbuf.setStencilWriteMask(vk::StencilFaceFlagBits::eFrontAndBack,
                                       front.stencil_write_mask);
        } else {
            cmdbuf.setStencilWriteMask(vk::StencilFaceFlagBits::eFront, front.stencil_write_mask);
            cmdbuf.setStencilWriteMask(vk::StencilFaceFlagBits::eBack, back.stencil_write_mask);
        }
        if (front.stencil_mask == back.stencil_mask) {
            cmdbuf.setStencilCompareMask(vk::StencilFaceFlagBits::eFrontAndBack,
                                         front.stencil_mask);
        } else {
            cmdbuf.setStencilCompareMask(vk::StencilFaceFlagBits::eFront, front.stencil_mask);
            cmdbuf.setStencilCompareMask(vk::StencilFaceFlagBits::eBack, back.stencil_mask);
        }
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

void Rasterizer::ScopeMarkerBegin(const std::string_view& str) {
    if (Config::nullGpu() || !Config::vkMarkersEnabled()) {
        return;
    }

    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT{
        .pLabelName = str.data(),
    });
}

void Rasterizer::ScopeMarkerEnd() {
    if (Config::nullGpu() || !Config::vkMarkersEnabled()) {
        return;
    }

    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.endDebugUtilsLabelEXT();
}

void Rasterizer::ScopedMarkerInsert(const std::string_view& str) {
    if (Config::nullGpu() || !Config::vkMarkersEnabled()) {
        return;
    }

    const auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.insertDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT{
        .pLabelName = str.data(),
    });
}

} // namespace Vulkan
