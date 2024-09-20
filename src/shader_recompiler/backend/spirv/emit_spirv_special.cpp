// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <iterator>
#include <boost/container/small_vector.hpp>
#include "common/assert.h"
#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"
#include "shader_recompiler/ir/debug_print.h"
#include "shader_recompiler/ir/opcodes.h"

namespace Shader::Backend::SPIRV {

void EmitPrologue(EmitContext& ctx) {
    ctx.DefineBufferOffsets();
}

void ConvertDepthMode(EmitContext& ctx) {
    const Id type{ctx.F32[1]};
    const Id position{ctx.OpLoad(ctx.F32[4], ctx.output_position)};
    const Id z{ctx.OpCompositeExtract(type, position, 2u)};
    const Id w{ctx.OpCompositeExtract(type, position, 3u)};
    const Id screen_depth{ctx.OpFMul(type, ctx.OpFAdd(type, z, w), ctx.Constant(type, 0.5f))};
    const Id vector{ctx.OpCompositeInsert(ctx.F32[4], screen_depth, position, 2u)};
    ctx.OpStore(ctx.output_position, vector);
}

void EmitEpilogue(EmitContext& ctx) {
    if (ctx.stage == Stage::Vertex && ctx.runtime_info.vs_info.emulate_depth_negative_one_to_one) {
        ConvertDepthMode(ctx);
    }
}

void EmitDiscard(EmitContext& ctx) {
    ctx.OpDemoteToHelperInvocationEXT();
}

void EmitDiscardCond(EmitContext& ctx, Id condition) {
    const Id kill_label{ctx.OpLabel()};
    const Id merge_label{ctx.OpLabel()};
    ctx.OpSelectionMerge(merge_label, spv::SelectionControlMask::MaskNone);
    ctx.OpBranchConditional(condition, kill_label, merge_label);
    ctx.AddLabel(kill_label);
    ctx.OpDemoteToHelperInvocationEXT();
    ctx.OpBranch(merge_label);
    ctx.AddLabel(merge_label);
}

void EmitEmitVertex(EmitContext& ctx, const IR::Value& stream) {
    throw NotImplementedException("Geometry streams");
}

void EmitEndPrimitive(EmitContext& ctx, const IR::Value& stream) {
    throw NotImplementedException("Geometry streams");
}

static bool isEmptyInst(IR::Value val) {
    if (auto* inst = val.TryInstRecursive()) {
        return inst->GetOpcode() == IR::Opcode::Void;
    }
    return false;
}

void EmitVaArg(EmitContext& ctx, IR::Inst* inst, Id arg, Id next) {
    IR::Value next_val = inst->Arg(1);
    u32 va_arglist_idx;
    if (isEmptyInst(next_val)) {
        va_arglist_idx = ctx.va_arg_lists.size();
        ctx.va_arg_lists.emplace_back();
    } else {
        va_arglist_idx = next_val.Inst()->Flags<VariadicArgInfo>().va_arg_idx;
    }

    ctx.va_arg_lists[va_arglist_idx].push_back(arg);
    auto va_info = inst->Flags<VariadicArgInfo>();
    va_info.va_arg_idx.Assign(va_arglist_idx);
    inst->SetFlags(va_info);
}

Id EmitStringLiteral(EmitContext& ctx, IR::Inst* inst) {
    // ctx.
    return ctx.String(inst->StringLiteral());
}

void EmitDebugPrint(EmitContext& ctx, IR::Inst* inst, Id fmt) {
    IR::Value arglist = inst->Arg(1);
    boost::container::small_vector<Id, 4> fmt_arg_operands;
    if (!isEmptyInst(arglist)) {
        u32 va_arglist_idx = arglist.Inst()->Flags<VariadicArgInfo>().va_arg_idx;
        const auto& va_arglist = ctx.va_arg_lists[va_arglist_idx];
        // reverse the order
        std::copy(va_arglist.rbegin(), va_arglist.rend(), std::back_inserter(fmt_arg_operands));
    }

    ctx.OpDebugPrintf(fmt, fmt_arg_operands);
}

} // namespace Shader::Backend::SPIRV
