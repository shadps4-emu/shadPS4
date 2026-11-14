// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stack>
#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"
#include "shader_recompiler/ir/microinstruction.h"
#include "shader_recompiler/ordered_count.h"

namespace Shader::Backend::SPIRV {

using namespace OrderedCount;

// Emulate WorkgroupId by bumping an index used by all dispatched workgroups.
// This is to guarantee forward progress in a shader that uses DS_ORDERED_COUNT,
// because the order that Vulkan driver launches workgroups is arbitrary in terms
// of the real gl_WorkgroupId.
// Assumes some host workgroup cant be preempted indefinitely in favor of some
// other workgroup launched after
void EmitContext::InitEmulatedWorkgroupIndex() {
    const Id if_label{OpLabel()};
    const Id merge_label{OpLabel()};
    const Id shared_ptr = EmitSharedMemoryAccess(u32_zero_value, PointerType::U32, true);
    const Id is_first{OpIEqual(U1[1], OpLoad(U32[1], local_invocation_index), u32_zero_value)};
    OpSelectionMerge(merge_label, spv::SelectionControlMask::MaskNone);
    OpBranchConditional(is_first, if_label, merge_label);

    AddLabel(if_label);

    const auto& utility_buffer = buffers[ordered_count_utility_buffer_binding];
    const Id offset = utility_buffer.Offset(PointerSize::B32);
    Id index = ConstU32(u32(UtilityBufferOffsets::NextWorkgroupIndex) >> 2);
    if (Sirit::ValidId(offset)) { // TODO necessary?
        index = OpIAdd(U32[1], index, offset);
    }
    const auto [id, buffer_ptr_type] = utility_buffer.Alias(PointerType::U32);
    const auto buffer_ptr = OpAccessChain(buffer_ptr_type, id, u32_zero_value, index);
    const Id scope{ConstU32(static_cast<u32>(spv::Scope::Device))};
    const Id semantics{u32_zero_value};
    const Id workgroup_index_val = OpAtomicIIncrement(U32[1], buffer_ptr, scope, semantics);

    OpStore(shared_ptr, workgroup_index_val);
    OpBranch(merge_label);

    AddLabel(merge_label);
    EmitBarrier(*this);
    workgroup_index_id = OpLoad(U32[1], shared_ptr);
    EmitBarrier(*this);
}

void EmitContext::InitEmulatedWorkgroupId() {
    const Id num_workgroups{OpLoad(U32[3], num_workgroups_id)};
    const Id num_workgroups_x{OpCompositeExtract(U32[1], num_workgroups, 0)};
    const Id num_workgroups_y{OpCompositeExtract(U32[1], num_workgroups, 1)};

    const Id id_x{OpUMod(U32[1], workgroup_index_id, num_workgroups_x)};
    const Id id_y{
        OpUMod(U32[1], OpUDiv(U32[1], workgroup_index_id, num_workgroups_x), num_workgroups_y)};
    const Id id_z{
        OpUDiv(U32[1], workgroup_index_id, OpIMul(U32[1], num_workgroups_x, num_workgroups_y))};

    emulated_workgroup_id = OpCompositeConstruct(U32[3], id_x, id_y, id_z);
}

// Based on psuedo GLSL below.
// If the inst is acquire-only, don't advance the counter.
// If the inst is release, we may be able to skip the acquire if
// there was a previous acquire-only operation, so keep a flag to track that.
// Ignore this possibility if there are no acquire-only insts in the shader.
//
// TODO not actually sure if atomic should be performed when doing acquire only
//
/*
buffer UtilityBuffer {
    uint counters[];
};

uint current_counter = 0;
bool holding_ordered_counter = false;

template<bool add, bool release>
uint ordered_count(uint value, uint gds_offset, uint append_term, bool elected, bool is_active)
{
    uint prev_count;
    if (elected) {
        if (!release || !holding_ordered_counter) {
            for (;;) {
                uint next_writer = atomicLoad(counters[current_counter], gl_ScopeDevice,
                                        gl_StorageSemanticsBuffer, gl_SemanticsAcquire);
                if (next_writer == append_term) {
                    break;
                }
            }
        }
        if (is_active) {
            if (add) {
                prev_count = atomicAdd(gds_buffer[gds_offset >> 2], value);
            } else {
                // swap
                prev_count = atomicExchange(gds_buffer[gds_offset >> 2], value);
            }
        }
        if (release) {
            atomicStore(counters[current_counter], append_term + 1, gl_ScopeDevice,
                                        gl_StorageSemanticsBuffer, gl_SemanticsRelease);
        }
    }
    holding_ordered_counter = !release;
    ++current_counter;
    return prev_count;
}
*/
void EmitContext::DefineOrderedCountFunction(OrderedCount::Op op, bool release) {
    const Id ordered_count_func_type{TypeFunction(U32[1], U32[1], U32[1], U32[1], U1[1], U1[1])};
    const Id fn{OpFunction(U32[1], spv::FunctionControlMask::MaskNone, ordered_count_func_type)};
    std::string fn_name = "ordered_count";
    fn_name += [&]() {
        switch (op) {
        case OrderedCount::Op::Add:
            return "_add";
        case OrderedCount::Op::Swap:
            return "_swap";
        default:
            UNREACHABLE();
        }
    }();
    if (info.uses_ordered_count_acquire_only) {
        fn_name += release ? "_release" : "_acquire";
    }
    Name(fn, fn_name);

    const Id value{OpFunctionParameter(U32[1])};
    Name(value, "value");
    const Id gds_offset{OpFunctionParameter(U32[1])};
    Name(gds_offset, "gds_offset");
    const Id append_term{OpFunctionParameter(U32[1])};
    Name(append_term, "append_term");
    const Id elected{OpFunctionParameter(U1[1])};
    Name(elected, "elected");
    const Id is_active{OpFunctionParameter(U1[1])};
    Name(is_active, "is_active");

    std::stack<Id> merge_stack;

    auto push_if_stmt = [&](Id cond) {
        const Id if_label{OpLabel()};
        const Id merge_label{OpLabel()};
        OpSelectionMerge(merge_label, spv::SelectionControlMask::MaskNone);
        OpBranchConditional(cond, if_label, merge_label);
        merge_stack.push(merge_label);
        AddLabel(if_label);
    };

    auto pop_if_stmt = [&] {
        const Id merge_label{merge_stack.top()};
        OpBranch(merge_label);
        merge_stack.pop();
        AddLabel(merge_label);
    };

    auto skip_when_release_only = [&]() {
        if (info.uses_ordered_count_acquire_only) {
            const Id holding_lock{OpLoad(U1[1], holding_ordered_counter_flag)};
            push_if_stmt(OpLogicalNot(U1[1], holding_lock));
        }
    };

    auto end_skip = [&]() {
        if (info.uses_ordered_count_acquire_only) {
            pop_if_stmt();
        }
    };

    AddLabel(OpLabel());

    const Id prev_count_var{DefineVar<false>(U32[1], spv::StorageClass::Function)};
    Name(prev_count_var, "prev_count");

    const Id device_scope{ConstU32(static_cast<u32>(spv::Scope::Device))};

    const Id gds_index{OpShiftRightLogical(U32[1], gds_offset, ConstU32(2u))};
    auto [gds_buffer, gds_pointer_type] = buffers[gds_buffer_binding].Alias(PointerType::U32);
    const Id gds_ptr{OpAccessChain(gds_pointer_type, gds_buffer, u32_zero_value, gds_index)};

    const Id current_counter{OpLoad(U32[1], current_counter_var)};
    auto [util_buffer, util_pointer_type] =
        buffers[ordered_count_utility_buffer_binding].Alias(PointerType::U32);
    const Id counter_array_offset{
        OpIMul(U32[1], current_counter, ConstU32(OrderedCount::CounterStride))};
    const Id counter_abs_offset{OpIAdd(
        U32[1], counter_array_offset, ConstU32(u32(OrderedCount::UtilityBufferOffsets::Counters)))};
    const Id counter_abs_index{OpShiftRightLogical(U32[1], counter_abs_offset, ConstU32(2u))};
    const Id counter_ptr{
        OpAccessChain(util_pointer_type, util_buffer, u32_zero_value, counter_abs_index)};

    push_if_stmt(elected);

    skip_when_release_only();

    const Id acquire_loop_header{OpLabel()};
    const Id acquire_loop_merge{OpLabel()};
    OpBranch(acquire_loop_header);

    AddLabel(acquire_loop_header);
    // TODO should use volatile too, but needs vulkan memory model (which means other refactors)
    const auto acquire_semantics{spv::MemorySemanticsMask::UniformMemory |
                                 spv::MemorySemanticsMask::Acquire};
    const Id next_writer{
        OpAtomicLoad(U32[1], counter_ptr, device_scope, ConstU32(u32(acquire_semantics)))};
    const Id success{OpIEqual(U1[1], next_writer, append_term)};
    OpLoopMerge(acquire_loop_merge, acquire_loop_header, spv::LoopControlMask::MaskNone);
    OpBranchConditional(success, acquire_loop_merge, acquire_loop_header);

    AddLabel(acquire_loop_merge);

    end_skip();

    push_if_stmt(is_active);

    const Id prev_count = [&]() {
        const Id semantics{u32_zero_value};
        switch (op) {
        case OrderedCount::Op::Add:
            return OpAtomicIAdd(U32[1], gds_ptr, device_scope, semantics, value);
        case OrderedCount::Op::Swap:
            return OpAtomicExchange(U32[1], gds_ptr, device_scope, semantics, value);
        default:
            UNREACHABLE();
        }
    }();
    OpStore(prev_count_var, prev_count);

    pop_if_stmt(); // is_active

    if (release) {
        const auto release_semantics{spv::MemorySemanticsMask::UniformMemory |
                                     spv::MemorySemanticsMask::Release};
        const Id inc_counter_val{OpIAdd(U32[1], append_term, u32_one_value)};
        OpAtomicStore(counter_ptr, device_scope, ConstU32(u32(release_semantics)), inc_counter_val);
    }

    pop_if_stmt(); // elected

    if (info.uses_ordered_count_acquire_only) {
        OpStore(holding_ordered_counter_flag, release ? false_value : true_value);
    }

    if (release) {
        OpStore(current_counter_var, OpIAdd(U32[1], current_counter, u32_one_value));
    }

    OpReturnValue(OpLoad(U32[1], prev_count_var));
    OpFunctionEnd();

    auto key = std::make_pair(op, release);
    ordered_count_functions[key] = fn;
}

Id EmitOrderedCount(EmitContext& ctx, IR::Inst* inst, Id value, Id gds_offset, Id append_term,
                    Id elected, Id is_active) {
    const auto flags = inst->Flags<OrderedCount::Flags>();
    const OrderedCount::Op op = flags.instruction_type.Value();
    const bool release = flags.wave_release.Value();

    const Id function = ctx.ordered_count_functions[std::make_pair(op, release)];
    ASSERT(Sirit::ValidId(function));

    return ctx.OpFunctionCall(ctx.U32[1], function, value, gds_offset, append_term, elected,
                              is_active);
}

} // namespace Shader::Backend::SPIRV