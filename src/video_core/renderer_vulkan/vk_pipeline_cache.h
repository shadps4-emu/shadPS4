// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <tsl/robin_map.h>
#include "common/object_pool.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/profile.h"
#include "shader_recompiler/specialization.h"
#include "video_core/renderer_vulkan/vk_compute_pipeline.h"
#include "video_core/renderer_vulkan/vk_graphics_pipeline.h"

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

    explicit Program(const Shader::Info& info_) : info{info_} {}
};

struct GuestProgram {
    Shader::Stage stage;
    std::span<const u32, AmdGpu::Liverpool::NumShaderUserData> user_data;
    std::span<const u32> code;
    u64 hash;

    explicit GuestProgram(const auto* pgm, Shader::Stage stage_)
        : stage{stage_}, user_data{pgm->user_data}, code{pgm->Code()} {
        const auto* bininfo = AmdGpu::Liverpool::GetBinaryInfo(*pgm);
        hash = bininfo->shader_hash;
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

    std::tuple<const Shader::Info*, vk::ShaderModule, u64> GetProgram(const GuestProgram& pgm,
                                                                      u32& binding);

private:
    bool RefreshGraphicsKey();
    bool RefreshComputeKey();

    void DumpShader(std::span<const u32> code, u64 hash, Shader::Stage stage, size_t perm_idx,
                    std::string_view ext);
    vk::ShaderModule CompileModule(Shader::Info& info, std::span<const u32> code, size_t perm_idx,
                                   u32& binding);

    Shader::Info BuildShaderInfo(const GuestProgram& pgm, const AmdGpu::Liverpool::Regs& regs);
    Shader::StageSpecialization BuildStageSpec(const Shader::Info& info, u32 binding);

private:
    const Instance& instance;
    Scheduler& scheduler;
    AmdGpu::Liverpool* liverpool;
    vk::UniquePipelineCache pipeline_cache;
    vk::UniquePipelineLayout pipeline_layout;
    Shader::Profile profile{};
    tsl::robin_map<size_t, Program*> program_cache;
    Common::ObjectPool<Shader::IR::Inst> inst_pool;
    Common::ObjectPool<Shader::IR::Block> block_pool;
    Common::ObjectPool<Program> program_pool;
    tsl::robin_map<size_t, std::unique_ptr<ComputePipeline>> compute_pipelines;
    tsl::robin_map<GraphicsPipelineKey, std::unique_ptr<GraphicsPipeline>> graphics_pipelines;
    std::array<const Shader::Info*, MaxShaderStages> infos{};
    std::array<vk::ShaderModule, MaxShaderStages> modules{};
    GraphicsPipelineKey graphics_key{};
    u64 compute_key{};
};

} // namespace Vulkan
