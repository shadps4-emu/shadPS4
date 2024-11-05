// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <bitset>

#include "common/types.h"
#include "shader_recompiler/backend/bindings.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/passes/srt.h"

namespace Shader {

struct BufferSpecialization {
    u16 stride : 14;
    u16 is_storage : 1;

    auto operator<=>(const BufferSpecialization&) const = default;
};

struct TextureBufferSpecialization {
    bool is_integer = false;

    auto operator<=>(const TextureBufferSpecialization&) const = default;
};

struct ImageSpecialization {
    AmdGpu::ImageType type = AmdGpu::ImageType::Color2D;
    bool is_integer = false;

    auto operator<=>(const ImageSpecialization&) const = default;
};

struct FMaskSpecialization {
    u32 width;
    u32 height;

    auto operator<=>(const FMaskSpecialization&) const = default;
};

/**
 * Alongside runtime information, this structure also checks bound resources
 * for compatibility. Can be used as a key for storing shader permutations.
 * Is separate from runtime information, because resource layout can only be deduced
 * after the first compilation of a module.
 */
struct StageSpecialization {
    static constexpr size_t MaxStageResources = 64;

    const Shader::Info* info;
    RuntimeInfo runtime_info;
    std::bitset<MaxStageResources> bitset{};
    boost::container::small_vector<BufferSpecialization, 16> buffers;
    boost::container::small_vector<TextureBufferSpecialization, 8> tex_buffers;
    boost::container::small_vector<ImageSpecialization, 16> images;
    boost::container::small_vector<FMaskSpecialization, 8> fmasks;
    Backend::Bindings start{};

    explicit StageSpecialization(const Shader::Info& info_, RuntimeInfo runtime_info_,
                                 Backend::Bindings start_)
        : info{&info_}, runtime_info{runtime_info_}, start{start_} {
        u32 binding{};
        if (info->has_readconst) {
            binding++;
        }
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
                         spec.type = sharp.IsPartialCubemap() ? AmdGpu::ImageType::Color2DArray
                                                              : sharp.GetType();
                         spec.is_integer = AmdGpu::IsInteger(sharp.GetNumberFmt());
                     });
        ForEachSharp(binding, fmasks, info->fmasks,
                     [](auto& spec, const auto& desc, AmdGpu::Image sharp) {
                         spec.width = sharp.width;
                         spec.height = sharp.height;
                     });
    }

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

    bool operator==(const StageSpecialization& other) const {
        if (start != other.start) {
            return false;
        }
        if (runtime_info != other.runtime_info) {
            return false;
        }
        u32 binding{};
        if (info->has_readconst != other.info->has_readconst) {
            return false;
        }
        if (info->has_readconst) {
            binding++;
        }
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
        for (u32 i = 0; i < fmasks.size(); i++) {
            if (other.bitset[binding++] && fmasks[i] != other.fmasks[i]) {
                return false;
            }
        }
        return true;
    }
};

} // namespace Shader
