// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <optional>

#include "shader_recompiler/frontend/copy_shader.h"
#include "shader_recompiler/frontend/decode.h"
#include "shader_recompiler/ir/attribute.h"

namespace Shader {

static bool IsDwordBufferLoad(const Gcn::GcnInst& inst) {
    return inst.opcode == Gcn::Opcode::BUFFER_LOAD_DWORD ||
           inst.opcode == Gcn::Opcode::BUFFER_LOAD_DWORDX2 ||
           inst.opcode == Gcn::Opcode::BUFFER_LOAD_DWORDX3 ||
           inst.opcode == Gcn::Opcode::BUFFER_LOAD_DWORDX4;
}

static u32 GetBufferLoadDwords(const Gcn::GcnInst& inst) {
    switch (inst.opcode) {
    case Gcn::Opcode::BUFFER_LOAD_DWORD:
        return 1;
    case Gcn::Opcode::BUFFER_LOAD_DWORDX2:
        return 2;
    case Gcn::Opcode::BUFFER_LOAD_DWORDX3:
        return 3;
    case Gcn::Opcode::BUFFER_LOAD_DWORDX4:
        return 4;
    default:
        return 0;
    }
}

static std::optional<s32> ReadConstant(const Gcn::InstOperand& operand,
                                       const std::array<std::optional<s32>, 256>& sources) {
    switch (operand.field) {
    case Gcn::OperandField::ConstZero:
        return 0;
    case Gcn::OperandField::SignedConstIntPos:
        return operand.code - Gcn::OperandFieldRange::SignedConstIntPosMin + 1;
    case Gcn::OperandField::SignedConstIntNeg:
        return static_cast<s32>(Gcn::OperandFieldRange::SignedConstIntNegMin) -
               static_cast<s32>(operand.code) - 1;
    case Gcn::OperandField::LiteralConst:
        return static_cast<s32>(operand.code);
    case Gcn::OperandField::ScalarGPR:
        if (operand.code < sources.size()) {
            return sources[operand.code];
        }
        return std::nullopt;
    default:
        return std::nullopt;
    }
}

CopyShaderData ParseCopyShader(std::span<const u32> code) {
    Gcn::GcnCodeSlice code_slice{code.data(), code.data() + code.size()};
    Gcn::GcnDecodeContext decoder;

    std::array<s32, 64> offsets{};
    offsets.fill(-1);

    std::array<std::optional<s32>, 256> sources{};

    CopyShaderData data{};
    auto last_attr{IR::Attribute::Position0};
    while (!code_slice.atEnd()) {
        auto inst = decoder.decodeInstruction(code_slice);
        switch (inst.opcode) {
        case Gcn::Opcode::S_ADD_U32: {
            const auto lhs = ReadConstant(inst.src[0], sources);
            const auto rhs = ReadConstant(inst.src[1], sources);
            if (lhs && rhs) {
                sources[inst.dst[0].code] = *lhs + *rhs;
            }
            break;
        }
        case Gcn::Opcode::S_AND_B32: {
            const auto lhs = ReadConstant(inst.src[0], sources);
            const auto rhs = ReadConstant(inst.src[1], sources);
            if (lhs && rhs) {
                sources[inst.dst[0].code] = *lhs & *rhs;
            }
            break;
        }
        case Gcn::Opcode::S_OR_B32: {
            const auto lhs = ReadConstant(inst.src[0], sources);
            const auto rhs = ReadConstant(inst.src[1], sources);
            if (lhs && rhs) {
                sources[inst.dst[0].code] = *lhs | *rhs;
            }
            break;
        }
        case Gcn::Opcode::S_LSHL_B32: {
            const auto lhs = ReadConstant(inst.src[0], sources);
            const auto rhs = ReadConstant(inst.src[1], sources);
            if (lhs && rhs) {
                sources[inst.dst[0].code] = *lhs << *rhs;
            }
            break;
        }
        case Gcn::Opcode::S_LSHR_B32: {
            const auto lhs = ReadConstant(inst.src[0], sources);
            const auto rhs = ReadConstant(inst.src[1], sources);
            if (lhs && rhs) {
                sources[inst.dst[0].code] = static_cast<s32>(static_cast<u32>(*lhs) >> *rhs);
            }
            break;
        }
        case Gcn::Opcode::S_MOVK_I32: {
            sources[inst.dst[0].code] = inst.control.sopk.simm;
            break;
        }
        case Gcn::Opcode::S_MOV_B32: {
            sources[inst.dst[0].code] = ReadConstant(inst.src[0], sources);
            break;
        }
        case Gcn::Opcode::S_ADDK_I32: {
            if (sources[inst.dst[0].code]) {
                *sources[inst.dst[0].code] += inst.control.sopk.simm;
            }
            break;
        }
        case Gcn::Opcode::S_BFM_B32: {
            const auto src0 = ReadConstant(inst.src[0], sources);
            const auto src1 = ReadConstant(inst.src[1], sources);
            if (src0 && src1) {
                sources[inst.dst[0].code] = ((1 << *src0) - 1) << *src1;
            }
            break;
        }
        case Gcn::Opcode::EXP: {
            const auto& exp = inst.control.exp;
            const IR::Attribute semantic = static_cast<IR::Attribute>(exp.target);
            for (int i = 0; i < inst.src_count; ++i) {
                if ((exp.en & (1 << i)) == 0) {
                    continue;
                }
                if (inst.src[i].code >= offsets.size()) {
                    continue;
                }
                const auto ofs = offsets[inst.src[i].code];
                if (ofs != -1) {
                    data.attr_map[ofs] = {semantic, i};
                    if (semantic > last_attr) {
                        last_attr = semantic;
                    }
                }
            }
            break;
        }
        default: {
            if (!IsDwordBufferLoad(inst)) {
                break;
            }
            if (inst.src[1].code >= offsets.size()) {
                break;
            }
            s32 offset = inst.control.mubuf.offset;
            if (inst.src[3].field != Gcn::OperandField::ConstZero) {
                const auto soffset = ReadConstant(inst.src[3], sources);
                if (!soffset) {
                    break;
                }
                offset += *soffset;
            }
            const u32 dwords = GetBufferLoadDwords(inst);
            for (u32 i = 0; i < dwords && inst.src[1].code + i < offsets.size(); ++i) {
                offsets[inst.src[1].code + i] = offset + static_cast<s32>(i * sizeof(u32));
            }
            data.num_comps += dwords;
            break;
        }
        }
    }

    if (!IsPosition(last_attr) && data.attr_map.size() >= 2) {
        data.num_attrs = static_cast<u32>(last_attr) - static_cast<u32>(IR::Attribute::Param0) + 1;
        const auto it = data.attr_map.begin();
        const u32 comp_stride = std::next(it)->first - it->first;
        data.output_vertices = comp_stride / 64;
    }

    return data;
}

} // namespace Shader
