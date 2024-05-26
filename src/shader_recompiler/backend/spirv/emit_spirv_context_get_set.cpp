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

void EmitGetUserData(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

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

Id EmitReadConstBuffer(EmitContext& ctx, u32 handle, Id index) {
    const Id buffer = ctx.buffers[handle];
    const Id type = ctx.info.buffers[handle].is_storage ? ctx.storage_f32 : ctx.uniform_f32;
    const Id ptr{ctx.OpAccessChain(type, buffer, ctx.ConstU32(0U), index)};
    return ctx.OpLoad(ctx.F32[1], ptr);
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

Id EmitLoadBufferF32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    UNREACHABLE();
}

Id EmitLoadBufferF32x2(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    UNREACHABLE();
}

Id EmitLoadBufferF32x3(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    UNREACHABLE();
}

Id EmitLoadBufferF32x4(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    const auto info = inst->Flags<IR::BufferInstInfo>();
    const Id buffer = ctx.buffers[handle];
    const Id type = ctx.info.buffers[handle].is_storage ? ctx.storage_f32 : ctx.uniform_f32;
    if (info.index_enable && info.offset_enable) {
        UNREACHABLE();
    } else if (info.index_enable) {
        boost::container::static_vector<Id, 4> ids;
        for (u32 i = 0; i < 4; i++) {
            const Id index{ctx.OpIAdd(ctx.U32[1], address, ctx.ConstU32(i))};
            const Id ptr{ctx.OpAccessChain(type, buffer, ctx.ConstU32(0U), index)};
            ids.push_back(ctx.OpLoad(ctx.F32[1], ptr));
        }
        return ctx.OpCompositeConstruct(ctx.F32[4], ids);
    }
    UNREACHABLE();
}

} // namespace Shader::Backend::SPIRV
