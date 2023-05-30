#pragma once

#include "zydis/Zydis.h"

class Disassembler
{
public:
	Disassembler();
	~Disassembler();
	void printInst(ZydisDecodedInstruction& inst, ZydisDecodedOperand* operands);
	void printInstruction(void* code);

private:
	ZydisDecoder   m_decoder;
	ZydisFormatter m_formatter;
};