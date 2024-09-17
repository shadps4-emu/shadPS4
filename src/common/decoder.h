// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <Zydis/Zydis.h>
#include "common/types.h"

namespace Common {

class Decoder {
public:
    Decoder();
    ~Decoder();

    void printInst(ZydisDecodedInstruction& inst, ZydisDecodedOperand* operands, u64 address);
    void printInstruction(void* code, u64 address);
    ZyanStatus decodeInstruction(ZydisDecodedInstruction& inst, ZydisDecodedOperand* operands,
                                 void* data, u64 size = 15);

    static Decoder& Instance() {
        static Decoder instance;
        return instance;
    }

private:
    ZydisDecoder m_decoder;
    ZydisFormatter m_formatter;
};

} // namespace Common
