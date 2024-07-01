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
    case IR::Attribute::RenderTarget1:
    case IR::Attribute::RenderTarget2:
    case IR::Attribute::RenderTarget3: {
        const u32 index = u32(attr) - u32(IR::Attribute::RenderTarget0);
        if (ctx.frag_num_comp[index] > 1) {
            return ctx.OpAccessChain(ctx.output_f32, ctx.frag_color[index], ctx.ConstU32(element));
        } else {
            return ctx.frag_color[index];
        }
    }
    case IR::Attribute::Depth:
        return ctx.frag_depth;
    default:
        throw NotImplementedException("Read attribute {}", attr);
    }
    }
}
} // Anonymous namespace

Id EmitGetUserData(EmitContext& ctx, IR::ScalarReg reg) {
    return ctx.ConstU32(ctx.info.user_data[static_cast<size_t>(reg)]);
}

void EmitGetThreadBitScalarReg(EmitContext& ctx) {
    throw LogicError("Unreachable instruction");
}

void EmitSetThreadBitScalarReg(EmitContext& ctx) {
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
    const auto& buffer = ctx.buffers[handle];
    const Id ptr{ctx.OpAccessChain(buffer.pointer_type, buffer.id, ctx.u32_zero_value, index)};
    return ctx.OpLoad(buffer.data_types->Get(1), ptr);
}

Id EmitReadConstBufferU32(EmitContext& ctx, u32 handle, Id index) {
    return ctx.OpBitcast(ctx.U32[1], EmitReadConstBuffer(ctx, handle, index));
}

Id EmitGetAttribute(EmitContext& ctx, IR::Attribute attr, u32 comp) {
    if (IR::IsParam(attr)) {
        const u32 index{u32(attr) - u32(IR::Attribute::Param0)};
        const auto& param{ctx.input_params.at(index)};
        if (!ValidId(param.id)) {
            // Attribute is disabled or varying component is not written
            return ctx.ConstF32(comp == 3 ? 1.0f : 0.0f);
        }
        if (param.num_components > 1) {
            const Id pointer{ctx.OpAccessChain(param.pointer_type, param.id, ctx.ConstU32(comp))};
            return ctx.OpLoad(param.component_type, pointer);
        } else {
            return ctx.OpLoad(param.component_type, param.id);
        }
    }
    switch (attr) {
    case IR::Attribute::FragCoord: {
        const Id coord = ctx.OpLoad(
            ctx.F32[1], ctx.OpAccessChain(ctx.input_f32, ctx.frag_coord, ctx.ConstU32(comp)));
        if (comp == 3) {
            return ctx.OpFDiv(ctx.F32[1], ctx.ConstF32(1.f), coord);
        }
        return coord;
    }
    default:
        throw NotImplementedException("Read attribute {}", attr);
    }
}

Id EmitGetAttributeU32(EmitContext& ctx, IR::Attribute attr, u32 comp) {
    switch (attr) {
    case IR::Attribute::VertexId:
        return ctx.OpLoad(ctx.U32[1], ctx.vertex_index);
    case IR::Attribute::WorkgroupId:
        return ctx.OpCompositeExtract(ctx.U32[1], ctx.OpLoad(ctx.U32[3], ctx.workgroup_id), comp);
    case IR::Attribute::LocalInvocationId:
        return ctx.OpCompositeExtract(ctx.U32[1], ctx.OpLoad(ctx.U32[3], ctx.local_invocation_id),
                                      comp);
    case IR::Attribute::IsFrontFace:
        return ctx.OpSelect(ctx.U32[1], ctx.OpLoad(ctx.U1[1], ctx.front_facing), ctx.u32_one_value,
                            ctx.u32_zero_value);
    default:
        throw NotImplementedException("Read U32 attribute {}", attr);
    }
}

void EmitSetAttribute(EmitContext& ctx, IR::Attribute attr, Id value, u32 element) {
    const Id pointer{OutputAttrPointer(ctx, attr, element)};
    ctx.OpStore(pointer, value);
}

Id EmitLoadBufferF32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    const auto info = inst->Flags<IR::BufferInstInfo>();
    const auto& buffer = ctx.buffers[handle];
    if (info.index_enable && info.offset_enable) {
        UNREACHABLE();
    } else if (info.index_enable) {
        const Id ptr{
            ctx.OpAccessChain(buffer.pointer_type, buffer.id, ctx.u32_zero_value, address)};
        return ctx.OpLoad(buffer.data_types->Get(1), ptr);
    }
    UNREACHABLE();
}

Id EmitLoadBufferU32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    return EmitLoadBufferF32(ctx, inst, handle, address);
}

Id EmitLoadBufferF32x2(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    UNREACHABLE();
}

Id EmitLoadBufferF32x3(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    const auto info = inst->Flags<IR::BufferInstInfo>();
    const auto& buffer = ctx.buffers[handle];
    boost::container::static_vector<Id, 3> ids;
    for (u32 i = 0; i < 3; i++) {
        const Id index{ctx.OpIAdd(ctx.U32[1], address, ctx.ConstU32(i))};
        const Id ptr{ctx.OpAccessChain(buffer.pointer_type, buffer.id, ctx.u32_zero_value, index)};
        ids.push_back(ctx.OpLoad(buffer.data_types->Get(1), ptr));
    }
    return ctx.OpCompositeConstruct(buffer.data_types->Get(3), ids);
}

Id EmitLoadBufferF32x4(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    const auto info = inst->Flags<IR::BufferInstInfo>();
    const auto& buffer = ctx.buffers[handle];
    boost::container::static_vector<Id, 4> ids;
    for (u32 i = 0; i < 4; i++) {
        const Id index{ctx.OpIAdd(ctx.U32[1], address, ctx.ConstU32(i))};
        const Id ptr{ctx.OpAccessChain(buffer.pointer_type, buffer.id, ctx.u32_zero_value, index)};
        ids.push_back(ctx.OpLoad(buffer.data_types->Get(1), ptr));
    }
    return ctx.OpCompositeConstruct(buffer.data_types->Get(4), ids);
}

void EmitStoreBufferF32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    EmitStoreBufferU32(ctx, inst, handle, address, value);
}

void EmitStoreBufferF32x2(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    UNREACHABLE();
}

void EmitStoreBufferF32x3(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    const auto info = inst->Flags<IR::BufferInstInfo>();
    const auto& buffer = ctx.buffers[handle];
    if (info.index_enable && info.offset_enable) {
        UNREACHABLE();
    } else if (info.index_enable) {
        for (u32 i = 0; i < 3; i++) {
            const Id index{ctx.OpIAdd(ctx.U32[1], address, ctx.ConstU32(i))};
            const Id ptr{
                ctx.OpAccessChain(buffer.pointer_type, buffer.id, ctx.u32_zero_value, index)};
            ctx.OpStore(ptr, ctx.OpCompositeExtract(ctx.F32[1], value, i));
        }
        return;
    }
    UNREACHABLE();
}

void EmitStoreBufferF32x4(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    const auto info = inst->Flags<IR::BufferInstInfo>();
    const auto& buffer = ctx.buffers[handle];
    if (info.index_enable && info.offset_enable) {
        UNREACHABLE();
    } else if (info.index_enable) {
        for (u32 i = 0; i < 4; i++) {
            const Id index{ctx.OpIAdd(ctx.U32[1], address, ctx.ConstU32(i))};
            const Id ptr{
                ctx.OpAccessChain(buffer.pointer_type, buffer.id, ctx.u32_zero_value, index)};
            ctx.OpStore(ptr, ctx.OpCompositeExtract(ctx.F32[1], value, i));
        }
        return;
    }
    UNREACHABLE();
}

void EmitStoreBufferU32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    const auto info = inst->Flags<IR::BufferInstInfo>();
    const auto& buffer = ctx.buffers[handle];
    if (info.index_enable && info.offset_enable) {
        UNREACHABLE();
    } else if (info.index_enable) {
        const Id ptr{
            ctx.OpAccessChain(buffer.pointer_type, buffer.id, ctx.u32_zero_value, address)};
        ctx.OpStore(ptr, value);
        return;
    }
    UNREACHABLE();
}

} // namespace Shader::Backend::SPIRV
