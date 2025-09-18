// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

constexpr s32 F64ToF32Exp = +1023 - 127;
constexpr s32 F32ToF64Exp = +127 - 1023;

static IR::F32 PackedF64ToF32(IR::IREmitter& ir, const IR::Value& packed) {
    const IR::U32 lo{ir.CompositeExtract(packed, 0)};
    const IR::U32 hi{ir.CompositeExtract(packed, 1)};
    const IR::U32 sign{ir.BitFieldExtract(hi, ir.Imm32(31), ir.Imm32(1))};
    const IR::U32 exp{ir.BitFieldExtract(hi, ir.Imm32(20), ir.Imm32(11))};
    const IR::U32 mantissa_hi{ir.BitFieldExtract(hi, ir.Imm32(0), ir.Imm32(20))};
    const IR::U32 mantissa_lo{ir.BitFieldExtract(lo, ir.Imm32(29), ir.Imm32(3))};
    const IR::U32 mantissa{
        ir.BitwiseOr(ir.ShiftLeftLogical(mantissa_hi, ir.Imm32(3)), mantissa_lo)};
    const IR::U32 exp_if_subnorm{
        ir.Select(ir.IEqual(exp, ir.Imm32(0)), ir.Imm32(0), ir.IAdd(exp, ir.Imm32(F64ToF32Exp)))};
    const IR::U32 exp_if_infnan{
        ir.Select(ir.IEqual(exp, ir.Imm32(0x7ff)), ir.Imm32(0xff), exp_if_subnorm)};
    const IR::U32 result{
        ir.BitwiseOr(ir.ShiftLeftLogical(sign, ir.Imm32(31)),
                     ir.BitwiseOr(ir.ShiftLeftLogical(exp_if_infnan, ir.Imm32(23)), mantissa))};
    return ir.BitCast<IR::F32>(result);
}

IR::Value F32ToPackedF64(IR::IREmitter& ir, const IR::Value& raw) {
    const IR::U32 value{ir.BitCast<IR::U32>(IR::F32(raw))};
    const IR::U32 sign{ir.BitFieldExtract(value, ir.Imm32(31), ir.Imm32(1))};
    const IR::U32 exp{ir.BitFieldExtract(value, ir.Imm32(23), ir.Imm32(8))};
    const IR::U32 mantissa{ir.BitFieldExtract(value, ir.Imm32(0), ir.Imm32(23))};
    const IR::U32 mantissa_hi{ir.BitFieldExtract(mantissa, ir.Imm32(3), ir.Imm32(20))};
    const IR::U32 mantissa_lo{ir.BitFieldExtract(mantissa, ir.Imm32(0), ir.Imm32(3))};
    const IR::U32 exp_if_subnorm{
        ir.Select(ir.IEqual(exp, ir.Imm32(0)), ir.Imm32(0), ir.IAdd(exp, ir.Imm32(F32ToF64Exp)))};
    const IR::U32 exp_if_infnan{
        ir.Select(ir.IEqual(exp, ir.Imm32(0xff)), ir.Imm32(0x7ff), exp_if_subnorm)};
    const IR::U32 lo{ir.ShiftLeftLogical(mantissa_lo, ir.Imm32(29))};
    const IR::U32 hi{
        ir.BitwiseOr(ir.ShiftLeftLogical(sign, ir.Imm32(31)),
                     ir.BitwiseOr(ir.ShiftLeftLogical(exp_if_infnan, ir.Imm32(20)), mantissa_hi))};
    return ir.CompositeConstruct(lo, hi);
}

static IR::Opcode Replace(IR::Opcode op) {
    switch (op) {
    case IR::Opcode::FPAbs64:
        return IR::Opcode::FPAbs32;
    case IR::Opcode::FPAdd64:
        return IR::Opcode::FPAdd32;
    case IR::Opcode::FPFma64:
        return IR::Opcode::FPFma32;
    case IR::Opcode::FPMax64:
        return IR::Opcode::FPMax32;
    case IR::Opcode::FPMin64:
        return IR::Opcode::FPMin32;
    case IR::Opcode::FPMul64:
        return IR::Opcode::FPMul32;
    case IR::Opcode::FPDiv64:
        return IR::Opcode::FPDiv32;
    case IR::Opcode::FPNeg64:
        return IR::Opcode::FPNeg32;
    case IR::Opcode::FPRecip64:
        return IR::Opcode::FPRecip32;
    case IR::Opcode::FPRecipSqrt64:
        return IR::Opcode::FPRecipSqrt32;
    case IR::Opcode::FPSaturate64:
        return IR::Opcode::FPSaturate32;
    case IR::Opcode::FPClamp64:
        return IR::Opcode::FPClamp32;
    case IR::Opcode::FPRoundEven64:
        return IR::Opcode::FPRoundEven32;
    case IR::Opcode::FPFloor64:
        return IR::Opcode::FPFloor32;
    case IR::Opcode::FPCeil64:
        return IR::Opcode::FPCeil32;
    case IR::Opcode::FPTrunc64:
        return IR::Opcode::FPTrunc32;
    case IR::Opcode::FPFract64:
        return IR::Opcode::FPFract32;
    case IR::Opcode::FPFrexpSig64:
        return IR::Opcode::FPFrexpSig32;
    case IR::Opcode::FPFrexpExp64:
        return IR::Opcode::FPFrexpExp32;
    case IR::Opcode::FPOrdEqual64:
        return IR::Opcode::FPOrdEqual32;
    case IR::Opcode::FPUnordEqual64:
        return IR::Opcode::FPUnordEqual32;
    case IR::Opcode::FPOrdNotEqual64:
        return IR::Opcode::FPOrdNotEqual32;
    case IR::Opcode::FPUnordNotEqual64:
        return IR::Opcode::FPUnordNotEqual32;
    case IR::Opcode::FPOrdLessThan64:
        return IR::Opcode::FPOrdLessThan32;
    case IR::Opcode::FPUnordLessThan64:
        return IR::Opcode::FPUnordLessThan32;
    case IR::Opcode::FPOrdGreaterThan64:
        return IR::Opcode::FPOrdGreaterThan32;
    case IR::Opcode::FPUnordGreaterThan64:
        return IR::Opcode::FPUnordGreaterThan32;
    case IR::Opcode::FPOrdLessThanEqual64:
        return IR::Opcode::FPOrdLessThanEqual32;
    case IR::Opcode::FPUnordLessThanEqual64:
        return IR::Opcode::FPUnordLessThanEqual32;
    case IR::Opcode::FPOrdGreaterThanEqual64:
        return IR::Opcode::FPOrdGreaterThanEqual32;
    case IR::Opcode::FPUnordGreaterThanEqual64:
        return IR::Opcode::FPUnordGreaterThanEqual32;
    case IR::Opcode::FPIsNan64:
        return IR::Opcode::FPIsNan32;
    case IR::Opcode::FPIsInf64:
        return IR::Opcode::FPIsInf32;
    case IR::Opcode::ConvertS32F64:
        return IR::Opcode::ConvertS32F32;
    case IR::Opcode::ConvertF32F64:
        return IR::Opcode::Identity;
    case IR::Opcode::ConvertF64F32:
        return IR::Opcode::Identity;
    case IR::Opcode::ConvertF64S32:
        return IR::Opcode::ConvertF32S32;
    case IR::Opcode::ConvertF64U32:
        return IR::Opcode::ConvertF32U32;
    default:
        return op;
    }
}

static void Lower(IR::Block& block, IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::PackDouble2x32: {
        IR::IREmitter ir(block, IR::Block::InstructionList::s_iterator_to(inst));
        inst.ReplaceUsesWith(PackedF64ToF32(ir, inst.Arg(0)));
        break;
    }
    case IR::Opcode::UnpackDouble2x32: {
        IR::IREmitter ir(block, IR::Block::InstructionList::s_iterator_to(inst));
        inst.ReplaceUsesWith(F32ToPackedF64(ir, inst.Arg(0)));
        break;
    }
    default:
        inst.ReplaceOpcode(Replace(inst.GetOpcode()));
        break;
    }
}

void LowerFp64ToFp32(IR::Program& program) {
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            Lower(*block, inst);
        }
    }
}

} // namespace Shader::Optimization
