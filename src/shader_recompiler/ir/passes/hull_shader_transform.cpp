// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma clang optimize off
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

/**
 * Tessellation shaders pass outputs to the next shader using LDS.
 * The Hull shader stage receives input control points stored in LDS.
 *
 * The LDS layout is:
 * - TCS inputs for patch 0
 * - TCS inputs for patch 1
 * - TCS inputs for patch 2
 * - ...
 * - TCS outputs for patch 0
 * - TCS outputs for patch 1
 * - TCS outputs for patch 2
 * - ...
 * - Per-patch TCS outputs for patch 0
 * - Per-patch TCS outputs for patch 1
 * - Per-patch TCS outputs for patch 2
 *
 * If the Hull stage does not write any new control points the driver will
 * optimize LDS layout so input and output control point spaces overlap.
 *
 * Tessellation factors are stored in the per-patch TCS output block
 * as well as a factor V# that is automatically bound by the driver.
 *
 * This pass attempts to resolve LDS accesses to attribute accesses and correctly
 * write to the tessellation factor tables. For the latter we replace the
 * buffer store instruction to factor writes according to their offset.
 *
 * LDS stores can either be output control point writes or per-patch data writes.
 * This is detected by looking at how the address is formed. In any case the calculation
 * will be of the form a * b + c. For output control points a = output_control_point_id
 * while for per-patch writes a = patch_id.
 *
 * Both patch_id and output_control_point_id are packed in VGPR1 by the driver and shader
 * uses V_BFE_U32 to extract them. We use the starting bit_pos to determine which is which.
 *
 * LDS reads are more tricky as amount of different calculations performed can vary.
 * The final result, if output control point space is distinct, is of the form:
 * patch_id * input_control_point_stride * num_control_points_per_input_patch + a
 * The value "a" can be anything in the range of [0, input_control_point_stride]
 *
 * This pass does not attempt to deduce the exact attribute referenced by "a" but rather
 * only using "a" itself index into input attributes. Those are defined as an array in the shader
 * layout (location = 0) in vec4[num_control_points_per_input_patch] attr[];
 * ...
 * float value = attr[a / in_stride][(a % in_stride) >> 4][(a & 0xF) >> 2];
 *
 * This requires knowing in_stride which is not provided to us by the guest.
 * To deduce it we perform a breadth first search on the arguments of a DS_READ*
 * looking for a buffer load with offset = 0. This will be the buffer holding tessellation
 * constants and it contains the value of in_stride we can read at compile time.
 *
 * NOTE: This pass must be run before constant propagation as it relies on relatively specific
 * pattern matching that might be mutated that that optimization pass.
 *
 */

void HullShaderTransform(const IR::Program& program) {
    LOG_INFO(Render_Vulkan, "{}", IR::DumpProgram(program));
    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
            const auto opcode = inst.GetOpcode();
            switch (opcode) {
            case IR::Opcode::StoreBufferU32:
            case IR::Opcode::StoreBufferU32x2:
            case IR::Opcode::StoreBufferU32x3:
            case IR::Opcode::StoreBufferU32x4: {
                const auto info = inst.Flags<IR::BufferInstInfo>();
                if (!info.globally_coherent) {
                    break;
                }
                const auto GetValue = [&](IR::Value data) -> IR::F32 {
                    if (auto* inst = data.TryInstRecursive();
                        inst && inst->GetOpcode() == IR::Opcode::BitCastU32F32) {
                        return IR::F32{inst->Arg(0)};
                    }
                    return ir.BitCast<IR::F32, IR::U32>(IR::U32{data});
                };
                const u32 num_dwords = u32(opcode) - u32(IR::Opcode::StoreBufferU32) + 1;
                const auto factor_idx = info.inst_offset.Value() >> 2;
                const IR::Value data = inst.Arg(2);
                inst.Invalidate();
                if (num_dwords == 1) {
                    ir.SetPatch(IR::PatchFactor(factor_idx), GetValue(data));
                    break;
                }
                auto* inst = data.TryInstRecursive();
                ASSERT(inst && (inst->GetOpcode() == IR::Opcode::CompositeConstructU32x2 ||
                                inst->GetOpcode() == IR::Opcode::CompositeConstructU32x3 ||
                                inst->GetOpcode() == IR::Opcode::CompositeConstructU32x4));
                for (s32 i = 0; i < num_dwords; i++) {
                    ir.SetPatch(IR::PatchFactor(factor_idx + i), GetValue(inst->Arg(i)));
                }
                break;
            }
            case IR::Opcode::WriteSharedU32:
            case IR::Opcode::WriteSharedU64: {
                const u32 num_dwords = opcode == IR::Opcode::WriteSharedU32 ? 1 : 2;
                const IR::Value data = inst.Arg(1);
                const auto [data_lo, data_hi] = [&] -> std::pair<IR::U32, IR::U32> {
                    if (num_dwords == 1) {
                        return {IR::U32{data}, IR::U32{}};
                    }
                    const auto* prod = data.InstRecursive();
                    return {IR::U32{prod->Arg(0)}, IR::U32{prod->Arg(1)}};
                }();
                const IR::Inst* ds_offset = inst.Arg(0).InstRecursive();
                const u32 offset_dw = ds_offset->Arg(1).U32() >> 4;
                IR::Inst* prod = ds_offset->Arg(0).TryInstRecursive();
                ASSERT(prod && (prod->GetOpcode() == IR::Opcode::IAdd32 ||
                                prod->GetOpcode() == IR::Opcode::IMul32));
                if (prod->GetOpcode() == IR::Opcode::IAdd32) {
                    prod = prod->Arg(0).TryInstRecursive();
                    ASSERT(prod && prod->GetOpcode() == IR::Opcode::IMul32);
                }
                prod = prod->Arg(0).TryInstRecursive();
                ASSERT(prod && prod->GetOpcode() == IR::Opcode::BitFieldSExtract &&
                       prod->Arg(2).IsImmediate() && prod->Arg(2).U32() == 24);
                prod = prod->Arg(0).TryInstRecursive();
                ASSERT(prod && prod->GetOpcode() == IR::Opcode::BitFieldUExtract);
                const u32 bit_pos = prod->Arg(1).U32();
                const auto SetOutput = [&ir](IR::U32 value, u32 offset_dw, bool is_patch_const) {
                    const IR::F32 data = ir.BitCast<IR::F32, IR::U32>(value);
                    if (!is_patch_const) {
                        const u32 param = offset_dw >> 2;
                        const u32 comp = offset_dw & 3;
                        ir.SetAttribute(IR::Attribute::Param0 + param, data, comp);
                    } else {
                        ir.SetPatch(IR::PatchGeneric(offset_dw), data);
                    }
                };
                ASSERT_MSG(bit_pos == 0 || bit_pos == 8, "Unknown bit extract pos {}", bit_pos);
                const bool is_patch_const = bit_pos == 0;
                SetOutput(data_lo, offset_dw, is_patch_const);
                if (num_dwords > 1) {
                    SetOutput(data_hi, offset_dw + 1, is_patch_const);
                }
                inst.Invalidate();
                break;
            }
            default:
                break;
            }
        }
    }
    LOG_INFO(Render_Vulkan, "{}", IR::DumpProgram(program));
}

} // namespace Shader::Optimization
