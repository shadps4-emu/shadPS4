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
