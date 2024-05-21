// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "shader_recompiler/frontend/instruction.h"

namespace Shader::Gcn {

u32 GcnInst::BranchTarget(u32 pc) const {
    const s16 simm = static_cast<s16>(control.sopp.simm * 4);
    const u32 target = pc + simm + 4;
    return target;
}

bool GcnInst::IsTerminateInstruction() const {
    return IsUnconditionalBranch() || IsConditionalBranch() || IsFork() ||
           opcode == Opcode::S_ENDPGM;
}

bool GcnInst::IsUnconditionalBranch() const {
    return opcode == Opcode::S_BRANCH;
}

bool GcnInst::IsFork() const {
    return opcode == Opcode::S_CBRANCH_I_FORK || opcode == Opcode::S_CBRANCH_G_FORK ||
           opcode == Opcode::S_CBRANCH_JOIN;
}

bool GcnInst::IsConditionalBranch() const {
    switch (opcode) {
    case Opcode::S_CBRANCH_SCC0:
    case Opcode::S_CBRANCH_SCC1:
    case Opcode::S_CBRANCH_VCCZ:
    case Opcode::S_CBRANCH_VCCNZ:
    case Opcode::S_CBRANCH_EXECZ:
    case Opcode::S_CBRANCH_EXECNZ:
        return true;
    case Opcode::S_CBRANCH_CDBGSYS:
    case Opcode::S_CBRANCH_CDBGUSER:
    case Opcode::S_CBRANCH_CDBGSYS_OR_USER:
    case Opcode::S_CBRANCH_CDBGSYS_AND_USER:
        UNIMPLEMENTED();
        return true;
    default:
        break;
    }
    return false;
}

} // namespace Shader::Gcn
