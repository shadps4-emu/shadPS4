// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <bitset>
#include <boost/container/small_vector.hpp>
#include <tsl/robin_map.h>
#include "common/object_pool.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/profile.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/renderer_vulkan/vk_common.h"

namespace Vulkan {

class Instance;

struct BufferSpecialization {
    u16 stride : 14;
    u16 is_storage : 1;

    auto operator<=>(const BufferSpecialization&) const = default;
};

struct TextureBufferSpecialization {
    bool is_integer;

    auto operator<=>(const TextureBufferSpecialization&) const = default;
};

struct ImageSpecialization {
    AmdGpu::ImageType type;
    bool is_integer;

    auto operator<=>(const ImageSpecialization&) const = default;
};

struct StageSpecialization {
    static constexpr size_t MaxStageResources = 32;

    const Shader::Info* info;
    std::bitset<MaxStageResources> bitset{};
    boost::container::small_vector<BufferSpecialization, 16> buffers;
    boost::container::small_vector<TextureBufferSpecialization, 8> tex_buffers;
    boost::container::small_vector<ImageSpecialization, 8> images;
    u32 start_binding{};

    void ForEachSharp(u32& binding, auto& spec_list, auto& desc_list, auto&& func) {
        for (const auto& desc : desc_list) {
            auto& spec = spec_list.emplace_back();
            const auto sharp = desc.GetSharp(*info);
            if (!sharp) {
                binding++;
                continue;
            }
            bitset.set(binding++);
            func(spec, desc, sharp);
        }
    }

    StageSpecialization(const Shader::Info& info_, u32 start_binding_)
        : info{&info_}, start_binding{start_binding_} {
        u32 binding{};
        ForEachSharp(binding, buffers, info->buffers,
                     [](auto& spec, const auto& desc, AmdGpu::Buffer sharp) {
                         spec.stride = sharp.GetStride();
                         spec.is_storage = desc.IsStorage(sharp);
                     });
        ForEachSharp(binding, tex_buffers, info->texture_buffers,
                     [](auto& spec, const auto& desc, AmdGpu::Buffer sharp) {
                         spec.is_integer = AmdGpu::IsInteger(sharp.GetNumberFmt());
                     });
        ForEachSharp(binding, images, info->images,
                     [](auto& spec, const auto& desc, AmdGpu::Image sharp) {
                         spec.type = sharp.GetType();
                         spec.is_integer = AmdGpu::IsInteger(sharp.GetNumberFmt());
                     });
    }

    bool operator==(const StageSpecialization& other) const {
        if (start_binding != other.start_binding) {
            return false;
        }
        u32 binding{};
        for (u32 i = 0; i < buffers.size(); i++) {
            if (other.bitset[binding++] && buffers[i] != other.buffers[i]) {
                return false;
            }
        }
        for (u32 i = 0; i < tex_buffers.size(); i++) {
            if (other.bitset[binding++] && tex_buffers[i] != other.tex_buffers[i]) {
                return false;
            }
        }
        for (u32 i = 0; i < images.size(); i++) {
            if (other.bitset[binding++] && images[i] != other.images[i]) {
                return false;
            }
        }
        return true;
    }
};

struct Program {
    struct Module {
        vk::ShaderModule module;
        StageSpecialization spec;
    };

    Shader::Info info;
    boost::container::small_vector<Module, 8> modules;
};

Shader::Info MakeShaderInfo(Shader::Stage stage, std::span<const u32, 16> user_data, u64 pgm_base,
                            u64 hash, const AmdGpu::Liverpool::Regs& regs);

[[nodiscard]] inline u64 HashCombine(const u64 seed, const u64 hash) {
    return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

class ShaderCache {
public:
    explicit ShaderCache(const Instance& instance, AmdGpu::Liverpool* liverpool);
    ~ShaderCache() = default;

    void DumpShader(std::span<const u32> code, u64 hash, Shader::Stage stage, size_t perm_idx,
                    std::string_view ext);

    vk::ShaderModule CompileModule(Shader::Info& info, std::span<const u32> code, size_t perm_idx,
                                   u32& binding);

    std::tuple<const Shader::Info*, vk::ShaderModule, u64> GetProgram(const auto* pgm,
                                                                      Shader::Stage stage,
                                                                      u32& binding) {
        // Fetch program for binaryinfo hash.
        const auto* bininfo = AmdGpu::Liverpool::GetBinaryInfo(*pgm);
        const u64 hash = bininfo->shader_hash;
        auto [it_pgm, new_program] = program_cache.try_emplace(hash);
        u64 stage_key{};
        if (new_program) {
            const VAddr pgm_base = pgm->template Address<VAddr>();
            auto program = program_pool.Create();
            program->info = MakeShaderInfo(stage, pgm->user_data, pgm_base, hash, liverpool->regs);
            u32 start_binding = binding;
            const auto module = CompileModule(program->info, pgm->Code(), 0, binding);
            program->modules.emplace_back(module,
                                          StageSpecialization{program->info, start_binding});
            it_pgm.value() = program;
            return std::make_tuple(&program->info, module, HashCombine(hash, 0));
        }

        Program* program = it_pgm->second;
        const auto& info = program->info;
        size_t perm_idx = program->modules.size();
        StageSpecialization spec{info, binding};
        vk::ShaderModule module{};

        const auto it = std::ranges::find(program->modules, spec, &Program::Module::spec);
        if (it == program->modules.end()) {
            auto new_info = MakeShaderInfo(stage, pgm->user_data, info.pgm_base, info.pgm_hash,
                                           liverpool->regs);
            module = CompileModule(new_info, pgm->Code(), perm_idx, binding);
            program->modules.emplace_back(module, std::move(spec));
        } else {
            binding += info.NumBindings();
            module = it->module;
            perm_idx = std::distance(program->modules.begin(), it);
        }
        return std::make_tuple(&info, module, HashCombine(hash, perm_idx));
    }

private:
    const Instance& instance;
    AmdGpu::Liverpool* liverpool;
    Shader::Profile profile{};
    tsl::robin_map<size_t, Program*> program_cache;
    Common::ObjectPool<Shader::IR::Inst> inst_pool;
    Common::ObjectPool<Shader::IR::Block> block_pool;
    Common::ObjectPool<Program> program_pool;
};

} // namespace Vulkan
