// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"
#include "shader_recompiler/backend/asm_x64/x64_utils.h"

using namespace Xbyak;
using namespace Xbyak::util;

namespace Shader::Backend::X64 {

EmitContext::EmitContext(const IR::Program& program_, Xbyak::CodeGenerator& code_)
    : program(program_), code(code_) {
    for (IR::Block* block : program.blocks) {
        block_labels[block] = {};
    }
    AllocateRegisters();
}

Reg64& EmitContext::TempGPReg(bool reserve) {
    ASSERT(temp_gp_reg_index < temp_gp_regs.size());
    u64 idx = temp_gp_reg_index;
    if (reserve) {
        temp_gp_reg_index++;
    }
    Reg64& reg = temp_gp_regs[idx];
    if (idx > num_scratch_gp_regs &&
        std::ranges::find(preserved_regs, reg) == preserved_regs.end()) {
        preserved_regs.push_back(reg);
        code.push(reg);
    }
    return reg;
}

Xmm& EmitContext::TempXmmReg(bool reserve) {
    ASSERT(temp_xmm_reg_index < temp_xmm_regs.size());
    u64 idx = temp_xmm_reg_index;
    if (reserve) {
        temp_xmm_reg_index++;
    }
    Xmm& reg = temp_xmm_regs[idx];
    if (idx > num_scratch_xmm_regs &&
        std::ranges::find(preserved_regs, reg) == preserved_regs.end()) {
        preserved_regs.push_back(reg);
        code.sub(rsp, 16);
        code.movups(ptr[rsp], reg);
    }
    return reg;
}

void EmitContext::PopTempGPReg() {
    ASSERT(temp_gp_reg_index > 0);
    temp_gp_reg_index--;
}

void EmitContext::PopTempXmmReg() {
    ASSERT(temp_xmm_reg_index > 0);
    temp_xmm_reg_index--;
}

void EmitContext::ResetTempRegs() {
    temp_gp_reg_index = 0;
    temp_xmm_reg_index = 0;
}

const Operands& EmitContext::Def(IR::Inst* inst) {
    return inst_to_operands.at(inst);
}

Operands EmitContext::Def(const IR::Value& value) {
    if (!value.IsImmediate()) {
        return Def(value.InstRecursive());
    }
    Operands operands;
    Reg64& tmp = TempGPReg(false);
    switch (value.Type()) {
    case IR::Type::U1:
        operands.push_back(TempGPReg().cvt8());
        code.mov(operands.back().Reg(), value.U1());
        break;
    case IR::Type::U8:
        operands.push_back(TempGPReg().cvt8());
        code.mov(operands.back().Reg(), value.U8());
        break;
    case IR::Type::U16:
        operands.push_back(TempGPReg().cvt16());
        code.mov(operands.back().Reg(), value.U16());
        break;
    case IR::Type::U32:
        operands.push_back(TempGPReg().cvt32());
        code.mov(operands.back().Reg(), value.U32());
        break;
    case IR::Type::F32: {
        code.mov(tmp.cvt32(), std::bit_cast<u32>(value.F32()));
        Xmm& xmm32 = TempXmmReg();
        code.movd(xmm32, tmp.cvt32());
        operands.push_back(xmm32);
        break;
    }
    case IR::Type::U64:
        operands.push_back(TempGPReg());
        code.mov(operands.back().Reg(), value.U64());
        break;
    case IR::Type::F64: {
        code.mov(tmp, std::bit_cast<u64>(value.F64()));
        Xmm& xmm64 = TempXmmReg();
        code.movq(xmm64, tmp);
        operands.push_back(xmm64);
        break;
    }
    case IR::Type::ScalarReg:
        operands.push_back(TempGPReg().cvt32());
        code.mov(operands.back().Reg(), std::bit_cast<u32>(value.ScalarReg()));
        break;
    case IR::Type::VectorReg:
        operands.push_back(TempXmmReg().cvt32());
        code.mov(operands.back().Reg(), std::bit_cast<u32>(value.VectorReg()));
        break;
    case IR::Type::Attribute:
        operands.push_back(TempGPReg());
        code.mov(operands.back().Reg(), std::bit_cast<u64>(value.Attribute()));
        break;
    case IR::Type::Patch:
        operands.push_back(TempGPReg());
        code.mov(operands.back().Reg(), std::bit_cast<u64>(value.Patch()));
        break;
    default:
        UNREACHABLE_MSG("Unsupported value type: {}", IR::NameOf(value.Type()));
        break;
    }
    return operands;
}

std::optional<std::reference_wrapper<const EmitContext::PhiAssignmentList>>
EmitContext::PhiAssignments(IR::Block* block) const {
    auto it = phi_assignments.find(block);
    if (it != phi_assignments.end()) {
        return std::cref(it->second);
    }
    return std::nullopt;
}

void EmitContext::Prologue() {
    if (inst_stack_space > 0) {
        code.sub(rsp, inst_stack_space);
        code.mov(r11, rsp);
    }
}

void EmitContext::Epilogue() {
    for (auto it = preserved_regs.rbegin(); it != preserved_regs.rend(); ++it) {
        Reg& reg = *it;
        if (reg.isMMX()) {
            code.movups(reg.cvt128(), ptr[rsp]);
            code.add(rsp, 16);
        } else {
            code.pop(reg);
        }
    }
    preserved_regs.clear();
    if (inst_stack_space > 0) {
        code.add(rsp, inst_stack_space);
    }
}

void EmitContext::SpillInst(RegAllocContext& ctx, const ActiveInstInterval& interval,
                            ActiveIntervalList& active_intervals) {
    const auto get_operand = [&](IR::Inst* inst) -> Operand {
        size_t current_sp = inst_stack_space;
        inst_stack_space += 8;
        switch (GetRegBytesOfType(IR::Value(inst))) {
        case 1:
            return byte[r11 + current_sp];
        case 2:
            return word[r11 + current_sp];
        case 4:
            return dword[r11 + current_sp];
        case 8:
            return qword[r11 + current_sp];
        default:
            UNREACHABLE_MSG("Unsupported register size: {}", GetRegBytesOfType(inst));
            return {};
        }
    };
    auto spill_candidate = std::max_element(
        active_intervals.begin(), active_intervals.end(),
        [](const ActiveInstInterval& a, const ActiveInstInterval& b) { return a.end < b.end; });
    if (spill_candidate == active_intervals.end() || spill_candidate->end <= interval.start) {
        inst_to_operands[interval.inst][interval.component] = get_operand(interval.inst);
    } else {
        Operands& operands = inst_to_operands[spill_candidate->inst];
        OperandHolder op = operands[spill_candidate->component];
        inst_to_operands[interval.inst][interval.component] =
            op.IsXmm() ? op : ResizeRegToType(op.Reg(), interval.inst);
        operands[spill_candidate->component] = get_operand(spill_candidate->inst);
        *spill_candidate = interval;
    }
}

void EmitContext::AdjustInstInterval(InstInterval& interval, const FlatInstList& insts) {
    IR::Inst* inst = interval.inst;
    size_t dist = std::distance(insts.begin(), std::find(insts.begin(), insts.end(), inst));
    interval.start = dist;
    interval.end = dist;
    for (const auto& use : inst->Uses()) {
        if (use.user->GetOpcode() == IR::Opcode::Phi) {
            // We assign the value at the end of the phi block
            IR::Inst& last_inst = use.user->PhiBlock(use.operand)->back();
            dist = std::distance(insts.begin(), std::find(insts.begin(), insts.end(), &last_inst));
            interval.start = std::min(interval.start, dist);
            interval.end = std::max(interval.end, dist);
        } else {
            dist = std::distance(insts.begin(), std::find(insts.begin(), insts.end(), use.user));
            interval.end = std::max(interval.end, dist);
        }
    }
    if (inst->GetOpcode() == IR::Opcode::Phi) {
        for (size_t i = 0; i < inst->NumArgs(); i++) {
            IR::Block* block = inst->PhiBlock(i);
            dist =
                std::distance(insts.begin(), std::find(insts.begin(), insts.end(), &block->back()));
            interval.start = std::min(interval.start, dist);
            interval.end = std::max(interval.end, dist);
            phi_assignments[block].emplace_back(inst, inst->Arg(i));
        }
    }
}

// Rregister utilization:
// Instruction registers:
//  General purpose registers: rcx, rdx, rsi, r8, r9, r10
//  XMM registers: xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6
//
// Value / temporary registers:
//  General purpose registers: rax (scratch), rbx, r12, r13, r14, r15
//  XMM registers: xmm7 (scratch), xmm7 (scratch), xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14,
//  xmm15
//
// r11: Stack pointer for spilled instructions
// rdi: User data pointer
// rsp: Stack pointer
//
// If instruction registers are never used, will be used as temporary registers
void EmitContext::AllocateRegisters() {
    const std::array<Reg64, 6> initial_gp_inst_regs = {rcx, rdx, rsi, r8, r9, r10};
    const std::array<Xmm, 7> initial_xmm_inst_regs = {xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6};
    const std::array<Reg64, 6> initial_gp_temp_regs = {rax, rbx, r12, r13, r14, r15};
    const std::array<Xmm, 9> initial_xmm_temp_regs = {xmm7,  xmm8,  xmm9,  xmm10, xmm11,
                                                      xmm12, xmm13, xmm14, xmm15};

    boost::container::small_vector<InstInterval, 64> intervals;
    FlatInstList insts;
    // We copy insts tot the flat list for faster iteration
    for (IR::Block* block : program.blocks) {
        insts.reserve(insts.size() + block->size());
        for (IR::Inst& inst : *block) {
            insts.push_back(&inst);
        }
    }
    for (IR::Inst* inst : insts) {
        if (inst->GetOpcode() == IR::Opcode::ConditionRef || inst->Type() == IR::Type::Void) {
            continue;
        }
        intervals.emplace_back(inst, 0, 0);
        AdjustInstInterval(intervals.back(), insts);
    }
    std::sort(intervals.begin(), intervals.end(),
              [](const InstInterval& a, const InstInterval& b) { return a.start < b.start; });
    RegAllocContext ctx;
    ctx.free_gp_regs.insert(ctx.free_gp_regs.end(), initial_gp_inst_regs.begin(),
                            initial_gp_inst_regs.end());
    ctx.free_xmm_regs.insert(ctx.free_xmm_regs.end(), initial_xmm_inst_regs.begin(),
                             initial_xmm_inst_regs.end());
    boost::container::static_vector<Reg64, 6> unused_gp_inst_regs;
    boost::container::static_vector<Xmm, 7> unused_xmm_inst_regs;
    unused_gp_inst_regs.insert(unused_gp_inst_regs.end(), ctx.free_gp_regs.begin(),
                               ctx.free_gp_regs.end());
    unused_xmm_inst_regs.insert(unused_xmm_inst_regs.end(), ctx.free_xmm_regs.begin(),
                                ctx.free_xmm_regs.end());
    for (const InstInterval& interval : intervals) {
        // Free old interval resources
        for (auto it = ctx.active_gp_intervals.begin(); it != ctx.active_gp_intervals.end();) {
            if (it->end < interval.start) {
                Reg64 reg = inst_to_operands[it->inst][it->component].Reg().cvt64();
                ctx.free_gp_regs.push_back(reg);
                it = ctx.active_gp_intervals.erase(it);
            } else {
                ++it;
            }
        }
        for (auto it = ctx.active_xmm_intervals.begin(); it != ctx.active_xmm_intervals.end();) {
            if (it->end < interval.start) {
                Xmm reg = inst_to_operands[it->inst][it->component].Xmm();
                ctx.free_xmm_regs.push_back(reg);
                it = ctx.active_xmm_intervals.erase(it);
            } else {
                ++it;
            }
        }
        u8 num_components = GetNumComponentsOfType(interval.inst);
        bool is_floating = IsFloatingType(interval.inst);
        auto& operands = inst_to_operands[interval.inst];
        operands.resize(num_components);
        if (is_floating) {
            for (size_t i = 0; i < num_components; ++i) {
                ActiveInstInterval active(interval, i);
                if (!ctx.free_xmm_regs.empty()) {
                    Xmm& reg = ctx.free_xmm_regs.back();
                    ctx.free_xmm_regs.pop_back();
                    operands[active.component] = reg;
                    unused_xmm_inst_regs.erase(
                        std::remove(unused_xmm_inst_regs.begin(), unused_xmm_inst_regs.end(), reg),
                        unused_xmm_inst_regs.end());
                    ctx.active_xmm_intervals.push_back(active);
                } else {
                    SpillInst(ctx, active, ctx.active_xmm_intervals);
                }
            }
        } else {
            for (size_t i = 0; i < num_components; ++i) {
                ActiveInstInterval active(interval, i);
                if (!ctx.free_gp_regs.empty()) {
                    Reg64& reg = ctx.free_gp_regs.back();
                    ctx.free_gp_regs.pop_back();
                    operands[active.component] = ResizeRegToType(reg, active.inst);
                    unused_gp_inst_regs.erase(
                        std::remove(unused_gp_inst_regs.begin(), unused_gp_inst_regs.end(), reg),
                        unused_gp_inst_regs.end());
                    ctx.active_gp_intervals.push_back(active);
                } else {
                    SpillInst(ctx, active, ctx.active_gp_intervals);
                }
            }
        }
    }
    temp_gp_regs.insert(temp_gp_regs.end(), unused_gp_inst_regs.begin(), unused_gp_inst_regs.end());
    temp_xmm_regs.insert(temp_xmm_regs.end(), unused_xmm_inst_regs.begin(),
                         unused_xmm_inst_regs.end());
    num_scratch_gp_regs = unused_gp_inst_regs.size() + 1;   // rax is scratch
    num_scratch_xmm_regs = unused_xmm_inst_regs.size() + 1; // xmm7 is scratch
    temp_gp_regs.insert(temp_gp_regs.end(), initial_gp_temp_regs.begin(),
                        initial_gp_temp_regs.end());
    temp_xmm_regs.insert(temp_xmm_regs.end(), initial_xmm_temp_regs.begin(),
                         initial_xmm_temp_regs.end());
}

} // namespace Shader::Backend::X64