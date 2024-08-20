// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <tsl/robin_map.h>
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"
#include "video_core/renderer_vulkan/vk_compute_pipeline.h"
#include "video_core/renderer_vulkan/vk_graphics_pipeline.h"

namespace Shader {
struct Info;
}

namespace Vulkan {

class Instance;
class Scheduler;

class PipelineCache {
    static constexpr size_t MaxShaderStages = 5;

public:
    explicit PipelineCache(const Instance& instance, Scheduler& scheduler,
                           AmdGpu::Liverpool* liverpool);
    ~PipelineCache() = default;

    const GraphicsPipeline* GetGraphicsPipeline();

    const ComputePipeline* GetComputePipeline();

private:
    void RefreshGraphicsKey();
    void DumpShader(std::span<const u32> code, u64 hash, Shader::Stage stage, std::string_view ext);

    std::unique_ptr<GraphicsPipeline> CreateGraphicsPipeline();
    std::unique_ptr<ComputePipeline> CreateComputePipeline();

private:
    const Instance& instance;
    Scheduler& scheduler;
    AmdGpu::Liverpool* liverpool;
    vk::UniquePipelineCache pipeline_cache;
    vk::UniquePipelineLayout pipeline_layout;
    tsl::robin_map<size_t, std::unique_ptr<Program>> program_cache;
    tsl::robin_map<size_t, std::unique_ptr<ComputePipeline>> compute_pipelines;
    tsl::robin_map<GraphicsPipelineKey, std::unique_ptr<GraphicsPipeline>> graphics_pipelines;
    std::array<const Program*, MaxShaderStages> programs{};
    Shader::Profile profile{};
    GraphicsPipelineKey graphics_key{};
    u64 compute_key{};
    Common::ObjectPool<Shader::IR::Inst> inst_pool;
    Common::ObjectPool<Shader::IR::Block> block_pool;
};

} // namespace Vulkan
