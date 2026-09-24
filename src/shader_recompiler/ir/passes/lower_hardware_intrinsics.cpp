// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

static void LowerCmpClass(IR::Block& block, IR::Inst& inst) {
    ASSERT_MSG(inst.Arg(1).IsImmediate(), "Unable to resolve compare operation");
    const auto class_mask = static_cast<IR::FloatClassFunc>(inst.Arg(1).U32());
    if ((class_mask & IR::FloatClassFunc::NaN) == IR::FloatClassFunc::NaN) {
        inst.ReplaceOpcode(IR::Opcode::FPIsNan32);
    } else if ((class_mask & IR::FloatClassFunc::Infinity) == IR::FloatClassFunc::Infinity) {
        inst.ReplaceOpcode(IR::Opcode::FPIsInf32);
    } else if ((class_mask & IR::FloatClassFunc::Finite) == IR::FloatClassFunc::Finite) {
        IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
        const IR::F32 value = IR::F32{inst.Arg(0)};
        inst.ReplaceUsesWithAndRemove(
            ir.LogicalNot(ir.LogicalOr(ir.FPIsNan(value), ir.FPIsInf(value))));
    } else {
        UNREACHABLE();
    }
}

static void LowerPackedAncillary(IR::Block& block, IR::Inst& inst) {
    if (inst.Arg(0).IsImmediate() || !inst.Arg(1).IsImmediate() || !inst.Arg(2).IsImmediate()) {
        return;
    }
    IR::Inst* value = inst.Arg(0).Inst();
    if (value->GetOpcode() != IR::Opcode::GetAttributeU32 ||
        value->Arg(0).Attribute() != IR::Attribute::PackedAncillary) {
        return;
    }
    const u32 offset = inst.Arg(1).U32();
    const u32 bits = inst.Arg(2).U32();
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    if (offset >= 8 && offset + bits <= 12) {
        const auto sample_index = ir.GetAttributeU32(IR::Attribute::SampleIndex);
        if (offset == 8 && bits == 4) {
            inst.ReplaceUsesWithAndRemove(sample_index);
        } else {
            inst.ReplaceUsesWithAndRemove(
                ir.BitFieldExtract(sample_index, ir.Imm32(offset - 8), ir.Imm32(bits)));
        }
    } else if (offset >= 16 && offset + bits <= 27) {
        const auto mrt_index = ir.GetAttributeU32(IR::Attribute::RenderTargetIndex);
        if (offset == 16 && bits == 11) {
            inst.ReplaceUsesWithAndRemove(mrt_index);
        } else {
            inst.ReplaceUsesWithAndRemove(
                ir.BitFieldExtract(mrt_index, ir.Imm32(offset - 16), ir.Imm32(bits)));
        }
    } else {
        UNREACHABLE_MSG("Unhandled bitfield extract from ancillary VGPR offset={}, bits={}", offset,
                        bits);
    }
    value->ReplaceUsesWithAndRemove(ir.Imm32(0U));
}

static void LowerMaskedBitCount(IR::Block& block, IR::Inst& inst) {
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    const IR::U32 thread_mask{
        ir.GetAttributeU32(IR::Attribute::SubgroupLtMask, inst.Arg(2).U1() ? 1 : 0)};
    const IR::U32 masked_value{ir.BitCount(ir.BitwiseAnd(IR::U32{inst.Arg(0)}, thread_mask))};
    inst.ReplaceUsesWithAndRemove(ir.IAdd(masked_value, IR::U32{inst.Arg(1)}));
}

static void Lower(IR::Block& block, IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::FPCmpClass32:
        return LowerCmpClass(block, inst);
    case IR::Opcode::BitFieldUExtract:
        return LowerPackedAncillary(block, inst);
    case IR::Opcode::MaskedBitCount32:
        return LowerMaskedBitCount(block, inst);
    default:
        break;
    }
}

void LowerHardwareIntrinsics(IR::Program& program) {
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            Lower(*block, inst);
        }
    }
}

} // namespace Shader::Optimization
