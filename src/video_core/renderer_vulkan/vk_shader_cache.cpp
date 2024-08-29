// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/io_file.h"
#include "common/path_util.h"
#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/recompiler.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_platform.h"
#include "video_core/renderer_vulkan/vk_shader_cache.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"

namespace Vulkan {

using Shader::VsOutput;

void BuildVsOutputs(Shader::Info& info, const AmdGpu::Liverpool::VsOutputControl& ctl) {
    const auto add_output = [&](VsOutput x, VsOutput y, VsOutput z, VsOutput w) {
        if (x != VsOutput::None || y != VsOutput::None || z != VsOutput::None ||
            w != VsOutput::None) {
            info.vs_outputs.emplace_back(Shader::VsOutputMap{x, y, z, w});
        }
    };
    // VS_OUT_MISC_VEC
    add_output(ctl.use_vtx_point_size ? VsOutput::PointSprite : VsOutput::None,
               ctl.use_vtx_edge_flag
                   ? VsOutput::EdgeFlag
                   : (ctl.use_vtx_gs_cut_flag ? VsOutput::GsCutFlag : VsOutput::None),
               ctl.use_vtx_kill_flag
                   ? VsOutput::KillFlag
                   : (ctl.use_vtx_render_target_idx ? VsOutput::GsMrtIndex : VsOutput::None),
               ctl.use_vtx_viewport_idx ? VsOutput::GsVpIndex : VsOutput::None);
    // VS_OUT_CCDIST0
    add_output(ctl.IsClipDistEnabled(0)
                   ? VsOutput::ClipDist0
                   : (ctl.IsCullDistEnabled(0) ? VsOutput::CullDist0 : VsOutput::None),
               ctl.IsClipDistEnabled(1)
                   ? VsOutput::ClipDist1
                   : (ctl.IsCullDistEnabled(1) ? VsOutput::CullDist1 : VsOutput::None),
               ctl.IsClipDistEnabled(2)
                   ? VsOutput::ClipDist2
                   : (ctl.IsCullDistEnabled(2) ? VsOutput::CullDist2 : VsOutput::None),
               ctl.IsClipDistEnabled(3)
                   ? VsOutput::ClipDist3
                   : (ctl.IsCullDistEnabled(3) ? VsOutput::CullDist3 : VsOutput::None));
    // VS_OUT_CCDIST1
    add_output(ctl.IsClipDistEnabled(4)
                   ? VsOutput::ClipDist4
                   : (ctl.IsCullDistEnabled(4) ? VsOutput::CullDist4 : VsOutput::None),
               ctl.IsClipDistEnabled(5)
                   ? VsOutput::ClipDist5
                   : (ctl.IsCullDistEnabled(5) ? VsOutput::CullDist5 : VsOutput::None),
               ctl.IsClipDistEnabled(6)
                   ? VsOutput::ClipDist6
                   : (ctl.IsCullDistEnabled(6) ? VsOutput::CullDist6 : VsOutput::None),
               ctl.IsClipDistEnabled(7)
                   ? VsOutput::ClipDist7
                   : (ctl.IsCullDistEnabled(7) ? VsOutput::CullDist7 : VsOutput::None));
}

Shader::Info MakeShaderInfo(const GuestProgram& pgm, const AmdGpu::Liverpool::Regs& regs) {
    Shader::Info info{};
    info.user_data = pgm.user_data;
    info.pgm_base = VAddr(pgm.code.data());
    info.pgm_hash = pgm.hash;
    info.stage = pgm.stage;
    switch (pgm.stage) {
    case Shader::Stage::Vertex: {
        info.num_user_data = regs.vs_program.settings.num_user_regs;
        info.num_input_vgprs = regs.vs_program.settings.vgpr_comp_cnt;
        BuildVsOutputs(info, regs.vs_output_control);
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
        info.tgid_enable = {cs_pgm.IsTgidEnabled(0), cs_pgm.IsTgidEnabled(1),
                            cs_pgm.IsTgidEnabled(2)};
        info.shared_memory_size = cs_pgm.SharedMemSize();
        break;
    }
    default:
        break;
    }
    return info;
}

[[nodiscard]] inline u64 HashCombine(const u64 seed, const u64 hash) {
    return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

ShaderCache::ShaderCache(const Instance& instance_, AmdGpu::Liverpool* liverpool_)
    : instance{instance_}, liverpool{liverpool_}, inst_pool{8192}, block_pool{512} {
    profile = Shader::Profile{
        .supported_spirv = 0x00010600U,
        .subgroup_size = instance.SubgroupSize(),
        .support_explicit_workgroup_layout = true,
    };
}

vk::ShaderModule ShaderCache::CompileModule(Shader::Info& info, std::span<const u32> code,
                                            size_t perm_idx, u32& binding) {
    LOG_INFO(Render_Vulkan, "Compiling {} shader {:#x} {}", info.stage, info.pgm_hash,
             perm_idx != 0 ? "(permutation)" : "");

    if (Config::dumpShaders()) {
        DumpShader(code, info.pgm_hash, info.stage, perm_idx, "bin");
    }

    block_pool.ReleaseContents();
    inst_pool.ReleaseContents();
    const auto ir_program = Shader::TranslateProgram(inst_pool, block_pool, code, info, profile);

    // Compile IR to SPIR-V
    const auto spv = Shader::Backend::SPIRV::EmitSPIRV(profile, ir_program, binding);
    if (Config::dumpShaders()) {
        DumpShader(spv, info.pgm_hash, info.stage, perm_idx, "spv");
    }

    // Create module and set name to hash in renderdoc
    const auto module = CompileSPV(spv, instance.GetDevice());
    ASSERT(module != VK_NULL_HANDLE);
    const auto name = fmt::format("{}_{:#x}_{}", info.stage, info.pgm_hash, perm_idx);
    Vulkan::SetObjectName(instance.GetDevice(), module, name);
    return module;
}

Program* ShaderCache::CreateProgram(const GuestProgram& pgm, u32& binding) {
    Program* program = program_pool.Create(MakeShaderInfo(pgm, liverpool->regs));
    u32 start_binding = binding;
    const auto module = CompileModule(program->info, pgm.code, 0, binding);
    program->modules.emplace_back(module, StageSpecialization{program->info, start_binding});
    return program;
}

std::tuple<const Shader::Info*, vk::ShaderModule, u64> ShaderCache::GetProgram(
    const GuestProgram& pgm, u32& binding) {
    auto [it_pgm, new_program] = program_cache.try_emplace(pgm.hash);
    if (new_program) {
        auto program = CreateProgram(pgm, binding);
        const auto module = program->modules.back().module;
        it_pgm.value() = program;
        return std::make_tuple(&program->info, module, HashCombine(pgm.hash, 0));
    }

    Program* program = it_pgm->second;
    const auto& info = program->info;
    size_t perm_idx = program->modules.size();
    StageSpecialization spec{info, binding};
    vk::ShaderModule module{};

    const auto it = std::ranges::find(program->modules, spec, &Program::Module::spec);
    if (it == program->modules.end()) {
        auto new_info = MakeShaderInfo(pgm, liverpool->regs);
        module = CompileModule(new_info, pgm.code, perm_idx, binding);
        program->modules.emplace_back(module, std::move(spec));
    } else {
        binding += info.NumBindings();
        module = it->module;
        perm_idx = std::distance(program->modules.begin(), it);
    }
    return std::make_tuple(&info, module, HashCombine(pgm.hash, perm_idx));
}

void ShaderCache::DumpShader(std::span<const u32> code, u64 hash, Shader::Stage stage,
                             size_t perm_idx, std::string_view ext) {
    using namespace Common::FS;
    const auto dump_dir = GetUserPath(PathType::ShaderDir) / "dumps";
    if (!std::filesystem::exists(dump_dir)) {
        std::filesystem::create_directories(dump_dir);
    }
    const auto filename = fmt::format("{}_{:#018x}_{}.{}", stage, hash, perm_idx, ext);
    const auto file = IOFile{dump_dir / filename, FileAccessMode::Write};
    file.WriteSpan(code);
}

} // namespace Vulkan
