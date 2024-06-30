// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {

Id EmitSelectU1(EmitContext& ctx, Id cond, Id true_value, Id false_value) {
    return ctx.OpSelect(ctx.U1[1], cond, true_value, false_value);
}

Id EmitSelectU8(EmitContext&, Id, Id, Id) {
    UNREACHABLE_MSG("SPIR-V Instruction");
}

Id EmitSelectU16(EmitContext& ctx, Id cond, Id true_value, Id false_value) {
    return ctx.OpSelect(ctx.U16, cond, true_value, false_value);
}

Id EmitSelectU32(EmitContext& ctx, Id cond, Id true_value, Id false_value) {
    return ctx.OpSelect(ctx.U32[1], cond, true_value, false_value);
}

Id EmitSelectU64(EmitContext& ctx, Id cond, Id true_value, Id false_value) {
    return ctx.OpSelect(ctx.U64, cond, true_value, false_value);
}

Id EmitSelectF16(EmitContext& ctx, Id cond, Id true_value, Id false_value) {
    return ctx.OpSelect(ctx.F16[1], cond, true_value, false_value);
}

Id EmitSelectF32(EmitContext& ctx, Id cond, Id true_value, Id false_value) {
    return ctx.OpSelect(ctx.F32[1], cond, true_value, false_value);
}

Id EmitSelectF64(EmitContext& ctx, Id cond, Id true_value, Id false_value) {
    return ctx.OpSelect(ctx.F64[1], cond, true_value, false_value);
}

} // namespace Shader::Backend::SPIRV
