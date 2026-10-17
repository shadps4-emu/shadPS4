// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/copy_shader.h"
#include "shader_recompiler/frontend/decode.h"
#include "shader_recompiler/ir/attribute.h"

namespace Shader {

static s32 GetOperandValue(const Gcn::InstOperand& op, const std::array<s32, 256>& sources) {
    switch (op.field) {
    case Gcn::OperandField::ScalarGPR:
        return (op.code < sources.size() && sources[op.code] != -1) ? sources[op.code] : 0;
    case Gcn::OperandField::ConstZero:
        return 0;
    case Gcn::OperandField::SignedConstIntPos:
        return static_cast<s32>(op.code) - Gcn::OperandFieldRange::SignedConstIntPosMin + 1;
    case Gcn::OperandField::SignedConstIntNeg:
        return -static_cast<s32>(op.code) + Gcn::OperandFieldRange::SignedConstIntNegMin - 1;
    case Gcn::OperandField::LiteralConst:
        return static_cast<s32>(op.code);
    default:
        return 0;
    }
}

CopyShaderData ParseCopyShader(std::span<const u32> code) {
    Gcn::GcnCodeSlice code_slice{code.data(), code.data() + code.size()};
    Gcn::GcnDecodeContext decoder;

    constexpr u32 token_mov_vcchi = 0xBEEB03FF;
    ASSERT_MSG(code[0] == token_mov_vcchi, "First instruction is not s_mov_b32 vcc_hi, #imm");

    std::array<s32, 64> offsets{};
    offsets.fill(-1);

    std::array<s32, 256> sources{};
    sources.fill(-1);

    CopyShaderData data{};
    auto last_attr{IR::Attribute::Position0};
    while (!code_slice.atEnd()) {
        auto inst = decoder.decodeInstruction(code_slice);
        switch (inst.opcode) {
        case Gcn::Opcode::S_LSHL_B32: {
            const auto src0 = GetOperandValue(inst.src[0], sources);
            const auto src1 = GetOperandValue(inst.src[1], sources) & 0x1f;
            sources[inst.dst[0].code] = src0 << src1;
            break;
        }
        case Gcn::Opcode::S_LSHR_B32: {
            const auto src0 = static_cast<u32>(GetOperandValue(inst.src[0], sources));
            const auto src1 = GetOperandValue(inst.src[1], sources) & 0x1f;
            sources[inst.dst[0].code] = static_cast<s32>(src0 >> src1);
            break;
        }
        case Gcn::Opcode::S_ASHR_I32: {
            const auto src0 = GetOperandValue(inst.src[0], sources);
            const auto src1 = GetOperandValue(inst.src[1], sources) & 0x1f;
            sources[inst.dst[0].code] = src0 >> src1;
            break;
        }
        case Gcn::Opcode::S_MOVK_I32: {
            sources[inst.dst[0].code] = static_cast<s16>(inst.control.sopk.simm);
            break;
        }
        case Gcn::Opcode::S_MOV_B32: {
            sources[inst.dst[0].code] = GetOperandValue(inst.src[0], sources);
            break;
        }
        case Gcn::Opcode::S_ADDK_I32: {
            sources[inst.dst[0].code] += static_cast<s16>(inst.control.sopk.simm);
            break;
        }
        case Gcn::Opcode::S_ADD_I32:
        case Gcn::Opcode::S_ADD_U32: {
            sources[inst.dst[0].code] =
                GetOperandValue(inst.src[0], sources) + GetOperandValue(inst.src[1], sources);
            break;
        }
        case Gcn::Opcode::S_SUB_I32:
        case Gcn::Opcode::S_SUB_U32: {
            sources[inst.dst[0].code] =
                GetOperandValue(inst.src[0], sources) - GetOperandValue(inst.src[1], sources);
            break;
        }
        case Gcn::Opcode::S_MUL_I32: {
            sources[inst.dst[0].code] =
                GetOperandValue(inst.src[0], sources) * GetOperandValue(inst.src[1], sources);
            break;
        }
        case Gcn::Opcode::S_AND_B32: {
            sources[inst.dst[0].code] =
                GetOperandValue(inst.src[0], sources) & GetOperandValue(inst.src[1], sources);
            break;
        }
        case Gcn::Opcode::S_OR_B32: {
            sources[inst.dst[0].code] =
                GetOperandValue(inst.src[0], sources) | GetOperandValue(inst.src[1], sources);
            break;
        }
        case Gcn::Opcode::S_BFM_B32: {
            const auto src0 = GetOperandValue(inst.src[0], sources) & 0x1f;
            const auto src1 = GetOperandValue(inst.src[1], sources) & 0x1f;
            sources[inst.dst[0].code] = ((1u << src0) - 1u) << src1;
            break;
        }
        case Gcn::Opcode::EXP: {
            const auto& exp = inst.control.exp;
            const IR::Attribute semantic = static_cast<IR::Attribute>(exp.target);
            for (int i = 0; i < inst.src_count; ++i) {
                if ((exp.en & (1 << i)) == 0) {
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
        case Gcn::Opcode::BUFFER_LOAD_DWORD: {
            ASSERT_MSG(inst.src[1].code < offsets.size(),
                       "offsets array for geometry shaders is too short");
            offsets[inst.src[1].code] = inst.control.mubuf.offset;
            if (inst.src[3].field != Gcn::OperandField::ConstZero) {
                const s32 soffset = GetOperandValue(inst.src[3], sources);
                offsets[inst.src[1].code] += soffset;
            }
            data.num_comps++;
            break;
        }
        default:
            break;
        }
    }

    if (!IsPosition(last_attr)) {
        data.num_attrs = static_cast<u32>(last_attr) - static_cast<u32>(IR::Attribute::Param0) + 1;
    }
    if (data.attr_map.size() > 1) {
        const auto it = data.attr_map.begin();
        const u32 comp_stride = std::next(it)->first - it->first;
        data.output_vertices = comp_stride / 64;
    }

    return data;
}

} // namespace Shader
