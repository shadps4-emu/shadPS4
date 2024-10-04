// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

#include <magic_enum.hpp>

namespace Shader::Backend::SPIRV {
namespace {

Id VsOutputAttrPointer(EmitContext& ctx, VsOutput output) {
    switch (output) {
    case VsOutput::ClipDist0:
    case VsOutput::ClipDist1:
    case VsOutput::ClipDist2:
    case VsOutput::ClipDist3:
    case VsOutput::ClipDist4:
    case VsOutput::ClipDist5:
    case VsOutput::ClipDist6:
    case VsOutput::ClipDist7: {
        const u32 index = u32(output) - u32(VsOutput::ClipDist0);
        const Id clip_num{ctx.ConstU32(index)};
        ASSERT_MSG(Sirit::ValidId(ctx.clip_distances), "Clip distance used but not defined");
        return ctx.OpAccessChain(ctx.output_f32, ctx.clip_distances, clip_num);
    }
    case VsOutput::CullDist0:
    case VsOutput::CullDist1:
    case VsOutput::CullDist2:
    case VsOutput::CullDist3:
    case VsOutput::CullDist4:
    case VsOutput::CullDist5:
    case VsOutput::CullDist6:
    case VsOutput::CullDist7: {
        const u32 index = u32(output) - u32(VsOutput::CullDist0);
        const Id cull_num{ctx.ConstU32(index)};
        ASSERT_MSG(Sirit::ValidId(ctx.cull_distances), "Cull distance used but not defined");
        return ctx.OpAccessChain(ctx.output_f32, ctx.cull_distances, cull_num);
    }
    default:
        UNREACHABLE();
    }
}

Id OutputAttrPointer(EmitContext& ctx, IR::Attribute attr, u32 element) {
    if (IR::IsParam(attr)) {
        const u32 index{u32(attr) - u32(IR::Attribute::Param0)};
        const auto& info{ctx.output_params.at(index)};
        if (info.num_components == 1) {
            return info.id;
        } else {
            return ctx.OpAccessChain(info.pointer_type, info.id, ctx.ConstU32(element));
        }
    }
    if (IR::IsMrt(attr)) {
        const u32 index{u32(attr) - u32(IR::Attribute::RenderTarget0)};
        const auto& info{ctx.frag_outputs.at(index)};
        if (info.num_components == 1) {
            return info.id;
        } else {
            return ctx.OpAccessChain(info.pointer_type, info.id, ctx.ConstU32(element));
        }
    }
    switch (attr) {
    case IR::Attribute::Position0: {
        return ctx.OpAccessChain(ctx.output_f32, ctx.output_position, ctx.ConstU32(element));
    }
    case IR::Attribute::Position1:
    case IR::Attribute::Position2:
    case IR::Attribute::Position3: {
        const u32 index = u32(attr) - u32(IR::Attribute::Position1);
        return VsOutputAttrPointer(ctx, ctx.runtime_info.vs_info.outputs[index][element]);
    }
    case IR::Attribute::Depth:
        return ctx.frag_depth;
    default:
        throw NotImplementedException("Write attribute {}", attr);
    }
}

std::pair<Id, bool> OutputAttrComponentType(EmitContext& ctx, IR::Attribute attr) {
    if (IR::IsParam(attr)) {
        const u32 index{u32(attr) - u32(IR::Attribute::Param0)};
        const auto& info{ctx.output_params.at(index)};
        return {info.component_type, info.is_integer};
    }
    if (IR::IsMrt(attr)) {
        const u32 index{u32(attr) - u32(IR::Attribute::RenderTarget0)};
        const auto& info{ctx.frag_outputs.at(index)};
        return {info.component_type, info.is_integer};
    }
    switch (attr) {
    case IR::Attribute::Position0:
    case IR::Attribute::Position1:
    case IR::Attribute::Position2:
    case IR::Attribute::Position3:
    case IR::Attribute::Depth:
        return {ctx.F32[1], false};
    default:
        throw NotImplementedException("Write attribute {}", attr);
    }
}
} // Anonymous namespace

Id EmitGetUserData(EmitContext& ctx, IR::ScalarReg reg) {
    const u32 index = ctx.binding.user_data + ctx.info.ud_mask.Index(reg);
    const u32 half = PushData::UdRegsIndex + (index >> 2);
    const Id ud_ptr{ctx.OpAccessChain(ctx.TypePointer(spv::StorageClass::PushConstant, ctx.U32[1]),
                                      ctx.push_data_block, ctx.ConstU32(half),
                                      ctx.ConstU32(index & 3))};
    const Id ud_reg{ctx.OpLoad(ctx.U32[1], ud_ptr)};
    ctx.Name(ud_reg, fmt::format("ud_{}", u32(reg)));
    return ud_reg;
}

void EmitGetThreadBitScalarReg(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetThreadBitScalarReg(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetScalarRegister(EmitContext&) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetScalarRegister(EmitContext&) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetVectorRegister(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetVectorRegister(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetGotoVariable(EmitContext&) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetGotoVariable(EmitContext&) {
    UNREACHABLE_MSG("Unreachable instruction");
}

Id EmitReadConst(EmitContext& ctx) {
    return ctx.u32_zero_value;
    UNREACHABLE_MSG("Unreachable instruction");
}

Id EmitReadConstBuffer(EmitContext& ctx, u32 handle, Id index) {
    auto& buffer = ctx.buffers[handle];
    index = ctx.OpIAdd(ctx.U32[1], index, buffer.offset_dwords);
    const Id ptr{ctx.OpAccessChain(buffer.pointer_type, buffer.id, ctx.u32_zero_value, index)};
    return ctx.OpLoad(buffer.data_types->Get(1), ptr);
}

Id EmitReadStepRate(EmitContext& ctx, int rate_idx) {
    return ctx.OpLoad(
        ctx.U32[1], ctx.OpAccessChain(ctx.TypePointer(spv::StorageClass::PushConstant, ctx.U32[1]),
                                      ctx.push_data_block,
                                      rate_idx == 0 ? ctx.u32_zero_value : ctx.u32_one_value));
}

Id EmitGetAttribute(EmitContext& ctx, IR::Attribute attr, u32 comp) {
    if (IR::IsParam(attr)) {
        const u32 index{u32(attr) - u32(IR::Attribute::Param0)};
        const auto& param{ctx.input_params.at(index)};
        if (param.buffer_handle < 0) {
            if (!ValidId(param.id)) {
                // Attribute is disabled or varying component is not written
                return ctx.ConstF32(comp == 3 ? 1.0f : 0.0f);
            }

            Id result;
            if (param.is_default) {
                result = ctx.OpCompositeExtract(param.component_type, param.id, comp);
            } else if (param.num_components > 1) {
                const Id pointer{
                    ctx.OpAccessChain(param.pointer_type, param.id, ctx.ConstU32(comp))};
                result = ctx.OpLoad(param.component_type, pointer);
            } else {
                result = ctx.OpLoad(param.component_type, param.id);
            }
            if (param.is_integer) {
                result = ctx.OpBitcast(ctx.F32[1], result);
            }
            return result;
        } else {
            const auto step_rate = EmitReadStepRate(ctx, param.id.value);
            const auto offset = ctx.OpIAdd(
                ctx.U32[1],
                ctx.OpIMul(
                    ctx.U32[1],
                    ctx.OpUDiv(ctx.U32[1], ctx.OpLoad(ctx.U32[1], ctx.instance_id), step_rate),
                    ctx.ConstU32(param.num_components)),
                ctx.ConstU32(comp));
            return EmitReadConstBuffer(ctx, param.buffer_handle, offset);
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
    case IR::Attribute::InstanceId:
        return ctx.OpLoad(ctx.U32[1], ctx.instance_id);
    case IR::Attribute::InstanceId0:
        return EmitReadStepRate(ctx, 0);
    case IR::Attribute::InstanceId1:
        return EmitReadStepRate(ctx, 1);
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
    if (attr == IR::Attribute::Position1) {
        LOG_WARNING(Render_Vulkan, "Ignoring pos1 export");
        return;
    }
    const Id pointer{OutputAttrPointer(ctx, attr, element)};
    const auto component_type{OutputAttrComponentType(ctx, attr)};
    if (component_type.second) {
        ctx.OpStore(pointer, ctx.OpBitcast(component_type.first, value));
    } else {
        ctx.OpStore(pointer, value);
    }
}

template <u32 N>
static Id EmitLoadBufferU32xN(EmitContext& ctx, u32 handle, Id address) {
    auto& buffer = ctx.buffers[handle];
    address = ctx.OpIAdd(ctx.U32[1], address, buffer.offset);
    const Id index = ctx.OpShiftRightLogical(ctx.U32[1], address, ctx.ConstU32(2u));
    if constexpr (N == 1) {
        const Id ptr{ctx.OpAccessChain(buffer.pointer_type, buffer.id, ctx.u32_zero_value, index)};
        return ctx.OpLoad(buffer.data_types->Get(1), ptr);
    } else {
        boost::container::static_vector<Id, N> ids;
        for (u32 i = 0; i < N; i++) {
            const Id index_i = ctx.OpIAdd(ctx.U32[1], index, ctx.ConstU32(i));
            const Id ptr{
                ctx.OpAccessChain(buffer.pointer_type, buffer.id, ctx.u32_zero_value, index_i)};
            ids.push_back(ctx.OpLoad(buffer.data_types->Get(1), ptr));
        }
        return ctx.OpCompositeConstruct(buffer.data_types->Get(N), ids);
    }
}

Id EmitLoadBufferU32(EmitContext& ctx, IR::Inst*, u32 handle, Id address) {
    return EmitLoadBufferU32xN<1>(ctx, handle, address);
}

Id EmitLoadBufferU32x2(EmitContext& ctx, IR::Inst*, u32 handle, Id address) {
    return EmitLoadBufferU32xN<2>(ctx, handle, address);
}

Id EmitLoadBufferU32x3(EmitContext& ctx, IR::Inst*, u32 handle, Id address) {
    return EmitLoadBufferU32xN<3>(ctx, handle, address);
}

Id EmitLoadBufferU32x4(EmitContext& ctx, IR::Inst*, u32 handle, Id address) {
    return EmitLoadBufferU32xN<4>(ctx, handle, address);
}

Id EmitLoadBufferFormatF32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    const auto& buffer = ctx.texture_buffers[handle];
    const Id tex_buffer = ctx.OpLoad(buffer.image_type, buffer.id);
    const Id coord = ctx.OpIAdd(ctx.U32[1], address, buffer.coord_offset);
    Id texel = buffer.is_storage ? ctx.OpImageRead(buffer.result_type, tex_buffer, coord)
                                 : ctx.OpImageFetch(buffer.result_type, tex_buffer, coord);
    if (buffer.is_integer) {
        texel = ctx.OpBitcast(ctx.F32[4], texel);
    }
    return texel;
}

template <u32 N>
static void EmitStoreBufferU32xN(EmitContext& ctx, u32 handle, Id address, Id value) {
    auto& buffer = ctx.buffers[handle];
    address = ctx.OpIAdd(ctx.U32[1], address, buffer.offset);
    const Id index = ctx.OpShiftRightLogical(ctx.U32[1], address, ctx.ConstU32(2u));
    if constexpr (N == 1) {
        const Id ptr{ctx.OpAccessChain(buffer.pointer_type, buffer.id, ctx.u32_zero_value, index)};
        ctx.OpStore(ptr, value);
    } else {
        for (u32 i = 0; i < N; i++) {
            const Id index_i = ctx.OpIAdd(ctx.U32[1], index, ctx.ConstU32(i));
            const Id ptr =
                ctx.OpAccessChain(buffer.pointer_type, buffer.id, ctx.u32_zero_value, index_i);
            ctx.OpStore(ptr, ctx.OpCompositeExtract(buffer.data_types->Get(1), value, i));
        }
    }
}

void EmitStoreBufferU32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    EmitStoreBufferU32xN<1>(ctx, handle, address, value);
}

void EmitStoreBufferU32x2(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    EmitStoreBufferU32xN<2>(ctx, handle, address, value);
}

void EmitStoreBufferU32x3(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    EmitStoreBufferU32xN<3>(ctx, handle, address, value);
}

void EmitStoreBufferU32x4(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    EmitStoreBufferU32xN<4>(ctx, handle, address, value);
}

void EmitStoreBufferFormatF32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    const auto& buffer = ctx.texture_buffers[handle];
    const Id tex_buffer = ctx.OpLoad(buffer.image_type, buffer.id);
    const Id coord = ctx.OpIAdd(ctx.U32[1], address, buffer.coord_offset);
    if (buffer.is_integer) {
        value = ctx.OpBitcast(buffer.result_type, value);
    }
    ctx.OpImageWrite(tex_buffer, coord, value);
}

} // namespace Shader::Backend::SPIRV
