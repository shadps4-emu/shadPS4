// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/runtime_info.h"
#include "video_core/renderer_vulkan/renderer_vulkan.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_pipeline_cache.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_cache.h"

extern std::unique_ptr<Vulkan::RendererVulkan> renderer;

namespace Vulkan {

PipelineCache::PipelineCache(const Instance& instance_, Scheduler& scheduler_,
                             AmdGpu::Liverpool* liverpool_)
    : instance{instance_}, scheduler{scheduler_}, liverpool{liverpool_},
      shader_cache{std::make_unique<ShaderCache>(instance, liverpool)} {
    pipeline_cache = instance.GetDevice().createPipelineCacheUnique({});
}

PipelineCache::~PipelineCache() = default;

const GraphicsPipeline* PipelineCache::GetGraphicsPipeline() {
    const auto& regs = liverpool->regs;
    // Tessellation is unsupported so skip the draw to avoid locking up the driver.
    if (regs.primitive_type == Liverpool::PrimitiveType::PatchPrimitive) {
        return nullptr;
    }
    // There are several cases (e.g. FCE, FMask/HTile decompression) where we don't need to do an
    // actual draw hence can skip pipeline creation.
    if (regs.color_control.mode == Liverpool::ColorControl::OperationMode::EliminateFastClear) {
        LOG_TRACE(Render_Vulkan, "FCE pass skipped");
        return nullptr;
    }
    if (regs.color_control.mode == Liverpool::ColorControl::OperationMode::FmaskDecompress) {
        // TODO: check for a valid MRT1 to promote the draw to the resolve pass.
        LOG_TRACE(Render_Vulkan, "FMask decompression pass skipped");
        return nullptr;
    }
    if (!RefreshGraphicsKey()) {
        return nullptr;
    }
    const auto [it, is_new] = graphics_pipelines.try_emplace(graphics_key);
    if (is_new) {
        it.value() = std::make_unique<GraphicsPipeline>(instance, scheduler, graphics_key,
                                                        *pipeline_cache, infos, modules);
    }
    const GraphicsPipeline* pipeline = it->second.get();
    return pipeline;
}

const ComputePipeline* PipelineCache::GetComputePipeline() {
    if (!RefreshComputeKey()) {
        return nullptr;
    }
    const auto [it, is_new] = compute_pipelines.try_emplace(compute_key);
    if (is_new) {
        it.value() = std::make_unique<ComputePipeline>(instance, scheduler, *pipeline_cache,
                                                       compute_key, *infos[0], modules[0]);
    }
    const ComputePipeline* pipeline = it->second.get();
    return pipeline;
}

bool ShouldSkipShader(u64 shader_hash, const char* shader_type) {
    static constexpr std::array<u64, 0> skip_hashes = {};
    if (std::ranges::contains(skip_hashes, shader_hash)) {
        LOG_WARNING(Render_Vulkan, "Skipped {} shader hash {:#x}.", shader_type, shader_hash);
        return true;
    }
    return false;
}

bool PipelineCache::RefreshGraphicsKey() {
    auto& regs = liverpool->regs;
    auto& key = graphics_key;

    key.depth = regs.depth_control;
    key.depth.depth_write_enable.Assign(regs.depth_control.depth_write_enable.Value() &&
                                        !regs.depth_render_control.depth_clear_enable);
    key.depth_bounds_min = regs.depth_bounds_min;
    key.depth_bounds_max = regs.depth_bounds_max;
    key.depth_bias_enable = regs.polygon_control.enable_polygon_offset_back ||
                            regs.polygon_control.enable_polygon_offset_front ||
                            regs.polygon_control.enable_polygon_offset_para;
    if (regs.polygon_control.enable_polygon_offset_front) {
        key.depth_bias_const_factor = regs.poly_offset.front_offset;
        key.depth_bias_slope_factor = regs.poly_offset.front_scale;
    } else {
        key.depth_bias_const_factor = regs.poly_offset.back_offset;
        key.depth_bias_slope_factor = regs.poly_offset.back_scale;
    }
    key.depth_bias_clamp = regs.poly_offset.depth_bias;
    key.stencil = regs.stencil_control;
    key.stencil_ref_front = regs.stencil_ref_front;
    key.stencil_ref_back = regs.stencil_ref_back;
    key.prim_type = regs.primitive_type;
    key.enable_primitive_restart = regs.enable_primitive_restart & 1;
    key.primitive_restart_index = regs.primitive_restart_index;
    key.polygon_mode = regs.polygon_control.PolyMode();
    key.cull_mode = regs.polygon_control.CullingMode();
    key.clip_space = regs.clipper_control.clip_space;
    key.front_face = regs.polygon_control.front_face;
    key.num_samples = regs.aa_config.NumSamples();

    const auto& db = regs.depth_buffer;
    const auto ds_format = LiverpoolToVK::DepthFormat(db.z_info.format, db.stencil_info.format);

    if (db.z_info.format != AmdGpu::Liverpool::DepthBuffer::ZFormat::Invalid) {
        key.depth_format = ds_format;
    } else {
        key.depth_format = vk::Format::eUndefined;
    }
    if (key.depth.depth_enable) {
        key.depth.depth_enable.Assign(key.depth_format != vk::Format::eUndefined);
    }

    if (db.stencil_info.format != AmdGpu::Liverpool::DepthBuffer::StencilFormat::Invalid) {
        key.stencil_format = key.depth_format;
    } else {
        key.stencil_format = vk::Format::eUndefined;
    }
    if (key.depth.stencil_enable) {
        key.depth.stencil_enable.Assign(key.stencil_format != vk::Format::eUndefined);
    }

    const auto skip_cb_binding =
        regs.color_control.mode == AmdGpu::Liverpool::ColorControl::OperationMode::Disable;

    // `RenderingInfo` is assumed to be initialized with a contiguous array of valid color
    // attachments. This might be not a case as HW color buffers can be bound in an arbitrary order.
    // We need to do some arrays compaction at this stage
    key.color_formats.fill(vk::Format::eUndefined);
    key.blend_controls.fill({});
    key.write_masks.fill({});
    int remapped_cb{};
    for (auto cb = 0u; cb < Liverpool::NumColorBuffers; ++cb) {
        auto const& col_buf = regs.color_buffers[cb];
        if (skip_cb_binding || !col_buf || !regs.color_target_mask.GetMask(cb)) {
            continue;
        }
        const auto base_format =
            LiverpoolToVK::SurfaceFormat(col_buf.info.format, col_buf.NumFormat());
        const auto is_vo_surface = renderer->IsVideoOutSurface(col_buf);
        key.color_formats[remapped_cb] = LiverpoolToVK::AdjustColorBufferFormat(
            base_format, col_buf.info.comp_swap.Value(), false /*is_vo_surface*/);
        key.blend_controls[remapped_cb] = regs.blend_control[cb];
        key.blend_controls[remapped_cb].enable.Assign(key.blend_controls[remapped_cb].enable &&
                                                      !col_buf.info.blend_bypass);
        key.write_masks[remapped_cb] = vk::ColorComponentFlags{regs.color_target_mask.GetMask(cb)};
        key.cb_shader_mask = regs.color_shader_mask;

        ++remapped_cb;
    }

    u32 binding{};
    for (u32 i = 0; i < MaxShaderStages; i++) {
        if (!regs.stage_enable.IsStageEnabled(i)) {
            key.stage_hashes[i] = 0;
            infos[i] = nullptr;
            continue;
        }
        auto* pgm = regs.ProgramForStage(i);
        if (!pgm || !pgm->Address<u32*>()) {
            key.stage_hashes[i] = 0;
            infos[i] = nullptr;
            continue;
        }
        const auto* bininfo = Liverpool::GetBinaryInfo(*pgm);
        if (!bininfo->Valid()) {
            key.stage_hashes[i] = 0;
            infos[i] = nullptr;
            continue;
        }
        if (ShouldSkipShader(bininfo->shader_hash, "graphics")) {
            return false;
        }
        const auto stage = Shader::Stage{i};
        const GuestProgram guest_pgm{pgm, stage};
        std::tie(infos[i], modules[i], key.stage_hashes[i]) =
            shader_cache->GetProgram(guest_pgm, binding);
    }
    return true;
}

bool PipelineCache::RefreshComputeKey() {
    u32 binding{};
    const auto* cs_pgm = &liverpool->regs.cs_program;
    const GuestProgram guest_pgm{cs_pgm, Shader::Stage::Compute};
    if (ShouldSkipShader(guest_pgm.hash, "compute")) {
        return false;
    }
    std::tie(infos[0], modules[0], compute_key) = shader_cache->GetProgram(guest_pgm, binding);
    return true;
}

} // namespace Vulkan
