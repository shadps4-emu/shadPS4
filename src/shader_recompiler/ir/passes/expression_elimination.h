// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/container/small_vector.hpp>

#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/microinstruction.h"
#include "shader_recompiler/ir/opcodes.h"

namespace Shader::Optimization {

static bool IsAllowedInst(const IR::Inst& inst) {
    return inst.GetOpcode() == IR::Opcode::Ballot ||
           inst.GetOpcode() == IR::Opcode::UnpackUint2x32 ||
           inst.GetOpcode() == IR::Opcode::BitCastU32F32 ||
           inst.GetOpcode() == IR::Opcode::BitCastF32U32;
}

static bool IsIdenticalInst(const IR::Inst* a, const IR::Inst* b) {
    if (a->GetOpcode() != b->GetOpcode()) {
        return false;
    }
    for (size_t i = 0; i < a->NumArgs(); i++) {
        if (a->Arg(i) != b->Arg(i)) {
            return false;
        }
    }
    return true;
}

static void RunLocalCSE(IR::Block* block) {
    boost::container::small_vector<IR::Inst*, 8> inst_list;
    auto it = std::ranges::find_if_not(*block, IR::IsPhi);
    for (; it != block->end(); ++it) {
        IR::Inst& inst{*it};
        if (!IsAllowedInst(inst)) {
            continue;
        }
        const auto it = std::ranges::find_if(
            inst_list, [&](const IR::Inst* b) { return IsIdenticalInst(&inst, b); });
        if (it != inst_list.end()) {
            inst.ReplaceUsesWithAndRemove(IR::Value{*it});
        } else {
            inst_list.emplace_back(&inst);
        }
    }
}

} // namespace Shader::Optimization
