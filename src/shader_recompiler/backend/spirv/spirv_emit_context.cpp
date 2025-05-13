// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/div_ceil.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"
#include "shader_recompiler/frontend/fetch_shader.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/amdgpu/types.h"

#include <boost/container/static_vector.hpp>
#include <fmt/format.h>

#include <numbers>
#include <string_view>

namespace Shader::Backend::SPIRV {
namespace {

std::string_view StageName(Stage stage) {
    switch (stage) {
    case Stage::Vertex:
        return "vs";
    case Stage::Local:
        return "ls";
    case Stage::Export:
        return "es";
    case Stage::Hull:
        return "hs";
    case Stage::Geometry:
        return "gs";
    case Stage::Fragment:
        return "fs";
    case Stage::Compute:
        return "cs";
    }
    UNREACHABLE_MSG("Invalid hw stage {}", u32(stage));
}

static constexpr u32 NumVertices(AmdGpu::PrimitiveType type) {
    switch (type) {
    case AmdGpu::PrimitiveType::PointList:
        return 1u;
    case AmdGpu::PrimitiveType::LineList:
    case AmdGpu::PrimitiveType::LineStrip:
        return 2u;
    case AmdGpu::PrimitiveType::TriangleList:
    case AmdGpu::PrimitiveType::TriangleStrip:
    case AmdGpu::PrimitiveType::RectList:
        return 3u;
    case AmdGpu::PrimitiveType::AdjTriangleList:
        return 6u;
    case AmdGpu::PrimitiveType::AdjLineList:
        return 4u;
    default:
        UNREACHABLE();
    }
}

template <typename... Args>
void Name(EmitContext& ctx, Id object, std::string_view format_str, Args&&... args) {
    ctx.Name(object, fmt::format(fmt::runtime(format_str), StageName(ctx.stage),
                                 std::forward<Args>(args)...)
                         .c_str());
}

} // Anonymous namespace

EmitContext::EmitContext(const Profile& profile_, const RuntimeInfo& runtime_info_, Info& info_,
                         Bindings& binding_)
    : Sirit::Module(profile_.supported_spirv), info{info_}, runtime_info{runtime_info_},
      profile{profile_}, stage{info.stage}, l_stage{info.l_stage}, binding{binding_} {
    AddCapability(spv::Capability::Shader);
    DefineArithmeticTypes();
    DefineInterfaces();
    DefineSharedMemory();
    DefineBuffers();
    DefineImagesAndSamplers();
    DefineFunctions();
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
    case IR::Type::StringLiteral:
        return String(value.StringLiteral());
    default:
        throw NotImplementedException("Immediate type {}", value.Type());
    }
}

void EmitContext::DefineArithmeticTypes() {
    void_id = Name(TypeVoid(), "void_id");
    U1[1] = Name(TypeBool(), "bool_id");
    U8 = Name(TypeUInt(8), "u8_id");
    U16 = Name(TypeUInt(16), "u16_id");
    if (info.uses_fp16) {
        F16[1] = Name(TypeFloat(16), "f16_id");
        U16 = Name(TypeUInt(16), "u16_id");
    }
    if (info.uses_fp64) {
        F64[1] = Name(TypeFloat(64), "f64_id");
    }
    F32[1] = Name(TypeFloat(32), "f32_id");
    S32[1] = Name(TypeSInt(32), "i32_id");
    U32[1] = Name(TypeUInt(32), "u32_id");
    U64 = Name(TypeUInt(64), "u64_id");

    for (u32 i = 2; i <= 4; i++) {
        if (info.uses_fp16) {
            F16[i] = Name(TypeVector(F16[1], i), fmt::format("f16vec{}_id", i));
        }
        if (info.uses_fp64) {
            F64[i] = Name(TypeVector(F64[1], i), fmt::format("f64vec{}_id", i));
        }
        F32[i] = Name(TypeVector(F32[1], i), fmt::format("f32vec{}_id", i));
        S32[i] = Name(TypeVector(S32[1], i), fmt::format("i32vec{}_id", i));
        U32[i] = Name(TypeVector(U32[1], i), fmt::format("u32vec{}_id", i));
        U1[i] = Name(TypeVector(U1[1], i), fmt::format("bvec{}_id", i));
    }

    true_value = ConstantTrue(U1[1]);
    false_value = ConstantFalse(U1[1]);
    u32_one_value = ConstU32(1U);
    u32_zero_value = ConstU32(0U);
    f32_zero_value = ConstF32(0.0f);

    pi_x2 = ConstF32(2.0f * float{std::numbers::pi});

    input_f32 = Name(TypePointer(spv::StorageClass::Input, F32[1]), "input_f32");
    input_u32 = Name(TypePointer(spv::StorageClass::Input, U32[1]), "input_u32");
    input_s32 = Name(TypePointer(spv::StorageClass::Input, S32[1]), "input_s32");

    output_f32 = Name(TypePointer(spv::StorageClass::Output, F32[1]), "output_f32");
    output_u32 = Name(TypePointer(spv::StorageClass::Output, U32[1]), "output_u32");
    output_s32 = Name(TypePointer(spv::StorageClass::Output, S32[1]), "output_s32");

    full_result_i32x2 = Name(TypeStruct(S32[1], S32[1]), "full_result_i32x2");
    full_result_u32x2 = Name(TypeStruct(U32[1], U32[1]), "full_result_u32x2");
    frexp_result_f32 = Name(TypeStruct(F32[1], S32[1]), "frexp_result_f32");
    if (info.uses_fp64) {
        frexp_result_f64 = Name(TypeStruct(F64[1], S32[1]), "frexp_result_f64");
    }
}

void EmitContext::DefineInterfaces() {
    DefinePushDataBlock();
    DefineInputs();
    DefineOutputs();
}

const VectorIds& GetAttributeType(EmitContext& ctx, AmdGpu::NumberFormat fmt) {
    switch (GetNumberClass(fmt)) {
    case AmdGpu::NumberClass::Float:
        return ctx.F32;
    case AmdGpu::NumberClass::Sint:
        return ctx.S32;
    case AmdGpu::NumberClass::Uint:
        return ctx.U32;
    default:
        break;
    }
    UNREACHABLE_MSG("Invalid attribute type {}", fmt);
}

EmitContext::SpirvAttribute EmitContext::GetAttributeInfo(AmdGpu::NumberFormat fmt, Id id,
                                                          u32 num_components, bool output) {
    switch (GetNumberClass(fmt)) {
    case AmdGpu::NumberClass::Float:
        return {id, output ? output_f32 : input_f32, F32[1], num_components, false};
    case AmdGpu::NumberClass::Uint:
        return {id, output ? output_u32 : input_u32, U32[1], num_components, true};
    case AmdGpu::NumberClass::Sint:
        return {id, output ? output_s32 : input_s32, S32[1], num_components, true};
    default:
        break;
    }
    UNREACHABLE_MSG("Invalid attribute type {}", fmt);
}

Id EmitContext::GetBufferSize(const u32 sharp_idx) {
    const auto& srt_flatbuf = buffers.back();
    ASSERT(srt_flatbuf.buffer_type == BufferType::ReadConstUbo);
    const auto [id, pointer_type] = srt_flatbuf[BufferAlias::U32];

    const auto rsrc1{
        OpLoad(U32[1], OpAccessChain(pointer_type, id, u32_zero_value, ConstU32(sharp_idx + 1)))};
    const auto rsrc2{
        OpLoad(U32[1], OpAccessChain(pointer_type, id, u32_zero_value, ConstU32(sharp_idx + 2)))};

    const auto stride{OpBitFieldUExtract(U32[1], rsrc1, ConstU32(16u), ConstU32(14u))};
    const auto num_records{rsrc2};

    const auto stride_zero{OpIEqual(U1[1], stride, u32_zero_value)};
    const auto stride_size{OpIMul(U32[1], num_records, stride)};
    return OpSelect(U32[1], stride_zero, num_records, stride_size);
}

void EmitContext::DefineBufferProperties() {
    for (u32 i = 0; i < buffers.size(); i++) {
        BufferDefinition& buffer = buffers[i];
        if (buffer.buffer_type != BufferType::Guest) {
            continue;
        }
        const u32 binding = buffer.binding;
        const u32 half = PushData::BufOffsetIndex + (binding >> 4);
        const u32 comp = (binding & 0xf) >> 2;
        const u32 offset = (binding & 0x3) << 3;
        const Id ptr{OpAccessChain(TypePointer(spv::StorageClass::PushConstant, U32[1]),
                                   push_data_block, ConstU32(half), ConstU32(comp))};
        const Id value{OpLoad(U32[1], ptr)};
        buffer.offset = OpBitFieldUExtract(U32[1], value, ConstU32(offset), ConstU32(8U));
        Name(buffer.offset, fmt::format("buf{}_off", binding));
        buffer.offset_dwords = OpShiftRightLogical(U32[1], buffer.offset, ConstU32(2U));
        Name(buffer.offset_dwords, fmt::format("buf{}_dword_off", binding));

        // Only need to load size if performing bounds checks and the buffer is both guest and not
        // inline.
        if (!profile.supports_robust_buffer_access && buffer.buffer_type == BufferType::Guest) {
            const BufferResource& desc = info.buffers[i];
            if (desc.sharp_idx == std::numeric_limits<u32>::max()) {
                buffer.size = ConstU32(desc.inline_cbuf.GetSize());
            } else {
                buffer.size = GetBufferSize(desc.sharp_idx);
            }
            Name(buffer.size, fmt::format("buf{}_size", binding));
            buffer.size_shorts = OpShiftRightLogical(U32[1], buffer.size, ConstU32(1U));
            Name(buffer.size_shorts, fmt::format("buf{}_short_size", binding));
            buffer.size_dwords = OpShiftRightLogical(U32[1], buffer.size, ConstU32(2U));
            Name(buffer.size_dwords, fmt::format("buf{}_dword_size", binding));
        }
    }
}

void EmitContext::DefineInterpolatedAttribs() {
    if (!profile.needs_manual_interpolation) {
        return;
    }
    // Iterate all input attributes, load them and manually interpolate.
    for (s32 i = 0; i < runtime_info.fs_info.num_inputs; i++) {
        const auto& input = runtime_info.fs_info.inputs[i];
        const u32 semantic = input.param_index;
        auto& params = input_params[semantic];
        if (input.is_flat || params.is_loaded) {
            continue;
        }
        const Id p_array{OpLoad(TypeArray(F32[4], ConstU32(3U)), params.id)};
        const Id p0{OpCompositeExtract(F32[4], p_array, 0U)};
        const Id p1{OpCompositeExtract(F32[4], p_array, 1U)};
        const Id p2{OpCompositeExtract(F32[4], p_array, 2U)};
        const Id p10{OpFSub(F32[4], p1, p0)};
        const Id p20{OpFSub(F32[4], p2, p0)};
        const Id bary_coord{OpLoad(F32[3], gl_bary_coord_id)};
        const Id bary_coord_y{OpCompositeExtract(F32[1], bary_coord, 1)};
        const Id bary_coord_z{OpCompositeExtract(F32[1], bary_coord, 2)};
        const Id p10_y{OpVectorTimesScalar(F32[4], p10, bary_coord_y)};
        const Id p20_z{OpVectorTimesScalar(F32[4], p20, bary_coord_z)};
        params.id = OpFAdd(F32[4], p0, OpFAdd(F32[4], p10_y, p20_z));
        Name(params.id, fmt::format("fs_in_attr{}", semantic));
        params.is_loaded = true;
    }
}

void EmitContext::DefineWorkgroupIndex() {
    const Id workgroup_id_val{OpLoad(U32[3], workgroup_id)};
    const Id workgroup_x{OpCompositeExtract(U32[1], workgroup_id_val, 0)};
    const Id workgroup_y{OpCompositeExtract(U32[1], workgroup_id_val, 1)};
    const Id workgroup_z{OpCompositeExtract(U32[1], workgroup_id_val, 2)};
    const Id num_workgroups{OpLoad(U32[3], num_workgroups_id)};
    const Id num_workgroups_x{OpCompositeExtract(U32[1], num_workgroups, 0)};
    const Id num_workgroups_y{OpCompositeExtract(U32[1], num_workgroups, 1)};
    workgroup_index_id =
        OpIAdd(U32[1], OpIAdd(U32[1], workgroup_x, OpIMul(U32[1], workgroup_y, num_workgroups_x)),
               OpIMul(U32[1], workgroup_z, OpIMul(U32[1], num_workgroups_x, num_workgroups_y)));
    Name(workgroup_index_id, "workgroup_index");
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

void EmitContext::DefineInputs() {
    if (info.uses_lane_id) {
        subgroup_local_invocation_id = DefineVariable(
            U32[1], spv::BuiltIn::SubgroupLocalInvocationId, spv::StorageClass::Input);
        Decorate(subgroup_local_invocation_id, spv::Decoration::Flat);
    }
    switch (l_stage) {
    case LogicalStage::Vertex: {
        vertex_index = DefineVariable(U32[1], spv::BuiltIn::VertexIndex, spv::StorageClass::Input);
        base_vertex = DefineVariable(U32[1], spv::BuiltIn::BaseVertex, spv::StorageClass::Input);
        instance_id = DefineVariable(U32[1], spv::BuiltIn::InstanceIndex, spv::StorageClass::Input);

        const auto fetch_shader = Gcn::ParseFetchShader(info);
        if (!fetch_shader) {
            break;
        }
        for (const auto& attrib : fetch_shader->attributes) {
            ASSERT(attrib.semantic < IR::NumParams);
            const auto sharp = attrib.GetSharp(info);
            const Id type{GetAttributeType(*this, sharp.GetNumberFmt())[4]};
            if (attrib.UsesStepRates()) {
                const u32 rate_idx =
                    attrib.GetStepRate() == Gcn::VertexAttribute::InstanceIdType::OverStepRate0 ? 0
                                                                                                : 1;
                const u32 num_components = AmdGpu::NumComponents(sharp.GetDataFmt());
                const auto buffer =
                    std::ranges::find_if(info.buffers, [&attrib](const auto& buffer) {
                        return buffer.instance_attrib == attrib.semantic;
                    });
                // Note that we pass index rather than Id
                input_params[attrib.semantic] = SpirvAttribute{
                    .id = {rate_idx},
                    .pointer_type = input_u32,
                    .component_type = U32[1],
                    .num_components = std::min<u16>(attrib.num_elements, num_components),
                    .is_integer = true,
                    .is_loaded = false,
                    .buffer_handle = int(buffer - info.buffers.begin()),
                };
            } else {
                Id id{DefineInput(type, attrib.semantic)};
                if (attrib.GetStepRate() == Gcn::VertexAttribute::InstanceIdType::Plain) {
                    Name(id, fmt::format("vs_instance_attr{}", attrib.semantic));
                } else {
                    Name(id, fmt::format("vs_in_attr{}", attrib.semantic));
                }
                input_params[attrib.semantic] =
                    GetAttributeInfo(sharp.GetNumberFmt(), id, 4, false);
            }
        }
        break;
    }
    case LogicalStage::Fragment:
        if (info.loads.GetAny(IR::Attribute::FragCoord)) {
            frag_coord = DefineVariable(F32[4], spv::BuiltIn::FragCoord, spv::StorageClass::Input);
        }
        if (info.stores.Get(IR::Attribute::Depth)) {
            frag_depth = DefineVariable(F32[1], spv::BuiltIn::FragDepth, spv::StorageClass::Output);
        }
        if (info.loads.Get(IR::Attribute::IsFrontFace)) {
            front_facing =
                DefineVariable(U1[1], spv::BuiltIn::FrontFacing, spv::StorageClass::Input);
        }
        if (profile.needs_manual_interpolation) {
            gl_bary_coord_id =
                DefineVariable(F32[3], spv::BuiltIn::BaryCoordKHR, spv::StorageClass::Input);
        }
        for (s32 i = 0; i < runtime_info.fs_info.num_inputs; i++) {
            const auto& input = runtime_info.fs_info.inputs[i];
            const u32 semantic = input.param_index;
            ASSERT(semantic < IR::NumParams);
            if (input.IsDefault()) {
                input_params[semantic] = {
                    MakeDefaultValue(*this, input.default_value), input_f32, F32[1], 4, false, true,
                };
                continue;
            }
            const IR::Attribute param{IR::Attribute::Param0 + input.param_index};
            const u32 num_components = info.loads.NumComponents(param);
            const Id type{F32[num_components]};
            Id attr_id{};
            if (profile.needs_manual_interpolation && !input.is_flat) {
                attr_id = DefineInput(TypeArray(type, ConstU32(3U)), semantic);
                Decorate(attr_id, spv::Decoration::PerVertexKHR);
                Name(attr_id, fmt::format("fs_in_attr{}_p", semantic));
            } else {
                attr_id = DefineInput(type, semantic);
                Name(attr_id, fmt::format("fs_in_attr{}", semantic));
            }
            if (input.is_flat) {
                Decorate(attr_id, spv::Decoration::Flat);
            }
            input_params[semantic] =
                GetAttributeInfo(AmdGpu::NumberFormat::Float, attr_id, num_components, false);
        }
        break;
    case LogicalStage::Compute:
        if (info.loads.GetAny(IR::Attribute::WorkgroupIndex) ||
            info.loads.GetAny(IR::Attribute::WorkgroupId)) {
            workgroup_id =
                DefineVariable(U32[3], spv::BuiltIn::WorkgroupId, spv::StorageClass::Input);
        }
        if (info.loads.GetAny(IR::Attribute::WorkgroupIndex)) {
            num_workgroups_id =
                DefineVariable(U32[3], spv::BuiltIn::NumWorkgroups, spv::StorageClass::Input);
        }
        if (info.loads.GetAny(IR::Attribute::LocalInvocationId)) {
            local_invocation_id =
                DefineVariable(U32[3], spv::BuiltIn::LocalInvocationId, spv::StorageClass::Input);
        }
        break;
    case LogicalStage::Geometry: {
        primitive_id = DefineVariable(U32[1], spv::BuiltIn::PrimitiveId, spv::StorageClass::Input);
        const auto gl_per_vertex =
            Name(TypeStruct(TypeVector(F32[1], 4), F32[1], TypeArray(F32[1], ConstU32(1u))),
                 "gl_PerVertex");
        MemberName(gl_per_vertex, 0, "gl_Position");
        MemberName(gl_per_vertex, 1, "gl_PointSize");
        MemberName(gl_per_vertex, 2, "gl_ClipDistance");
        MemberDecorate(gl_per_vertex, 0, spv::Decoration::BuiltIn,
                       static_cast<std::uint32_t>(spv::BuiltIn::Position));
        MemberDecorate(gl_per_vertex, 1, spv::Decoration::BuiltIn,
                       static_cast<std::uint32_t>(spv::BuiltIn::PointSize));
        MemberDecorate(gl_per_vertex, 2, spv::Decoration::BuiltIn,
                       static_cast<std::uint32_t>(spv::BuiltIn::ClipDistance));
        Decorate(gl_per_vertex, spv::Decoration::Block);
        const auto num_verts_in = NumVertices(runtime_info.gs_info.in_primitive);
        const auto vertices_in = TypeArray(gl_per_vertex, ConstU32(num_verts_in));
        gl_in = Name(DefineVar(vertices_in, spv::StorageClass::Input), "gl_in");
        interfaces.push_back(gl_in);

        const auto num_params = runtime_info.gs_info.in_vertex_data_size / 4 - 1u;
        for (int param_id = 0; param_id < num_params; ++param_id) {
            const Id type{TypeArray(F32[4], ConstU32(num_verts_in))};
            const Id id{DefineInput(type, param_id)};
            Name(id, fmt::format("gs_in_attr{}", param_id));
            input_params[param_id] = {id, input_f32, F32[1], 4};
        }
        break;
    }
    case LogicalStage::TessellationControl: {
        invocation_id =
            DefineVariable(U32[1], spv::BuiltIn::InvocationId, spv::StorageClass::Input);
        patch_vertices =
            DefineVariable(U32[1], spv::BuiltIn::PatchVertices, spv::StorageClass::Input);
        primitive_id = DefineVariable(U32[1], spv::BuiltIn::PrimitiveId, spv::StorageClass::Input);

        const u32 num_attrs = Common::AlignUp(runtime_info.hs_info.ls_stride, 16) >> 4;
        if (num_attrs > 0) {
            const Id per_vertex_type{TypeArray(F32[4], ConstU32(num_attrs))};
            // The input vertex count isn't statically known, so make length 32 (what glslang does)
            const Id patch_array_type{TypeArray(per_vertex_type, ConstU32(32u))};
            input_attr_array = DefineInput(patch_array_type, 0);
            Name(input_attr_array, "in_attrs");
        }
        break;
    }
    case LogicalStage::TessellationEval: {
        tess_coord = DefineInput(F32[3], std::nullopt, spv::BuiltIn::TessCoord);
        primitive_id = DefineVariable(U32[1], spv::BuiltIn::PrimitiveId, spv::StorageClass::Input);

        const u32 num_attrs = Common::AlignUp(runtime_info.vs_info.hs_output_cp_stride, 16) >> 4;
        if (num_attrs > 0) {
            const Id per_vertex_type{TypeArray(F32[4], ConstU32(num_attrs))};
            // The input vertex count isn't statically known, so make length 32 (what glslang does)
            const Id patch_array_type{TypeArray(per_vertex_type, ConstU32(32u))};
            input_attr_array = DefineInput(patch_array_type, 0);
            Name(input_attr_array, "in_attrs");
        }

        const u32 patch_base_location = num_attrs;
        for (size_t index = 0; index < 30; ++index) {
            if (!(info.uses_patches & (1U << index))) {
                continue;
            }
            const Id id{DefineInput(F32[4], patch_base_location + index)};
            Decorate(id, spv::Decoration::Patch);
            Name(id, fmt::format("patch_in{}", index));
            patches[index] = id;
        }
        break;
    }
    default:
        break;
    }
}

void EmitContext::DefineOutputs() {
    switch (l_stage) {
    case LogicalStage::Vertex: {
        // No point in defining builtin outputs (i.e. position) unless next stage is fragment?
        // Might cause problems linking with tcs

        output_position = DefineVariable(F32[4], spv::BuiltIn::Position, spv::StorageClass::Output);
        const bool has_extra_pos_stores = info.stores.Get(IR::Attribute::Position1) ||
                                          info.stores.Get(IR::Attribute::Position2) ||
                                          info.stores.Get(IR::Attribute::Position3);
        if (has_extra_pos_stores) {
            const Id type{TypeArray(F32[1], ConstU32(8U))};
            clip_distances =
                DefineVariable(type, spv::BuiltIn::ClipDistance, spv::StorageClass::Output);
            cull_distances =
                DefineVariable(type, spv::BuiltIn::CullDistance, spv::StorageClass::Output);
        }
        if (stage == Shader::Stage::Local && runtime_info.ls_info.links_with_tcs) {
            const u32 num_attrs = Common::AlignUp(runtime_info.ls_info.ls_stride, 16) >> 4;
            if (num_attrs > 0) {
                const Id type{TypeArray(F32[4], ConstU32(num_attrs))};
                output_attr_array = DefineOutput(type, 0);
                Name(output_attr_array, "out_attrs");
            }
        } else {
            for (u32 i = 0; i < IR::NumParams; i++) {
                const IR::Attribute param{IR::Attribute::Param0 + i};
                if (!info.stores.GetAny(param)) {
                    continue;
                }
                const u32 num_components = info.stores.NumComponents(param);
                const Id id{DefineOutput(F32[num_components], i)};
                Name(id, fmt::format("out_attr{}", i));
                output_params[i] =
                    GetAttributeInfo(AmdGpu::NumberFormat::Float, id, num_components, true);
            }
        }
        break;
    }
    case LogicalStage::TessellationControl: {
        if (info.stores_tess_level_outer) {
            const Id type{TypeArray(F32[1], ConstU32(4U))};
            output_tess_level_outer =
                DefineOutput(type, std::nullopt, spv::BuiltIn::TessLevelOuter);
            Decorate(output_tess_level_outer, spv::Decoration::Patch);
        }
        if (info.stores_tess_level_inner) {
            const Id type{TypeArray(F32[1], ConstU32(2U))};
            output_tess_level_inner =
                DefineOutput(type, std::nullopt, spv::BuiltIn::TessLevelInner);
            Decorate(output_tess_level_inner, spv::Decoration::Patch);
        }

        const u32 num_attrs = Common::AlignUp(runtime_info.hs_info.hs_output_cp_stride, 16) >> 4;
        if (num_attrs > 0) {
            const Id per_vertex_type{TypeArray(F32[4], ConstU32(num_attrs))};
            // The input vertex count isn't statically known, so make length 32 (what glslang does)
            const Id patch_array_type{TypeArray(
                per_vertex_type, ConstU32(runtime_info.hs_info.NumOutputControlPoints()))};
            output_attr_array = DefineOutput(patch_array_type, 0);
            Name(output_attr_array, "out_attrs");
        }

        const u32 patch_base_location = num_attrs;
        for (size_t index = 0; index < 30; ++index) {
            if (!(info.uses_patches & (1U << index))) {
                continue;
            }
            const Id id{DefineOutput(F32[4], patch_base_location + index)};
            Decorate(id, spv::Decoration::Patch);
            Name(id, fmt::format("patch_out{}", index));
            patches[index] = id;
        }
        break;
    }
    case LogicalStage::TessellationEval: {
        output_position = DefineVariable(F32[4], spv::BuiltIn::Position, spv::StorageClass::Output);
        const bool has_extra_pos_stores = info.stores.Get(IR::Attribute::Position1) ||
                                          info.stores.Get(IR::Attribute::Position2) ||
                                          info.stores.Get(IR::Attribute::Position3);
        if (has_extra_pos_stores) {
            const Id type{TypeArray(F32[1], ConstU32(8U))};
            clip_distances =
                DefineVariable(type, spv::BuiltIn::ClipDistance, spv::StorageClass::Output);
            cull_distances =
                DefineVariable(type, spv::BuiltIn::CullDistance, spv::StorageClass::Output);
        }
        for (u32 i = 0; i < IR::NumParams; i++) {
            const IR::Attribute param{IR::Attribute::Param0 + i};
            if (!info.stores.GetAny(param)) {
                continue;
            }
            const u32 num_components = info.stores.NumComponents(param);
            const Id id{DefineOutput(F32[num_components], i)};
            Name(id, fmt::format("out_attr{}", i));
            output_params[i] =
                GetAttributeInfo(AmdGpu::NumberFormat::Float, id, num_components, true);
        }
        break;
    }
    case LogicalStage::Fragment:
        for (u32 i = 0; i < IR::NumRenderTargets; i++) {
            const IR::Attribute mrt{IR::Attribute::RenderTarget0 + i};
            if (!info.stores.GetAny(mrt)) {
                continue;
            }
            const u32 num_components = info.stores.NumComponents(mrt);
            const AmdGpu::NumberFormat num_format{runtime_info.fs_info.color_buffers[i].num_format};
            const Id type{GetAttributeType(*this, num_format)[num_components]};
            const Id id{DefineOutput(type, i)};
            Name(id, fmt::format("frag_color{}", i));
            frag_outputs[i] = GetAttributeInfo(num_format, id, num_components, true);
        }
        break;
    case LogicalStage::Geometry: {
        output_position = DefineVariable(F32[4], spv::BuiltIn::Position, spv::StorageClass::Output);

        for (u32 attr_id = 0; attr_id < info.gs_copy_data.num_attrs; attr_id++) {
            const Id id{DefineOutput(F32[4], attr_id)};
            Name(id, fmt::format("out_attr{}", attr_id));
            output_params[attr_id] = {id, output_f32, F32[1], 4u};
        }
        break;
    }
    case LogicalStage::Compute:
        break;
    default:
        UNREACHABLE();
    }
}

void EmitContext::DefinePushDataBlock() {
    // Create push constants block for instance steps rates
    const Id struct_type{Name(TypeStruct(U32[1], U32[1], F32[1], F32[1], F32[1], F32[1], U32[4],
                                         U32[4], U32[4], U32[4], U32[4], U32[4]),
                              "AuxData")};
    Decorate(struct_type, spv::Decoration::Block);
    MemberName(struct_type, PushData::Step0Index, "sr0");
    MemberName(struct_type, PushData::Step1Index, "sr1");
    MemberName(struct_type, PushData::XOffsetIndex, "xoffset");
    MemberName(struct_type, PushData::YOffsetIndex, "yoffset");
    MemberName(struct_type, PushData::XScaleIndex, "xscale");
    MemberName(struct_type, PushData::YScaleIndex, "yscale");
    MemberName(struct_type, PushData::UdRegsIndex + 0, "ud_regs0");
    MemberName(struct_type, PushData::UdRegsIndex + 1, "ud_regs1");
    MemberName(struct_type, PushData::UdRegsIndex + 2, "ud_regs2");
    MemberName(struct_type, PushData::UdRegsIndex + 3, "ud_regs3");
    MemberName(struct_type, PushData::BufOffsetIndex + 0, "buf_offsets0");
    MemberName(struct_type, PushData::BufOffsetIndex + 1, "buf_offsets1");
    MemberDecorate(struct_type, PushData::Step0Index, spv::Decoration::Offset, 0U);
    MemberDecorate(struct_type, PushData::Step1Index, spv::Decoration::Offset, 4U);
    MemberDecorate(struct_type, PushData::XOffsetIndex, spv::Decoration::Offset, 8U);
    MemberDecorate(struct_type, PushData::YOffsetIndex, spv::Decoration::Offset, 12U);
    MemberDecorate(struct_type, PushData::XScaleIndex, spv::Decoration::Offset, 16U);
    MemberDecorate(struct_type, PushData::YScaleIndex, spv::Decoration::Offset, 20U);
    MemberDecorate(struct_type, PushData::UdRegsIndex + 0, spv::Decoration::Offset, 24U);
    MemberDecorate(struct_type, PushData::UdRegsIndex + 1, spv::Decoration::Offset, 40U);
    MemberDecorate(struct_type, PushData::UdRegsIndex + 2, spv::Decoration::Offset, 56U);
    MemberDecorate(struct_type, PushData::UdRegsIndex + 3, spv::Decoration::Offset, 72U);
    MemberDecorate(struct_type, PushData::BufOffsetIndex + 0, spv::Decoration::Offset, 88U);
    MemberDecorate(struct_type, PushData::BufOffsetIndex + 1, spv::Decoration::Offset, 104U);
    push_data_block = DefineVar(struct_type, spv::StorageClass::PushConstant);
    Name(push_data_block, "push_data");
    interfaces.push_back(push_data_block);
}

EmitContext::BufferSpv EmitContext::DefineBuffer(bool is_storage, bool is_written, u32 elem_shift,
                                                 BufferType buffer_type, Id data_type) {
    // Define array type.
    const Id max_num_items = ConstU32(u32(profile.max_ubo_size) >> elem_shift);
    const Id record_array_type{is_storage ? TypeRuntimeArray(data_type)
                                          : TypeArray(data_type, max_num_items)};
    // Define block struct type. Don't perform decorations twice on the same Id.
    const Id struct_type{TypeStruct(record_array_type)};
    if (std::ranges::find(buf_type_ids, record_array_type.value, &Id::value) ==
        buf_type_ids.end()) {
        Decorate(record_array_type, spv::Decoration::ArrayStride, 1 << elem_shift);
        Decorate(struct_type, spv::Decoration::Block);
        MemberName(struct_type, 0, "data");
        MemberDecorate(struct_type, 0, spv::Decoration::Offset, 0U);
        buf_type_ids.push_back(record_array_type);
    }
    // Define buffer binding interface.
    const auto storage_class =
        is_storage ? spv::StorageClass::StorageBuffer : spv::StorageClass::Uniform;
    const Id struct_pointer_type{TypePointer(storage_class, struct_type)};
    const Id pointer_type = TypePointer(storage_class, data_type);
    const Id id{AddGlobalVariable(struct_pointer_type, storage_class)};
    Decorate(id, spv::Decoration::Binding, binding.unified);
    Decorate(id, spv::Decoration::DescriptorSet, 0U);
    if (is_storage && !is_written) {
        Decorate(id, spv::Decoration::NonWritable);
    }
    switch (buffer_type) {
    case Shader::BufferType::GdsBuffer:
        Name(id, "gds_buffer");
        break;
    case Shader::BufferType::ReadConstUbo:
        Name(id, "srt_flatbuf_ubo");
        break;
    case Shader::BufferType::SharedMemory:
        Name(id, "ssbo_shmem");
        break;
    default:
        Name(id, fmt::format("{}_{}", is_storage ? "ssbo" : "ubo", binding.buffer));
        break;
    }
    interfaces.push_back(id);
    return {id, pointer_type};
};

void EmitContext::DefineBuffers() {
    if (!profile.supports_robust_buffer_access && !info.has_readconst) {
        // In case ReadConstUbo has not already been bound by IR and is needed
        // to query buffer sizes, bind it now.
        info.buffers.push_back({
            .used_types = IR::Type::U32,
            .inline_cbuf = AmdGpu::Buffer::Null(),
            .buffer_type = BufferType::ReadConstUbo,
        });
    }
    for (const auto& desc : info.buffers) {
        const auto buf_sharp = desc.GetSharp(info);
        const bool is_storage = desc.IsStorage(buf_sharp, profile);

        // Define aliases depending on the shader usage.
        auto& spv_buffer = buffers.emplace_back(binding.buffer++, desc.buffer_type);
        if (True(desc.used_types & IR::Type::U32)) {
            spv_buffer[BufferAlias::U32] =
                DefineBuffer(is_storage, desc.is_written, 2, desc.buffer_type, U32[1]);
        }
        if (True(desc.used_types & IR::Type::F32)) {
            spv_buffer[BufferAlias::F32] =
                DefineBuffer(is_storage, desc.is_written, 2, desc.buffer_type, F32[1]);
        }
        if (True(desc.used_types & IR::Type::U16)) {
            spv_buffer[BufferAlias::U16] =
                DefineBuffer(is_storage, desc.is_written, 1, desc.buffer_type, U16);
        }
        if (True(desc.used_types & IR::Type::U8)) {
            spv_buffer[BufferAlias::U8] =
                DefineBuffer(is_storage, desc.is_written, 0, desc.buffer_type, U8);
        }
        ++binding.unified;
    }
}

spv::ImageFormat GetFormat(const AmdGpu::Image& image) {
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format32 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Uint) {
        return spv::ImageFormat::R32ui;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format32 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Sint) {
        return spv::ImageFormat::R32i;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format32 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Float) {
        return spv::ImageFormat::R32f;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format32_32 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Float) {
        return spv::ImageFormat::Rg32f;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format32_32 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Uint) {
        return spv::ImageFormat::Rg32ui;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format32_32_32_32 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Uint) {
        return spv::ImageFormat::Rgba32ui;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format16 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Float) {
        return spv::ImageFormat::R16f;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format16 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Uint) {
        return spv::ImageFormat::R16ui;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format16_16 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Float) {
        return spv::ImageFormat::Rg16f;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format16_16 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Snorm) {
        return spv::ImageFormat::Rg16Snorm;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format8_8 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Unorm) {
        return spv::ImageFormat::Rg8;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format16_16_16_16 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Float) {
        return spv::ImageFormat::Rgba16f;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format16_16_16_16 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Unorm) {
        return spv::ImageFormat::Rgba16;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format8 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Unorm) {
        return spv::ImageFormat::R8;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format8_8_8_8 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Unorm) {
        return spv::ImageFormat::Rgba8;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format8_8_8_8 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Uint) {
        return spv::ImageFormat::Rgba8ui;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format10_11_11 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Float) {
        return spv::ImageFormat::R11fG11fB10f;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format32_32_32_32 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Float) {
        return spv::ImageFormat::Rgba32f;
    }
    UNREACHABLE_MSG("Unknown storage format data_format={}, num_format={}", image.GetDataFmt(),
                    image.GetNumberFmt());
}

Id ImageType(EmitContext& ctx, const ImageResource& desc, Id sampled_type) {
    const auto image = desc.GetSharp(ctx.info);
    const auto format = desc.is_atomic ? GetFormat(image) : spv::ImageFormat::Unknown;
    const auto type = image.GetViewType(desc.is_array);
    const u32 sampled = desc.is_written ? 2 : 1;
    switch (type) {
    case AmdGpu::ImageType::Color1D:
        return ctx.TypeImage(sampled_type, spv::Dim::Dim1D, false, false, false, sampled, format);
    case AmdGpu::ImageType::Color1DArray:
        return ctx.TypeImage(sampled_type, spv::Dim::Dim1D, false, true, false, sampled, format);
    case AmdGpu::ImageType::Color2D:
        return ctx.TypeImage(sampled_type, spv::Dim::Dim2D, false, false, false, sampled, format);
    case AmdGpu::ImageType::Color2DArray:
        return ctx.TypeImage(sampled_type, spv::Dim::Dim2D, false, true, false, sampled, format);
    case AmdGpu::ImageType::Color2DMsaa:
        return ctx.TypeImage(sampled_type, spv::Dim::Dim2D, false, false, true, sampled, format);
    case AmdGpu::ImageType::Color3D:
        return ctx.TypeImage(sampled_type, spv::Dim::Dim3D, false, false, false, sampled, format);
    default:
        break;
    }
    throw InvalidArgument("Invalid texture type {}", type);
}

void EmitContext::DefineImagesAndSamplers() {
    for (const auto& image_desc : info.images) {
        const auto sharp = image_desc.GetSharp(info);
        const auto nfmt = sharp.GetNumberFmt();
        const bool is_integer = AmdGpu::IsInteger(nfmt);
        const bool is_storage = image_desc.is_written;
        const VectorIds& data_types = GetAttributeType(*this, nfmt);
        const Id sampled_type = data_types[1];
        const Id image_type{ImageType(*this, image_desc, sampled_type)};
        const Id pointer_type{TypePointer(spv::StorageClass::UniformConstant, image_type)};
        const Id id{AddGlobalVariable(pointer_type, spv::StorageClass::UniformConstant)};
        Decorate(id, spv::Decoration::Binding, binding.unified++);
        Decorate(id, spv::Decoration::DescriptorSet, 0U);
        Name(id, fmt::format("{}_{}{}", stage, "img", image_desc.sharp_idx));
        images.push_back({
            .data_types = &data_types,
            .id = id,
            .sampled_type = is_storage ? sampled_type : TypeSampledImage(image_type),
            .pointer_type = pointer_type,
            .image_type = image_type,
            .view_type = sharp.GetViewType(image_desc.is_array),
            .is_integer = is_integer,
            .is_storage = is_storage,
        });
        interfaces.push_back(id);
    }
    if (std::ranges::any_of(info.images, &ImageResource::is_atomic)) {
        image_u32 = TypePointer(spv::StorageClass::Image, U32[1]);
        image_f32 = TypePointer(spv::StorageClass::Image, F32[1]);
    }
    if (info.samplers.empty()) {
        return;
    }
    sampler_type = TypeSampler();
    sampler_pointer_type = TypePointer(spv::StorageClass::UniformConstant, sampler_type);
    for (const auto& samp_desc : info.samplers) {
        const Id id{AddGlobalVariable(sampler_pointer_type, spv::StorageClass::UniformConstant)};
        Decorate(id, spv::Decoration::Binding, binding.unified++);
        Decorate(id, spv::Decoration::DescriptorSet, 0U);
        Name(id, fmt::format("{}_{}{}", stage, "samp", samp_desc.sharp_idx));
        samplers.push_back(id);
        interfaces.push_back(id);
    }
}

void EmitContext::DefineSharedMemory() {
    if (!info.uses_shared) {
        return;
    }
    ASSERT(info.stage == Stage::Compute);
    const u32 shared_memory_size = runtime_info.cs_info.shared_memory_size;
    const u32 num_elements{Common::DivCeil(shared_memory_size, 4U)};
    const Id type{TypeArray(U32[1], ConstU32(num_elements))};
    shared_memory_u32_type = TypePointer(spv::StorageClass::Workgroup, type);
    shared_u32 = TypePointer(spv::StorageClass::Workgroup, U32[1]);
    shared_memory_u32 = AddGlobalVariable(shared_memory_u32_type, spv::StorageClass::Workgroup);
    Name(shared_memory_u32, "shared_mem");
    interfaces.push_back(shared_memory_u32);
}

Id EmitContext::DefineFloat32ToUfloatM5(u32 mantissa_bits, const std::string_view name) {
    // https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/util/format_r11g11b10f.h
    const auto func_type{TypeFunction(U32[1], F32[1])};
    const auto func{OpFunction(U32[1], spv::FunctionControlMask::MaskNone, func_type)};
    const auto value{OpFunctionParameter(F32[1])};
    Name(func, name);
    AddLabel();

    const auto raw_value{OpBitcast(U32[1], value)};
    const auto exponent{
        OpBitcast(S32[1], OpBitFieldSExtract(U32[1], raw_value, ConstU32(23U), ConstU32(8U)))};
    const auto sign{OpBitFieldUExtract(U32[1], raw_value, ConstU32(31U), ConstU32(1U))};

    const auto is_zero{OpLogicalOr(U1[1], OpIEqual(U1[1], raw_value, ConstU32(0U)),
                                   OpIEqual(U1[1], sign, ConstU32(1U)))};
    const auto is_nan{OpIsNan(U1[1], value)};
    const auto is_inf{OpIsInf(U1[1], value)};
    const auto is_denorm{OpSLessThanEqual(U1[1], exponent, ConstS32(-15))};

    const auto denorm_mantissa{OpConvertFToU(
        U32[1],
        OpRoundEven(F32[1], OpFMul(F32[1], value,
                                   ConstF32(static_cast<float>(1 << (mantissa_bits + 14))))))};
    const auto denorm_overflow{
        OpINotEqual(U1[1], OpShiftRightLogical(U32[1], denorm_mantissa, ConstU32(mantissa_bits)),
                    ConstU32(0U))};
    const auto denorm{
        OpSelect(U32[1], denorm_overflow, ConstU32(1U << mantissa_bits), denorm_mantissa)};

    const auto norm_mantissa{OpConvertFToU(
        U32[1],
        OpRoundEven(F32[1],
                    OpLdexp(F32[1], value,
                            OpISub(S32[1], ConstS32(static_cast<int>(mantissa_bits)), exponent))))};
    const auto norm_overflow{
        OpUGreaterThanEqual(U1[1], norm_mantissa, ConstU32(2U << mantissa_bits))};
    const auto norm_final_mantissa{OpBitwiseAnd(
        U32[1],
        OpSelect(U32[1], norm_overflow, OpShiftRightLogical(U32[1], norm_mantissa, ConstU32(1U)),
                 norm_mantissa),
        ConstU32((1U << mantissa_bits) - 1))};
    const auto norm_final_exponent{OpBitcast(
        U32[1],
        OpIAdd(S32[1],
               OpSelect(S32[1], norm_overflow, OpIAdd(S32[1], exponent, ConstS32(1)), exponent),
               ConstS32(15)))};
    const auto norm{OpBitFieldInsert(U32[1], norm_final_mantissa, norm_final_exponent,
                                     ConstU32(mantissa_bits), ConstU32(5U))};

    const auto result{OpSelect(U32[1], is_zero, ConstU32(0U),
                               OpSelect(U32[1], is_nan, ConstU32(31u << mantissa_bits | 1U),
                                        OpSelect(U32[1], is_inf, ConstU32(31U << mantissa_bits),
                                                 OpSelect(U32[1], is_denorm, denorm, norm))))};

    OpReturnValue(result);
    OpFunctionEnd();
    return func;
}

Id EmitContext::DefineUfloatM5ToFloat32(u32 mantissa_bits, const std::string_view name) {
    // https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/util/format_r11g11b10f.h
    const auto func_type{TypeFunction(F32[1], U32[1])};
    const auto func{OpFunction(F32[1], spv::FunctionControlMask::MaskNone, func_type)};
    const auto value{OpFunctionParameter(U32[1])};
    Name(func, name);
    AddLabel();

    const auto raw_mantissa{
        OpBitFieldUExtract(U32[1], value, ConstU32(0U), ConstU32(mantissa_bits))};
    const auto mantissa{OpConvertUToF(F32[1], raw_mantissa)};
    const auto exponent{OpBitcast(
        S32[1], OpBitFieldSExtract(U32[1], value, ConstU32(mantissa_bits), ConstU32(5U)))};

    const auto is_exp_neg_one{OpIEqual(U1[1], exponent, ConstS32(-1))};
    const auto is_exp_zero{OpIEqual(U1[1], exponent, ConstS32(0))};

    const auto is_zero{OpIEqual(U1[1], value, ConstU32(0u))};
    const auto is_nan{
        OpLogicalAnd(U1[1], is_exp_neg_one, OpINotEqual(U1[1], raw_mantissa, ConstU32(0u)))};
    const auto is_inf{
        OpLogicalAnd(U1[1], is_exp_neg_one, OpIEqual(U1[1], raw_mantissa, ConstU32(0u)))};
    const auto is_denorm{
        OpLogicalAnd(U1[1], is_exp_zero, OpINotEqual(U1[1], raw_mantissa, ConstU32(0u)))};

    const auto denorm{OpFMul(F32[1], mantissa, ConstF32(1.f / (1 << 20)))};
    const auto norm{OpLdexp(
        F32[1],
        OpFAdd(F32[1],
               OpFMul(F32[1], mantissa, ConstF32(1.f / static_cast<float>(1 << mantissa_bits))),
               ConstF32(1.f)),
        exponent)};

    const auto result{OpSelect(F32[1], is_zero, ConstF32(0.f),
                               OpSelect(F32[1], is_nan, ConstF32(NAN),
                                        OpSelect(F32[1], is_inf, ConstF32(INFINITY),
                                                 OpSelect(F32[1], is_denorm, denorm, norm))))};

    OpReturnValue(result);
    OpFunctionEnd();
    return func;
}

void EmitContext::DefineFunctions() {
    if (info.uses_pack_10_11_11) {
        f32_to_uf11 = DefineFloat32ToUfloatM5(6, "f32_to_uf11");
        f32_to_uf10 = DefineFloat32ToUfloatM5(5, "f32_to_uf10");
    }
    if (info.uses_unpack_10_11_11) {
        uf11_to_f32 = DefineUfloatM5ToFloat32(6, "uf11_to_f32");
        uf10_to_f32 = DefineUfloatM5ToFloat32(5, "uf10_to_f32");
    }
}

} // namespace Shader::Backend::SPIRV
