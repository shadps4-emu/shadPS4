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
    : Sirit::Module(profile_.supported_spirv), profile{profile_}, stage{program.stage} {
    u32& uniform_binding{bindings.unified};
    u32& storage_binding{bindings.unified};
    u32& texture_binding{bindings.unified};
    u32& image_binding{bindings.unified};
    AddCapability(spv::Capability::Shader);
    DefineArithmeticTypes();
    DefineInterfaces(program);
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

    output_f32 = Name(TypePointer(spv::StorageClass::Output, F32[1]), "output_f32");
    output_u32 = Name(TypePointer(spv::StorageClass::Output, U32[1]), "output_u32");
}

void EmitContext::DefineInterfaces(const IR::Program& program) {
    DefineInputs(program);
    DefineOutputs(program);
}

void EmitContext::DefineInputs(const IR::Program& program) {
    switch (stage) {
    case Stage::Vertex:
        vertex_index = DefineVariable(U32[1], spv::BuiltIn::VertexIndex, spv::StorageClass::Input);
        base_vertex = DefineVariable(U32[1], spv::BuiltIn::BaseVertex, spv::StorageClass::Input);
        break;
    default:
        break;
    }
}

void EmitContext::DefineOutputs(const IR::Program& program) {
    switch (stage) {
    case Stage::Vertex:
        output_position = DefineVariable(F32[4], spv::BuiltIn::Position, spv::StorageClass::Output);
        break;
    case Stage::Fragment:
        frag_color[0] = DefineOutput(F32[4], 0);
        Name(frag_color[0], fmt::format("frag_color{}", 0));
        interfaces.push_back(frag_color[0]);
        break;
    default:
        break;
    }
}

} // namespace Shader::Backend::SPIRV
