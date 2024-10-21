// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <Zydis/Zydis.h>
#include "common/singleton.h"
#include "common/types.h"

namespace Common {

class DecoderImpl {
public:
    DecoderImpl();
    ~DecoderImpl();

    std::string disassembleInst(ZydisDecodedInstruction& inst, ZydisDecodedOperand* operands,
                                u64 address);
    void printInst(ZydisDecodedInstruction& inst, ZydisDecodedOperand* operands, u64 address);
    void printInstruction(void* code, u64 address);
    ZyanStatus decodeInstruction(ZydisDecodedInstruction& inst, ZydisDecodedOperand* operands,
                                 void* data, u64 size = 15);

private:
    ZydisDecoder m_decoder;
    ZydisFormatter m_formatter;
};

using Decoder = Common::Singleton<DecoderImpl>;

} // namespace Common
