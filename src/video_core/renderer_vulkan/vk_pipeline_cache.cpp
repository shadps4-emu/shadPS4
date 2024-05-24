// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include "common/scope_exit.h"
#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/recompiler.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_pipeline_cache.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"

namespace Vulkan {

Shader::Info MakeShaderInfo(Shader::Stage stage, std::span<const u32, 16> user_data,
                            AmdGpu::Liverpool::Regs& regs) {
    Shader::Info info{user_data};
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
    const vk::PipelineLayoutCreateInfo layout_info = {
        .setLayoutCount = 0U,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };
    pipeline_layout = instance.GetDevice().createPipelineLayoutUnique(layout_info);
    pipeline_cache = instance.GetDevice().createPipelineCacheUnique({});
}

void PipelineCache::BindPipeline() {
    SCOPE_EXIT {
        const auto cmdbuf = scheduler.CommandBuffer();
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->Handle());
    };

    if (pipeline) {
        return;
    }

    const auto get_program = [&](const AmdGpu::Liverpool::ShaderProgram& pgm, Shader::Stage stage) {
        const u32* token = pgm.Address<u32>();

        // Retrieve shader header.
        Shader::BinaryInfo bininfo;
        std::memcpy(&bininfo, token + (token[1] + 1) * 2, sizeof(bininfo));

        // Lookup if the shader already exists.
        const auto it = module_map.find(bininfo.shader_hash);
        if (it != module_map.end()) {
            return *it->second;
        }

        // Compile and cache shader.
        const auto data = std::span{token, bininfo.length / sizeof(u32)};
        block_pool.ReleaseContents();
        inst_pool.ReleaseContents();
        const auto info = MakeShaderInfo(stage, pgm.user_data, liverpool->regs);
        auto program = Shader::TranslateProgram(inst_pool, block_pool, data, std::move(info));
        const auto code = Shader::Backend::SPIRV::EmitSPIRV(Shader::Profile{}, program);

        static int counter = 0;
        std::ofstream file(fmt::format("shader{}.spv", counter++), std::ios::out | std::ios::binary);
        file.write((const char*)code.data(), code.size() * sizeof(u32));
        file.close();

        return CompileSPV(code, instance.GetDevice());
    };

    // Retrieve shader stage modules.
    // TODO: Only do this when program address is changed.
    stages[0] = get_program(liverpool->regs.vs_program, Shader::Stage::Vertex);
    stages[4] = get_program(liverpool->regs.ps_program, Shader::Stage::Fragment);

    // Bind pipeline.
    // TODO: Read entire key based on reg state.
    graphics_key.prim_type = liverpool->regs.primitive_type;
    graphics_key.polygon_mode = liverpool->regs.polygon_control.PolyMode();
    pipeline = std::make_unique<GraphicsPipeline>(instance, graphics_key, *pipeline_cache,
                                                  *pipeline_layout, stages);
}

} // namespace Vulkan
