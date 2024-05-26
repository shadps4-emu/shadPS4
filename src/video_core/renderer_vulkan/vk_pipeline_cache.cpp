// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/recompiler.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/amdgpu/resource.h"
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
    case Shader::Stage::Fragment: {
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
}

const GraphicsPipeline* PipelineCache::GetPipeline() {
    RefreshKey();
    const auto [it, is_new] = graphics_pipelines.try_emplace(graphics_key);
    if (is_new) {
        it.value() = CreatePipeline();
    }
    const GraphicsPipeline* pipeline = it->second.get();
    return pipeline;
}

void PipelineCache::RefreshKey() {
    auto& regs = liverpool->regs;
    auto& key = graphics_key;

    key.depth = regs.depth_control;
    key.stencil = regs.stencil_control;
    key.stencil_ref_front = regs.stencil_ref_front;
    key.stencil_ref_back = regs.stencil_ref_back;
    key.prim_type = regs.primitive_type;
    key.polygon_mode = regs.polygon_control.PolyMode();

    const auto& db = regs.depth_buffer;
    key.depth_format = key.depth.depth_enable
                           ? LiverpoolToVK::DepthFormat(db.z_info.format, db.stencil_info.format)
                           : vk::Format::eUndefined;
    for (u32 i = 0; i < Liverpool::NumColorBuffers; i++) {
        const auto& cb = regs.color_buffers[i];
        key.color_formats[i] = cb.base_address
                                   ? LiverpoolToVK::SurfaceFormat(cb.info.format, cb.NumFormat())
                                   : vk::Format::eUndefined;
    }

    for (u32 i = 0; i < MaxShaderStages; i++) {
        auto* pgm = regs.ProgramForStage(i);
        if (!pgm || !pgm->Address<u32>()) {
            key.stage_hashes[i] = 0;
            continue;
        }
        const u32* code = pgm->Address<u32>();

        Shader::BinaryInfo bininfo;
        std::memcpy(&bininfo, code + (code[1] + 1) * 2, sizeof(bininfo));
        key.stage_hashes[i] = bininfo.shader_hash;
    }
}

std::unique_ptr<GraphicsPipeline> PipelineCache::CreatePipeline() {
    const auto& regs = liverpool->regs;

    std::array<Shader::IR::Program, MaxShaderStages> programs;
    std::array<const Shader::Info*, MaxShaderStages> infos{};

    for (u32 i = 0; i < MaxShaderStages; i++) {
        if (!graphics_key.stage_hashes[i]) {
            stages[i] = VK_NULL_HANDLE;
            continue;
        }
        auto* pgm = regs.ProgramForStage(i);
        const u32* code = pgm->Address<u32>();

        Shader::BinaryInfo bininfo;
        std::memcpy(&bininfo, code + (code[1] + 1) * 2, sizeof(bininfo));
        const u32 num_dwords = bininfo.length / sizeof(u32);

        const auto it = module_map.find(bininfo.shader_hash);
        if (it != module_map.end()) {
            stages[i] = *it->second;
            continue;
        }

        block_pool.ReleaseContents();
        inst_pool.ReleaseContents();

        // Recompile shader to IR.
        const auto stage = Shader::Stage{i};
        const Shader::Info info = MakeShaderInfo(stage, pgm->user_data, regs);
        programs[i] = Shader::TranslateProgram(inst_pool, block_pool, std::span{code, num_dwords},
                                               std::move(info));

        // Compile IR to SPIR-V
        const auto profile = Shader::Profile{.supported_spirv = 0x00010600U};
        const auto spv_code = Shader::Backend::SPIRV::EmitSPIRV(profile, programs[i]);
        std::ofstream file("shader0.spv", std::ios::out | std::ios::binary);
        file.write((const char*)spv_code.data(), spv_code.size() * 4);
        file.close();

        stages[i] = CompileSPV(spv_code, instance.GetDevice());
        infos[i] = &programs[i].info;
    }

    return std::make_unique<GraphicsPipeline>(instance, scheduler, graphics_key, *pipeline_cache,
                                              infos, stages);
}

} // namespace Vulkan
