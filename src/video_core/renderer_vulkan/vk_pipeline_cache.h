// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <tsl/robin_map.h>
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/object_pool.h"
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

    const GraphicsPipeline* GetPipeline();

private:
    void RefreshKey();

    std::unique_ptr<GraphicsPipeline> CreatePipeline();

private:
    const Instance& instance;
    Scheduler& scheduler;
    AmdGpu::Liverpool* liverpool;
    vk::UniquePipelineCache pipeline_cache;
    vk::UniquePipelineLayout pipeline_layout;
    tsl::robin_map<size_t, vk::UniqueShaderModule> module_map;
    std::array<vk::ShaderModule, MaxShaderStages> stages{};
    tsl::robin_map<PipelineKey, std::unique_ptr<GraphicsPipeline>> graphics_pipelines;
    PipelineKey graphics_key{};
    Shader::ObjectPool<Shader::IR::Inst> inst_pool;
    Shader::ObjectPool<Shader::IR::Block> block_pool;
};

} // namespace Vulkan
