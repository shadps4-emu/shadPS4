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

struct Program {
    using Module = std::pair<u64, vk::ShaderModule>;
    Shader::Info info;
    boost::container::small_vector<Module, 8> modules;
};

Shader::Info MakeShaderInfo(Shader::Stage stage, std::span<const u32, 16> user_data, u64 pgm_base,
                            u64 hash, const AmdGpu::Liverpool::Regs& regs);

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
    void RefreshComputeKey();
    void DumpShader(std::span<const u32> code, u64 hash, Shader::Stage stage, size_t perm_idx,
                    std::string_view ext);

    vk::ShaderModule CompileModule(Shader::Info& info, std::span<const u32> code, size_t perm_idx,
                                   u32& binding);

    std::tuple<const Shader::Info*, vk::ShaderModule, u64> GetProgram(const auto* pgm,
                                                                      Shader::Stage stage,
                                                                      u32& binding) {
        // Fetch program for binaryinfo hash.
        const auto* bininfo = Liverpool::GetBinaryInfo(*pgm);
        const u64 hash = bininfo->shader_hash;
        auto [it_pgm, new_program] = program_cache.try_emplace(hash);
        u64 stage_key{};
        if (new_program) {
            // Create a new program and a module with current runtime state.
            const VAddr pgm_base = pgm->template Address<VAddr>();
            auto program = program_pool.Create();
            program->info = MakeShaderInfo(stage, pgm->user_data, pgm_base, hash, liverpool->regs);
            u32 start_binding = binding;
            const auto module = CompileModule(program->info, pgm->Code(), 0, start_binding);
            stage_key = program->info.GetStageSpecializedKey(binding);
            program->modules.emplace_back(stage_key, module);
            it_pgm.value() = program;
        } else {
            stage_key = it_pgm->second->info.GetStageSpecializedKey(binding);
        }

        Program* program = it_pgm->second;
        const auto& info = program->info;
        vk::ShaderModule module{};

        // Compile specialized module with current runtime state.
        const auto it = std::ranges::find(program->modules, stage_key, &Program::Module::first);
        if (it == program->modules.end()) {
            auto new_info = MakeShaderInfo(stage, pgm->user_data, info.pgm_base, info.pgm_hash,
                                           liverpool->regs);
            const size_t perm_idx = program->modules.size();
            module = CompileModule(new_info, pgm->Code(), perm_idx, binding);
            program->modules.emplace_back(stage_key, module);
        } else {
            binding += info.NumBindings();
            module = it->second;
        }

        const u64 full_hash = HashCombine(hash, stage_key);
        return std::make_tuple(&info, module, full_hash);
    }

private:
    const Instance& instance;
    Scheduler& scheduler;
    AmdGpu::Liverpool* liverpool;
    vk::UniquePipelineCache pipeline_cache;
    vk::UniquePipelineLayout pipeline_layout;
    tsl::robin_map<size_t, Program*> program_cache;
    tsl::robin_map<size_t, std::unique_ptr<ComputePipeline>> compute_pipelines;
    tsl::robin_map<GraphicsPipelineKey, std::unique_ptr<GraphicsPipeline>> graphics_pipelines;
    std::array<const Shader::Info*, MaxShaderStages> infos{};
    std::array<vk::ShaderModule, MaxShaderStages> modules{};
    Shader::Profile profile{};
    GraphicsPipelineKey graphics_key{};
    u64 compute_key{};
    Common::ObjectPool<Shader::IR::Inst> inst_pool;
    Common::ObjectPool<Shader::IR::Block> block_pool;
    Common::ObjectPool<Program> program_pool;
};

} // namespace Vulkan
