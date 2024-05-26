// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/container/static_vector.hpp>
#include <fmt/format.h>
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {
namespace {

std::string_view StageName(Stage stage) {
    switch (stage) {
    case Stage::Vertex:
        return "vs";
    case Stage::TessellationControl:
        return "tcs";
    case Stage::TessellationEval:
        return "tes";
    case Stage::Geometry:
        return "gs";
    case Stage::Fragment:
        return "fs";
    case Stage::Compute:
        return "cs";
    }
    throw InvalidArgument("Invalid stage {}", u32(stage));
}

template <typename... Args>
void Name(EmitContext& ctx, Id object, std::string_view format_str, Args&&... args) {
    ctx.Name(object, fmt::format(fmt::runtime(format_str), StageName(ctx.stage),
                                 std::forward<Args>(args)...)
                         .c_str());
}

} // Anonymous namespace

EmitContext::EmitContext(const Profile& profile_, IR::Program& program, Bindings& bindings)
    : Sirit::Module(profile_.supported_spirv), info{program.info}, profile{profile_},
      stage{program.info.stage} {
    u32& uniform_binding{bindings.unified};
    u32& storage_binding{bindings.unified};
    u32& texture_binding{bindings.unified};
    u32& image_binding{bindings.unified};
    AddCapability(spv::Capability::Shader);
    DefineArithmeticTypes();
    DefineInterfaces(program);
    DefineBuffers(program.info);
}

EmitContext::~EmitContext() = default;

Id EmitContext::Def(const IR::Value& value) {
    if (!value.IsImmediate()) {
        return value.InstRecursive()->Definition<Id>();
    }
    switch (value.Type()) {
    case IR::Type::Void:
        return Id{};
    case IR::Type::U1:
        return value.U1() ? true_value : false_value;
    case IR::Type::U32:
        return ConstU32(value.U32());
    case IR::Type::U64:
        return Constant(U64, value.U64());
    case IR::Type::F32:
        return ConstF32(value.F32());
    case IR::Type::F64:
        return Constant(F64[1], value.F64());
    default:
        throw NotImplementedException("Immediate type {}", value.Type());
    }
}

void EmitContext::DefineArithmeticTypes() {
    void_id = Name(TypeVoid(), "void_id");
    U1[1] = Name(TypeBool(), "bool_id");
    // F16[1] = Name(TypeFloat(16), "f16_id");
    F32[1] = Name(TypeFloat(32), "f32_id");
    // F64[1] = Name(TypeFloat(64), "f64_id");
    S32[1] = Name(TypeSInt(32), "i32_id");
    U32[1] = Name(TypeUInt(32), "u32_id");
    // U8 = Name(TypeSInt(8), "u8");
    // S8 = Name(TypeUInt(8), "s8");
    // U16 = Name(TypeUInt(16), "u16_id");
    // S16 = Name(TypeSInt(16), "s16_id");
    // U64 = Name(TypeUInt(64), "u64_id");

    for (u32 i = 2; i <= 4; i++) {
        // F16[i] = Name(TypeVector(F16[1], i), fmt::format("f16vec{}_id", i));
        F32[i] = Name(TypeVector(F32[1], i), fmt::format("f32vec{}_id", i));
        // F64[i] = Name(TypeVector(F64[1], i), fmt::format("f64vec{}_id", i));
        S32[i] = Name(TypeVector(S32[1], i), fmt::format("i32vec{}_id", i));
        U32[i] = Name(TypeVector(U32[1], i), fmt::format("u32vec{}_id", i));
        U1[i] = Name(TypeVector(U1[1], i), fmt::format("bvec{}_id", i));
    }

    true_value = ConstantTrue(U1[1]);
    false_value = ConstantFalse(U1[1]);
    u32_zero_value = ConstU32(0U);
    f32_zero_value = ConstF32(0.0f);

    input_f32 = Name(TypePointer(spv::StorageClass::Input, F32[1]), "input_f32");
    input_u32 = Name(TypePointer(spv::StorageClass::Input, U32[1]), "input_u32");
    input_s32 = Name(TypePointer(spv::StorageClass::Input, S32[1]), "input_s32");

    output_f32 = Name(TypePointer(spv::StorageClass::Output, F32[1]), "output_f32");
    output_u32 = Name(TypePointer(spv::StorageClass::Output, U32[1]), "output_u32");
}

void EmitContext::DefineInterfaces(const IR::Program& program) {
    DefineInputs(program.info);
    DefineOutputs(program.info);
}

Id GetAttributeType(EmitContext& ctx, AmdGpu::NumberFormat fmt) {
    switch (fmt) {
    case AmdGpu::NumberFormat::Float:
    case AmdGpu::NumberFormat::Unorm:
        return ctx.F32[4];
    case AmdGpu::NumberFormat::Sint:
        return ctx.S32[4];
    case AmdGpu::NumberFormat::Uint:
        return ctx.U32[4];
    case AmdGpu::NumberFormat::Sscaled:
        return ctx.F32[4];
    case AmdGpu::NumberFormat::Uscaled:
        return ctx.F32[4];
    default:
        break;
    }
    throw InvalidArgument("Invalid attribute type {}", fmt);
}

EmitContext::SpirvAttribute EmitContext::GetAttributeInfo(AmdGpu::NumberFormat fmt, Id id) {
    switch (fmt) {
    case AmdGpu::NumberFormat::Float:
    case AmdGpu::NumberFormat::Unorm:
        return {id, input_f32, F32[1], 4};
    case AmdGpu::NumberFormat::Uint:
        return {id, input_u32, U32[1], 4};
    case AmdGpu::NumberFormat::Sint:
        return {id, input_s32, S32[1], 4};
    case AmdGpu::NumberFormat::Sscaled:
        return {id, input_f32, F32[1], 4};
    case AmdGpu::NumberFormat::Uscaled:
        return {id, input_f32, F32[1], 4};
    default:
        break;
    }
    throw InvalidArgument("Invalid attribute type {}", fmt);
}

Id MakeDefaultValue(EmitContext& ctx, u32 default_value) {
    switch (default_value) {
    case 0:
        return ctx.ConstF32(0.f, 0.f, 0.f, 0.f);
    case 1:
        return ctx.ConstF32(0.f, 0.f, 0.f, 1.f);
    case 2:
        return ctx.ConstF32(1.f, 1.f, 1.f, 0.f);
    case 3:
        return ctx.ConstF32(1.f, 1.f, 1.f, 1.f);
    default:
        UNREACHABLE();
    }
}

void EmitContext::DefineInputs(const Info& info) {
    switch (stage) {
    case Stage::Vertex:
        vertex_index = DefineVariable(U32[1], spv::BuiltIn::VertexIndex, spv::StorageClass::Input);
        base_vertex = DefineVariable(U32[1], spv::BuiltIn::BaseVertex, spv::StorageClass::Input);
        for (const auto& input : info.vs_inputs) {
            const Id type{GetAttributeType(*this, input.fmt)};
            const Id id{DefineInput(type, input.binding)};
            Name(id, fmt::format("vs_in_attr{}", input.binding));
            input_params[input.binding] = GetAttributeInfo(input.fmt, id);
            interfaces.push_back(id);
        }
        break;
    case Stage::Fragment:
        for (const auto& input : info.ps_inputs) {
            if (input.is_default) {
                input_params[input.semantic] = {MakeDefaultValue(*this, input.default_value),
                                                input_f32, F32[1]};
                continue;
            }
            const IR::Attribute param{IR::Attribute::Param0 + input.param_index};
            const u32 num_components = info.loads.NumComponents(param);
            const Id type{F32[num_components]};
            const Id id{DefineInput(type, input.semantic)};
            if (input.is_flat) {
                Decorate(id, spv::Decoration::Flat);
            }
            Name(id, fmt::format("fs_in_attr{}", input.semantic));
            input_params[input.semantic] = {id, input_f32, F32[1], num_components};
            interfaces.push_back(id);
        }
    default:
        break;
    }
}

void EmitContext::DefineOutputs(const Info& info) {
    switch (stage) {
    case Stage::Vertex:
        output_position = DefineVariable(F32[4], spv::BuiltIn::Position, spv::StorageClass::Output);
        for (u32 i = 0; i < IR::NumParams; i++) {
            const IR::Attribute param{IR::Attribute::Param0 + i};
            if (!info.stores.GetAny(param)) {
                continue;
            }
            const u32 num_components = info.stores.NumComponents(param);
            const Id id{DefineOutput(F32[num_components], i)};
            Name(id, fmt::format("out_attr{}", i));
            output_params[i] = {id, output_f32, F32[1], num_components};
            interfaces.push_back(id);
        }
        break;
    case Stage::Fragment:
        for (u32 i = 0; i < IR::NumRenderTargets; i++) {
            const IR::Attribute mrt{IR::Attribute::RenderTarget0 + i};
            if (!info.stores.GetAny(mrt)) {
                continue;
            }
            frag_color[i] = DefineOutput(F32[4], i);
            Name(frag_color[i], fmt::format("frag_color{}", i));
            interfaces.push_back(frag_color[i]);
        }
        break;
    default:
        break;
    }
}

void EmitContext::DefineBuffers(const Info& info) {
    const auto define_buffer = [&](const BufferResource& buffer, Id type, u32 element_size,
                                   char type_char, u32 index) {
        ASSERT(buffer.stride % element_size == 0);
        const u32 num_elements = buffer.stride * buffer.num_records / element_size;
        const Id record_array_type{TypeArray(F32[1], ConstU32(num_elements))};
        Decorate(record_array_type, spv::Decoration::ArrayStride, element_size);

        const Id struct_type{TypeStruct(record_array_type)};
        const auto name =
            fmt::format("{}_cbuf_block_{}{}", stage, type_char, element_size * CHAR_BIT);
        Name(struct_type, name);
        Decorate(struct_type, spv::Decoration::Block);
        MemberName(struct_type, 0, "data");
        MemberDecorate(struct_type, 0, spv::Decoration::Offset, 0U);

        const auto storage_class =
            buffer.is_storage ? spv::StorageClass::StorageBuffer : spv::StorageClass::Uniform;
        const Id struct_pointer_type{TypePointer(storage_class, struct_type)};
        if (buffer.is_storage) {
            storage_f32 = TypePointer(storage_class, type);
        } else {
            uniform_f32 = TypePointer(storage_class, type);
        }
        const Id id{AddGlobalVariable(struct_pointer_type, storage_class)};
        Decorate(id, spv::Decoration::Binding, binding);
        Decorate(id, spv::Decoration::DescriptorSet, 0U);
        Name(id, fmt::format("c{}", index));

        binding++;
        buffers.push_back(id);
        interfaces.push_back(id);
    };

    for (u32 i = 0; const auto& buffer : info.buffers) {
        ASSERT(True(buffer.used_types & IR::Type::F32));
        define_buffer(buffer, F32[1], 4, 'f', i);
        i++;
    }
}

} // namespace Shader::Backend::SPIRV
