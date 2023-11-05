#pragma once

#include <Zydis/Zydis.h>
#include "../types.h"

class Disassembler
{
public:
	Disassembler();
	~Disassembler();
	void printInst(ZydisDecodedInstruction& inst, ZydisDecodedOperand* operands,u64 address);
	void printInstruction(void* code,u64 address);

private:
	ZydisDecoder   m_decoder;
	ZydisFormatter m_formatter;
};
