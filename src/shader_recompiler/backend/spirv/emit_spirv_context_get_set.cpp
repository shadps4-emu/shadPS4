// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {
namespace {

Id OutputAttrPointer(EmitContext& ctx, IR::Attribute attr, u32 element) {
    if (IR::IsParam(attr)) {
        const u32 index{u32(attr) - u32(IR::Attribute::Param0)};
        const auto& info{ctx.output_params.at(index)};
        if (info.num_components == 1) {
            return info.id;
        } else {
            return ctx.OpAccessChain(ctx.output_f32, info.id, ctx.ConstU32(element));
        }
    }
    switch (attr) {
    case IR::Attribute::Position0: {
        return ctx.OpAccessChain(ctx.output_f32, ctx.output_position, ctx.ConstU32(element));
    case IR::Attribute::RenderTarget0:
        return ctx.OpAccessChain(ctx.output_f32, ctx.frag_color[0], ctx.ConstU32(element));
    }
    default:
        throw NotImplementedException("Read attribute {}", attr);
    }
}
} // Anonymous namespace

void EmitGetScalarRegister(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

void EmitSetScalarRegister(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

void EmitGetVectorRegister(EmitContext& ctx) {
    throw LogicError("Unreachable instruction");
}

void EmitSetVectorRegister(EmitContext& ctx) {
    throw LogicError("Unreachable instruction");
}

void EmitSetGotoVariable(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

void EmitGetGotoVariable(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitReadConst(EmitContext& ctx) {
    throw LogicError("Unreachable instruction");
}

Id EmitReadConstBuffer(EmitContext& ctx, const IR::Value& binding, const IR::Value& addr,
                       const IR::Value& offset) {
    throw LogicError("Unreachable instruction");
}

Id EmitReadConstBufferF32(EmitContext& ctx, const IR::Value& binding, const IR::Value& addr,
                          const IR::Value& offset) {
    throw LogicError("Unreachable instruction");
}

Id EmitGetAttribute(EmitContext& ctx, IR::Attribute attr, u32 comp) {
    if (IR::IsParam(attr)) {
        const u32 index{u32(attr) - u32(IR::Attribute::Param0)};
        const auto& param{ctx.input_params.at(index)};
        if (!ValidId(param.id)) {
            // Attribute is disabled or varying component is not written
            return ctx.ConstF32(comp == 3 ? 1.0f : 0.0f);
        }
        const Id pointer{ctx.OpAccessChain(param.pointer_type, param.id, ctx.ConstU32(comp))};
        return ctx.OpLoad(param.component_type, pointer);
    }
    throw NotImplementedException("Read attribute {}", attr);
}

Id EmitGetAttributeU32(EmitContext& ctx, IR::Attribute attr, u32 comp) {
    switch (attr) {
    case IR::Attribute::VertexId:
        return ctx.OpLoad(ctx.U32[1], ctx.vertex_index);
    default:
        throw NotImplementedException("Read U32 attribute {}", attr);
    }
}

void EmitSetAttribute(EmitContext& ctx, IR::Attribute attr, Id value, u32 element) {
    const Id pointer{OutputAttrPointer(ctx, attr, element)};
    ctx.OpStore(pointer, value);
}

} // namespace Shader::Backend::SPIRV
