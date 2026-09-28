// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/container/static_vector.hpp>
#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"
#include "shader_recompiler/ir/microinstruction.h"

namespace Shader::Backend::SPIRV {

struct ImageOperands {
    void Add(spv::ImageOperandsMask new_mask, Id value) {
        if (!Sirit::ValidId(value)) {
            return;
        }
        mask = static_cast<spv::ImageOperandsMask>(static_cast<u32>(mask) |
                                                   static_cast<u32>(new_mask));
        operands.push_back(value);
    }
    void Add(spv::ImageOperandsMask new_mask, Id value1, Id value2) {
        mask = static_cast<spv::ImageOperandsMask>(static_cast<u32>(mask) |
                                                   static_cast<u32>(new_mask));
        operands.push_back(value1);
        operands.push_back(value2);
    }

    void AddOffset(EmitContext& ctx, const IR::Value& offset,
                   bool can_use_runtime_offsets = false) {
        if (offset.IsEmpty()) {
            return;
        }
        if (offset.IsImmediate()) {
            const s32 operand = offset.U32();
            Add(spv::ImageOperandsMask::ConstOffset, ctx.ConstS32(operand));
            return;
        }
        IR::Inst* const inst{offset.Inst()};
        if (inst->AreAllArgsImmediates()) {
            switch (inst->GetOpcode()) {
            case IR::Opcode::CompositeConstructU32x2:
                Add(spv::ImageOperandsMask::ConstOffset,
                    ctx.ConstS32(static_cast<s32>(inst->Arg(0).U32()),
                                 static_cast<s32>(inst->Arg(1).U32())));
                return;
            case IR::Opcode::CompositeConstructU32x3:
                Add(spv::ImageOperandsMask::ConstOffset,
                    ctx.ConstS32(static_cast<s32>(inst->Arg(0).U32()),
                                 static_cast<s32>(inst->Arg(1).U32()),
                                 static_cast<s32>(inst->Arg(2).U32())));
                return;
            default:
                break;
            }
        }
        if (can_use_runtime_offsets) {
            Add(spv::ImageOperandsMask::Offset, ctx.Def(offset));
        } else {
            LOG_WARNING(Render_Vulkan,
                        "Runtime offset provided to unsupported image sample instruction");
        }
    }

    void AddDerivatives(EmitContext& ctx, Id derivatives_dx, Id derivatives_dy) {
        if (!Sirit::ValidId(derivatives_dx) || !Sirit::ValidId(derivatives_dy)) {
            return;
        }
        Add(spv::ImageOperandsMask::Grad, derivatives_dx, derivatives_dy);
    }

    spv::ImageOperandsMask mask{};
    boost::container::static_vector<Id, 4> operands;
};

template <bool is_float>
static Id FixImageCoords(EmitContext& ctx, Id coords, AmdGpu::ImageType image_type) {
    const auto coord_type = is_float ? ctx.F32 : ctx.U32;
    const auto zero = is_float ? ctx.f32_zero_value : ctx.u32_zero_value;

    switch (image_type) {
    case AmdGpu::ImageType::Color1D: {
        // Lowered to 2D with height 1
        const auto x = coords;
        return ctx.OpCompositeConstruct(coord_type[2], x, zero);
    }
    case AmdGpu::ImageType::Color1DArray: {
        // Lowered to 2D array with height 1
        const auto x = ctx.OpCompositeExtract(coord_type[1], coords, 0U);
        const auto slice = ctx.OpCompositeExtract(coord_type[1], coords, 1U);
        return ctx.OpCompositeConstruct(coord_type[3], x, zero, slice);
    }
    case AmdGpu::ImageType::Cube: {
        // Lowered to 2D array
        if (is_float) {
            const auto x = ctx.OpCompositeExtract(coord_type[1], coords, 0U);
            const auto y = ctx.OpCompositeExtract(coord_type[1], coords, 1U);
            const auto face = ctx.OpCompositeExtract(coord_type[1], coords, 2U);

            // AMD cube math results in coordinates in the range [1.0, 2.0]. We need
            // to convert this to the range [0.0, 1.0] to get correct results.
            const auto one = ctx.ConstF32(1.f);
            const auto fixed_x = ctx.OpFSub(coord_type[1], x, one);
            const auto fixed_y = ctx.OpFSub(coord_type[1], y, one);
            const auto fixed_face = ctx.OpFma(
                coord_type[1],
                ctx.OpFloor(coord_type[1], ctx.OpFDiv(coord_type[1], face, ctx.ConstF32(8.f))),
                ctx.ConstF32(-2.f), face);
            return ctx.OpCompositeConstruct(coord_type[3], fixed_x, fixed_y, fixed_face);
        }
        return coords;
    }
    default:
        return coords;
    }
}

Id EmitImageHandle(EmitContext& ctx, Id, Id) {
    UNREACHABLE_MSG("Unreachable instruction");
}

Id EmitImageSampleRaw(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address1, Id address2,
                      Id address3, Id address4) {
    UNREACHABLE_MSG("Unreachable instruction");
}

Id EmitImageSampleImplicitLod(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id bias,
                              const IR::Value& offset) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id image = ctx.OpLoad(texture.image_type, texture.id);
    const Id result_type = texture.data_types->Get(4);
    const Id sampler = ctx.OpLoad(ctx.sampler_type, ctx.samplers[handle >> 16]);
    const Id sampled_image = ctx.OpSampledImage(texture.sampled_type, image, sampler);
    const Id fixed_coords = FixImageCoords<true>(ctx, coords, texture.view_type);
    ImageOperands operands;
    operands.Add(spv::ImageOperandsMask::Bias, bias);
    operands.AddOffset(ctx, offset);
    const Id sample = ctx.OpImageSampleImplicitLod(result_type, sampled_image, fixed_coords,
                                                   operands.mask, operands.operands);
    return texture.is_integer ? ctx.OpBitcast(ctx.F32[4], sample) : sample;
}

Id EmitImageSampleExplicitLod(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id lod,
                              const IR::Value& offset) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id image = ctx.OpLoad(texture.image_type, texture.id);
    const Id result_type = texture.data_types->Get(4);
    const Id sampler = ctx.OpLoad(ctx.sampler_type, ctx.samplers[handle >> 16]);
    const Id sampled_image = ctx.OpSampledImage(texture.sampled_type, image, sampler);
    const Id fixed_coords = FixImageCoords<true>(ctx, coords, texture.view_type);
    ImageOperands operands;
    operands.Add(spv::ImageOperandsMask::Lod, lod);
    operands.AddOffset(ctx, offset);
    const Id sample = ctx.OpImageSampleExplicitLod(result_type, sampled_image, fixed_coords,
                                                   operands.mask, operands.operands);
    return texture.is_integer ? ctx.OpBitcast(ctx.F32[4], sample) : sample;
}

Id EmitImageSampleDrefImplicitLod(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id dref,
                                  Id bias, const IR::Value& offset) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id image = ctx.OpLoad(texture.image_type, texture.id);
    const Id result_type = texture.data_types->Get(1);
    const Id sampler = ctx.OpLoad(ctx.sampler_type, ctx.samplers[handle >> 16]);
    const Id sampled_image = ctx.OpSampledImage(texture.sampled_type, image, sampler);
    const Id fixed_coords = FixImageCoords<true>(ctx, coords, texture.view_type);
    ImageOperands operands;
    operands.Add(spv::ImageOperandsMask::Bias, bias);
    operands.AddOffset(ctx, offset);
    const Id sample = ctx.OpImageSampleDrefImplicitLod(result_type, sampled_image, fixed_coords,
                                                       dref, operands.mask, operands.operands);
    const Id sample_typed = texture.is_integer ? ctx.OpBitcast(ctx.F32[1], sample) : sample;
    return ctx.OpCompositeConstruct(ctx.F32[4], sample_typed, ctx.f32_zero_value,
                                    ctx.f32_zero_value, ctx.f32_zero_value);
}

Id EmitImageSampleDrefExplicitLod(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id dref,
                                  Id lod, const IR::Value& offset) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id image = ctx.OpLoad(texture.image_type, texture.id);
    const Id result_type = texture.data_types->Get(1);
    const Id sampler = ctx.OpLoad(ctx.sampler_type, ctx.samplers[handle >> 16]);
    const Id sampled_image = ctx.OpSampledImage(texture.sampled_type, image, sampler);
    const Id fixed_coords = FixImageCoords<true>(ctx, coords, texture.view_type);
    ImageOperands operands;
    operands.Add(spv::ImageOperandsMask::Lod, lod);
    operands.AddOffset(ctx, offset);
    const Id sample = ctx.OpImageSampleDrefExplicitLod(result_type, sampled_image, fixed_coords,
                                                       dref, operands.mask, operands.operands);
    const Id sample_typed = texture.is_integer ? ctx.OpBitcast(ctx.F32[1], sample) : sample;
    return ctx.OpCompositeConstruct(ctx.F32[4], sample_typed, ctx.f32_zero_value,
                                    ctx.f32_zero_value, ctx.f32_zero_value);
}

Id EmitImageGather(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords,
                   const IR::Value& offset) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id image = ctx.OpLoad(texture.image_type, texture.id);
    const Id result_type = texture.data_types->Get(4);
    const Id sampler = ctx.OpLoad(ctx.sampler_type, ctx.samplers[handle >> 16]);
    const Id sampled_image = ctx.OpSampledImage(texture.sampled_type, image, sampler);
    const Id fixed_coords = FixImageCoords<true>(ctx, coords, texture.view_type);
    const u32 comp = inst->Flags<IR::TextureInstInfo>().gather_comp.Value();
    ImageOperands operands;
    operands.AddOffset(ctx, offset, true);
    const Id texels = ctx.OpImageGather(result_type, sampled_image, fixed_coords,
                                        ctx.ConstU32(comp), operands.mask, operands.operands);
    return texture.is_integer ? ctx.OpBitcast(ctx.F32[4], texels) : texels;
}

Id EmitImageGatherDref(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords,
                       const IR::Value& offset, Id dref) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id image = ctx.OpLoad(texture.image_type, texture.id);
    const Id result_type = texture.data_types->Get(4);
    const Id sampler = ctx.OpLoad(ctx.sampler_type, ctx.samplers[handle >> 16]);
    const Id sampled_image = ctx.OpSampledImage(texture.sampled_type, image, sampler);
    const Id fixed_coords = FixImageCoords<true>(ctx, coords, texture.view_type);
    ImageOperands operands;
    operands.AddOffset(ctx, offset, true);
    const Id texels = ctx.OpImageDrefGather(result_type, sampled_image, fixed_coords, dref,
                                            operands.mask, operands.operands);
    return texture.is_integer ? ctx.OpBitcast(ctx.F32[4], texels) : texels;
}

Id EmitImageQueryDimensions(EmitContext& ctx, IR::Inst* inst, u32 handle, Id lod, bool has_mips) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id image = ctx.OpLoad(texture.image_type, texture.id);
    const Id zero = ctx.u32_zero_value;
    const auto mips{[&] { return has_mips ? ctx.OpImageQueryLevels(ctx.U32[1], image) : zero; }};
    const bool uses_lod{texture.view_type != AmdGpu::ImageType::Color2DMsaa && !texture.is_storage};
    const auto query{[&](Id type) {
        return uses_lod ? ctx.OpImageQuerySizeLod(type, image, lod)
                        : ctx.OpImageQuerySize(type, image);
    }};
    switch (texture.view_type) {
    case AmdGpu::ImageType::Color1D: {
        const auto width = ctx.OpCompositeExtract(ctx.U32[1], query(ctx.U32[2]), 0U);
        return ctx.OpCompositeConstruct(ctx.U32[4], width, zero, zero, mips());
    }
    case AmdGpu::ImageType::Color1DArray: {
        const auto base = query(ctx.U32[3]);
        const auto width = ctx.OpCompositeExtract(ctx.U32[1], base, 0U);
        const auto slices = ctx.OpCompositeExtract(ctx.U32[1], base, 2U);
        return ctx.OpCompositeConstruct(ctx.U32[4], width, slices, zero, mips());
    }
    case AmdGpu::ImageType::Color2D:
    case AmdGpu::ImageType::Color2DMsaa:
        return ctx.OpCompositeConstruct(ctx.U32[4], query(ctx.U32[2]), zero, mips());
    case AmdGpu::ImageType::Color2DArray:
    case AmdGpu::ImageType::Cube:
    case AmdGpu::ImageType::Color3D:
        return ctx.OpCompositeConstruct(ctx.U32[4], query(ctx.U32[3]), mips());
    default:
        UNREACHABLE_MSG("SPIR-V Instruction");
    }
}

Id EmitImageQueryLod(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id image = ctx.OpLoad(texture.image_type, texture.id);
    const Id sampler = ctx.OpLoad(ctx.sampler_type, ctx.samplers[handle >> 16]);
    const Id sampled_image = ctx.OpSampledImage(texture.sampled_type, image, sampler);
    const Id fixed_coords = FixImageCoords<true>(ctx, coords, texture.view_type);
    return ctx.OpImageQueryLod(ctx.F32[2], sampled_image, fixed_coords);
}

Id EmitImageGradient(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id derivatives_dx,
                     Id derivatives_dy, const IR::Value& offset, const IR::Value& lod_clamp) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id image = ctx.OpLoad(texture.image_type, texture.id);
    const Id result_type = texture.data_types->Get(4);
    const Id sampler = ctx.OpLoad(ctx.sampler_type, ctx.samplers[handle >> 16]);
    const Id sampled_image = ctx.OpSampledImage(texture.sampled_type, image, sampler);
    const Id fixed_coords = FixImageCoords<true>(ctx, coords, texture.view_type);
    ImageOperands operands;
    operands.AddDerivatives(ctx, derivatives_dx, derivatives_dy);
    operands.AddOffset(ctx, offset);
    const Id sample = ctx.OpImageSampleExplicitLod(result_type, sampled_image, fixed_coords,
                                                   operands.mask, operands.operands);
    return texture.is_integer ? ctx.OpBitcast(ctx.F32[4], sample) : sample;
}

Id EmitImageRead(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id lod, Id ms) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id color_type = texture.data_types->Get(4);
    const Id fixed_coords = FixImageCoords<false>(ctx, coords, texture.view_type);
    ImageOperands operands;
    Id texel;
    if (!texture.is_storage) {
        const Id image = ctx.OpLoad(texture.image_type, texture.id);
        if (texture.view_type == AmdGpu::ImageType::Color2DMsaa) {
            // GCN hardware wraps out-of-range MSAA sample indices
            if (Sirit::ValidId(ms)) {
                const Id sample_count = ctx.OpImageQuerySamples(ctx.U32[1], image);
                const Id wrapped_ms = ctx.OpUMod(ctx.U32[1], ms, sample_count);
                operands.Add(spv::ImageOperandsMask::Sample, wrapped_ms);
            }
        } else {
            if (Sirit::ValidId(ms)) {
                LOG_ERROR(Render_Recompiler, "image is not MS but ms operand is provided");
            }
            operands.Add(spv::ImageOperandsMask::Lod, lod);
            operands.Add(spv::ImageOperandsMask::Sample, ms);
        }
        texel = ctx.OpImageFetch(color_type, image, fixed_coords, operands.mask, operands.operands);
    } else {
        Id image_ptr = texture.id;
        if (ctx.profile.supports_image_load_store_lod) {
            operands.Add(spv::ImageOperandsMask::Lod, lod);
        } else if (Sirit::ValidId(lod)) {
#if 1
            // It's confusing what interactions will cause this code path so leave it as
            // unreachable until a case is found.
            // Normally IMAGE_LOAD_MIP should translate -> OpImageFetch
            UNREACHABLE_MSG("Unsupported ImageRead with Lod");
#else
            LOG_WARNING(Render, "Fallback for ImageRead with LOD");
            ASSERT(texture.mip_fallback_mode == MipStorageFallbackMode::DynamicIndex);
            const Id single_image_ptr_type =
                ctx.TypePointer(spv::StorageClass::UniformConstant, texture.image_type);
            image_ptr = ctx.OpAccessChain(single_image_ptr_type, image_ptr, std::array{lod});
#endif
        }
        const Id image = ctx.OpLoad(texture.image_type, image_ptr);
        texel = ctx.OpImageRead(color_type, image, fixed_coords, operands.mask, operands.operands);
    }
    return texture.is_integer ? ctx.OpBitcast(ctx.F32[4], texel) : texel;
}

void EmitImageWrite(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id lod, Id ms,
                    Id color) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    Id image_ptr = texture.id;
    const Id color_type = texture.data_types->Get(4);
    const Id fixed_coords = FixImageCoords<false>(ctx, coords, texture.view_type);
    ImageOperands operands;
    operands.Add(spv::ImageOperandsMask::Sample, ms);
    if (ctx.profile.supports_image_load_store_lod) {
        operands.Add(spv::ImageOperandsMask::Lod, lod);
    } else if (Sirit::ValidId(lod)) {
        LOG_WARNING(Render, "Fallback for ImageWrite with LOD");
        ASSERT(texture.mip_fallback_mode == MipStorageFallbackMode::DynamicIndex);
        const Id single_image_ptr_type =
            ctx.TypePointer(spv::StorageClass::UniformConstant, texture.image_type);
        image_ptr = ctx.OpAccessChain(single_image_ptr_type, image_ptr, std::array{lod});
    }
    const Id image = ctx.OpLoad(texture.image_type, image_ptr);
    const Id texel = texture.is_integer ? ctx.OpBitcast(color_type, color) : color;
    ctx.OpImageWrite(image, fixed_coords, texel, operands.mask, operands.operands);
}

static Id SelectCubeResult(EmitContext& ctx, Id x, Id y, Id z, Id x_res, Id y_res, Id z_res) {
    const auto abs_x = ctx.OpFAbs(ctx.F32[1], x);
    const auto abs_y = ctx.OpFAbs(ctx.F32[1], y);
    const auto abs_z = ctx.OpFAbs(ctx.F32[1], z);

    const auto z_face_cond{ctx.OpLogicalAnd(ctx.U1[1],
                                            ctx.OpFOrdGreaterThanEqual(ctx.U1[1], abs_z, abs_x),
                                            ctx.OpFOrdGreaterThanEqual(ctx.U1[1], abs_z, abs_y))};
    const auto y_face_cond{ctx.OpFOrdGreaterThanEqual(ctx.U1[1], abs_y, abs_x)};

    return ctx.OpSelect(ctx.F32[1], z_face_cond, z_res,
                        ctx.OpSelect(ctx.F32[1], y_face_cond, y_res, x_res));
}

Id EmitCubeFaceIndex(EmitContext& ctx, IR::Inst* inst, Id x, Id y, Id z) {
    if (ctx.profile.supports_native_cube_calc) {
        return ctx.OpCubeFaceIndexAMD(ctx.F32[1], ctx.OpCompositeConstruct(ctx.F32[3], x, y, z));
    }

    const auto x_neg_cond{ctx.OpFOrdLessThan(ctx.U1[1], x, ctx.f32_zero_value)};
    const auto y_neg_cond{ctx.OpFOrdLessThan(ctx.U1[1], y, ctx.f32_zero_value)};
    const auto z_neg_cond{ctx.OpFOrdLessThan(ctx.U1[1], z, ctx.f32_zero_value)};
    const auto x_face{ctx.OpSelect(ctx.F32[1], x_neg_cond, ctx.ConstF32(1.f), ctx.ConstF32(0.f))};
    const auto y_face{ctx.OpSelect(ctx.F32[1], y_neg_cond, ctx.ConstF32(3.f), ctx.ConstF32(2.f))};
    const auto z_face{ctx.OpSelect(ctx.F32[1], z_neg_cond, ctx.ConstF32(5.f), ctx.ConstF32(4.f))};

    return SelectCubeResult(ctx, x, y, z, x_face, y_face, z_face);
}

Id EmitCubeFaceCoordS(EmitContext& ctx, IR::Inst* inst, Id x, Id y, Id z) {
    const auto x_neg_cond{ctx.OpFOrdLessThan(ctx.U1[1], x, ctx.f32_zero_value)};
    const auto z_neg_cond{ctx.OpFOrdLessThan(ctx.U1[1], z, ctx.f32_zero_value)};
    const auto x_sc{ctx.OpSelect(ctx.F32[1], x_neg_cond, z, ctx.OpFNegate(ctx.F32[1], z))};
    const auto y_sc{x};
    const auto z_sc{ctx.OpSelect(ctx.F32[1], z_neg_cond, ctx.OpFNegate(ctx.F32[1], x), x)};

    return SelectCubeResult(ctx, x, y, z, x_sc, y_sc, z_sc);
}

Id EmitCubeFaceCoordT(EmitContext& ctx, IR::Inst* inst, Id x, Id y, Id z) {
    const auto y_neg_cond{ctx.OpFOrdLessThan(ctx.U1[1], y, ctx.f32_zero_value)};
    const auto x_z_tc{ctx.OpFNegate(ctx.F32[1], y)};
    const auto y_tc{ctx.OpSelect(ctx.F32[1], y_neg_cond, ctx.OpFNegate(ctx.F32[1], z), z)};

    return SelectCubeResult(ctx, x, y, z, x_z_tc, y_tc, x_z_tc);
}

Id EmitCubeFaceMajorAxis(EmitContext& ctx, IR::Inst* inst, Id x, Id y, Id z) {
    const auto two{ctx.ConstF32(2.f)};
    const auto x_major_axis{ctx.OpFMul(ctx.F32[1], x, two)};
    const auto y_major_axis{ctx.OpFMul(ctx.F32[1], y, two)};
    const auto z_major_axis{ctx.OpFMul(ctx.F32[1], z, two)};

    return SelectCubeResult(ctx, x, y, z, x_major_axis, y_major_axis, z_major_axis);
}

} // namespace Shader::Backend::SPIRV
