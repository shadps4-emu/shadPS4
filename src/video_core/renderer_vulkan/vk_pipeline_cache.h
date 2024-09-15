// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <tsl/robin_map.h>
#include "shader_recompiler/profile.h"
#include "shader_recompiler/recompiler.h"
#include "shader_recompiler/specialization.h"
#include "video_core/renderer_vulkan/vk_compute_pipeline.h"
#include "video_core/renderer_vulkan/vk_graphics_pipeline.h"
#include "video_core/renderer_vulkan/vk_resource_pool.h"

namespace Shader {
struct Info;
}

namespace Vulkan {

class Instance;
class Scheduler;
class ShaderCache;

struct Program {
    struct Module {
        vk::ShaderModule module;
        Shader::StageSpecialization spec;
    };

    Shader::Info info;
    boost::container::small_vector<Module, 8> modules;

    explicit Program(Shader::Stage stage, Shader::ShaderParams params) : info{stage, params} {}

    void AddPermut(vk::ShaderModule module, const Shader::StageSpecialization&& spec) {
        modules.emplace_back(module, std::move(spec));
    }
};

class PipelineCache {
    static constexpr size_t MaxShaderStages = 5;

public:
    explicit PipelineCache(const Instance& instance, Scheduler& scheduler,
                           AmdGpu::Liverpool* liverpool);
    ~PipelineCache();

    const GraphicsPipeline* GetGraphicsPipeline();

    const ComputePipeline* GetComputePipeline();

    std::tuple<const Shader::Info*, vk::ShaderModule, u64> GetProgram(Shader::Stage stage,
                                                                      Shader::ShaderParams params,
                                                                      u32& binding);

private:
    bool RefreshGraphicsKey();
    bool RefreshComputeKey();

    void DumpShader(std::span<const u32> code, u64 hash, Shader::Stage stage, size_t perm_idx,
                    std::string_view ext);
    vk::ShaderModule CompileModule(Shader::Info& info, const Shader::RuntimeInfo& runtime_info,
                                   std::span<const u32> code, size_t perm_idx, u32& binding);
    Shader::RuntimeInfo BuildRuntimeInfo(Shader::Stage stage);

private:
    const Instance& instance;
    Scheduler& scheduler;
    AmdGpu::Liverpool* liverpool;
    DescriptorHeap desc_heap;
    vk::UniquePipelineCache pipeline_cache;
    vk::UniquePipelineLayout pipeline_layout;
    Shader::Profile profile{};
    Shader::Pools pools;
    tsl::robin_map<size_t, Program*> program_cache;
    Common::ObjectPool<Program> program_pool;
    Common::ObjectPool<GraphicsPipeline> graphics_pipeline_pool;
    Common::ObjectPool<ComputePipeline> compute_pipeline_pool;
    tsl::robin_map<size_t, ComputePipeline*> compute_pipelines;
    tsl::robin_map<GraphicsPipelineKey, GraphicsPipeline*> graphics_pipelines;
    std::array<const Shader::Info*, MaxShaderStages> infos{};
    std::array<vk::ShaderModule, MaxShaderStages> modules{};
    GraphicsPipelineKey graphics_key{};
    u64 compute_key{};
};

} // namespace Vulkan
