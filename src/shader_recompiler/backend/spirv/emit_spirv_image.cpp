// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {

Id EmitImageSampleImplicitLod(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id bias_lc,
                              Id offset) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id image = ctx.OpLoad(texture.image_type, texture.id);
    const Id sampler = ctx.OpLoad(ctx.sampler_type, ctx.samplers[handle >> 16]);
    const Id sampled_image = ctx.OpSampledImage(texture.sampled_type, image, sampler);
    return ctx.OpImageSampleImplicitLod(ctx.F32[4], sampled_image, coords);
}

Id EmitImageSampleExplicitLod(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id bias_lc,
                              Id offset) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id image = ctx.OpLoad(texture.image_type, texture.id);
    const Id sampler = ctx.OpLoad(ctx.sampler_type, ctx.samplers[handle >> 16]);
    const Id sampled_image = ctx.OpSampledImage(texture.sampled_type, image, sampler);
    return ctx.OpImageSampleExplicitLod(ctx.F32[4], sampled_image, coords,
                                        spv::ImageOperandsMask::Lod, ctx.ConstF32(0.f));
}

Id EmitImageSampleDrefImplicitLod(EmitContext& ctx, IR::Inst* inst, const IR::Value& index,
                                  Id coords, Id dref, Id bias_lc, const IR::Value& offset) {
    throw NotImplementedException("SPIR-V Instruction");
}

Id EmitImageSampleDrefExplicitLod(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id dref,
                                  Id bias_lc, Id offset) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id image = ctx.OpLoad(texture.image_type, texture.id);
    const Id sampler = ctx.OpLoad(ctx.sampler_type, ctx.samplers[handle >> 16]);
    const Id sampled_image = ctx.OpSampledImage(texture.sampled_type, image, sampler);
    return ctx.OpImageSampleDrefExplicitLod(ctx.F32[1], sampled_image, coords, dref,
                                            spv::ImageOperandsMask::Lod, ctx.ConstF32(0.f));
}

Id EmitImageGather(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords,
                   const IR::Value& offset, const IR::Value& offset2) {
    throw NotImplementedException("SPIR-V Instruction");
}

Id EmitImageGatherDref(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords,
                       const IR::Value& offset, const IR::Value& offset2, Id dref) {
    throw NotImplementedException("SPIR-V Instruction");
}

Id EmitImageFetch(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords, Id offset,
                  Id lod, Id ms) {
    throw NotImplementedException("SPIR-V Instruction");
}

Id EmitImageQueryDimensions(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id lod,
                            const IR::Value& skip_mips_val) {
    throw NotImplementedException("SPIR-V Instruction");
}

Id EmitImageQueryLod(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords) {
    throw NotImplementedException("SPIR-V Instruction");
}

Id EmitImageGradient(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords,
                     Id derivatives, const IR::Value& offset, Id lod_clamp) {
    throw NotImplementedException("SPIR-V Instruction");
}

Id EmitImageRead(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords) {
    throw NotImplementedException("SPIR-V Instruction");
}

void EmitImageWrite(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords, Id color) {
    throw NotImplementedException("SPIR-V Instruction");
}

} // namespace Shader::Backend::SPIRV
