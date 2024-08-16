// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <boost/container/static_vector.hpp>
#include "common/assert.h"
#include "common/types.h"
#include "shader_recompiler/ir/attribute.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/ir/type.h"
#include "video_core/amdgpu/resource.h"

namespace Shader {

static constexpr size_t NumUserDataRegs = 16;

enum class Stage : u32 {
    Fragment,
    Vertex,
    Geometry,
    Export,
    Hull,
    Local,
    Compute,
};
constexpr u32 MaxStageTypes = 6;

[[nodiscard]] constexpr Stage StageFromIndex(size_t index) noexcept {
    return static_cast<Stage>(static_cast<size_t>(Stage::Vertex) + index);
}

enum class TextureType : u32 {
    Color1D,
    ColorArray1D,
    Color2D,
    ColorArray2D,
    Color3D,
    ColorCube,
    Buffer,
};
constexpr u32 NUM_TEXTURE_TYPES = 7;

enum class VsOutput : u32 {
    None,
    PointSprite,
    EdgeFlag,
    KillFlag,
    GsCutFlag,
    GsMrtIndex,
    GsVpIndex,
    CullDist0,
    CullDist1,
    CullDist2,
    CullDist3,
    CullDist4,
    CullDist5,
    CullDist6,
    CullDist7,
    ClipDist0,
    ClipDist1,
    ClipDist2,
    ClipDist3,
    ClipDist4,
    ClipDist5,
    ClipDist6,
    ClipDist7,
};
using VsOutputMap = std::array<VsOutput, 4>;

struct Info;

struct BufferResource {
    u32 sgpr_base;
    u32 dword_offset;
    u32 length;
    IR::Type used_types;
    AmdGpu::Buffer inline_cbuf;
    AmdGpu::DataFormat dfmt;
    AmdGpu::NumberFormat nfmt;
    bool is_storage{};
    bool is_instance_data{};
    bool is_written{};

    constexpr AmdGpu::Buffer GetVsharp(const Info& info) const noexcept;
};
using BufferResourceList = boost::container::static_vector<BufferResource, 16>;

struct ImageResource {
    u32 sgpr_base;
    u32 dword_offset;
    AmdGpu::ImageType type;
    AmdGpu::NumberFormat nfmt;
    bool is_storage;
    bool is_depth;
};
using ImageResourceList = boost::container::static_vector<ImageResource, 16>;

struct SamplerResource {
    u32 sgpr_base;
    u32 dword_offset;
    AmdGpu::Sampler inline_sampler{};
    u32 associated_image : 4;
    u32 disable_aniso : 1;

    constexpr AmdGpu::Sampler GetSsharp(const Info& info) const noexcept;
};
using SamplerResourceList = boost::container::static_vector<SamplerResource, 16>;

struct PushData {
    static constexpr size_t BufOffsetIndex = 2;

    u32 step0;
    u32 step1;
    std::array<u8, 32> buf_offsets;

    void AddOffset(u32 binding, u32 offset) {
        ASSERT(offset < 256 && binding < buf_offsets.size());
        buf_offsets[binding] = offset;
    }
};

struct Info {
    struct VsInput {
        enum InstanceIdType : u8 {
            None = 0,
            OverStepRate0 = 1,
            OverStepRate1 = 2,
            Plain = 3,
        };

        AmdGpu::NumberFormat fmt;
        u16 binding;
        u16 num_components;
        u8 sgpr_base;
        u8 dword_offset;
        InstanceIdType instance_step_rate;
        s32 instance_data_buf;
    };
    boost::container::static_vector<VsInput, 32> vs_inputs{};

    struct PsInput {
        u32 param_index;
        bool is_default;
        bool is_flat;
        u32 default_value;
    };
    boost::container::static_vector<PsInput, 32> ps_inputs{};

    struct AttributeFlags {
        bool Get(IR::Attribute attrib, u32 comp = 0) const {
            return flags[Index(attrib)] & (1 << comp);
        }

        bool GetAny(IR::Attribute attrib) const {
            return flags[Index(attrib)];
        }

        void Set(IR::Attribute attrib, u32 comp = 0) {
            flags[Index(attrib)] |= (1 << comp);
        }

        u32 NumComponents(IR::Attribute attrib) const {
            return 4;
        }

        static size_t Index(IR::Attribute attrib) {
            return static_cast<size_t>(attrib);
        }

        std::array<u8, IR::NumAttributes> flags;
    };
    AttributeFlags loads{};
    AttributeFlags stores{};
    boost::container::static_vector<VsOutputMap, 3> vs_outputs;

    BufferResourceList buffers;
    ImageResourceList images;
    SamplerResourceList samplers;

    std::array<u32, 3> workgroup_size{};
    std::array<bool, 3> tgid_enable;

    u32 num_user_data;
    u32 num_input_vgprs;
    std::span<const u32> user_data;
    Stage stage;

    uintptr_t pgm_base{};
    u64 pgm_hash{};
    u32 shared_memory_size{};
    bool has_storage_images{};
    bool has_discard{};
    bool has_image_gather{};
    bool has_image_query{};
    bool uses_group_quad{};
    bool uses_shared{};
    bool uses_fp16{};
    bool uses_step_rates{};
    bool translation_failed{}; // indicates that shader has unsupported instructions

    template <typename T>
    T ReadUd(u32 ptr_index, u32 dword_offset) const noexcept {
        T data;
        const u32* base = user_data.data();
        if (ptr_index != IR::NumScalarRegs) {
            std::memcpy(&base, &user_data[ptr_index], sizeof(base));
        }
        std::memcpy(&data, base + dword_offset, sizeof(T));
        return data;
    }
};

constexpr AmdGpu::Buffer BufferResource::GetVsharp(const Info& info) const noexcept {
    return inline_cbuf ? inline_cbuf : info.ReadUd<AmdGpu::Buffer>(sgpr_base, dword_offset);
}

constexpr AmdGpu::Sampler SamplerResource::GetSsharp(const Info& info) const noexcept {
    return inline_sampler ? inline_sampler : info.ReadUd<AmdGpu::Sampler>(sgpr_base, dword_offset);
}

} // namespace Shader

template <>
struct fmt::formatter<Shader::Stage> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(const Shader::Stage stage, format_context& ctx) const {
        constexpr static std::array names = {"fs", "vs", "gs", "es", "hs", "ls", "cs"};
        return fmt::format_to(ctx.out(), "{}", names[static_cast<size_t>(stage)]);
    }
};
