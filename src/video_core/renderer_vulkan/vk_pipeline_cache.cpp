// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/scope_exit.h"
#include "shader_recompiler/recompiler.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_pipeline_cache.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"

namespace Vulkan {

PipelineCache::PipelineCache(const Instance& instance_, Scheduler& scheduler_,
                             AmdGpu::Liverpool* liverpool_)
    : instance{instance_}, scheduler{scheduler_}, liverpool{liverpool_}, inst_pool{4096},
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
        const auto program = Shader::TranslateProgram(inst_pool, block_pool, stage, data);
        return CompileSPV(program, instance.GetDevice());
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
