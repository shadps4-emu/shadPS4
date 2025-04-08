// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/container/flat_map.hpp>
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/pools.h"

namespace Shader::IR {

// Given an IR program, this class is used to create a subprogram that contains
// only the blocks and instructions that relevant to a group of given instructions.
// Taking into account only the given instructions, the instructions that it uses and
// conditions.

struct SubProgram {
    SubProgram(Program* super_program, Pools& pools);

    Block* AddBlock(Block* orig_block);
    Inst* AddInst(Inst* orig_inst);

    Block* GetBlock(Block* orig_block);
    Inst* GetInst(Inst* orig_inst);

    Program GetSubProgram();

private:
    Inst* AddInst(Inst* orig_inst, std::optional<Block::InstructionList::iterator> insertion_point);
    void AddPhi(Inst* orig_phi, Inst* phi);

    void SetArg(Inst* inst, Inst* orig_inst, size_t index);
    void AddPhiOperand(Inst* phi, Block* block, const Value& arg);

    void BuildBlockListAndASL(Program& sub_program);
    void AddProlgueAndEpilogue(Program& sub_program);

    bool completed = false;
    Program* super_program;
    Pools& pools;
    boost::container::flat_map<Block*, Block*> orig_block_to_block;
    boost::container::flat_map<Inst*, Inst*> orig_inst_to_inst;
};

} // namespace Shader::IR
