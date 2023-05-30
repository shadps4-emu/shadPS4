#include "Disassembler.h"
#include <stdio.h>


Disassembler::Disassembler()
{
	ZydisDecoderInit(&m_decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
	ZydisFormatterInit(&m_formatter, ZYDIS_FORMATTER_STYLE_INTEL);
}

Disassembler::~Disassembler()
{
}

void Disassembler::printInstruction(void* code)//print a single instruction
{
	ZydisDecodedInstruction instruction;
	ZydisDecodedOperand     operands[ZYDIS_MAX_OPERAND_COUNT_VISIBLE];
	ZyanStatus              status = ZydisDecoderDecodeFull(&m_decoder, code, ZYDIS_MAX_INSTRUCTION_LENGTH,&instruction, operands);
	if (!ZYAN_SUCCESS(status))
	{
		printf("decode instruction failed at %p\n", code);
		printInst(instruction, operands);
	}
}

void Disassembler::printInst(ZydisDecodedInstruction& inst, ZydisDecodedOperand* operands)
{
	const int bufLen = 256;
	char      szBuffer[bufLen];
	ZydisFormatterFormatInstruction(&m_formatter, &inst, operands,inst.operand_count_visible, szBuffer, sizeof(szBuffer), 0,NULL);
    printf("instruction: %s\n", szBuffer);
}