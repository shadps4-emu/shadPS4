// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <xxhash.h>
#include "common/config.h"
#include "common/io_file.h"
#include "common/path_util.h"
#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/recompiler.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_pipeline_cache.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"

namespace Vulkan {

Shader::Info MakeShaderInfo(Shader::Stage stage, std::span<const u32, 16> user_data,
                            const AmdGpu::Liverpool::Regs& regs) {
    Shader::Info info{};
    info.user_data = user_data;
    info.stage = stage;
    switch (stage) {
    case Shader::Stage::Vertex: {
        info.num_user_data = regs.vs_program.settings.num_user_regs;
        break;
    }
    case Shader::Stage::Fragment: {
        info.num_user_data = regs.ps_program.settings.num_user_regs;
        for (u32 i = 0; i < regs.num_interp; i++) {
            info.ps_inputs.push_back({
                .param_index = regs.ps_inputs[i].input_offset.Value(),
                .is_default = bool(regs.ps_inputs[i].use_default),
                .is_flat = bool(regs.ps_inputs[i].flat_shade),
                .default_value = regs.ps_inputs[i].default_value,
            });
        }
        break;
    }
    case Shader::Stage::Compute: {
        const auto& cs_pgm = regs.cs_program;
        info.num_user_data = cs_pgm.settings.num_user_regs;
        info.workgroup_size = {cs_pgm.num_thread_x.full, cs_pgm.num_thread_y.full,
                               cs_pgm.num_thread_z.full};
        break;
    }
    default:
        break;
    }
    return info;
}

PipelineCache::PipelineCache(const Instance& instance_, Scheduler& scheduler_,
                             AmdGpu::Liverpool* liverpool_)
    : instance{instance_}, scheduler{scheduler_}, liverpool{liverpool_}, inst_pool{8192},
      block_pool{512} {
    pipeline_cache = instance.GetDevice().createPipelineCacheUnique({});
    profile = Shader::Profile{
        .supported_spirv = 0x00010600U,
    };
}

const GraphicsPipeline* PipelineCache::GetGraphicsPipeline() {
    RefreshGraphicsKey();
    const auto [it, is_new] = graphics_pipelines.try_emplace(graphics_key);
    if (is_new) {
        it.value() = CreateGraphicsPipeline();
    }
    const GraphicsPipeline* pipeline = it->second.get();
    return pipeline;
}

const ComputePipeline* PipelineCache::GetComputePipeline() {
    const auto& cs_pgm = liverpool->regs.cs_program;
    ASSERT(cs_pgm.Address() != nullptr);
    const auto code = cs_pgm.Code();
    compute_key = XXH3_64bits(code.data(), code.size_bytes());
    const auto [it, is_new] = compute_pipelines.try_emplace(compute_key);
    if (is_new) {
        it.value() = CreateComputePipeline();
    }
    const ComputePipeline* pipeline = it->second.get();
    return pipeline;
}

void PipelineCache::RefreshGraphicsKey() {
    auto& regs = liverpool->regs;
    auto& key = graphics_key;

    key.depth = regs.depth_control;
    key.stencil = regs.stencil_control;
    key.stencil_ref_front = regs.stencil_ref_front;
    key.stencil_ref_back = regs.stencil_ref_back;
    key.prim_type = regs.primitive_type;
    key.polygon_mode = regs.polygon_control.PolyMode();
    key.cull_mode = regs.polygon_control.CullingMode();
    key.front_face = regs.polygon_control.front_face;

    const auto& db = regs.depth_buffer;
    key.depth_format = key.depth.depth_enable
                           ? LiverpoolToVK::DepthFormat(db.z_info.format, db.stencil_info.format)
                           : vk::Format::eUndefined;
    // `RenderingInfo` is assumed to be initialized with a contiguous array of valid color
    // attachments. This might be not a case as HW color buffers can be bound in an arbitrary order.
    // We need to do some arrays compaction at this stage
    int remapped_cb{};
    for (auto cb = 0u; cb < Liverpool::NumColorBuffers; ++cb) {
        auto const& col_buf = regs.color_buffers[cb];
        if (!col_buf) {
            continue;
        }
        key.color_formats[remapped_cb] =
            LiverpoolToVK::SurfaceFormat(col_buf.info.format, col_buf.NumFormat());
        key.blend_controls[remapped_cb] = regs.blend_control[cb];
        key.write_masks[remapped_cb] = vk::ColorComponentFlags{regs.color_target_mask.GetMask(cb)};

        ++remapped_cb;
    }

    for (u32 i = 0; i < MaxShaderStages; i++) {
        auto* pgm = regs.ProgramForStage(i);
        if (!pgm || !pgm->Address<u32>()) {
            key.stage_hashes[i] = 0;
            continue;
        }
        const auto code = pgm->Code();
        key.stage_hashes[i] = XXH3_64bits(code.data(), code.size_bytes());
    }
}

std::unique_ptr<GraphicsPipeline> PipelineCache::CreateGraphicsPipeline() {
    const auto& regs = liverpool->regs;

    u32 binding{};
    std::array<Shader::IR::Program, MaxShaderStages> programs;
    std::array<const Shader::Info*, MaxShaderStages> infos{};

    for (u32 i = 0; i < MaxShaderStages; i++) {
        if (!graphics_key.stage_hashes[i]) {
            stages[i] = VK_NULL_HANDLE;
            continue;
        }
        auto* pgm = regs.ProgramForStage(i);
        const auto code = pgm->Code();

        const auto it = module_map.find(graphics_key.stage_hashes[i]);
        if (it != module_map.end()) {
            stages[i] = *it->second;
            continue;
        }

        // Dump shader code if requested.
        const auto stage = Shader::Stage{i};
        const u64 hash = graphics_key.stage_hashes[i];
        if (Config::dumpShaders()) {
            DumpShader(code, hash, stage, "bin");
        }

        block_pool.ReleaseContents();
        inst_pool.ReleaseContents();

        // Recompile shader to IR.
        const Shader::Info info = MakeShaderInfo(stage, pgm->user_data, regs);
        programs[i] = Shader::TranslateProgram(inst_pool, block_pool, code, std::move(info));

        // Compile IR to SPIR-V
        auto spv_code = Shader::Backend::SPIRV::EmitSPIRV(profile, programs[i], binding);
        stages[i] = CompileSPV(spv_code, instance.GetDevice());
        infos[i] = &programs[i].info;

        // Set module name to hash in renderdoc
        const auto name = fmt::format("{}_{:#x}", stage, hash);
        Vulkan::SetObjectName(instance.GetDevice(), stages[i], name);

        if (Config::dumpShaders()) {
            DumpShader(spv_code, hash, stage, "spv");
        }
    }

    return std::make_unique<GraphicsPipeline>(instance, scheduler, graphics_key, *pipeline_cache,
                                              infos, stages);
}

std::unique_ptr<ComputePipeline> PipelineCache::CreateComputePipeline() {
    const auto& cs_pgm = liverpool->regs.cs_program;
    const auto code = cs_pgm.Code();

    // Dump shader code if requested.
    if (Config::dumpShaders()) {
        DumpShader(code, compute_key, Shader::Stage::Compute, "bin");
    }

    block_pool.ReleaseContents();
    inst_pool.ReleaseContents();

    // Recompile shader to IR.
    const Shader::Info info =
        MakeShaderInfo(Shader::Stage::Compute, cs_pgm.user_data, liverpool->regs);
    auto program = Shader::TranslateProgram(inst_pool, block_pool, code, std::move(info));

    // Compile IR to SPIR-V
    u32 binding{};
    const auto spv_code = Shader::Backend::SPIRV::EmitSPIRV(profile, program, binding);
    const auto module = CompileSPV(spv_code, instance.GetDevice());

    if (Config::dumpShaders()) {
        DumpShader(spv_code, compute_key, Shader::Stage::Compute, "spv");
    }

    return std::make_unique<ComputePipeline>(instance, scheduler, *pipeline_cache, &program.info,
                                             module);
}

void PipelineCache::DumpShader(std::span<const u32> code, u64 hash, Shader::Stage stage,
                               std::string_view ext) {
    using namespace Common::FS;
    const auto dump_dir = GetUserPath(PathType::ShaderDir) / "dumps";
    if (!std::filesystem::exists(dump_dir)) {
        std::filesystem::create_directories(dump_dir);
    }
    const auto filename = fmt::format("{}_{:#X}.{}", stage, hash, ext);
    const auto file = IOFile{dump_dir / filename, FileAccessMode::Write};
    file.WriteSpan(code);
}

} // namespace Vulkan
