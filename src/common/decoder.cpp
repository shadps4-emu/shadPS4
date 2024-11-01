// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fmt/format.h>
#include "common/decoder.h"

namespace Common {

DecoderImpl::DecoderImpl() {
    ZydisDecoderInit(&m_decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    ZydisFormatterInit(&m_formatter, ZYDIS_FORMATTER_STYLE_INTEL);
}

DecoderImpl::~DecoderImpl() = default;

std::string DecoderImpl::disassembleInst(ZydisDecodedInstruction& inst,
                                         ZydisDecodedOperand* operands, u64 address) {
    const int bufLen = 256;
    char szBuffer[bufLen];
    ZydisFormatterFormatInstruction(&m_formatter, &inst, operands, inst.operand_count_visible,
                                    szBuffer, sizeof(szBuffer), address, ZYAN_NULL);
    return szBuffer;
}

void DecoderImpl::printInstruction(void* code, u64 address) {
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT_VISIBLE];
    ZyanStatus status =
        ZydisDecoderDecodeFull(&m_decoder, code, sizeof(code), &instruction, operands);
    if (!ZYAN_SUCCESS(status)) {
        fmt::print("decode instruction failed at {}\n", fmt::ptr(code));
    } else {
        printInst(instruction, operands, address);
    }
}

void DecoderImpl::printInst(ZydisDecodedInstruction& inst, ZydisDecodedOperand* operands,
                            u64 address) {
    std::string s = disassembleInst(inst, operands, address);
    fmt::print("instruction: {}\n", s);
}

ZyanStatus DecoderImpl::decodeInstruction(ZydisDecodedInstruction& inst,
                                          ZydisDecodedOperand* operands, void* data, u64 size) {
    return ZydisDecoderDecodeFull(&m_decoder, data, size, &inst, operands);
}

} // namespace Common
