// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <ranges>

#include "common/config.h"
#include "common/hash.h"
#include "common/io_file.h"
#include "common/path_util.h"
#include "core/debug_state.h"
#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/recompiler.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_pipeline_cache.h"
#include "video_core/renderer_vulkan/vk_presenter.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"

extern std::unique_ptr<Vulkan::Presenter> presenter;

namespace Vulkan {

using Shader::LogicalStage;
using Shader::Output;
using Shader::Stage;

constexpr static auto SpirvVersion1_6 = 0x00010600U;

constexpr static std::array DescriptorHeapSizes = {
    vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 8192},
    vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 1024},
    vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, 8192},
    vk::DescriptorPoolSize{vk::DescriptorType::eSampler, 1024},
};

static u32 MapOutputs(std::span<Shader::OutputMap, 3> outputs,
                      const AmdGpu::Liverpool::VsOutputControl& ctl) {
    u32 num_outputs = 0;

    if (ctl.vs_out_misc_enable) {
        auto& misc_vec = outputs[num_outputs++];
        misc_vec[0] = ctl.use_vtx_point_size ? Output::PointSprite : Output::None;
        misc_vec[1] = ctl.use_vtx_edge_flag
                          ? Output::EdgeFlag
                          : (ctl.use_vtx_gs_cut_flag ? Output::GsCutFlag : Output::None);
        misc_vec[2] = ctl.use_vtx_kill_flag
                          ? Output::KillFlag
                          : (ctl.use_vtx_render_target_idx ? Output::GsMrtIndex : Output::None);
        misc_vec[3] = ctl.use_vtx_viewport_idx ? Output::GsVpIndex : Output::None;
    }

    if (ctl.vs_out_ccdist0_enable) {
        auto& ccdist0 = outputs[num_outputs++];
        ccdist0[0] = ctl.IsClipDistEnabled(0)
                         ? Output::ClipDist0
                         : (ctl.IsCullDistEnabled(0) ? Output::CullDist0 : Output::None);
        ccdist0[1] = ctl.IsClipDistEnabled(1)
                         ? Output::ClipDist1
                         : (ctl.IsCullDistEnabled(1) ? Output::CullDist1 : Output::None);
        ccdist0[2] = ctl.IsClipDistEnabled(2)
                         ? Output::ClipDist2
                         : (ctl.IsCullDistEnabled(2) ? Output::CullDist2 : Output::None);
        ccdist0[3] = ctl.IsClipDistEnabled(3)
                         ? Output::ClipDist3
                         : (ctl.IsCullDistEnabled(3) ? Output::CullDist3 : Output::None);
    }

    if (ctl.vs_out_ccdist1_enable) {
        auto& ccdist1 = outputs[num_outputs++];
        ccdist1[0] = ctl.IsClipDistEnabled(4)
                         ? Output::ClipDist4
                         : (ctl.IsCullDistEnabled(4) ? Output::CullDist4 : Output::None);
        ccdist1[1] = ctl.IsClipDistEnabled(5)
                         ? Output::ClipDist5
                         : (ctl.IsCullDistEnabled(5) ? Output::CullDist5 : Output::None);
        ccdist1[2] = ctl.IsClipDistEnabled(6)
                         ? Output::ClipDist6
                         : (ctl.IsCullDistEnabled(6) ? Output::CullDist6 : Output::None);
        ccdist1[3] = ctl.IsClipDistEnabled(7)
                         ? Output::ClipDist7
                         : (ctl.IsCullDistEnabled(7) ? Output::CullDist7 : Output::None);
    }

    return num_outputs;
}

const Shader::RuntimeInfo& PipelineCache::BuildRuntimeInfo(Stage stage, LogicalStage l_stage) {
    auto& info = runtime_infos[u32(l_stage)];
    const auto& regs = liverpool->regs;
    const auto BuildCommon = [&](const auto& program) {
        info.num_user_data = program.settings.num_user_regs;
        info.num_input_vgprs = program.settings.vgpr_comp_cnt;
        info.num_allocated_vgprs = program.NumVgprs();
        info.fp_denorm_mode32 = program.settings.fp_denorm_mode32;
        info.fp_round_mode32 = program.settings.fp_round_mode32;
    };
    info.Initialize(stage);
    switch (stage) {
    case Stage::Local: {
        BuildCommon(regs.ls_program);
        Shader::TessellationDataConstantBuffer tess_constants;
        const auto* hull_info = infos[u32(Shader::LogicalStage::TessellationControl)];
        hull_info->ReadTessConstantBuffer(tess_constants);
        info.ls_info.ls_stride = tess_constants.ls_stride;
        break;
    }
    case Stage::Hull: {
        BuildCommon(regs.hs_program);
        info.hs_info.num_input_control_points = regs.ls_hs_config.hs_input_control_points.Value();
        info.hs_info.num_threads = regs.ls_hs_config.hs_output_control_points.Value();
        info.hs_info.tess_type = regs.tess_config.type;

        // We need to initialize most hs_info fields after finding the V# with tess constants
        break;
    }
    case Stage::Export: {
        BuildCommon(regs.es_program);
        info.es_info.vertex_data_size = regs.vgt_esgs_ring_itemsize;
        break;
    }
    case Stage::Vertex: {
        BuildCommon(regs.vs_program);
        info.vs_info.step_rate_0 = regs.vgt_instance_step_rate_0;
        info.vs_info.step_rate_1 = regs.vgt_instance_step_rate_1;
        info.vs_info.num_outputs = MapOutputs(info.vs_info.outputs, regs.vs_output_control);
        info.vs_info.emulate_depth_negative_one_to_one =
            !instance.IsDepthClipControlSupported() &&
            regs.clipper_control.clip_space == Liverpool::ClipSpace::MinusWToW;
        info.vs_info.clip_disable = regs.IsClipDisabled();
        if (l_stage == LogicalStage::TessellationEval) {
            info.vs_info.tess_type = regs.tess_config.type;
            info.vs_info.tess_topology = regs.tess_config.topology;
            info.vs_info.tess_partitioning = regs.tess_config.partitioning;
        }
        break;
    }
    case Stage::Geometry: {
        BuildCommon(regs.gs_program);
        auto& gs_info = info.gs_info;
        gs_info.num_outputs = MapOutputs(gs_info.outputs, regs.vs_output_control);
        gs_info.output_vertices = regs.vgt_gs_max_vert_out;
        gs_info.num_invocations =
            regs.vgt_gs_instance_cnt.IsEnabled() ? regs.vgt_gs_instance_cnt.count : 1;
        gs_info.in_primitive = regs.primitive_type;
        for (u32 stream_id = 0; stream_id < Shader::GsMaxOutputStreams; ++stream_id) {
            gs_info.out_primitive[stream_id] =
                regs.vgt_gs_out_prim_type.GetPrimitiveType(stream_id);
        }
        gs_info.in_vertex_data_size = regs.vgt_esgs_ring_itemsize;
        gs_info.out_vertex_data_size = regs.vgt_gs_vert_itemsize[0];
        gs_info.mode = regs.vgt_gs_mode.mode;
        const auto params_vc = Liverpool::GetParams(regs.vs_program);
        gs_info.vs_copy = params_vc.code;
        gs_info.vs_copy_hash = params_vc.hash;
        DumpShader(gs_info.vs_copy, gs_info.vs_copy_hash, Shader::Stage::Vertex, 0, "copy.bin");
        break;
    }
    case Stage::Fragment: {
        BuildCommon(regs.ps_program);
        info.fs_info.en_flags = regs.ps_input_ena;
        info.fs_info.addr_flags = regs.ps_input_addr;
        const auto& ps_inputs = regs.ps_inputs;
        info.fs_info.num_inputs = regs.num_interp;
        const auto& cb0_blend = regs.blend_control[0];
        info.fs_info.dual_source_blending =
            LiverpoolToVK::IsDualSourceBlendFactor(cb0_blend.color_dst_factor) ||
            LiverpoolToVK::IsDualSourceBlendFactor(cb0_blend.color_src_factor);
        if (cb0_blend.separate_alpha_blend) {
            info.fs_info.dual_source_blending |=
                LiverpoolToVK::IsDualSourceBlendFactor(cb0_blend.alpha_dst_factor) ||
                LiverpoolToVK::IsDualSourceBlendFactor(cb0_blend.alpha_src_factor);
        }
        for (u32 i = 0; i < regs.num_interp; i++) {
            info.fs_info.inputs[i] = {
                .param_index = u8(ps_inputs[i].input_offset.Value()),
                .is_default = bool(ps_inputs[i].use_default),
                .is_flat = bool(ps_inputs[i].flat_shade),
                .default_value = u8(ps_inputs[i].default_value),
            };
        }
        for (u32 i = 0; i < Shader::MaxColorBuffers; i++) {
            info.fs_info.color_buffers[i] = graphics_key.color_buffers[i];
        }
        break;
    }
    case Stage::Compute: {
        const auto& cs_pgm = liverpool->GetCsRegs();
        info.num_user_data = cs_pgm.settings.num_user_regs;
        info.num_allocated_vgprs = cs_pgm.settings.num_vgprs * 4;
        info.cs_info.workgroup_size = {cs_pgm.num_thread_x.full, cs_pgm.num_thread_y.full,
                                       cs_pgm.num_thread_z.full};
        info.cs_info.tgid_enable = {cs_pgm.IsTgidEnabled(0), cs_pgm.IsTgidEnabled(1),
                                    cs_pgm.IsTgidEnabled(2)};
        info.cs_info.shared_memory_size = cs_pgm.SharedMemSize();
        break;
    }
    default:
        break;
    }
    return info;
}

PipelineCache::PipelineCache(const Instance& instance_, Scheduler& scheduler_,
                             AmdGpu::Liverpool* liverpool_)
    : instance{instance_}, scheduler{scheduler_}, liverpool{liverpool_},
      desc_heap{instance, scheduler.GetMasterSemaphore(), DescriptorHeapSizes} {
    const auto& vk12_props = instance.GetVk12Properties();
    profile = Shader::Profile{
        .supported_spirv = SpirvVersion1_6,
        .subgroup_size = instance.SubgroupSize(),
        .support_int8 = instance.IsShaderInt8Supported(),
        .support_int16 = instance.IsShaderInt16Supported(),
        .support_int64 = instance.IsShaderInt64Supported(),
        .support_float64 = instance.IsShaderFloat64Supported(),
        .support_fp32_denorm_preserve = bool(vk12_props.shaderDenormPreserveFloat32),
        .support_fp32_denorm_flush = bool(vk12_props.shaderDenormFlushToZeroFloat32),
        .support_fp32_round_to_zero = bool(vk12_props.shaderRoundingModeRTZFloat32),
        .support_legacy_vertex_attributes = instance_.IsLegacyVertexAttributesSupported(),
        .supports_image_load_store_lod = instance_.IsImageLoadStoreLodSupported(),
        .supports_native_cube_calc = instance_.IsAmdGcnShaderSupported(),
        .supports_trinary_minmax = instance_.IsAmdShaderTrinaryMinMaxSupported(),
        // TODO: Emitted bounds checks cause problems with phi control flow; needs to be fixed.
        .supports_robust_buffer_access = true, // instance_.IsRobustBufferAccess2Supported(),
        .supports_buffer_fp32_atomic_min_max =
            instance_.IsShaderAtomicFloatBuffer32MinMaxSupported(),
        .supports_image_fp32_atomic_min_max = instance_.IsShaderAtomicFloatImage32MinMaxSupported(),
        .supports_buffer_int64_atomics = instance_.IsBufferInt64AtomicsSupported(),
        .supports_shared_int64_atomics = instance_.IsSharedInt64AtomicsSupported(),
        .supports_workgroup_explicit_memory_layout =
            instance_.IsWorkgroupMemoryExplicitLayoutSupported(),
        .supports_amd_shader_explicit_vertex_parameter =
            instance_.IsAmdShaderExplicitVertexParameterSupported(),
        .supports_fragment_shader_barycentric = instance_.IsFragmentShaderBarycentricSupported(),
        .has_incomplete_fragment_shader_barycentric =
            instance_.IsFragmentShaderBarycentricSupported() &&
            instance.GetDriverID() == vk::DriverId::eMoltenvk,
        .needs_manual_interpolation = instance.IsFragmentShaderBarycentricSupported() &&
                                      instance.GetDriverID() == vk::DriverId::eNvidiaProprietary,
        .needs_lds_barriers = instance.GetDriverID() == vk::DriverId::eNvidiaProprietary ||
                              instance.GetDriverID() == vk::DriverId::eMoltenvk,
        .needs_buffer_offsets = instance.StorageMinAlignment() > 4,
        // When binding a UBO, we calculate its size considering the offset in the larger buffer
        // cache underlying resource. In some cases, it may produce sizes exceeding the system
        // maximum allowed UBO range, so we need to reduce the threshold to prevent issues.
        .max_ubo_size = instance.UniformMaxSize() - instance.UniformMinAlignment(),
        .max_viewport_width = instance.GetMaxViewportWidth(),
        .max_viewport_height = instance.GetMaxViewportHeight(),
        .max_shared_memory_size = instance.MaxComputeSharedMemorySize(),
    };
    auto [cache_result, cache] = instance.GetDevice().createPipelineCacheUnique({});
    ASSERT_MSG(cache_result == vk::Result::eSuccess, "Failed to create pipeline cache: {}",
               vk::to_string(cache_result));
    pipeline_cache = std::move(cache);
}

PipelineCache::~PipelineCache() = default;

const GraphicsPipeline* PipelineCache::GetGraphicsPipeline() {
    if (!RefreshGraphicsKey()) {
        return nullptr;
    }
    const auto [it, is_new] = graphics_pipelines.try_emplace(graphics_key);
    if (is_new) {
        it.value() = std::make_unique<GraphicsPipeline>(instance, scheduler, desc_heap, profile,
                                                        graphics_key, *pipeline_cache, infos,
                                                        runtime_infos, fetch_shader, modules);
        if (Config::collectShadersForDebug()) {
            for (auto stage = 0; stage < MaxShaderStages; ++stage) {
                if (infos[stage]) {
                    auto& m = modules[stage];
                    module_related_pipelines[m].emplace_back(graphics_key);
                }
            }
        }
    }
    return it->second.get();
}

const ComputePipeline* PipelineCache::GetComputePipeline() {
    if (!RefreshComputeKey()) {
        return nullptr;
    }
    const auto [it, is_new] = compute_pipelines.try_emplace(compute_key);
    if (is_new) {
        it.value() =
            std::make_unique<ComputePipeline>(instance, scheduler, desc_heap, profile,
                                              *pipeline_cache, compute_key, *infos[0], modules[0]);
        if (Config::collectShadersForDebug()) {
            auto& m = modules[0];
            module_related_pipelines[m].emplace_back(compute_key);
        }
    }
    return it->second.get();
}

bool PipelineCache::RefreshGraphicsKey() {
    std::memset(&graphics_key, 0, sizeof(GraphicsPipelineKey));

    auto& regs = liverpool->regs;
    auto& key = graphics_key;

    key.z_format = regs.depth_buffer.DepthValid() ? regs.depth_buffer.z_info.format.Value()
                                                  : Liverpool::DepthBuffer::ZFormat::Invalid;
    key.stencil_format = regs.depth_buffer.StencilValid()
                             ? regs.depth_buffer.stencil_info.format.Value()
                             : Liverpool::DepthBuffer::StencilFormat::Invalid;
    key.depth_clamp_enable = !regs.depth_render_override.disable_viewport_clamp;
    key.depth_clip_enable = regs.clipper_control.ZclipEnable();
    key.clip_space = regs.clipper_control.clip_space;
    key.provoking_vtx_last = regs.polygon_control.provoking_vtx_last;
    key.prim_type = regs.primitive_type;
    key.polygon_mode = regs.polygon_control.PolyMode();
    key.logic_op = regs.color_control.rop3;
    key.num_samples = regs.NumSamples();

    const bool skip_cb_binding =
        regs.color_control.mode == AmdGpu::Liverpool::ColorControl::OperationMode::Disable;

    // `RenderingInfo` is assumed to be initialized with a contiguous array of valid color
    // attachments. This might be not a case as HW color buffers can be bound in an arbitrary
    // order. We need to do some arrays compaction at this stage
    key.num_color_attachments = 0;
    key.color_buffers.fill({});
    key.blend_controls.fill({});
    key.write_masks.fill({});
    key.vertex_buffer_formats.fill(vk::Format::eUndefined);

    key.patch_control_points = 0;
    if (regs.stage_enable.hs_en.Value()) {
        key.patch_control_points = regs.ls_hs_config.hs_input_control_points.Value();
    }

    // First pass of bindings check to idenitfy formats and swizzles and pass them to rhe shader
    // recompiler.
    for (auto cb = 0u; cb < Liverpool::NumColorBuffers; ++cb) {
        auto const& col_buf = regs.color_buffers[cb];
        if (skip_cb_binding || !col_buf) {
            // No attachment bound and no incremented index.
            continue;
        }

        const auto remapped_cb = key.num_color_attachments++;
        if (!regs.color_target_mask.GetMask(cb)) {
            // Bound to null handle, skip over this attachment index.
            continue;
        }

        // Metal seems to have an issue where 8-bit unorm/snorm/sRGB outputs to render target
        // need a bias applied to round correctly; detect and set the flag for that here.
        const auto needs_unorm_fixup = instance.GetDriverID() == vk::DriverId::eMoltenvk &&
                                       (col_buf.GetNumberFmt() == AmdGpu::NumberFormat::Unorm ||
                                        col_buf.GetNumberFmt() == AmdGpu::NumberFormat::Snorm ||
                                        col_buf.GetNumberFmt() == AmdGpu::NumberFormat::Srgb) &&
                                       (col_buf.GetDataFmt() == AmdGpu::DataFormat::Format8 ||
                                        col_buf.GetDataFmt() == AmdGpu::DataFormat::Format8_8 ||
                                        col_buf.GetDataFmt() == AmdGpu::DataFormat::Format8_8_8_8);

        key.color_buffers[remapped_cb] = Shader::PsColorBuffer{
            .data_format = col_buf.GetDataFmt(),
            .num_format = col_buf.GetNumberFmt(),
            .num_conversion = col_buf.GetNumberConversion(),
            .export_format = regs.color_export_format.GetFormat(cb),
            .needs_unorm_fixup = needs_unorm_fixup,
            .swizzle = col_buf.Swizzle(),
        };
    }

    fetch_shader = std::nullopt;

    Shader::Backend::Bindings binding{};
    const auto& TryBindStage = [&](Shader::Stage stage_in, Shader::LogicalStage stage_out) -> bool {
        const auto stage_in_idx = static_cast<u32>(stage_in);
        const auto stage_out_idx = static_cast<u32>(stage_out);
        if (!regs.stage_enable.IsStageEnabled(stage_in_idx)) {
            key.stage_hashes[stage_out_idx] = 0;
            infos[stage_out_idx] = nullptr;
            return false;
        }

        const auto* pgm = regs.ProgramForStage(stage_in_idx);
        if (!pgm || !pgm->Address<u32*>()) {
            key.stage_hashes[stage_out_idx] = 0;
            infos[stage_out_idx] = nullptr;
            return false;
        }

        const auto& bininfo = Liverpool::GetBinaryInfo(*pgm);
        if (!bininfo.Valid()) {
            LOG_WARNING(Render_Vulkan, "Invalid binary info structure!");
            key.stage_hashes[stage_out_idx] = 0;
            infos[stage_out_idx] = nullptr;
            return false;
        }

        auto params = Liverpool::GetParams(*pgm);
        std::optional<Shader::Gcn::FetchShaderData> fetch_shader_;
        std::tie(infos[stage_out_idx], modules[stage_out_idx], fetch_shader_,
                 key.stage_hashes[stage_out_idx]) =
            GetProgram(stage_in, stage_out, params, binding);
        if (fetch_shader_) {
            fetch_shader = fetch_shader_;
        }
        return true;
    };

    const auto& IsGsFeaturesSupported = [&]() -> bool {
        // These checks are temporary until all functionality is implemented.
        return !regs.vgt_gs_mode.onchip && !regs.vgt_strmout_config.raw;
    };

    infos.fill(nullptr);
    TryBindStage(Stage::Fragment, LogicalStage::Fragment);

    const auto* fs_info = infos[static_cast<u32>(LogicalStage::Fragment)];
    key.mrt_mask = fs_info ? fs_info->mrt_mask : 0u;

    switch (regs.stage_enable.raw) {
    case Liverpool::ShaderStageEnable::VgtStages::EsGs: {
        if (!instance.IsGeometryStageSupported() || !IsGsFeaturesSupported()) {
            return false;
        }
        if (!TryBindStage(Stage::Export, LogicalStage::Vertex)) {
            return false;
        }
        if (!TryBindStage(Stage::Geometry, LogicalStage::Geometry)) {
            return false;
        }
        break;
    }
    case Liverpool::ShaderStageEnable::VgtStages::LsHs: {
        if (!instance.IsTessellationSupported() ||
            (regs.tess_config.type == AmdGpu::TessellationType::Isoline &&
             !instance.IsTessellationIsolinesSupported())) {
            return false;
        }
        if (!TryBindStage(Stage::Hull, LogicalStage::TessellationControl)) {
            return false;
        }
        if (!TryBindStage(Stage::Vertex, LogicalStage::TessellationEval)) {
            return false;
        }
        if (!TryBindStage(Stage::Local, LogicalStage::Vertex)) {
            return false;
        }
        break;
    }
    default: {
        TryBindStage(Stage::Vertex, LogicalStage::Vertex);
        break;
    }
    }

    const auto* vs_info = infos[static_cast<u32>(Shader::LogicalStage::Vertex)];
    if (vs_info && fetch_shader && !instance.IsVertexInputDynamicState()) {
        // Without vertex input dynamic state, the pipeline needs to specialize on format.
        // Stride will still be handled outside the pipeline using dynamic state.
        u32 vertex_binding = 0;
        for (const auto& attrib : fetch_shader->attributes) {
            const auto& buffer = attrib.GetSharp(*vs_info);
            ASSERT(vertex_binding < MaxVertexBufferCount);
            key.vertex_buffer_formats[vertex_binding++] =
                Vulkan::LiverpoolToVK::SurfaceFormat(buffer.GetDataFmt(), buffer.GetNumberFmt());
        }
    }

    // Second pass to fill remain CB pipeline key data
    for (auto cb = 0u, remapped_cb = 0u; cb < Liverpool::NumColorBuffers; ++cb) {
        auto const& col_buf = regs.color_buffers[cb];
        if (skip_cb_binding || !col_buf) {
            // No attachment bound and no incremented index.
            continue;
        }

        const u32 target_mask = regs.color_target_mask.GetMask(cb);
        if (!target_mask || (key.mrt_mask & (1u << cb)) == 0) {
            // Attachment is masked out by either color_target_mask or shader mrt_mask. In the case
            // of the latter we need to change format to undefined, and either way we need to
            // increment the index for the null attachment binding.
            key.color_buffers[remapped_cb++] = {};
            continue;
        }

        key.blend_controls[remapped_cb] = regs.blend_control[cb];
        key.blend_controls[remapped_cb].enable.Assign(key.blend_controls[remapped_cb].enable &&
                                                      !col_buf.info.blend_bypass);
        // Apply swizzle to target mask
        for (u32 i = 0; i < 4; i++) {
            if (target_mask & (1 << i)) {
                const auto swizzled_comp =
                    static_cast<u32>(key.color_buffers[remapped_cb].swizzle.array[i]);
                constexpr u32 min_comp = static_cast<u32>(AmdGpu::CompSwizzle::Red);
                const u32 comp = swizzled_comp >= min_comp ? swizzled_comp - min_comp : i;
                key.write_masks[remapped_cb] |= vk::ColorComponentFlagBits{1u << comp};
            }
        }
        key.cb_shader_mask.SetMask(remapped_cb, regs.color_shader_mask.GetMask(cb));
        ++remapped_cb;
    }

    return true;
}

bool PipelineCache::RefreshComputeKey() {
    Shader::Backend::Bindings binding{};
    const auto& cs_pgm = liverpool->GetCsRegs();
    const auto cs_params = Liverpool::GetParams(cs_pgm);
    std::tie(infos[0], modules[0], fetch_shader, compute_key.value) =
        GetProgram(Shader::Stage::Compute, LogicalStage::Compute, cs_params, binding);
    return true;
}

vk::ShaderModule PipelineCache::CompileModule(Shader::Info& info, Shader::RuntimeInfo& runtime_info,
                                              std::span<const u32> code, size_t perm_idx,
                                              Shader::Backend::Bindings& binding) {
    LOG_INFO(Render_Vulkan, "Compiling {} shader {:#x} {}", info.stage, info.pgm_hash,
             perm_idx != 0 ? "(permutation)" : "");
    DumpShader(code, info.pgm_hash, info.stage, perm_idx, "bin");

    const auto ir_program = Shader::TranslateProgram(code, pools, info, runtime_info, profile);
    auto spv = Shader::Backend::SPIRV::EmitSPIRV(profile, runtime_info, ir_program, binding);
    DumpShader(spv, info.pgm_hash, info.stage, perm_idx, "spv");

    vk::ShaderModule module;

    auto patch = GetShaderPatch(info.pgm_hash, info.stage, perm_idx, "spv");
    const bool is_patched = patch && Config::patchShaders();
    if (is_patched) {
        LOG_INFO(Loader, "Loaded patch for {} shader {:#x}", info.stage, info.pgm_hash);
        module = CompileSPV(*patch, instance.GetDevice());
    } else {
        module = CompileSPV(spv, instance.GetDevice());
    }

    const auto name = GetShaderName(info.stage, info.pgm_hash, perm_idx);
    Vulkan::SetObjectName(instance.GetDevice(), module, name);
    if (Config::collectShadersForDebug()) {
        DebugState.CollectShader(name, info.l_stage, module, spv, code,
                                 patch ? *patch : std::span<const u32>{}, is_patched);
    }
    return module;
}

PipelineCache::Result PipelineCache::GetProgram(Stage stage, LogicalStage l_stage,
                                                Shader::ShaderParams params,
                                                Shader::Backend::Bindings& binding) {
    auto runtime_info = BuildRuntimeInfo(stage, l_stage);
    auto [it_pgm, new_program] = program_cache.try_emplace(params.hash);
    if (new_program) {
        it_pgm.value() = std::make_unique<Program>(stage, l_stage, params);
        auto& program = it_pgm.value();
        auto start = binding;
        const auto module = CompileModule(program->info, runtime_info, params.code, 0, binding);
        const auto spec = Shader::StageSpecialization(program->info, runtime_info, profile, start);
        program->AddPermut(module, std::move(spec));
        return std::make_tuple(&program->info, module, spec.fetch_shader_data,
                               HashCombine(params.hash, 0));
    }
    it_pgm.value()->info.user_data = params.user_data;

    auto& program = it_pgm.value();
    auto& info = program->info;
    info.RefreshFlatBuf();
    const auto spec = Shader::StageSpecialization(info, runtime_info, profile, binding);
    size_t perm_idx = program->modules.size();
    vk::ShaderModule module{};

    const auto it = std::ranges::find(program->modules, spec, &Program::Module::spec);
    if (it == program->modules.end()) {
        auto new_info = Shader::Info(stage, l_stage, params);
        module = CompileModule(new_info, runtime_info, params.code, perm_idx, binding);
        program->AddPermut(module, std::move(spec));
    } else {
        info.AddBindings(binding);
        module = it->module;
        perm_idx = std::distance(program->modules.begin(), it);
    }
    return std::make_tuple(&info, module, spec.fetch_shader_data,
                           HashCombine(params.hash, perm_idx));
}

std::optional<vk::ShaderModule> PipelineCache::ReplaceShader(vk::ShaderModule module,
                                                             std::span<const u32> spv_code) {
    std::optional<vk::ShaderModule> new_module{};
    for (const auto& [_, program] : program_cache) {
        for (auto& m : program->modules) {
            if (m.module == module) {
                const auto& d = instance.GetDevice();
                d.destroyShaderModule(m.module);
                m.module = CompileSPV(spv_code, d);
                new_module = m.module;
            }
        }
    }
    if (module_related_pipelines.contains(module)) {
        auto& pipeline_keys = module_related_pipelines[module];
        for (auto& key : pipeline_keys) {
            if (std::holds_alternative<GraphicsPipelineKey>(key)) {
                auto& graphics_key = std::get<GraphicsPipelineKey>(key);
                graphics_pipelines.erase(graphics_key);
            } else if (std::holds_alternative<ComputePipelineKey>(key)) {
                auto& compute_key = std::get<ComputePipelineKey>(key);
                compute_pipelines.erase(compute_key);
            }
        }
    }
    return new_module;
}

std::string PipelineCache::GetShaderName(Shader::Stage stage, u64 hash,
                                         std::optional<size_t> perm) {
    if (perm) {
        return fmt::format("{}_{:#018x}_{}", stage, hash, *perm);
    }
    return fmt::format("{}_{:#018x}", stage, hash);
}

void PipelineCache::DumpShader(std::span<const u32> code, u64 hash, Shader::Stage stage,
                               size_t perm_idx, std::string_view ext) {
    if (!Config::dumpShaders()) {
        return;
    }

    using namespace Common::FS;
    const auto dump_dir = GetUserPath(PathType::ShaderDir) / "dumps";
    if (!std::filesystem::exists(dump_dir)) {
        std::filesystem::create_directories(dump_dir);
    }
    const auto filename = fmt::format("{}.{}", GetShaderName(stage, hash, perm_idx), ext);
    const auto file = IOFile{dump_dir / filename, FileAccessMode::Write};
    file.WriteSpan(code);
}

std::optional<std::vector<u32>> PipelineCache::GetShaderPatch(u64 hash, Shader::Stage stage,
                                                              size_t perm_idx,
                                                              std::string_view ext) {

    using namespace Common::FS;
    const auto patch_dir = GetUserPath(PathType::ShaderDir) / "patch";
    if (!std::filesystem::exists(patch_dir)) {
        std::filesystem::create_directories(patch_dir);
    }
    const auto filename = fmt::format("{}.{}", GetShaderName(stage, hash, perm_idx), ext);
    const auto filepath = patch_dir / filename;
    if (!std::filesystem::exists(filepath)) {
        return {};
    }
    const auto file = IOFile{patch_dir / filename, FileAccessMode::Read};
    std::vector<u32> code(file.GetSize() / sizeof(u32));
    file.Read(code);
    return code;
}

} // namespace Vulkan
