// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <bitset>

#include "common/types.h"
#include "shader_recompiler/runtime_info.h"

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

struct StageSpecialization {
    static constexpr size_t MaxStageResources = 32;

    const Shader::Info* info;
    std::bitset<MaxStageResources> bitset{};
    boost::container::small_vector<BufferSpecialization, 16> buffers;
    boost::container::small_vector<TextureBufferSpecialization, 8> tex_buffers;
    boost::container::small_vector<ImageSpecialization, 8> images;
    std::array<MrtSwizzle, MaxColorBuffers> mrt_swizzles;
    u32 start_binding{};

    explicit StageSpecialization(const Shader::Info& info_) : info{&info_} {}

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
        if (start_binding != other.start_binding) {
            return false;
        }
        if (info->stage == Shader::Stage::Fragment &&
            !std::ranges::equal(mrt_swizzles, other.mrt_swizzles)) {
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

} // namespace Shader
