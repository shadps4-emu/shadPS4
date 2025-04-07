// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>
#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>
#include "shader_recompiler/ir/program.h"

namespace Shader::Backend::X64 {

using Operands = boost::container::static_vector<Xbyak::Operand, 4>;

class EmitContext {
public:
    static constexpr size_t NumGPRegs = 16;
    static constexpr size_t NumXmmRegs = 16;

    using PhiAssignmentList = boost::container::small_vector<std::pair<IR::Inst*, IR::Value>, 4>;

    EmitContext(const IR::Program& program_, Xbyak::CodeGenerator& code_);

    [[nodiscard]] Xbyak::CodeGenerator& Code() const {
        return code;
    }

    [[nodiscard]] const IR::Program& Program() const {
        return program;
    }

    [[nodiscard]] Xbyak::Label& EndLabel() {
        return end_label;
    }

    [[nodiscard]] Xbyak::Label& BlockLabel(IR::Block* block) {
        return block_labels.at(block);
    }

    void SetEndFlag() {
        end_flag = true;
    }

    [[nodiscard]] bool EndFlag() {
        bool flag = end_flag;
        end_flag = false;
        return flag;
    }

    [[nodiscard]] Xbyak::Reg64& TempGPReg(bool reserve = true);
    [[nodiscard]] Xbyak::Xmm& TempXmmReg(bool reserve = true);
    void PopTempGPReg();
    void PopTempXmmReg();
    void ResetTempRegs();
    
    [[nodiscard]] const Xbyak::Reg64& UserData() const {return Xbyak::util::rdi;}

    [[nodiscard]] const Operands& Def(IR::Inst* inst);
    [[nodiscard]] Operands Def(const IR::Value& value);
    [[nodiscard]] std::optional<std::reference_wrapper<const EmitContext::PhiAssignmentList>>
    PhiAssignments(IR::Block* block) const;


    void Prologue();
    void Epilogue();

private:
    struct InstInterval {
        IR::Inst* inst;
        size_t start;
        size_t end;
    };

    struct ActiveInstInterval : InstInterval {
        size_t component;

        ActiveInstInterval(const InstInterval& interval, size_t component_)
            : InstInterval(interval), component(component_) {}
    };
    using ActiveIntervalList = boost::container::small_vector<ActiveInstInterval, 8>;

    struct RegAllocContext {
        boost::container::static_vector<Xbyak::Reg64, NumGPRegs> free_gp_regs;
        boost::container::static_vector<Xbyak::Xmm, NumXmmRegs> free_xmm_regs;
        boost::container::small_vector<size_t, 8> free_stack_slots;
        ActiveIntervalList active_gp_intervals;
        ActiveIntervalList active_xmm_intervals;
        ActiveIntervalList active_spill_intervals;
    };

    using FlatInstList = boost::container::small_vector<IR::Inst*, 64>;

    const IR::Program& program;
    Xbyak::CodeGenerator& code;

    // Map of blocks to their phi assignments
    boost::container::small_flat_map<IR::Block*, PhiAssignmentList, 8> phi_assignments;

    // Map of instructions to their operands
    boost::container::small_flat_map<IR::Inst*, Operands, 64> inst_to_operands;

    // Space used for spilled instructions
    size_t inst_stack_space = 0;

    // Temporary register allocation
    boost::container::static_vector<Xbyak::Reg64, NumGPRegs> temp_gp_regs;
    boost::container::static_vector<Xbyak::Xmm, NumXmmRegs> temp_xmm_regs;
    size_t temp_gp_reg_index = 0;
    size_t temp_xmm_reg_index = 0;
    size_t num_scratch_gp_regs = 0;
    size_t num_scratch_xmm_regs = 0;

    // Preseved registers
    boost::container::static_vector<Xbyak::Reg, NumGPRegs + NumXmmRegs> preserved_regs;

    // Labels
    boost::container::small_flat_map<IR::Block*, Xbyak::Label, 8> block_labels;
    Xbyak::Label end_label;

    // End flag, used to defer jump to end label
    bool end_flag = false;

    void SpillInst(RegAllocContext& ctx, const ActiveInstInterval& interval,
                   ActiveIntervalList& active_intervals);
    void AdjustInstInterval(InstInterval& interval, const FlatInstList& insts);
    void AllocateRegisters();
};

} // namespace Shader::Backend::X64