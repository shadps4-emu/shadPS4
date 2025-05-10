// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <sirit/sirit.h>

#include "shader_recompiler/backend/bindings.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/value.h"
#include "shader_recompiler/profile.h"

namespace Shader::Backend::SPIRV {

using Sirit::Id;

struct VectorIds {
    [[nodiscard]] Id& operator[](u32 index) {
        return ids[index - 1];
    }

    [[nodiscard]] const Id& operator[](u32 index) const {
        return ids[index - 1];
    }

    [[nodiscard]] Id& Get(u32 index) {
        return ids[index - 1];
    }

    [[nodiscard]] const Id& Get(u32 index) const {
        return ids[index - 1];
    }

    std::array<Id, 4> ids;
};

class EmitContext final : public Sirit::Module {
public:
    explicit EmitContext(const Profile& profile, const RuntimeInfo& runtime_info, Info& info,
                         Bindings& binding);
    ~EmitContext();

    Id Def(const IR::Value& value);

    void DefineBufferProperties();
    void DefineInterpolatedAttribs();
    void DefineWorkgroupIndex();

    [[nodiscard]] Id DefineInput(Id type, std::optional<u32> location = std::nullopt,
                                 std::optional<spv::BuiltIn> builtin = std::nullopt) {
        const Id input_id{DefineVariable(type, builtin, spv::StorageClass::Input)};
        if (location) {
            Decorate(input_id, spv::Decoration::Location, *location);
        }
        return input_id;
    }

    [[nodiscard]] Id DefineOutput(Id type, std::optional<u32> location = std::nullopt,
                                  std::optional<spv::BuiltIn> builtin = std::nullopt) {
        const Id output_id{DefineVariable(type, builtin, spv::StorageClass::Output)};
        if (location) {
            Decorate(output_id, spv::Decoration::Location, *location);
        }
        return output_id;
    }

    [[nodiscard]] Id DefineUniformConst(Id type, u32 set, u32 binding, bool readonly = false) {
        const Id uniform_id{DefineVar(type, spv::StorageClass::UniformConstant)};
        Decorate(uniform_id, spv::Decoration::DescriptorSet, set);
        Decorate(uniform_id, spv::Decoration::Binding, binding);
        if (readonly) {
            Decorate(uniform_id, spv::Decoration::NonWritable);
        }
        return uniform_id;
    }

    template <bool global = true>
    [[nodiscard]] Id DefineVar(Id type, spv::StorageClass storage_class,
                               std::optional<Id> initializer = std::nullopt) {
        const Id pointer_type_id{TypePointer(storage_class, type)};
        return global ? AddGlobalVariable(pointer_type_id, storage_class, initializer)
                      : AddLocalVariable(pointer_type_id, storage_class, initializer);
    }

    [[nodiscard]] Id DefineVariable(Id type, std::optional<spv::BuiltIn> builtin,
                                    spv::StorageClass storage_class,
                                    std::optional<Id> initializer = std::nullopt) {
        const Id id{DefineVar(type, storage_class, initializer)};
        if (builtin) {
            Decorate(id, spv::Decoration::BuiltIn, *builtin);
        }
        interfaces.push_back(id);
        return id;
    }

    [[nodiscard]] Id ConstU32(u32 value) {
        return Constant(U32[1], value);
    }

    template <typename... Args>
    [[nodiscard]] Id ConstU32(Args&&... values) {
        constexpr u32 size = static_cast<u32>(sizeof...(values));
        static_assert(size >= 2);
        const std::array constituents{Constant(U32[1], values)...};
        const Id type = size <= 4 ? U32[size] : TypeArray(U32[1], ConstU32(size));
        return ConstantComposite(type, constituents);
    }

    [[nodiscard]] Id ConstS32(s32 value) {
        return Constant(S32[1], value);
    }

    template <typename... Args>
    [[nodiscard]] Id ConstS32(Args&&... values) {
        constexpr u32 size = static_cast<u32>(sizeof...(values));
        static_assert(size >= 2);
        const std::array constituents{Constant(S32[1], values)...};
        const Id type = size <= 4 ? S32[size] : TypeArray(S32[1], ConstU32(size));
        return ConstantComposite(type, constituents);
    }

    [[nodiscard]] Id ConstF32(f32 value) {
        return Constant(F32[1], value);
    }

    template <typename... Args>
    [[nodiscard]] Id ConstF32(Args... values) {
        constexpr u32 size = static_cast<u32>(sizeof...(values));
        static_assert(size >= 2);
        const std::array constituents{Constant(F32[1], values)...};
        const Id type = size <= 4 ? F32[size] : TypeArray(F32[1], ConstU32(size));
        return ConstantComposite(type, constituents);
    }

    Info& info;
    const RuntimeInfo& runtime_info;
    const Profile& profile;
    Stage stage;
    LogicalStage l_stage{};

    Id void_id{};
    Id U8{};
    Id S8{};
    Id U16{};
    Id S16{};
    Id U64{};
    VectorIds F16{};
    VectorIds F32{};
    VectorIds F64{};
    VectorIds S32{};
    VectorIds U32{};
    VectorIds U1{};

    Id full_result_i32x2;
    Id full_result_u32x2;
    Id frexp_result_f32;
    Id frexp_result_f64;

    Id pi_x2;

    Id true_value{};
    Id false_value{};
    Id u32_one_value{};
    Id u32_zero_value{};
    Id f32_zero_value{};

    Id shared_u8{};
    Id shared_u16{};
    Id shared_u32{};
    Id shared_u32x2{};
    Id shared_u32x4{};

    Id input_u32{};
    Id input_f32{};
    Id input_s32{};
    Id output_u32{};
    Id output_f32{};
    Id output_s32{};

    Id gl_in{};

    boost::container::small_vector<Id, 16> interfaces;

    Id output_position{};
    Id primitive_id{};
    Id vertex_index{};
    Id instance_id{};
    Id push_data_block{};
    Id base_vertex{};
    Id frag_coord{};
    Id front_facing{};
    Id frag_depth{};
    Id clip_distances{};
    Id cull_distances{};

    Id patch_vertices{};
    Id output_tess_level_outer{};
    Id output_tess_level_inner{};
    Id tess_coord;
    std::array<Id, 30> patches{};

    Id workgroup_id{};
    Id num_workgroups_id{};
    Id workgroup_index_id{};
    Id local_invocation_id{};
    Id invocation_id{};
    Id subgroup_local_invocation_id{};
    Id image_u32{};
    Id image_f32{};

    Id shared_memory_u8{};
    Id shared_memory_u16{};
    Id shared_memory_u32{};
    Id shared_memory_u32x2{};
    Id shared_memory_u32x4{};

    Id shared_memory_u32_type{};

    Id interpolate_func{};
    Id gl_bary_coord_id{};

    struct TextureDefinition {
        const VectorIds* data_types;
        Id id;
        Id sampled_type;
        Id pointer_type;
        Id image_type;
        AmdGpu::ImageType view_type;
        bool is_integer = false;
        bool is_storage = false;
    };

    enum class BufferAlias : u32 {
        U8,
        U16,
        U32,
        F32,
        NumAlias,
    };

    struct BufferSpv {
        Id id;
        Id pointer_type;
    };

    struct BufferDefinition {
        u32 binding;
        BufferType buffer_type;
        Id offset;
        Id offset_dwords;
        Id size;
        Id size_shorts;
        Id size_dwords;
        std::array<BufferSpv, u32(BufferAlias::NumAlias)> aliases;

        const BufferSpv& operator[](BufferAlias alias) const {
            return aliases[u32(alias)];
        }

        BufferSpv& operator[](BufferAlias alias) {
            return aliases[u32(alias)];
        }
    };

    Bindings& binding;
    boost::container::small_vector<Id, 16> buf_type_ids;
    boost::container::small_vector<BufferDefinition, 16> buffers;
    boost::container::small_vector<TextureDefinition, 8> images;
    boost::container::small_vector<Id, 4> samplers;

    Id sampler_type{};
    Id sampler_pointer_type{};

    struct SpirvAttribute {
        Id id;
        Id pointer_type;
        Id component_type;
        u32 num_components;
        bool is_integer{};
        bool is_loaded{};
        s32 buffer_handle{-1};
    };
    Id input_attr_array;
    Id output_attr_array;
    std::array<SpirvAttribute, IR::NumParams> input_params{};
    std::array<SpirvAttribute, IR::NumParams> output_params{};
    std::array<SpirvAttribute, IR::NumRenderTargets> frag_outputs{};

    Id uf11_to_f32{};
    Id f32_to_uf11{};
    Id uf10_to_f32{};
    Id f32_to_uf10{};

private:
    void DefineArithmeticTypes();
    void DefineInterfaces();
    void DefineInputs();
    void DefineOutputs();
    void DefinePushDataBlock();
    void DefineBuffers();
    void DefineImagesAndSamplers();
    void DefineSharedMemory();
    void DefineFunctions();

    SpirvAttribute GetAttributeInfo(AmdGpu::NumberFormat fmt, Id id, u32 num_components,
                                    bool output);

    BufferSpv DefineBuffer(bool is_storage, bool is_written, u32 elem_shift, BufferType buffer_type,
                           Id data_type);

    Id DefineFloat32ToUfloatM5(u32 mantissa_bits, std::string_view name);
    Id DefineUfloatM5ToFloat32(u32 mantissa_bits, std::string_view name);

    Id GetBufferSize(u32 sharp_idx);
};

} // namespace Shader::Backend::SPIRV
