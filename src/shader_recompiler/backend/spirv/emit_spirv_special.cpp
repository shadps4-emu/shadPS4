// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <boost/container/small_vector.hpp>
#include "common/assert.h"
#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"
#include "shader_recompiler/ir/debug_print.h"

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

void EmitVaArg(EmitContext& ctx, IR::Inst* inst, Id arg, Id next) {
    IR::Value next_val = inst->Arg(1);
    u32 va_arglist_idx;
    if (next_val.IsEmpty()) {
        va_arglist_idx = ctx.va_arg_lists.size();
        ctx.va_arg_lists.emplace_back();
    } else {
        va_arglist_idx = next_val.Inst()->Flags<VariadicArgInfo>().va_arg_idx;
    }

    ctx.va_arg_lists[va_arglist_idx].push_back(arg);
    VariadicArgInfo va_info;
    va_info.va_arg_idx.Assign(va_arglist_idx);
}

void EmitDebugPrint(EmitContext& ctx, IR::Inst* inst) {
    const std::string& fmt =
        ctx.info.debug_print_strings.at(inst->Flags<DebugPrintInfo>().string_idx);
    Id fmt_id = ctx.String(fmt);

    IR::Value va_arg_list_val = inst->Arg(0);
    boost::container::small_vector<Id, 4> fmt_arg_operands;
    if (!va_arg_list_val.IsEmpty()) {
        u32 va_arglist_idx = va_arg_list_val.Inst()->Flags<VariadicArgInfo>().va_arg_idx;
        const auto& va_arglist = ctx.va_arg_lists[va_arglist_idx];
        // reverse the order
        std::copy(va_arglist.rbegin(), va_arglist.rend(), fmt_arg_operands.end());
    }

    ASSERT(fmt_arg_operands.size() == inst->Flags<DebugPrintInfo>().num_args);
    ctx.OpDebugPrintf(fmt_id, fmt_arg_operands);
}

} // namespace Shader::Backend::SPIRV
