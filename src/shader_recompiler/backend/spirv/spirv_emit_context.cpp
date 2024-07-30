// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/div_ceil.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

#include <boost/container/static_vector.hpp>
#include <fmt/format.h>

#include <numbers>

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
    throw InvalidArgument("Invalid stage {}", u32(stage));
}

template <typename... Args>
void Name(EmitContext& ctx, Id object, std::string_view format_str, Args&&... args) {
    ctx.Name(object, fmt::format(fmt::runtime(format_str), StageName(ctx.stage),
                                 std::forward<Args>(args)...)
                         .c_str());
}

} // Anonymous namespace

EmitContext::EmitContext(const Profile& profile_, IR::Program& program, u32& binding_)
    : Sirit::Module(profile_.supported_spirv), info{program.info}, profile{profile_},
      stage{program.info.stage}, binding{binding_} {
    AddCapability(spv::Capability::Shader);
    DefineArithmeticTypes();
    DefineInterfaces(program);
    DefineBuffers(info);
    DefineImagesAndSamplers(info);
    DefineSharedMemory();
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
    if (info.uses_fp16) {
        F16[1] = Name(TypeFloat(16), "f16_id");
        U16 = Name(TypeUInt(16), "u16_id");
    }
    F32[1] = Name(TypeFloat(32), "f32_id");
    S32[1] = Name(TypeSInt(32), "i32_id");
    U32[1] = Name(TypeUInt(32), "u32_id");
    U64 = Name(TypeUInt(64), "u64_id");

    for (u32 i = 2; i <= 4; i++) {
        if (info.uses_fp16) {
            F16[i] = Name(TypeVector(F16[1], i), fmt::format("f16vec{}_id", i));
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

    full_result_i32x2 = Name(TypeStruct(S32[1], S32[1]), "full_result_i32x2");
    full_result_u32x2 = Name(TypeStruct(U32[1], U32[1]), "full_result_u32x2");
}

void EmitContext::DefineInterfaces(const IR::Program& program) {
    DefineInputs(program.info);
    DefineOutputs(program.info);
}

Id GetAttributeType(EmitContext& ctx, AmdGpu::NumberFormat fmt) {
    switch (fmt) {
    case AmdGpu::NumberFormat::Float:
    case AmdGpu::NumberFormat::Unorm:
    case AmdGpu::NumberFormat::Snorm:
    case AmdGpu::NumberFormat::SnormNz:
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
    case AmdGpu::NumberFormat::Snorm:
    case AmdGpu::NumberFormat::SnormNz:
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
    case Stage::Vertex: {
        vertex_index = DefineVariable(U32[1], spv::BuiltIn::VertexIndex, spv::StorageClass::Input);
        base_vertex = DefineVariable(U32[1], spv::BuiltIn::BaseVertex, spv::StorageClass::Input);
        instance_id = DefineVariable(U32[1], spv::BuiltIn::InstanceIndex, spv::StorageClass::Input);

        // Create push constants block for instance steps rates
        const Id struct_type{Name(TypeStruct(U32[1], U32[1]), "instance_step_rates")};
        Decorate(struct_type, spv::Decoration::Block);
        MemberName(struct_type, 0, "sr0");
        MemberName(struct_type, 1, "sr1");
        MemberDecorate(struct_type, 0, spv::Decoration::Offset, 0U);
        MemberDecorate(struct_type, 1, spv::Decoration::Offset, 4U);
        instance_step_rates = DefineVar(struct_type, spv::StorageClass::PushConstant);
        Name(instance_step_rates, "step_rates");
        interfaces.push_back(instance_step_rates);

        for (const auto& input : info.vs_inputs) {
            const Id type{GetAttributeType(*this, input.fmt)};
            if (input.instance_step_rate == Info::VsInput::InstanceIdType::OverStepRate0 ||
                input.instance_step_rate == Info::VsInput::InstanceIdType::OverStepRate1) {

                const u32 rate_idx =
                    input.instance_step_rate == Info::VsInput::InstanceIdType::OverStepRate0 ? 0
                                                                                             : 1;
                // Note that we pass index rather than Id
                input_params[input.binding] = {
                    rate_idx, input_u32,
                    U32[1],   input.num_components,
                    false,    input.instance_data_buf,
                };
            } else {
                Id id{DefineInput(type, input.binding)};
                if (input.instance_step_rate == Info::VsInput::InstanceIdType::Plain) {
                    Name(id, fmt::format("vs_instance_attr{}", input.binding));
                } else {
                    Name(id, fmt::format("vs_in_attr{}", input.binding));
                }
                input_params[input.binding] = GetAttributeInfo(input.fmt, id);
                interfaces.push_back(id);
            }
        }
        break;
    }
    case Stage::Fragment:
        subgroup_id = DefineVariable(U32[1], spv::BuiltIn::SubgroupId, spv::StorageClass::Input);
        subgroup_local_invocation_id = DefineVariable(
            U32[1], spv::BuiltIn::SubgroupLocalInvocationId, spv::StorageClass::Input);
        Decorate(subgroup_local_invocation_id, spv::Decoration::Flat);
        frag_coord = DefineVariable(F32[4], spv::BuiltIn::FragCoord, spv::StorageClass::Input);
        frag_depth = DefineVariable(F32[1], spv::BuiltIn::FragDepth, spv::StorageClass::Output);
        front_facing = DefineVariable(U1[1], spv::BuiltIn::FrontFacing, spv::StorageClass::Input);
        for (const auto& input : info.ps_inputs) {
            const u32 semantic = input.param_index;
            if (input.is_default) {
                input_params[semantic] = {MakeDefaultValue(*this, input.default_value), F32[1],
                                          F32[1], 4, true};
                continue;
            }
            const IR::Attribute param{IR::Attribute::Param0 + input.param_index};
            const u32 num_components = info.loads.NumComponents(param);
            const Id type{F32[num_components]};
            const Id id{DefineInput(type, semantic)};
            if (input.is_flat) {
                Decorate(id, spv::Decoration::Flat);
            }
            Name(id, fmt::format("fs_in_attr{}", semantic));
            input_params[semantic] = {id, input_f32, F32[1], num_components};
            interfaces.push_back(id);
        }
        break;
    case Stage::Compute:
        workgroup_id = DefineVariable(U32[3], spv::BuiltIn::WorkgroupId, spv::StorageClass::Input);
        local_invocation_id =
            DefineVariable(U32[3], spv::BuiltIn::LocalInvocationId, spv::StorageClass::Input);
        break;
    default:
        break;
    }
}

void EmitContext::DefineOutputs(const Info& info) {
    switch (stage) {
    case Stage::Vertex: {
        output_position = DefineVariable(F32[4], spv::BuiltIn::Position, spv::StorageClass::Output);
        const std::array<Id, 8> zero{f32_zero_value, f32_zero_value, f32_zero_value,
                                     f32_zero_value, f32_zero_value, f32_zero_value,
                                     f32_zero_value, f32_zero_value};
        const Id type{TypeArray(F32[1], ConstU32(8U))};
        const Id initializer{ConstantComposite(type, zero)};
        clip_distances = DefineVariable(type, spv::BuiltIn::ClipDistance, spv::StorageClass::Output,
                                        initializer);
        cull_distances = DefineVariable(type, spv::BuiltIn::CullDistance, spv::StorageClass::Output,
                                        initializer);
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
    }
    case Stage::Fragment:
        for (u32 i = 0; i < IR::NumRenderTargets; i++) {
            const IR::Attribute mrt{IR::Attribute::RenderTarget0 + i};
            if (!info.stores.GetAny(mrt)) {
                continue;
            }
            const u32 num_components = info.stores.NumComponents(mrt);
            frag_color[i] = DefineOutput(F32[num_components], i);
            frag_num_comp[i] = num_components;
            Name(frag_color[i], fmt::format("frag_color{}", i));
            interfaces.push_back(frag_color[i]);
        }
        break;
    default:
        break;
    }
}

void EmitContext::DefineBuffers(const Info& info) {
    boost::container::small_vector<Id, 8> type_ids;
    for (u32 i = 0; const auto& buffer : info.buffers) {
        const auto* data_types = True(buffer.used_types & IR::Type::F32) ? &F32 : &U32;
        const Id data_type = (*data_types)[1];
        const Id record_array_type{TypeArray(data_type, ConstU32(buffer.length))};
        const Id struct_type{TypeStruct(record_array_type)};
        if (std::ranges::find(type_ids, record_array_type.value, &Id::value) == type_ids.end()) {
            Decorate(record_array_type, spv::Decoration::ArrayStride, 4);
            const auto name =
                buffer.is_instance_data
                    ? fmt::format("{}_instance_data{}_{}{}", stage, i, 'f',
                                  sizeof(float) * CHAR_BIT)
                    : fmt::format("{}_cbuf_block_{}{}", stage, 'f', sizeof(float) * CHAR_BIT);
            Name(struct_type, name);
            Decorate(struct_type, spv::Decoration::Block);
            MemberName(struct_type, 0, "data");
            MemberDecorate(struct_type, 0, spv::Decoration::Offset, 0U);
        }
        type_ids.push_back(record_array_type);

        const auto storage_class =
            buffer.is_storage ? spv::StorageClass::StorageBuffer : spv::StorageClass::Uniform;
        const Id struct_pointer_type{TypePointer(storage_class, struct_type)};
        const Id pointer_type = TypePointer(storage_class, data_type);
        const Id id{AddGlobalVariable(struct_pointer_type, storage_class)};
        Decorate(id, spv::Decoration::Binding, binding);
        Decorate(id, spv::Decoration::DescriptorSet, 0U);
        Name(id, fmt::format("{}_{}", buffer.is_storage ? "ssbo" : "cbuf", buffer.sgpr_base));

        binding++;
        buffers.push_back({
            .id = id,
            .data_types = data_types,
            .pointer_type = pointer_type,
            .buffer = buffer.GetVsharp(info),
        });
        interfaces.push_back(id);
        i++;
    }
}

spv::ImageFormat GetFormat(const AmdGpu::Image& image) {
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format32 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Uint) {
        return spv::ImageFormat::R32ui;
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
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format16_16 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Float) {
        return spv::ImageFormat::Rg16f;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format8_8 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Unorm) {
        return spv::ImageFormat::Rg8;
    }
    if (image.GetDataFmt() == AmdGpu::DataFormat::Format16_16_16_16 &&
        image.GetNumberFmt() == AmdGpu::NumberFormat::Float) {
        return spv::ImageFormat::Rgba16f;
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
    const auto image = ctx.info.ReadUd<AmdGpu::Image>(desc.sgpr_base, desc.dword_offset);
    const auto format = desc.is_storage ? GetFormat(image) : spv::ImageFormat::Unknown;
    const u32 sampled = desc.is_storage ? 2 : 1;
    switch (desc.type) {
    case AmdGpu::ImageType::Color1D:
        return ctx.TypeImage(sampled_type, spv::Dim::Dim1D, false, false, false, sampled, format);
    case AmdGpu::ImageType::Color1DArray:
        return ctx.TypeImage(sampled_type, spv::Dim::Dim1D, false, true, false, sampled, format);
    case AmdGpu::ImageType::Color2D:
        return ctx.TypeImage(sampled_type, spv::Dim::Dim2D, false, false, false, sampled, format);
    case AmdGpu::ImageType::Color2DArray:
        return ctx.TypeImage(sampled_type, spv::Dim::Dim2D, false, true, false, sampled, format);
    case AmdGpu::ImageType::Color3D:
        return ctx.TypeImage(sampled_type, spv::Dim::Dim3D, false, false, false, sampled, format);
    case AmdGpu::ImageType::Cube:
        return ctx.TypeImage(sampled_type, spv::Dim::Cube, false, false, false, sampled, format);
    default:
        break;
    }
    throw InvalidArgument("Invalid texture type {}", desc.type);
}

void EmitContext::DefineImagesAndSamplers(const Info& info) {
    for (const auto& image_desc : info.images) {
        const VectorIds* data_types = [&] {
            switch (image_desc.nfmt) {
            case AmdGpu::NumberFormat::Uint:
                return &U32;
            case AmdGpu::NumberFormat::Sint:
                return &S32;
            default:
                return &F32;
            }
        }();
        const Id sampled_type = data_types->Get(1);
        const Id image_type{ImageType(*this, image_desc, sampled_type)};
        const Id pointer_type{TypePointer(spv::StorageClass::UniformConstant, image_type)};
        const Id id{AddGlobalVariable(pointer_type, spv::StorageClass::UniformConstant)};
        Decorate(id, spv::Decoration::Binding, binding);
        Decorate(id, spv::Decoration::DescriptorSet, 0U);
        Name(id, fmt::format("{}_{}{}_{:02x}", stage, "img", image_desc.sgpr_base,
                             image_desc.dword_offset));
        images.push_back({
            .id = id,
            .data_types = data_types,
            .sampled_type = image_desc.is_storage ? sampled_type : TypeSampledImage(image_type),
            .pointer_type = pointer_type,
            .image_type = image_type,
        });
        interfaces.push_back(id);
        ++binding;
    }

    image_u32 = TypePointer(spv::StorageClass::Image, U32[1]);

    if (info.samplers.empty()) {
        return;
    }

    sampler_type = TypeSampler();
    sampler_pointer_type = TypePointer(spv::StorageClass::UniformConstant, sampler_type);
    for (const auto& samp_desc : info.samplers) {
        const Id id{AddGlobalVariable(sampler_pointer_type, spv::StorageClass::UniformConstant)};
        Decorate(id, spv::Decoration::Binding, binding);
        Decorate(id, spv::Decoration::DescriptorSet, 0U);
        Name(id, fmt::format("{}_{}{}_{:02x}", stage, "samp", samp_desc.sgpr_base,
                             samp_desc.dword_offset));
        samplers.push_back(id);
        interfaces.push_back(id);
        ++binding;
    }
}

void EmitContext::DefineSharedMemory() {
    static constexpr size_t DefaultSharedMemSize = 16_KB;
    if (!info.uses_shared) {
        return;
    }
    if (info.shared_memory_size == 0) {
        info.shared_memory_size = DefaultSharedMemSize;
    }
    const auto make{[&](Id element_type, u32 element_size) {
        const u32 num_elements{Common::DivCeil(info.shared_memory_size, element_size)};
        const Id array_type{TypeArray(element_type, ConstU32(num_elements))};
        Decorate(array_type, spv::Decoration::ArrayStride, element_size);

        const Id struct_type{TypeStruct(array_type)};
        MemberDecorate(struct_type, 0U, spv::Decoration::Offset, 0U);
        Decorate(struct_type, spv::Decoration::Block);

        const Id pointer{TypePointer(spv::StorageClass::Workgroup, struct_type)};
        const Id element_pointer{TypePointer(spv::StorageClass::Workgroup, element_type)};
        const Id variable{AddGlobalVariable(pointer, spv::StorageClass::Workgroup)};
        Decorate(variable, spv::Decoration::Aliased);
        interfaces.push_back(variable);

        return std::make_tuple(variable, element_pointer, pointer);
    }};
    if (profile.support_explicit_workgroup_layout) {
        AddExtension("SPV_KHR_workgroup_memory_explicit_layout");
        AddCapability(spv::Capability::WorkgroupMemoryExplicitLayoutKHR);
        if (info.uses_shared_u8) {
            AddCapability(spv::Capability::WorkgroupMemoryExplicitLayout8BitAccessKHR);
            std::tie(shared_memory_u8, shared_u8, std::ignore) = make(U8, 1);
        }
        if (info.uses_shared_u16) {
            AddCapability(spv::Capability::WorkgroupMemoryExplicitLayout16BitAccessKHR);
            std::tie(shared_memory_u16, shared_u16, std::ignore) = make(U16, 2);
        }
        std::tie(shared_memory_u32, shared_u32, shared_memory_u32_type) = make(U32[1], 4);
        std::tie(shared_memory_u32x2, shared_u32x2, std::ignore) = make(U32[2], 8);
        std::tie(shared_memory_u32x4, shared_u32x4, std::ignore) = make(U32[4], 16);
        return;
    }
    const u32 num_elements{Common::DivCeil(info.shared_memory_size, 4U)};
    const Id type{TypeArray(U32[1], ConstU32(num_elements))};
    shared_memory_u32_type = TypePointer(spv::StorageClass::Workgroup, type);

    shared_u32 = TypePointer(spv::StorageClass::Workgroup, U32[1]);
    shared_memory_u32 = AddGlobalVariable(shared_memory_u32_type, spv::StorageClass::Workgroup);
    interfaces.push_back(shared_memory_u32);
}

} // namespace Shader::Backend::SPIRV
