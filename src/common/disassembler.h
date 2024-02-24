// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <Zydis/Zydis.h>
#include "common/types.h"

namespace Common {

class Disassembler {
public:
    Disassembler();
    ~Disassembler();

    void printInst(ZydisDecodedInstruction& inst, ZydisDecodedOperand* operands, u64 address);
    void printInstruction(void* code, u64 address);

private:
    ZydisDecoder m_decoder;
    ZydisFormatter m_formatter;
};

} // namespace Common
