// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "operand_helper.h"

namespace Shader::IR {

static IR::U32 GetBufferAddressComponent(const Inst* buffer_inst, u32 comp) {
    Inst* address = buffer_inst->Arg(LoadBufferArgs::Address).InstRecursive();
    ASSERT(address->GetOpcode() == IR::Opcode::CompositeConstructU32x3);
    return IR::U32{address->Arg(comp).Resolve()};
}

U32 GetBufferIndexArg(const Inst* buffer_inst) {
    return GetBufferAddressComponent(buffer_inst, 0);
}

U32 GetBufferVOffsetArg(const Inst* buffer_inst) {
    return GetBufferAddressComponent(buffer_inst, 1);
}

U32 GetBufferSOffsetArg(const Inst* buffer_inst) {
    return GetBufferAddressComponent(buffer_inst, 2);
}

} // namespace Shader::IR